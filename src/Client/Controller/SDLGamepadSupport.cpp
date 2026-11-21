#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"
#include "../../Controller/ScreenJoystickHook.hpp"
#include "../../ClientFX/ClientFX.hpp"
#include "../../Server/Server.hpp"
#include "ConsoleMgr.hpp"

static ULONGLONG lastCursorStateChangeTime = 0;
static ULONGLONG cursorActivityStartTime = 0;
static double zoomMag = 0;
static int cursorMovementAccum = 0;
static int lastCursorX = 0;
static int lastCursorY = 0;
static int turretPrevDamageState = 0;
static int slowMoStateOffset = 0;
static int slowMoPlayerOffset = 0;
static uint16_t healthBefore = 0;
static uint16_t healthAfter = 0;
static uint16_t healthReceived = 0;
static uint16_t armorBefore = 0;
static uint16_t armorAfter = 0;
static bool updateGyroCamera = false;
static int bindMgr = 0;
static bool isAiming = false;
static bool isDoingMeleeAttack = false;
static bool isTakingDamage = false;
static ULONGLONG fallDamageTime = 0;
static ULONGLONG slowMoCommandTime = 0;

// =======================
// SDLGamepadSupport
// =======================

static bool __fastcall IsCommandOn_Hook(int thisPtr, int, int commandId)
{
	return (commandId < MAX_COMMAND_ID + 1 && g_Controller.commandActive[commandId]) || IsCommandOn(thisPtr, commandId);
}

static double __fastcall GetExtremalCommandValue_Hook(int thisPtr, int, int commandId)
{
	bindMgr = thisPtr;

	if (!g_Controller.isConnected || g_State.isConsoleOpen)
		return GetExtremalCommandValue(thisPtr, commandId);

	SDL_Gamepad* pGamepad = GetGamepad();
	if (!pGamepad)
		return GetExtremalCommandValue(thisPtr, commandId);

	static constexpr double DEAD_ZONE = 7849.0 / 32767.0;

	// Radial dead zone, rescaled so the value starts from 0 at its edge
	auto ReadStickAxis = [&](SDL_GamepadAxis axis, SDL_GamepadAxis otherAxis) -> double
	{
		double value = std::max(SDL_GetGamepadAxis(pGamepad, axis) / 32767.0, -1.0);
		double otherValue = std::max(SDL_GetGamepadAxis(pGamepad, otherAxis) / 32767.0, -1.0);

		double magnitude = std::sqrt(value * value + otherValue * otherValue);
		if (magnitude <= DEAD_ZONE)
			return 0.0;

		return value * std::min((magnitude - DEAD_ZONE) / (1.0 - DEAD_ZONE), 1.0) / magnitude;
	};

	auto ReadMovementAxis = [&](SDL_GamepadAxis axis, SDL_GamepadAxis otherAxis) -> double
	{
		if (std::abs(SDL_GetGamepadAxis(pGamepad, axis) / 32767.0) < DEAD_ZONE)
			return 0.0;

		return ReadStickAxis(axis, otherAxis);
	};

	auto ApplyZoomScale = [](double value) -> double
	{
		if (zoomMag > GPadZoomMagThreshold)
			return value * std::sqrt(zoomMag / GPadZoomMagThreshold);
		return value;
	};

	switch (commandId)
	{
		case 2:  return -ReadMovementAxis(SDL_GAMEPAD_AXIS_LEFTY, SDL_GAMEPAD_AXIS_LEFTX);                   // Forward
		case 5:  return  ReadMovementAxis(SDL_GAMEPAD_AXIS_LEFTX, SDL_GAMEPAD_AXIS_LEFTY);                   // Strafe
		case 22: return  ApplyZoomScale(ReadStickAxis(SDL_GAMEPAD_AXIS_RIGHTY, SDL_GAMEPAD_AXIS_RIGHTX));     // Pitch
		case 23: return  ApplyZoomScale(ReadStickAxis(SDL_GAMEPAD_AXIS_RIGHTX, SDL_GAMEPAD_AXIS_RIGHTY));     // Yaw
		default: return  GetExtremalCommandValue(thisPtr, commandId);
	}
}

static double __fastcall GetZoomMag_Hook(int thisPtr)
{
	zoomMag = GetZoomMag(thisPtr);
	return zoomMag;
}

static __declspec(naked) void HUDActivateObjectSetObject_Hook()
{
	__asm
	{
		// If we can open a door or pick up an item (nNewType is after the return address and the LTObjRef)
		cmp dword ptr [esp + 0x14], -1
		setne al
		mov g_State.canActivate, al
		jmp HUDActivateObjectSetObject
	}
}

static int __fastcall SetOperatingTurret_Hook(int thisPtr, int, int pTurret)
{
	// Operating a turret?
	g_State.isOperatingTurret = pTurret != 0;

	if (!pTurret)
	{
		turretPrevDamageState = 0;
	}

	return SetOperatingTurret(thisPtr, pTurret);
}

static int userProfile = 0;

static const wchar_t* GetControllerTriggerName(int thisPtr, int commandId)
{
	// Left Thumbstick movement
	switch (commandId)
	{
		case 0: return L"Left Thumbstick Up";
		case 1: return L"Left Thumbstick Down";
	}

	// Reload key alias
	if (commandId == 87) commandId = 88;

	// HUDWeapon
	bool shortName = false;
	if (commandId >= 30 && commandId <= 39) 
	{ 
		commandId = 77; 
		shortName = true; 
	}
	// HUDGrenadeList
	else if (commandId >= 40 && commandId <= 45) 
	{ 
		commandId = 73; 
		shortName = true; 
	}

	const wchar_t* name = GetGamepadButtonPrompt(commandId, shortName);
	if (name)
		return name;

	return GetTriggerNameFromCommandID(thisPtr, commandId);
}

static const wchar_t* __fastcall GetTriggerNameFromCommandID_Hook(int thisPtr, int, int commandId)
{
	if (g_State.isLoadingDefault)
		return GetTriggerNameFromCommandID(thisPtr, commandId);

	const wchar_t* name = ShouldShowControllerPrompts() ? GetControllerTriggerName(thisPtr, commandId) : GetTriggerNameFromCommandID(thisPtr, commandId);
	if (!g_State.hasButtonGlyphs || name[0] == L'\0' || commandId < 0 || commandId > MAX_COMMAND_ID)
		return name;

	static wchar_t s_promptChars[4][2];
	static int s_promptIndex = 0;
	wchar_t* prompt = s_promptChars[s_promptIndex++ & 3];

	userProfile = thisPtr;
	prompt[0] = COMMAND_PROMPT_CHAR + commandId;
	prompt[1] = L'\0';
	return prompt;
}

static void __fastcall UseCursor_Hook(int thisPtr, int, bool bUseCursor, bool bLockCursorToCenter)
{
	if (!g_State.pUseCursor)
	{
		g_State.pUseCursor = thisPtr;
	}

	bool prevAllowed = g_State.isAllowedToUseCursor;
	g_State.isAllowedToUseCursor = bUseCursor;
	g_State.shouldLockCursorToCenter = bLockCursorToCenter;

	if (bUseCursor != prevAllowed)
	{
		lastCursorStateChangeTime = GetTickCount64();
	}

	if (ShouldShowControllerPrompts() && !g_Controller.touchpadCursorActive)
	{
		bUseCursor = false;
	}

	UseCursor(thisPtr, bUseCursor, bLockCursorToCenter);
}

static bool __fastcall OnMouseMove_Hook(int thisPtr, int, int x, int y)
{
	int deltaX = x - lastCursorX;
	int deltaY = y - lastCursorY;
	lastCursorX = x;
	lastCursorY = y;

	if (ShouldShowControllerPrompts())
	{
		if (!IsTouchpadRecentlyUsed())
		{
			if (g_State.isAllowedToUseCursor && (deltaX != 0 || deltaY != 0))
			{
				ULONGLONG now = GetTickCount64();

				if ((now - lastCursorStateChangeTime) > 500)
				{
					if (now - cursorActivityStartTime > 500)
					{
						cursorMovementAccum = 0;
						cursorActivityStartTime = now;
					}

					cursorMovementAccum += abs(deltaX) + abs(deltaY);

					if (cursorMovementAccum > 150)
					{
						cursorMovementAccum = 0;
						cursorActivityStartTime = 0;
						OnKeyboardMouseInput();
					}
				}
			}

			x = 0;
			y = 0;
		}
	}

	return OnMouseMove(thisPtr, x, y);
}

static void __fastcall ForceMouseUpdate_Hook(DWORD* thisPtr, int)
{
	if (ShouldShowControllerPrompts() && !g_Controller.touchpadCursorActive)
	{
		return;
	}

	return ForceMouseUpdate(thisPtr);
}

static void __fastcall HUDSwapUpdate_Hook(int thisPtr, int)
{
	HUDSwapUpdate(thisPtr);
	g_State.canSwap = *(BYTE*)(thisPtr + 0x1C0) != 0;
}

static void __fastcall SwitchToScreen_Hook(int thisPtr, int, int pNewScreen)
{
	int currentScreenID = *(DWORD*)(pNewScreen + 0x10);

	if (currentScreenID == g_State.screenPerformanceCPU)
	{
		g_State.maxCurrentType = 3;
		g_State.currentType = 0;
	}
	else if (currentScreenID == g_State.screenPerformanceGPU)
	{
		g_State.maxCurrentType = 2;
		g_State.currentType = 0;
	}
	else
	{
		g_State.maxCurrentType = -1;
	}

	SwitchToScreen(thisPtr, pNewScreen);
}

static void __fastcall SetCurrentType_Hook(int thisPtr, int, int type)
{
	g_State.pCurrentType = thisPtr;
	SetCurrentType(thisPtr, type);
}

static void __fastcall UpdatePlayerMovement_Hook(int thisPtr, int)
{
	updateGyroCamera = true;
	UpdatePlayerMovement(thisPtr);
	updateGyroCamera = false;
}

static void __fastcall ApplyLocalRotationOffset_Hook(int thisPtr, int, float* vPYROffset)
{
	// Input lock of the bind manager
	bool isInputEnabled = bindMgr == 0 || *(bool*)(bindMgr + 0x4C);

	if (updateGyroCamera && isInputEnabled && g_Controller.isConnected && IsGyroEnabled() && !g_State.isConsoleOpen)
	{
		bool shouldApplyGyro = (GyroAimingMode == 0) 
			|| (GyroAimingMode == 1 && isAiming) 
			|| (GyroAimingMode == 2 && !isAiming);

		if (shouldApplyGyro)
		{
			float gyroYaw = 0.0f;
			float gyroPitch = 0.0f;

			GetProcessedGyroDelta(gyroYaw, gyroPitch);
			vPYROffset[0] += gyroPitch;
			vPYROffset[1] += gyroYaw;
		}
	}

	ApplyLocalRotationOffset(thisPtr, vPYROffset);
}

static void __fastcall BeginAim_Hook(BYTE* thisPtr, int)
{
	BeginAim(thisPtr);
	isAiming = *thisPtr;
}

static void __fastcall EndAim_Hook(BYTE* thisPtr, int)
{
	EndAim(thisPtr);
	isAiming = *thisPtr;
}

static void __fastcall CClientWeaponFire_Hook(DWORD* thisPtr, int)
{
	isDoingMeleeAttack = false;
	g_State.isUsingRemoteDetonator = false;

	DWORD ptrAmmo = thisPtr[106];

	if (ptrAmmo && RumbleEnabled)
	{
		const char* hAmmoData = *(const char**)ptrAmmo;
		if (hAmmoData)
		{
			const uint32_t weaponHash = HashHelper::FNV1aRuntime(hAmmoData);

			switch (weaponHash)
			{
				case HashHelper::WeaponHashes::Melee_RifleButt:
				case HashHelper::WeaponHashes::Melee_JumpKick:
				case HashHelper::WeaponHashes::Melee_SlideKick:
				case HashHelper::WeaponHashes::Melee_JabRight:
				case HashHelper::WeaponHashes::Melee_JabLeft:
				case HashHelper::WeaponHashes::Melee_RunKickRight:
				case HashHelper::WeaponHashes::Melee_RunKickLeft:
					isDoingMeleeAttack = true;
					break;

				case HashHelper::WeaponHashes::RemoteDetonator:
					g_State.isUsingRemoteDetonator = true;
					break;

				case HashHelper::WeaponHashes::Frag:
				case HashHelper::WeaponHashes::Proximity:
				case HashHelper::WeaponHashes::RemoteCharge:
				case HashHelper::WeaponHashes::DeployableTurretGrenade:
					SetGamepadRumble(15000, 5000, 100, 2);
					break;

				case HashHelper::WeaponHashes::SMG:
					SetGamepadRumble(18000, 25000, 70, 3);
					break;

				case HashHelper::WeaponHashes::Laser:
					SetGamepadRumble(10000, 30000, 80, 3);
					break;

				case HashHelper::WeaponHashes::AssaultRifle:
					SetGamepadRumble(30000, 20000, 80, 3);
					break;

				case HashHelper::WeaponHashes::Rifle:
				case HashHelper::WeaponHashes::AdvancedRifle:
					SetGamepadRumble(45000, 25000, 100, 3);
					break;

				case HashHelper::WeaponHashes::Pistol:
					SetGamepadRumble(35000, 35000, 110, 3);
					break;

				case HashHelper::WeaponHashes::NailGun:
					SetGamepadRumble(40000, 50000, 110, 3);
					break;

				case HashHelper::WeaponHashes::Minigun:
				case HashHelper::WeaponHashes::Turret_Ceiling:
					SetGamepadRumble(40000, 45000, 110, 3);
					break;

				case HashHelper::WeaponHashes::Plasma:
				case HashHelper::WeaponHashes::ChainLightningGun:
					SetGamepadRumble(55000, 55000, 220, 3);
					break;

				case HashHelper::WeaponHashes::Shotgun:
					SetGamepadRumble(45000, 45000, 250, 3);
					break;

				case HashHelper::WeaponHashes::Missile:
					SetGamepadRumble(50000, 50000, 250, 3);
					break;

				case HashHelper::WeaponHashes::GrenadeLauncher:
				case HashHelper::WeaponHashes::Cannon:
					SetGamepadRumble(60000, 60000, 250, 3);
					break;
			}
		}
	}

	CClientWeaponFire(thisPtr);
}

static unsigned int __fastcall UpdateHealth_Hook(DWORD* thisPtr, int, unsigned int newHealth)
{
	if (isTakingDamage)
	{
		healthBefore = static_cast<uint16_t>(thisPtr[1]);
	}

	healthReceived = static_cast<uint16_t>(newHealth);

	unsigned int result = UpdateHealth(thisPtr, newHealth);

	if (isTakingDamage)
	{
		healthAfter = static_cast<uint16_t>(thisPtr[1]);
	}

	return result;
}

static int __fastcall UpdateArmor_Hook(DWORD* thisPtr, int, unsigned int newArmor)
{
	if (isTakingDamage)
	{
		armorBefore = static_cast<uint16_t>(thisPtr[2]);
	}

	int result = UpdateArmor(thisPtr, newArmor);

	if (isTakingDamage)
	{
		armorAfter = static_cast<uint16_t>(thisPtr[2]);
	}

	return result;
}

static void __fastcall HandleMsgPlayerDamage_Hook(DWORD* thisPtr, int, int* a2)
{
	if (!RumbleEnabled)
	{
		HandleMsgPlayerDamage(thisPtr, a2);
		return;
	}

	isTakingDamage = true;
	healthBefore = 0;
	healthAfter = 0;
	armorBefore = 0;
	armorAfter = 0;

	auto isDeadHealth = [](uint16_t health) { return health == 0 || health == SHRT_MAX; };
	bool wasPlayerDead = isDeadHealth(healthReceived);

	float damageBefore[12];
	float* damageArray = (float*)((BYTE*)thisPtr + 428);
	memcpy(damageBefore, damageArray, sizeof(damageBefore));
	memset(damageArray, 0, sizeof(damageBefore));

	HandleMsgPlayerDamage(thisPtr, a2);

	isTakingDamage = false;

	float maxDamage = 0.0f;
	int damageSector = -1;

	for (int i = 0; i < 12; i++)
	{
		if (damageArray[i] > maxDamage)
		{
			maxDamage = damageArray[i];
			damageSector = i;
		}

		damageArray[i] = std::min(damageBefore[i] + damageArray[i], 1.0f);
	}

	if (fallDamageTime != 0)
	{
		bool isFallDamage = GetTickCount64() - fallDamageTime < 1000;
		fallDamageTime = 0;

		if (isFallDamage)
			return;
	}

	bool playerDied = !wasPlayerDead && isDeadHealth(healthReceived);

	uint16_t healthLost = 0;
	uint16_t armorLost = 0;

	if (!playerDied && healthBefore > healthAfter)
		healthLost = healthBefore - healthAfter;

	if (armorBefore > armorAfter)
		armorLost = armorBefore - armorAfter;

	uint16_t totalLost = healthLost + armorLost;

	if (totalLost == 0 && !playerDied)
		return;

	float damageScale;
	uint32_t duration;

	if (playerDied)
	{
		damageScale = 1.0f;
		duration = 400;
	}
	else
	{
		damageScale = static_cast<float>(totalLost) / 50.0f;
		if (damageScale > 1.0f) damageScale = 1.0f;
		if (damageScale < 0.15f) damageScale = 0.15f;
		duration = static_cast<uint32_t>(80 + damageScale * 170);
	}

	float healthPenalty = (healthLost > 0 || playerDied) ? 1.2f : 1.0f;

	uint32_t rawIntensity = static_cast<uint32_t>((25000 + damageScale * 40000) * healthPenalty);

	bool isRightSide = (damageSector == 0 || damageSector == 1 || damageSector == 11);
	if (isRightSide)
		rawIntensity = static_cast<uint32_t>(rawIntensity * 1.3f);

	if (rawIntensity > 65535) rawIntensity = 65535;
	uint16_t baseIntensity = static_cast<uint16_t>(rawIntensity);

	uint16_t lowFreq, highFreq;

	/*  Damage Sector Map:
	 *          3
	 *       4     2
	 *     5         1
	 *    6           0
	 *     7         11
	 *       8     10
	 *          9
	 */
	if (damageSector >= 0)
	{
		switch (damageSector)
		{
			// Pure right (3 o'clock) - kill left motor completely
			case 0:
				lowFreq = 0;
				highFreq = baseIntensity;
				break;

			// Right bias (2 & 4 o'clock)
			case 1:
			case 11:
				lowFreq = static_cast<uint16_t>(baseIntensity * 0.1f);
				highFreq = baseIntensity;
				break;

			// Pure left (9 o'clock) - kill right motor completely
			case 6:
				lowFreq = baseIntensity;
				highFreq = 0;
				break;

			// Left bias (8 & 10 o'clock)
			case 5:
			case 7:
				lowFreq = baseIntensity;
				highFreq = static_cast<uint16_t>(baseIntensity * 0.1f);
				break;

			// Front (12 o'clock) - balanced impact
			case 2:
			case 3:
			case 4:
				lowFreq = static_cast<uint16_t>(baseIntensity * 0.6f);
				highFreq = static_cast<uint16_t>(baseIntensity * 0.6f);
				break;

			// Back (6 o'clock) - heavy thud
			case 8:
			case 9:
			case 10:
				lowFreq = static_cast<uint16_t>(baseIntensity * 0.8f);
				highFreq = static_cast<uint16_t>(baseIntensity * 0.2f);
				break;

			default:
				lowFreq = baseIntensity;
				highFreq = baseIntensity;
				break;
			}
	}
	else
	{
		lowFreq = baseIntensity;
		highFreq = baseIntensity;
	}

	SetGamepadRumble(lowFreq, highFreq, duration, playerDied ? 6 : 5);
}

static void __fastcall CHUDMgr_StartFlicker_Hook(DWORD* thisPtr, int, float fDuration)
{
	CHUDMgr_StartFlicker(thisPtr, fDuration);

	uint32_t durationMs;

	if (fDuration <= 0.0f)
	{
		durationMs = 1000;
	}
	else
	{
		durationMs = static_cast<uint32_t>(fDuration * 1000.0f);
	}

	SetGamepadRumble(0, 8000, durationMs, 1);
}

static bool __cdecl CClientWeapon_WeaponPath_OnImpactCB_Hook(DWORD* rImpactData, int a2)
{
	if (isDoingMeleeAttack && *rImpactData)
	{
		SetGamepadRumble(52000, 42000, 120, 3);
		isDoingMeleeAttack = false;
	}

	return CClientWeapon_WeaponPath_OnImpactCB(rImpactData, a2);
}

static void __fastcall HandleFallLand_Hook(DWORD* thisPtr, int, float fDistFell, int surfaceType)
{
	HandleFallLand(thisPtr, fDistFell, surfaceType);

	if (!RumbleEnabled || fDistFell < 275.0f)
		return;

	int liquidState = *(int*)((char*)thisPtr + 192);
	if (liquidState > 0 && liquidState <= 3)
		return;

	uint16_t lowFreq, highFreq;
	uint32_t duration;
	int priority;

	if (fDistFell < 500.0f)
	{
		float t = (fDistFell - 275.0f) / 225.0f;
		lowFreq = static_cast<uint16_t>(2000 + t * 3000);
		highFreq = 0;
		duration = static_cast<uint32_t>(60 + t * 40);
		priority = 4;
	}
	else
	{
		// 500+: Damage territory
		float t = (fDistFell - 500.0f) / 900.0f;
		if (t > 1.0f) t = 1.0f;

		float scaled = t * t;

		lowFreq = static_cast<uint16_t>(25000 + scaled * 40535);
		highFreq = static_cast<uint16_t>(15000 + scaled * 50535);
		duration = static_cast<uint32_t>(250 + scaled * 350);
		priority = 5;
		fallDamageTime = GetTickCount64();
	}

	SetGamepadRumble(lowFreq, highFreq, duration, priority);
}

void __fastcall CTurretFX_SetDamageState_Hook(DWORD* thisPtr, int)
{
	uint32_t newDamageState = *(uint32_t*)((char*)thisPtr + 124);

	CTurretFX_SetDamageState(thisPtr);

	if (!RumbleEnabled || !g_State.isOperatingTurret)
		return;

	if (newDamageState > turretPrevDamageState)
	{
		uint16_t lowFreq, highFreq;
		uint32_t duration;

		switch (newDamageState)
		{
			default:
			case 1:
				lowFreq = 58000;
				highFreq = 48000;
				duration = 600;
				break;
			case 2:
				lowFreq = 62000;
				highFreq = 52000;
				duration = 650;
				break;
			case 3:
				lowFreq = 65000;
				highFreq = 65000;
				duration = 700;
				break;
			case 4:
				lowFreq = 65535;
				highFreq = 65535;
				duration = 2000;
				break;
		}

		SetGamepadRumble(lowFreq, highFreq, duration, 6);
	}
	else
	{
		SetGamepadRumble(12000, 8000, 120, 3);
	}

	turretPrevDamageState = newDamageState;
}

static bool __fastcall OnCommandOn_Hook(int thisPtr, int, int command)
{
	// COMMAND_ID_SLOWMO
	if (command == 106)
		slowMoCommandTime = GetTickCount64();

	return ClientShellOnCommandOn(thisPtr, command);
}

// The server answers within a few frames, scripted slowmo and the one ended by a cutscene or a level change have no command
static bool IsSlowMoToggledByPlayer()
{
	bool isToggled = slowMoCommandTime != 0 && GetTickCount64() - slowMoCommandTime < 3000;
	slowMoCommandTime = 0;
	return isToggled;
}

static void __fastcall EnterSlowMo_Hook(int thisPtr, int, int hSlowMoRecord, bool bTransition, double fSlowMoPeriod, bool bPlayer)
{
	EnterSlowMo(thisPtr, hSlowMoRecord, bTransition, fSlowMoPeriod, bPlayer);

	if (RumbleEnabled && hSlowMoRecord && bPlayer && IsSlowMoToggledByPlayer())
		SetGamepadRumble(40000, 10000, 350, 2);
}

static void __fastcall ExitingSlowMo_Hook(int thisPtr, int)
{
	// Entering or in slowmo
	bool isInSlowMo = *(uint32_t*)(thisPtr + slowMoStateOffset) <= 1;

	bool isPlayerSlowMo = *(bool*)(thisPtr + slowMoPlayerOffset);

	ExitingSlowMo(thisPtr);

	if (RumbleEnabled && isInSlowMo && isPlayerSlowMo && IsSlowMoToggledByPlayer())
		SetGamepadRumble(12000, 20000, 200, 2);
}

// =====================================
//  SDLGamepadSupport & HUDScaling
// =====================================

static float GetMaxHUDCustomScalingFactor()
{
	float scalingFactor = std::max(1.0f, std::min(g_State.currentWidth / BASE_WIDTH, g_State.currentHeight / BASE_HEIGHT));

	// Armor next to the slow-mo bar and flashlight above the health, in the 640x480 layout
	return std::min(g_State.currentWidth / 640.0f * 1.07f, g_State.currentHeight / 480.0f * 1.38f) / scalingFactor;
}

static void UpdateHUDScaling()
{
	if (!HUDScaling && !EchoPatchMenu) return;

	// Calculate the new scaling factor
	g_State.scalingFactor = std::min(g_State.currentWidth / BASE_WIDTH, g_State.currentHeight / BASE_HEIGHT);

	// Don't downscale the HUD
	if (g_State.scalingFactor < 1.0f) g_State.scalingFactor = 1.0f;

	// Do not change the scaling of the crosshair
	g_State.scalingFactorCrosshair = g_State.scalingFactor;

	// Apply custom scaling for the text
	g_State.scalingFactorText = g_State.scalingFactor * SmallTextCustomScalingFactor;

	// Apply custom scaling to calculated scaling
	g_State.scalingFactor *= std::min(HUDCustomScalingFactor, GetMaxHUDCustomScalingFactor());

	// Turned off in the EchoPatch menu
	if (!HUDScaling)
	{
		g_State.scalingFactor = 1.0f;
		g_State.scalingFactorCrosshair = 1.0f;
		g_State.scalingFactorText = 1.0f;
	}

	// The scoreboard is sized in pixels, at most as large as it is at 640x480 so it stays on the screen
	g_State.scalingFactorScores = std::min(g_State.scalingFactorText, std::min(g_State.currentWidth / 640.0f, g_State.currentHeight / 480.0f));

	// Scale the crosshair lines thickness and the width of the pause message
	if (g_State.crosshairThicknessAddr != 0 && g_State.pausedWidthAddr != 0)
	{
		MemoryHelper::WriteMemory<float>(g_State.crosshairThicknessAddr, std::max(1.0f, std::round(g_State.scalingFactorCrosshair)));
		MemoryHelper::WriteMemory<int>(g_State.pausedWidthAddr, static_cast<int>(std::lround(640 * g_State.scalingFactor)));
	}

	// Scale the bottom padding of the scoreboard frames, it holds the rows once their text is scaled
	if (g_State.scoresPaddingAddr != 0)
	{
		MemoryHelper::WriteMemory<BYTE>(g_State.scoresPaddingAddr, static_cast<BYTE>(std::min(std::lround(16 * g_State.scalingFactorScores), 127L)));
	}

	// If the resolution is updated
	if (g_State.CHUDMgr != 0)
	{
		bool isHUDVisible = *(bool*)(g_State.CHUDMgr + 0x8);
		HUDItemsState hudItemsState = SaveHUDItemsState();

		// Reinitialize the HUD
		HUDTerminate(g_State.CHUDMgr);
		HUDInit(g_State.CHUDMgr);
		HUDWeaponListReset(g_State.CHUDWeaponList);
		HUDWeaponListUpdateTriggerNames(g_State.CHUDWeaponList);
		HUDGrenadeListUpdateTriggerNames(g_State.CHUDGrenadeList);
		HUDPausedInit(g_State.CHUDPaused);
		g_State.updateHUD = true;

		*(bool*)(g_State.CHUDMgr + 0x8) = isHUDVisible;
		RestoreHUDItemsState(hudItemsState);

		// Update the size of the crosshair
		SetConsoleVariableFloat("CrosshairSize", g_State.crosshairSize * g_State.scalingFactorCrosshair);
		SetConsoleVariableFloat("PerturbScale", g_State.perturbScale * g_State.scalingFactorCrosshair);
	}
}

static void __fastcall ScreenDimsChanged_Hook(int thisPtr, int)
{
	ScreenDimsChanged(thisPtr);

	// Get the current resolution
	g_State.currentWidth = *(DWORD*)(thisPtr + 0x18);
	g_State.currentHeight = *(DWORD*)(thisPtr + 0x1C);

	g_TouchpadConfig.currentWidth = g_State.currentWidth;
	g_TouchpadConfig.currentHeight = g_State.currentHeight;

	UpdateHUDScaling();
}

// ============================================
//  SDLGamepadSupport & ConsoleEnabled
// ============================================

static int __stdcall HookedWindowProc_Hook(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	// Toggle key, first in the chain so its character never reaches the game UI
	if (ConsoleEnabled && Console::HandleKeyMessage(Msg, wParam, lParam))
		return 0;

	// Block input to game UI when console is visible, the game input itself is disabled from the main loop
	if (g_State.isConsoleOpen)
	{
		switch (Msg)
		{
			// The game handles these without passing them down, ImGui gets them here instead
			case WM_MOUSEMOVE:
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_LBUTTONDBLCLK:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_RBUTTONDBLCLK:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
			case WM_MBUTTONDBLCLK:
			case WM_MOUSEWHEEL:
			case WM_CHAR:
				Console::ForwardInput(hWnd, Msg, wParam, lParam);
				return 0;

			// ImGui cursor shapes (text input, resize)
			case WM_SETCURSOR:
				if (Console::ForwardInput(hWnd, Msg, wParam, lParam))
					return 1;
				break;
		}
	}

	if (SDLGamepadSupport)
	{
		switch (Msg)
		{
			case WM_KEYDOWN:
				if ((lParam & SIMULATED_KEY_FLAG) != SIMULATED_KEY_FLAG)
					OnKeyboardMouseInput();
			break;
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN:
			{
				if (!IsTouchpadRecentlyUsed())
					OnKeyboardMouseInput();
				break;
			}
		}
	}

	return HookedWindowProc(hWnd, Msg, wParam, lParam);
}

static bool __fastcall ChangeState_Hook(int thisPtr, int, int state, int screenId)
{
	bool result = ChangeState(thisPtr, state, screenId);
	g_State.isPlaying = *(int*)(thisPtr + 0x8) == 1;
	return result;
}

static void __fastcall MsgBoxShow_Hook(int thisPtr, int, const wchar_t* pString, int pCreate, int nFontSize, bool bDefaultReturn)
{
	MsgBoxShow(thisPtr, pString, pCreate, nFontSize, bDefaultReturn);
	g_State.isMsgBoxVisible = *(BYTE*)(thisPtr + 0x578);
}

static void __fastcall MsgBoxHide_Hook(int thisPtr, int, int a2)
{
	MsgBoxHide(thisPtr, a2);
	g_State.isMsgBoxVisible = *(BYTE*)(thisPtr + 0x578);
}

// ===========================================================
//  SDLGamepadSupport & FPSLimitMenuOption & EchoPatchMenu
// ===========================================================

static int stringDatabase = 0;

static const char* const inputStrings[] = { "IDS_QUICKSAVE", "ScreenFailure_PressAnyKey" };

static const wchar_t* GetControllerString(uint32_t strHash)
{
	switch (strHash)
	{
		case HashHelper::StringHashes::IDS_QUICKSAVE:
			return L"Quick save";

		case HashHelper::StringHashes::ScreenFailure_PressAnyKey:
			if (wchar_t glyph = GetGamepadButtonGlyph(false, GetGamepadMenuButton(false)))
			{
				static wchar_t text[96];
				swprintf_s(text, L"Press %c to return to the main menu.\nPress any other button to continue.", glyph);
				return text;
			}

			switch (GetGamepadStyle())
			{
				case GamepadStyle::PlayStation:
					return L"Press Circle to return to the main menu.\nPress any other button to continue.";
				case GamepadStyle::Nintendo:
					return L"Press B to return to the main menu.\nPress any other button to continue.";
				case GamepadStyle::Xbox:
				default:
					return L"Press B to return to the main menu.\nPress any other button to continue.";
			}
	}

	return nullptr;
}

static const wchar_t* __stdcall LoadGameString_Hook(int ptr, char* String)
{
	const uint32_t strHash = HashHelper::FNV1aRuntime(String);
	stringDatabase = ptr;

	switch (strHash)
	{
		case HashHelper::StringHashes::IDS_CONTROLLER_SENSITIVITY:			return L"Controller Sensitivity";
		case HashHelper::StringHashes::IDS_EDGE_ACCELERATION:				return L"Edge Acceleration";
		case HashHelper::StringHashes::IDS_GYRO_SENSITIVITY:				return L"Gyro Sensitivity";
		case HashHelper::StringHashes::IDS_GYRO_SMOOTHING:					return L"Gyro Smoothing";
		case HashHelper::StringHashes::IDS_HELP_CONTROLLER_SENSITIVITY:		return L"Adjust the sensitivity of the analog sticks.";
		case HashHelper::StringHashes::IDS_HELP_EDGE_ACCELERATION:			return L"Adjust the turn rate multiplier when the stick is pushed to the edge.";
		case HashHelper::StringHashes::IDS_HELP_RUMBLE:						return L"Enable or disable controller vibration feedback.";
		case HashHelper::StringHashes::IDS_HELP_GYRO_ENABLED:				return L"Enable or disable gyroscope aiming.";
		case HashHelper::StringHashes::IDS_HELP_GYRO_TYPE:					return L"Choose when gyro aiming is active.";
		case HashHelper::StringHashes::IDS_HELP_GYRO_PERSISTENCE_ENABLED:   return L"Automatically save and load gyro calibration data upon reconnection.";
		case HashHelper::StringHashes::IDS_HELP_GYRO_SENSITIVITY:			return L"Adjust the sensitivity of the gyroscope.";
		case HashHelper::StringHashes::IDS_HELP_GYRO_SMOOTHING:				return L"Adjust smoothing applied to gyroscope input.";
		case HashHelper::StringHashes::IDS_HELP_TOUCHPAD:					return L"Enable or disable touchpad functionality.";
		case HashHelper::StringHashes::IDS_HOLD_DURATION:					return L"Hold Duration";
		case HashHelper::StringHashes::IDS_HELP_BUTTON_MAPPING:				return L"Change the actions assigned to the controller buttons.";
		case HashHelper::StringHashes::IDS_HELP_BINDINGS_FACE:				return L"Change the actions assigned to the face buttons.";
		case HashHelper::StringHashes::IDS_HELP_BINDINGS_SHOULDERS:			return L"Change the actions assigned to the shoulder buttons and triggers.";
		case HashHelper::StringHashes::IDS_HELP_BINDINGS_DPAD:				return L"Change the actions assigned to the directional pad.";
		case HashHelper::StringHashes::IDS_HELP_BINDINGS_STICKS:			return L"Change the actions assigned to the stick buttons and menu buttons.";
		case HashHelper::StringHashes::IDS_HELP_BINDINGS_EXTRA:				return L"Change the actions assigned to the share/capture button and the back paddles.";
		case HashHelper::StringHashes::IDS_HELP_BINDING_PRESS:				return L"Action performed when the button is pressed.";
		case HashHelper::StringHashes::IDS_HELP_BINDING_HOLD:				return L"Action performed when the button is held. A button with a hold action performs its press action on release.";
		case HashHelper::StringHashes::IDS_HELP_HOLD_DURATION:				return L"How long a button must be held to perform its hold action. Applies to every button.";
		case HashHelper::StringHashes::IDS_HELP_RESTORE_BINDINGS:			return L"Restore the default controller button mapping.";
		case HashHelper::StringHashes::IDS_HELP_FPS_LIMIT:					return L"Maximum framerate when V Sync is on.";

		case HashHelper::StringHashes::IDS_SUPERSAMPLING:					return L"Supersampling";
		case HashHelper::StringHashes::IDS_FIELD_OF_VIEW:					return L"Field of View";
		case HashHelper::StringHashes::IDS_HUD_SIZE:						return L"HUD Size";
		case HashHelper::StringHashes::IDS_SMALL_TEXT_SIZE:					return L"Small Text Size";
		case HashHelper::StringHashes::IDS_SENSITIVITY_MULTIPLIER:			return L"Sensitivity Multiplier";
		case HashHelper::StringHashes::IDS_WEAPON_CAPACITY:					return L"Weapon Capacity";
		case HashHelper::StringHashes::IDS_GIB_FREQUENCY_CAP:				return L"Gib Frequency Cap";
		case HashHelper::StringHashes::IDS_HELP_ECHOPATCH:					return L"Change the settings of EchoPatch.";
		case HashHelper::StringHashes::IDS_HELP_ECHOPATCH_GRAPHICS:			return L"Reflections, supersampling, model detail and persistent world.";
		case HashHelper::StringHashes::IDS_HELP_ECHOPATCH_DISPLAY:			return L"Field of view, HUD scaling and cutscene letterbox.";
		case HashHelper::StringHashes::IDS_HELP_ECHOPATCH_MOUSE:			return L"Mouse sensitivity multiplier.";
		case HashHelper::StringHashes::IDS_HELP_ECHOPATCH_MEMORY:			return L"Memory usage of the game.";
		case HashHelper::StringHashes::IDS_HELP_ECHOPATCH_EXTRA:			return (g_State.CurrentFEARGame == FEAR) ? L"Flashlight, weapon capacity, door bashing, dismemberment and accuracy." : L"Flashlight, weapon capacity, dismemberment and accuracy.";		case HashHelper::StringHashes::IDS_HELP_HIGH_RESOLUTION_REFLECTIONS:	return L"Sharper reflections and security camera screens.";
		case HashHelper::StringHashes::IDS_HELP_SUPERSAMPLING:				return L"Renders the 3D scene at a higher resolution. Turns off FSAA.";
		case HashHelper::StringHashes::IDS_HELP_MAXIMUM_MODEL_DETAIL:		return L"Always uses the most detailed models.";
		case HashHelper::StringHashes::IDS_HELP_PERSISTENT_WORLD:			return L"Blood stains, bullet holes, debris and shell casings no longer disappear. Requires a restart.";
		case HashHelper::StringHashes::IDS_HELP_SAVE_PERSISTENT_WORLD:		return L"Keeps the persistent world when loading a save. Requires a restart.";
		case HashHelper::StringHashes::IDS_HELP_FIELD_OF_VIEW:				return L"Field of view of the player.";
		case HashHelper::StringHashes::IDS_HELP_HUD_SCALING:				return L"Scales the HUD with the resolution.";
		case HashHelper::StringHashes::IDS_HELP_HUD_SIZE:
		{
			static wchar_t text[96];
			swprintf_s(text, L"Size of the HUD when HUD Scaling is on, up to %.2fx at this resolution.", std::floor(GetMaxHUDCustomScalingFactor() * 100.0f) / 100.0f);
			return (g_State.currentWidth != 0) ? text : L"Size of the HUD when HUD Scaling is on.";
		}
		case HashHelper::StringHashes::IDS_HELP_SMALL_TEXT_SIZE:			return L"Size of the subtitles and other small texts when HUD Scaling is on.";
		case HashHelper::StringHashes::IDS_HELP_LETTERBOX:					return L"Black bars during cutscenes.";
		case HashHelper::StringHashes::IDS_HELP_SENSITIVITY_MULTIPLIER:		return L"Multiplies the mouse sensitivity of the game.";
		case HashHelper::StringHashes::IDS_HELP_REDUCE_TEXTURE_MEMORY:		return L"Experimental and potentially unstable. Keeps textures in video memory only. Requires a restart.";
		case HashHelper::StringHashes::IDS_HELP_4GB_PATCH:					return L"Lets the game use 4GB of memory instead of 2GB.";
		case HashHelper::StringHashes::IDS_HELP_INFINITE_FLASHLIGHT:		return L"The flashlight never runs out of battery. Requires a restart.";
		case HashHelper::StringHashes::IDS_HELP_CUSTOM_WEAPON_CAPACITY:		return L"Lets Weapon Capacity change how many weapons can be carried. Requires a restart.";
		case HashHelper::StringHashes::IDS_HELP_WEAPON_CAPACITY:			return L"How many weapons can be carried. Lowering it needs a new save from a new game. Requires a restart.";
		case HashHelper::StringHashes::IDS_HELP_DOOR_BASHING:				return L"Melee attacks and explosions bash doors open, like in the expansions. Requires a restart.";
		case HashHelper::StringHashes::IDS_HELP_UNCAP_DISMEMBERMENT:		return L"Removes the limits on how often bodies get dismembered and how many dismembered bodies a level can have. Requires a restart.";
		case HashHelper::StringHashes::IDS_HELP_GIB_FREQUENCY_CAP:			return L"How many kills have to pass after a body gibs before another body can gib. 0 removes the limit, the game uses 5. Requires a restart.";
		case HashHelper::StringHashes::IDS_HELP_ACCURACY_PENALTY:			return L"Off removes the spread from moving, turning and recoil, as if zoomed in and standing still. Requires a restart.";

		case HashHelper::StringHashes::IDS_SELECT:							return L"Select";
		case HashHelper::StringHashes::IDS_CATEGORY:						return L"Category";

		case HashHelper::StringHashes::IDS_QUICKSAVE:
		case HashHelper::StringHashes::ScreenFailure_PressAnyKey:
			if (g_State.hasButtonGlyphs)
			{
				static wchar_t s_inputStringChars[std::size(inputStrings)][2];

				for (int i = 0; i < (int)std::size(inputStrings); i++)
				{
					if (HashHelper::FNV1aRuntime(inputStrings[i]) == strHash)
					{
						s_inputStringChars[i][0] = INPUT_STRING_CHAR + i;
						return s_inputStringChars[i];
					}
				}
			}

			if (ShouldShowControllerPrompts())
			{
				return GetControllerString(strHash);
			}
			break;
		}

	return LoadGameString(ptr, String);
}

static bool __stdcall DEditLoadModule_Hook(const char* pszProjectPath)
{
	// Load StringEditRuntime.dll
	bool res = DEditLoadModule(pszProjectPath);

	// Get and hook 'LoadString'
	DWORD addr = ScanModuleSignature(g_State.GameClient, "8B 4C 24 18 03 C1 8B 0D ?? ?? ?? ?? 03 F7 85 C9", "LoadString", -1, false);

	if (addr != 0)
	{
		int StringEditRuntimePtr = MemoryHelper::ReadMemory<int>(addr + 0x8);
		int StringEditRuntime = MemoryHelper::ReadMemory<int>(StringEditRuntimePtr);
		int vTable = MemoryHelper::ReadMemory<int>(StringEditRuntime);
		int pLoadString = MemoryHelper::ReadMemory<int>(vTable + 0x1C);

		HookHelper::ApplyHook((void*)pLoadString, &LoadGameString_Hook, (LPVOID*)&LoadGameString);
	}

	return res;
}
