#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"
#include "../../Controller/ScreenJoystickHook.hpp"
#include "../../ClientFX/ClientFX.hpp"
#include "../../Server/Server.hpp"

// =======================
// SDLGamepadSupport
// =======================

static bool __fastcall IsCommandOn_Hook(int thisPtr, int, int commandId)
{
	return (commandId < MAX_COMMAND_ID + 1 && g_Controller.commandActive[commandId]) || IsCommandOn(thisPtr, commandId);
}

static double __fastcall GetExtremalCommandValue_Hook(int thisPtr, int, int commandId)
{
	if (!g_Controller.isConnected || g_State.isConsoleOpen)
		return GetExtremalCommandValue(thisPtr, commandId);

	SDL_Gamepad* pGamepad = GetGamepad();
	if (!pGamepad)
		return GetExtremalCommandValue(thisPtr, commandId);

	static constexpr int DEAD_ZONE = 7849;

	auto ReadAxis = [&](SDL_GamepadAxis axis) -> double
	{
		Sint16 value = SDL_GetGamepadAxis(pGamepad, axis);
		if (abs(value) < DEAD_ZONE)
			return 0.0;
		return value / 32768.0;
	};

	auto ApplyZoomScale = [](double value) -> double
	{
		if (g_State.zoomMag > GPadZoomMagThreshold)
			return value * (g_State.zoomMag / GPadZoomMagThreshold);
		return value;
	};

	switch (commandId)
	{
		case 2:  return -ReadAxis(SDL_GAMEPAD_AXIS_LEFTY);                      // Forward
		case 5:  return  ReadAxis(SDL_GAMEPAD_AXIS_LEFTX);                      // Strafe
		case 22: return  ApplyZoomScale(ReadAxis(SDL_GAMEPAD_AXIS_RIGHTY));     // Pitch
		case 23: return  ApplyZoomScale(ReadAxis(SDL_GAMEPAD_AXIS_RIGHTX));     // Yaw
		default: return  GetExtremalCommandValue(thisPtr, commandId);
	}
}

static double __fastcall GetZoomMag_Hook(int thisPtr)
{
	g_State.zoomMag = GetZoomMag(thisPtr);
	return g_State.zoomMag;
}

static int __fastcall HUDActivateObjectSetObject_Hook(int thisPtr, int, void** a2, int a3, int a4, int a5, int nNewType)
{
	// If we can open a door or pick up an item
	g_State.canActivate = nNewType != -1;
	return HUDActivateObjectSetObject(thisPtr, a2, a3, a4, a5, nNewType);
}

static int __fastcall SetOperatingTurret_Hook(int thisPtr, int, int pTurret)
{
	// Operating a turret?
	g_State.isOperatingTurret = pTurret != 0;

	if (!pTurret)
	{
		g_State.turretPrevDamageState = 0;
	}

	return SetOperatingTurret(thisPtr, pTurret);
}

static const wchar_t* __fastcall GetTriggerNameFromCommandID_Hook(int thisPtr, int, int commandId)
{
	if (!ShouldShowControllerPrompts() || g_State.isLoadingDefault)
		return GetTriggerNameFromCommandID(thisPtr, commandId);

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
		g_State.lastCursorStateChangeTime = GetTickCount64();
	}

	if (ShouldShowControllerPrompts() && !g_Controller.touchpadCursorActive)
	{
		bUseCursor = false;
	}

	UseCursor(thisPtr, bUseCursor, bLockCursorToCenter);
}

static bool __fastcall OnMouseMove_Hook(int thisPtr, int, int x, int y)
{
	if (ShouldShowControllerPrompts())
	{
		if (!IsTouchpadRecentlyUsed())
		{
			if (g_State.isAllowedToUseCursor && (x != 0 || y != 0))
			{
				ULONGLONG now = GetTickCount64();

				if ((now - g_State.lastCursorStateChangeTime) > 500)
				{
					if (now - g_State.cursorActivityStartTime > 500)
					{
						g_State.cursorMovementAccum = 0;
						g_State.cursorActivityStartTime = now;
					}

					g_State.cursorMovementAccum += abs(x) + abs(y);

					if (g_State.cursorMovementAccum > 150)
					{
						g_State.cursorMovementAccum = 0;
						g_State.cursorActivityStartTime = 0;
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
	g_State.updateGyroCamera = true;
	UpdatePlayerMovement(thisPtr);
	g_State.updateGyroCamera = false;
}

static void __fastcall ApplyLocalRotationOffset_Hook(int thisPtr, int, float* vPYROffset)
{
	if (g_State.updateGyroCamera && g_Controller.isConnected && IsGyroEnabled() && !g_State.isConsoleOpen)
	{
		bool shouldApplyGyro = (GyroAimingMode == 0) 
			|| (GyroAimingMode == 1 && g_State.isAiming) 
			|| (GyroAimingMode == 2 && !g_State.isAiming);

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
	g_State.isAiming = *thisPtr;
}

static void __fastcall EndAim_Hook(BYTE* thisPtr, int)
{
	EndAim(thisPtr);
	g_State.isAiming = *thisPtr;
}

static void __fastcall CClientWeaponFire_Hook(DWORD* thisPtr, int)
{
	g_State.isDoingMeleeAttack = false;
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
					g_State.isDoingMeleeAttack = true;
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
	if (g_State.isTakingDamage)
	{
		g_State.healthBefore = static_cast<uint16_t>(thisPtr[1]);
	}

	unsigned int result = UpdateHealth(thisPtr, newHealth);

	if (g_State.isTakingDamage)
	{
		g_State.healthAfter = static_cast<uint16_t>(thisPtr[1]);
	}

	return result;
}

static int __fastcall UpdateArmor_Hook(DWORD* thisPtr, int, unsigned int newArmor)
{
	if (g_State.isTakingDamage)
	{
		g_State.armorBefore = static_cast<uint16_t>(thisPtr[2]);
	}

	int result = UpdateArmor(thisPtr, newArmor);

	if (g_State.isTakingDamage)
	{
		g_State.armorAfter = static_cast<uint16_t>(thisPtr[2]);
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

	g_State.isTakingDamage = true;
	g_State.healthBefore = 0;
	g_State.healthAfter = 0;
	g_State.armorBefore = 0;
	g_State.armorAfter = 0;

	float damageBefore[12];
	float* damageArray = (float*)((BYTE*)thisPtr + 428);
	memcpy(damageBefore, damageArray, sizeof(damageBefore));

	HandleMsgPlayerDamage(thisPtr, a2);

	g_State.isTakingDamage = false;

	if (g_State.isFallDamage)
	{
		g_State.isFallDamage = false;
		return;
	}

	bool playerDied = (g_State.healthAfter == 0) || (g_State.healthAfter > g_State.healthBefore);

	uint16_t healthLost = 0;
	uint16_t armorLost = 0;

	if (!playerDied && g_State.healthBefore > g_State.healthAfter)
		healthLost = g_State.healthBefore - g_State.healthAfter;

	if (g_State.armorBefore > g_State.armorAfter)
		armorLost = g_State.armorBefore - g_State.armorAfter;

	uint16_t totalLost = healthLost + armorLost;

	if (totalLost == 0 && !playerDied)
		return;

	float maxDelta = 0.0f;
	int damageSector = -1;

	for (int i = 0; i < 12; i++)
	{
		float delta = damageArray[i] - damageBefore[i];
		if (delta > maxDelta && delta > 0.01f)
		{
			maxDelta = delta;
			damageSector = i;
		}
	}

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
	if (g_State.isDoingMeleeAttack && *rImpactData)
	{
		SetGamepadRumble(52000, 42000, 120, 3);
		g_State.isDoingMeleeAttack = false;
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
		g_State.isFallDamage = true;
	}

	SetGamepadRumble(lowFreq, highFreq, duration, priority);
}

void __fastcall CTurretFX_SetDamageState_Hook(DWORD* thisPtr, int)
{
	uint32_t newDamageState = *(uint32_t*)((char*)thisPtr + 124);

	CTurretFX_SetDamageState(thisPtr);

	if (!RumbleEnabled || !g_State.isOperatingTurret)
		return;

	if (newDamageState > g_State.turretPrevDamageState)
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

	g_State.turretPrevDamageState = newDamageState;
}

static const wchar_t* __stdcall LoadGameString_Hook(int ptr, char* String)
{
	const uint32_t strHash = HashHelper::FNV1aRuntime(String);

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

		case HashHelper::StringHashes::IDS_QUICKSAVE:
			if (ShouldShowControllerPrompts())
			{
				return L"Quick save";
			}
			break;

		case HashHelper::StringHashes::ScreenFailure_PressAnyKey:
			if (ShouldShowControllerPrompts())
			{
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

// =====================================
//  SDLGamepadSupport & HUDScaling
// =====================================

static void __fastcall ScreenDimsChanged_Hook(int thisPtr, int)
{
	ScreenDimsChanged(thisPtr);

	// Get the current resolution
	g_State.currentWidth = *(DWORD*)(thisPtr + 0x18);
	g_State.currentHeight = *(DWORD*)(thisPtr + 0x1C);

	g_TouchpadConfig.currentWidth = g_State.currentWidth;
	g_TouchpadConfig.currentHeight = g_State.currentHeight;

	if (!HUDScaling) return;

	// Calculate the new scaling factor
	g_State.scalingFactor = std::sqrt((g_State.currentWidth * g_State.currentHeight) / BASE_AREA);

	// Don't downscale the HUD
	if (g_State.scalingFactor < 1.0f) g_State.scalingFactor = 1.0f;

	// Do not change the scaling of the crosshair
	g_State.scalingFactorCrosshair = g_State.scalingFactor;

	// Apply custom scaling for the text
	g_State.scalingFactorText = g_State.scalingFactor * SmallTextCustomScalingFactor;

	// Apply custom scaling to calculated scaling
	g_State.scalingFactor *= HUDCustomScalingFactor;

	// Reset slow-mo bar update flag and HUDHealth.AdditionalInt index
	g_State.slowMoBarUpdated = false;
	g_State.healthAdditionalIntIndex = 0;

	// If the resolution is updated
	if (g_State.CHUDMgr != 0)
	{
		// Reinitialize the HUD
		HUDTerminate(g_State.CHUDMgr);
		HUDInit(g_State.CHUDMgr);
		HUDWeaponListReset(g_State.CHUDWeaponList);
		HUDWeaponListUpdateTriggerNames(g_State.CHUDWeaponList);
		HUDGrenadeListUpdateTriggerNames(g_State.CHUDGrenadeList);
		HUDPausedInit(g_State.CHUDPaused);
		g_State.updateHUD = true;

		// Update the size of the crosshair
		SetConsoleVariableFloat("CrosshairSize", g_State.crosshairSize * g_State.scalingFactorCrosshair);
		SetConsoleVariableFloat("PerturbScale", 0.5f * g_State.scalingFactorCrosshair);
	}
}

// ============================================
//  SDLGamepadSupport & ConsoleEnabled
// ============================================

static int __stdcall HookedWindowProc_Hook(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	// Block input to game UI when console is visible
	if (g_State.isConsoleOpen)
	{
		if (!g_State.wasConsoleOpened)
		{
			SetInputState(false);
			g_State.wasConsoleOpened = true;
		}

		if (Msg == WM_MOUSEWHEEL)
		{
			ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam);
			return 0;
		}

		switch (Msg)
		{
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
				return 0;
		}
	}
	else if (g_State.wasConsoleOpened)
	{
		SetInputState(!g_State.wasInputDisabled);
		g_State.wasConsoleOpened = false;
	}

	if (SDLGamepadSupport)
	{
		switch (Msg)
		{
			case WM_KEYDOWN:
				if (g_Controller.simulatedKeyPressCount > 0)
					g_Controller.simulatedKeyPressCount--;
				else
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
	g_State.isPlaying = state == 1;
	return ChangeState(thisPtr, state, screenId);
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
