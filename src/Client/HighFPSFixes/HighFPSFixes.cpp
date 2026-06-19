#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"
#include "../../Controller/ScreenJoystickHook.hpp"
#include "../../ClientFX/ClientFX.hpp"
#include "../../Server/Server.hpp"

// ======================
// HighFPSFixes
// ======================

static bool __fastcall LoadFxDll_Hook(int thisPtr, int, char* Source, char a3)
{
	// Load the DLL
	char result = LoadFxDll(thisPtr, Source, a3);

	// Get the path
	char* clientFXPath = ((char*)thisPtr + 0x24);

	// Get the handle
	wchar_t wFileName[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, clientFXPath, -1, wFileName, MAX_PATH);
	HMODULE clientFxDll = GetModuleHandleW(wFileName);

	if (clientFxDll)
	{
		g_State.GameClientFX = clientFxDll;
		ApplyClientFXPatch();
	}

	return result;
}

static void __fastcall UpdateOnGround_Hook(int thisPtr, int)
{
	bool* pJumped = reinterpret_cast<bool*>(thisPtr + 0x78);

	// Detect jump start
	if (!g_State.previousJumpState && *pJumped)
	{
		g_State.jumpElapsedTime = 0.0;
	}

	UpdateOnGround(thisPtr);

	// Maintain jump state for a few frames
	if (g_State.jumpElapsedTime >= 0.0)
	{
		if (g_State.jumpElapsedTime < TARGET_FRAME_TIME)
		{
			*pJumped = true;
			g_State.jumpElapsedTime += g_State.simulationFrameTime;
		}
		else
		{
			g_State.jumpElapsedTime = -1.0; // Mark as inactive
		}
	}

	// Update tracking state
	g_State.previousJumpState = *pJumped;
}

static void __fastcall UpdateWaveProp_Hook(int thisPtr, int, float frameDelta)
{
	// Updates water wave propagation at fixed time intervals for consistent simulation
	g_State.waveUpdateAccumulator += frameDelta;

	if (g_State.waveUpdateAccumulator > TARGET_FRAME_TIME * 5)
		g_State.waveUpdateAccumulator = TARGET_FRAME_TIME * 5;

	while (g_State.waveUpdateAccumulator >= TARGET_FRAME_TIME)
	{
		UpdateWaveProp(thisPtr, TARGET_FRAME_TIME);
		g_State.waveUpdateAccumulator -= TARGET_FRAME_TIME;
	}
}

static double __fastcall GetMaxRecentVelocityMag_Hook(int thisPtr, int)
{
	if (!g_State.useVelocitySmoothing)
	{
		return GetMaxRecentVelocityMag(thisPtr);
	}

	double dt = g_State.simulationFrameTime;
	bool dtUsable = (dt > 0.0 && dt <= 0.2);

	float* currentPos = (float*)((char*)thisPtr - 12); // m_vLastPos is 12 bytes before m_vLastVel

	// Game's actual idle cutoff. Used for block detection and the walk/idle hysteresis,
	// and as a floor on the reported value: the game animates idle below it, so a slow
	// but real slide must report at least this to animate walk.
	float forceIdleVel = *(float*)((char*)thisPtr - 900);
	if (!(forceIdleVel > 0.0f && forceIdleVel < 10000.0f))
	{
		forceIdleVel = 45.0f;
	}

	bool reportingIdle = (g_State.lastReportedVelocity < 0.1);

	// INSTANT: a confirmed walk-speed position delta in a single frame commits to walk
	// immediately, before a window can resolve (covers the start of a move and breaking
	// free of a wall). Gate on real ground covered this frame, not just speed: in slowmo
	// a tiny dt turns wall jitter into a huge frameSpeed at near-zero displacement, so
	// INSTANT_MIN_DISP keeps jitter/slides out and defers them to the window.
	const double INSTANT_MIN_DISP = 3.0;

	if (reportingIdle && dtUsable && g_State.prevPosValid)
	{
		float fdx = currentPos[0] - g_State.prevPosX;
		float fdy = currentPos[1] - g_State.prevPosY;
		float fdz = currentPos[2] - g_State.prevPosZ;
		double frameDisp = sqrt(fdx * fdx + fdy * fdy + fdz * fdz);
		double frameSpeed = frameDisp / dt;

		if (frameSpeed >= forceIdleVel && frameDisp >= INSTANT_MIN_DISP && frameDisp < 250.0) // 250: teleport guard
		{
			double startSpeed = std::max(std::min(frameSpeed, 400.0), (double)forceIdleVel);
			g_State.lastReportedVelocity = startSpeed;
			g_State.prevWindowSpeed = startSpeed;
			g_State.impededWindowCount = 0;
			g_State.velocityAccumulator = 0.0;
			g_State.velocityTimeAccumulator = 0.0;
			g_State.moveGraceUsed = false;
			g_State.prevPosX = currentPos[0];
			g_State.prevPosY = currentPos[1];
			g_State.prevPosZ = currentPos[2];
			g_State.prevPosValid = true;
			return startSpeed;
		}
	}

	// GRACE: one optimistic walk frame to bridge the gap until a sim step can confirm the
	// move. Re-armed only by INSTANT or by smoothing turning off, and suppressed once a
	// block is confirmed so it can't bounce a wall stop back to walk.
	if (reportingIdle && !g_State.moveGraceUsed && g_State.impededWindowCount < 2)
	{
		g_State.moveGraceUsed = true;
		g_State.lastReportedVelocity = (double)forceIdleVel + 1.0;
		g_State.prevWindowSpeed = (double)forceIdleVel + 1.0;
		g_State.velocityAccumulator = 0.0;
		g_State.velocityTimeAccumulator = 0.0;
		return g_State.lastReportedVelocity;
	}

	// No usable sim step this frame (dt==0 render frame or a hitch): hold the last
	// decision rather than fall back to raw, which is frozen/jittery here.
	if (!dtUsable)
	{
		return g_State.lastReportedVelocity;
	}

	double rawVelocity = GetMaxRecentVelocityMag(thisPtr);

	// Remember this position for next frame's INSTANT delta.
	g_State.prevPosX = currentPos[0];
	g_State.prevPosY = currentPos[1];
	g_State.prevPosZ = currentPos[2];
	g_State.prevPosValid = true;

	if (g_State.velocityTimeAccumulator == 0.0)
	{
		g_State.windowStartX = currentPos[0];
		g_State.windowStartY = currentPos[1];
		g_State.windowStartZ = currentPos[2];
		g_State.velocityAccumulator = 0.0;
	}

	// Reject spike samples: physics jitter can produce absurd per-frame raw values
	// (700-40000+) at extreme FPS while displacement is zero
	if (rawVelocity < 600.0 || dt >= 0.005)
	{
		g_State.velocityAccumulator += rawVelocity * dt;
	}
	g_State.velocityTimeAccumulator += dt;

	bool timeIsUp = g_State.velocityTimeAccumulator >= 0.05;
	double runningAvg = g_State.velocityAccumulator / g_State.velocityTimeAccumulator;

	// Confirmed-block recovery trigger: idle, but the running average says we're moving again.
	bool isStartingToMove = g_State.lastReportedVelocity < 0.1 && runningAvg > std::max(10.0, (double)forceIdleVel * 0.5);

	// Resolve only on a full window. Net displacement is measured from windowStart, so
	// resolving early (mid-window) would see ~zero displacement and misjudge movement.
	if (timeIsUp)
	{
		float dx = currentPos[0] - g_State.windowStartX;
		float dy = currentPos[1] - g_State.windowStartY;
		float dz = currentPos[2] - g_State.windowStartZ;

		double netDisplacement = sqrt(dx * dx + dy * dy + dz * dz);
		double displacementVelocity = netDisplacement / g_State.velocityTimeAccumulator;
		double avgRawVelocity = g_State.velocityAccumulator / g_State.velocityTimeAccumulator;

		// How much of intended movement actually happened
		// Normal ≈ 0.9-1.0, angled wall ≈ 0.05-0.08, straight wall ≈ 0.0
		double velocityEfficiency = (avgRawVelocity > 5.0) ? (displacementVelocity / avgRawVelocity) : 1.0;

		// Block detection scaled off the game's idle threshold. The efficiency term on
		// the low-speed clause exempts a slow but directed slide (high efficiency) so it
		// isn't treated as grinding a wall.
		bool isBlocked = (avgRawVelocity < forceIdleVel * 1.1 && displacementVelocity < forceIdleVel && velocityEfficiency < 0.5) || (velocityEfficiency < 0.15 && avgRawVelocity > forceIdleVel * 0.6);

		// Protect against slowmo timing (raw ≈ 0 but displacement spikes)
		if (avgRawVelocity < 5.0 && displacementVelocity > 200.0)
		{
			isBlocked = false;
		}

		// Use whichever signal shows movement (raw for normal play, displacement for slowmo)
		double effectiveVelocity = std::max(avgRawVelocity, displacementVelocity);

		// Cap displacement spikes from teleports/glitches to normal walking speed
		if (displacementVelocity > avgRawVelocity && displacementVelocity > 400.0)
		{
			effectiveVelocity = std::max(avgRawVelocity, 400.0);
		}

		// Poisoned window: raw is absurdly high but player didn't actually move
		if (avgRawVelocity > 600.0 && displacementVelocity < 50.0)
		{
			g_State.velocityAccumulator = 0.0;
			g_State.velocityTimeAccumulator = 0.0;
			return g_State.lastReportedVelocity;
		}

		// Snapshot blocked state before updating the counter, so isStartingToMove
		// sees the previous window's confirmed block status
		bool wasConfirmedBlocked = (g_State.impededWindowCount >= 2);

		if (isStartingToMove && wasConfirmedBlocked)
		{
			// Recovering from wall block: require both raw intent and real
			// displacement to prevent brush-oscillation restarts
			double reentryRawThreshold = forceIdleVel * 2.0;
			double reentryDispThreshold = forceIdleVel * 0.75;

			if (avgRawVelocity >= reentryRawThreshold && displacementVelocity >= reentryDispThreshold)
			{
				g_State.lastReportedVelocity = std::max(effectiveVelocity, (double)forceIdleVel);
				g_State.prevWindowSpeed = g_State.lastReportedVelocity;
				g_State.impededWindowCount = 0;
			}
			else
			{
				g_State.lastReportedVelocity = 0.0;
				g_State.prevWindowSpeed = 0.0;
				g_State.impededWindowCount = 2;
				g_State.moveGraceUsed = true;
			}
		}
		else
		{
			if (isBlocked)
			{
				g_State.impededWindowCount++;
			}
			else
			{
				g_State.impededWindowCount = 0;
			}

			// One displacement-driven walk/idle decision for both steady movement and
			// normal (non-blocked) starts, so a per-frame raw spike can't force walk
			// against the window's displacement verdict. Hysteresis (enter above leave)
			// stops a slide whose speed straddles the cutoff from flip-flopping, a
			// directed slide uses a lower bar so it animates walk like vanilla.
			bool directed = (velocityEfficiency >= 0.5);
			double slideEnter = directed ? (forceIdleVel * 0.45) : forceIdleVel;
			double slideLeave = directed ? (forceIdleVel * 0.30) : (forceIdleVel * 0.9);
			bool wasMoving = (g_State.lastReportedVelocity >= forceIdleVel);
			bool moving = wasMoving ? (displacementVelocity >= slideLeave) : (displacementVelocity >= slideEnter);

			// Require 2 consecutive blocked windows (100ms) to confirm wall collision
			if (g_State.impededWindowCount >= 2 || !moving)
			{
				g_State.lastReportedVelocity = 0.0;
				g_State.prevWindowSpeed = 0.0;
				g_State.moveGraceUsed = true;
			}
			else
			{
				// Floor the output at the idle cutoff: the game idles below it, so a slow
				// directed slide we judged "moving" must report >= it to animate walk.
				double out = std::max(effectiveVelocity, (double)forceIdleVel);

				// Instant response when accelerating, smoothed decay when decelerating
				if (out > g_State.prevWindowSpeed)
				{
					g_State.lastReportedVelocity = out;
					g_State.prevWindowSpeed = out;
				}
				else
				{
					// Sharp velocity collapse (wall hit while moving), snap down
					// instead of smoothing through the idle threshold over multiple windows
					double ratio = (g_State.prevWindowSpeed > 1.0) ? (out / g_State.prevWindowSpeed) : 1.0;

					if (ratio < 0.25)
					{
						g_State.prevWindowSpeed = out;
					}
					else
					{
						double decayFactor = std::min(g_State.velocityTimeAccumulator / 0.05, 1.0);
						g_State.prevWindowSpeed = g_State.prevWindowSpeed * (1.0 - decayFactor * 0.5) + out * (decayFactor * 0.5);
					}

					g_State.lastReportedVelocity = std::max(g_State.prevWindowSpeed, (double)forceIdleVel);
				}
			}
		}

		g_State.velocityAccumulator = 0.0;
		g_State.velocityTimeAccumulator = 0.0;
	}

	return g_State.lastReportedVelocity;
}

static void __cdecl PolyGridFXCollisionHandlerCB_Hook(int hBody1, int hBody2, int* a3, int* a4, float a5, BYTE* a6, int a7)
{
	uint32_t b1 = static_cast<uint32_t>(hBody1);
	uint32_t b2 = static_cast<uint32_t>(hBody2);

	// Create order-independent key so (A,B) and (B,A) collisions map to the same entry
	uint64_t key = (b1 < b2) ? (uint64_t(b2) << 32) | b1 : (uint64_t(b1) << 32) | b2;

	double currentGameTime = g_State.totalGameTime;

	// Search for existing entry in circular buffer cache
	GlobalState::SplashEntry* foundEntry = nullptr;
	for (auto& entry : g_State.splashCache)
	{
		if (entry.key == key)
		{
			foundEntry = &entry;
			break;
		}
	}

	// Only process splash if this pair hasn't splashed this frame
	if (!foundEntry || (currentGameTime - foundEntry->lastTime) >= TARGET_FRAME_TIME)
	{
		PolyGridFXCollisionHandlerCB(hBody1, hBody2, a3, a4, a5, a6, a7);

		if (foundEntry)
		{
			foundEntry->lastTime = currentGameTime;
		}
		else
		{
			// Overwrite oldest entry in circular buffer (size 64)
			g_State.splashCache[g_State.splashIndex] = { key, currentGameTime };
			g_State.splashIndex = (g_State.splashIndex + 1) % g_State.splashCache.size();
		}
	}
}

static void __fastcall UpdateNormalControlFlags_Hook(int thisPtr, int)
{
	g_State.useVelocitySmoothing = true;
	UpdateNormalControlFlags(thisPtr);
	g_State.useVelocitySmoothing = false;
}

static void __fastcall UpdateNormalFriction_Hook(int thisPtr, int)
{
	g_State.inFriction = true;
	UpdateNormalFriction(thisPtr);
	g_State.inFriction = false;
}

static double __fastcall GetTimerElapsedS_Hook(int thisPtr, int)
{
	// When sliding on friction, clamp the reported frame time
	if (g_State.inFriction)
	{
		double elapsedTime = GetTimerElapsedS(thisPtr);
		if (elapsedTime < TARGET_FRAME_TIME)
		{
			return TARGET_FRAME_TIME;
		}
		return elapsedTime;
	}

	return GetTimerElapsedS(thisPtr);
}

void __fastcall MoveLocalSolidObject_Hook(int* thisPtr, int)
{
	bool bBodyInLiquid = *((BYTE*)thisPtr + 48);
	bool bSwimJumped = *((BYTE*)thisPtr + 121);

	g_State.pendingVelocityFix = (bBodyInLiquid && bSwimJumped);

	MoveLocalSolidObject(thisPtr);
}

void __fastcall EnterSlowMo_Hook(int* pThis, int, int hRec, bool bTrans, double fPeriod, bool bPlayer)
{
	double* pCharge = (double*)((char*)pThis + 880);
	double fCurrentCharge = *pCharge;

	// Clamp period to current charge (fixes visual jump)
	if (bPlayer && fCurrentCharge > 0.0 && fPeriod > fCurrentCharge)
	{
		fPeriod = fCurrentCharge;
	}

	g_State.clientSlowMoCharge = fCurrentCharge;

	EnterSlowMo(pThis, hRec, bTrans, fPeriod, bPlayer);
}

void __fastcall UpdateSlowMo_Hook(int* pThis, int)
{
	double* pCharge = (double*)((char*)pThis + 880);

	// Keep server in sync with client charge
	g_State.clientSlowMoCharge = *pCharge;

	UpdateSlowMo(pThis);
}

unsigned int __fastcall HandleMsgSlowMo_Hook(int* pThis, int, void* pMsg)
{
	double* pCharge = (double*)((char*)pThis + 880);
	int* pState = (int*)((char*)pThis + 640);

	double fOldCharge = *pCharge;
	int nOldState = *pState;

	unsigned int result = HandleMsgSlowMo(pThis, pMsg);

	double fNewCharge = *pCharge;
	int nNewState = *pState;

	g_State.clientSlowMoCharge = fNewCharge;

	// Recharge protection, don't let server reset recharged amount
	if (nOldState == 3 && nNewState == 3)
	{
		if (fNewCharge < fOldCharge)
		{
			*pCharge = fOldCharge;
			g_State.clientSlowMoCharge = fOldCharge;
		}
	}
	// Jump protection, prevent charge jumping up during active slowmo
	else if (nNewState != 3)
	{
		if (fOldCharge > 0.0 && fNewCharge > fOldCharge)
		{
			*pCharge = fOldCharge;
			g_State.clientSlowMoCharge = fOldCharge;
		}
	}

	return result;
}
