#pragma once

#include "../Globals.cpp"
#include "../Controller/Controller.hpp"
#include "../Controller/ScreenJoystickHook.hpp"
#include "../ClientFX/ClientFX.hpp"
#include "../Server/Server.hpp"

static void ApplyHighFPSFixesClientPatch()
{
    if (!HighFPSFixes) return;

    DWORD addr_SurfaceJumpImpulse = ScanModuleSignature(g_State.GameClient, "C7 44 24 1C 00 00 00 00 C7 44 24 10 00 00 00 00 EB", "SurfaceJumpImpulse");
    DWORD addr_HeightOffset = ScanModuleSignature(g_State.GameClient, "D9 E1 D9 54 24 1C D8 1D ?? ?? ?? ?? DF E0 F6 C4 41 0F 85", "HeightOffset");
    DWORD addr_UpdateOnGround = ScanModuleSignature(g_State.GameClient, "83 EC 3C 53 55 56 57 8B F1", "UpdateOnGround");
    DWORD addr_UpdateWaveProp = ScanModuleSignature(g_State.GameClient, "D9 44 24 04 83 EC ?? D8 1D", "UpdateWaveProp");
    DWORD addr_UpdateNormalFriction = ScanModuleSignature(g_State.GameClient, "83 EC 3C 56 8B F1 8B 46 28 F6 C4 08 C7 44 24 34", "UpdateNormalFriction");
    DWORD addr_GetTimerElapsedS = ScanModuleSignature(g_State.GameClient, "04 51 8B C8 FF 52 3C 85 C0 5E", "GetTimerElapsedS");
    DWORD addr_GetMaxRecentVelocityMag = ScanModuleSignature(g_State.GameClient, "F6 C4 41 75 2F 8D 8E 34 04 00 00 E8", "GetMaxRecentVelocityMag");
    DWORD addr_UpdateNormalControlFlags = ScanModuleSignature(g_State.GameClient, "55 8B EC 83 E4 F8 83 EC 18 53 55 56 57 8B F1 E8", "UpdateNormalControlFlags");
    DWORD addr_PolyGridFXCollisionHandlerCB = ScanModuleSignature(g_State.GameClient, "83 EC 54 53 33 DB 3B CB ?? 74 05", "PolyGridFXCollisionHandlerCB");
	DWORD addr_HandleMsgSlowMo = ScanModuleSignature(g_State.GameClient, "55 8B EC 83 E4 F8 81 EC 34 01 00 00 53 56 8B 75 08", "HandleMsgSlowMo");
	DWORD addr_MoveLocalSolidObject = ScanModuleSignature(g_State.GameClient, "81 EC A4 00 00 00 57 8B F9 8B 8F D8 03 00 00 E8", "MoveLocalSolidObject");
	DWORD addr_EnterSlowMo = ScanModuleSignature(g_State.GameClient, "DD 05 ?? ?? ?? ?? 89 86 18 03 00 00", "EnterSlowMo", 3);
	DWORD addr_UpdateSlowMo = ScanModuleSignature(g_State.GameClient, "83 EC 08 56 8B F1 8B 86 80 02 00 00 83 F8 01", "UpdateSlowMo");
    addr_GetMaxRecentVelocityMag = MemoryHelper::ResolveRelativeAddress(addr_GetMaxRecentVelocityMag, 0xC);

    if (addr_SurfaceJumpImpulse == 0 ||
        addr_HeightOffset == 0 ||
        addr_UpdateOnGround == 0 ||
        addr_GetMaxRecentVelocityMag == 0 ||
        addr_UpdateNormalControlFlags == 0 ||
        addr_UpdateNormalFriction == 0 ||
        addr_GetTimerElapsedS == 0 ||
        addr_UpdateWaveProp == 0 ||
		addr_PolyGridFXCollisionHandlerCB == 0 ||
		addr_HandleMsgSlowMo == 0 ||
		addr_EnterSlowMo == 0 ||
		addr_UpdateSlowMo == 0 ||
		addr_MoveLocalSolidObject == 0) {
        return;
    }

    MemoryHelper::MakeNOP(addr_SurfaceJumpImpulse, 0x10);
	MemoryHelper::WriteMemory<uint16_t>(addr_HeightOffset + 0x11, 0xE990);
    HookHelper::ApplyHook((void*)addr_GetMaxRecentVelocityMag, &GetMaxRecentVelocityMag_Hook, (LPVOID*)&GetMaxRecentVelocityMag);
    HookHelper::ApplyHook((void*)addr_UpdateNormalControlFlags, &UpdateNormalControlFlags_Hook, (LPVOID*)&UpdateNormalControlFlags);
    HookHelper::ApplyHook((void*)addr_UpdateOnGround, &UpdateOnGround_Hook, (LPVOID*)&UpdateOnGround);
    HookHelper::ApplyHook((void*)addr_UpdateWaveProp, &UpdateWaveProp_Hook, (LPVOID*)&UpdateWaveProp);
    HookHelper::ApplyHook((void*)addr_UpdateNormalFriction, &UpdateNormalFriction_Hook, (LPVOID*)&UpdateNormalFriction);
    HookHelper::ApplyHook((void*)(addr_GetTimerElapsedS - 0x20), &GetTimerElapsedS_Hook, (LPVOID*)&GetTimerElapsedS);
    HookHelper::ApplyHook((void*)(addr_PolyGridFXCollisionHandlerCB - 0x6), &PolyGridFXCollisionHandlerCB_Hook, (LPVOID*)&PolyGridFXCollisionHandlerCB);
	HookHelper::ApplyHook((void*)(addr_MoveLocalSolidObject), &MoveLocalSolidObject_Hook, (LPVOID*)&MoveLocalSolidObject);
	HookHelper::ApplyHook((void*)(addr_HandleMsgSlowMo), &HandleMsgSlowMo_Hook,(LPVOID*)&HandleMsgSlowMo);
	HookHelper::ApplyHook((void*)(addr_EnterSlowMo), &EnterSlowMo_Hook, (LPVOID*)&EnterSlowMo);
	HookHelper::ApplyHook((void*)(addr_UpdateSlowMo),&UpdateSlowMo_Hook,(LPVOID*)&UpdateSlowMo);
}

static void ApplyMouseAimMultiplierClientPatch()
{
    if (MouseAimMultiplier == 1.0f) return;

    DWORD addr_MouseAimMultiplier = ScanModuleSignature(g_State.GameClient, "89 4C 24 14 DB 44 24 14 8D 44 24 20 6A 01 50 D8 0D", "MouseAimMultiplier");

    if (addr_MouseAimMultiplier != 0)
    {
        // Write the updated multiplier
        g_State.overrideSensitivity = g_State.overrideSensitivity * MouseAimMultiplier;
        MemoryHelper::WriteMemory<uint32_t>(addr_MouseAimMultiplier + 0x11, reinterpret_cast<uintptr_t>(&g_State.overrideSensitivity));
    }
}

static void ApplyXPWidescreenClientPatch()
{
    if (DisableXPWidescreenFiltering && g_State.CurrentFEARGame == FEARXP)
    {
        // Disable non-4:3 filtering
        MemoryHelper::MakeNOP((DWORD)g_State.GameClient + 0x10DDB0, 24);
    }
}

static void ApplySkipSplashScreenClientPatch()
{
    if (!SkipSplashScreen) return;

    DWORD addr = ScanModuleSignature(g_State.GameClient, "53 8B 5C 24 08 55 8B 6C 24 14 56 8D 43 FF 83 F8", "SkipSplashScreen");

    if (addr != 0)
    {
        MemoryHelper::MakeNOP(addr + 0x13D, 8);
    }
}

static void ApplyDisableLetterboxClientPatch()
{
    if (!DisableLetterbox) return;

    DWORD addr = ScanModuleSignature(g_State.GameClient, "83 EC 54 53 55 56 57 8B", "DisableLetterbox");

    if (addr != 0)
    {
        MemoryHelper::WriteMemory<uint8_t>(addr, 0xC3);
    }
}

static void ApplyPersistentWorldClientPatch()
{
    if (!EnablePersistentWorldState) return;

    DWORD addr_ShellCasing = ScanModuleSignature(g_State.GameClient, "D9 86 88 00 00 00 D8 64 24", "ShellCasing");
    DWORD addr_DecalSaving = ScanModuleSignature(g_State.GameClient, "FF 52 0C ?? 8D ?? ?? ?? 00 00 E8 ?? ?? ?? FF 8B", "DecalSaving");
    DWORD addr_Decal = ScanModuleSignature(g_State.GameClient, "DF E0 F6 C4 01 75 34 DD 44 24", "Decal");
    DWORD addr_FX = ScanModuleSignature(g_State.GameClient, "8B CE FF ?? 04 84 C0 75 ?? 8B ?? 8B CE FF ?? 08 56 E8", "CreateFX", 1);
    DWORD addr_Shatter = ScanModuleSignature(g_State.GameClient, "8B C8 E8 ?? ?? ?? 00 D9 5C 24 ?? D9", "Shatter");
    addr_Shatter = MemoryHelper::ResolveRelativeAddress(addr_Shatter, 0x3);

    if (addr_ShellCasing == 0 ||
        addr_DecalSaving == 0 ||
        addr_Decal == 0 ||
        addr_FX == 0 ||
        addr_Shatter == 0) {
        return;
    }

    MemoryHelper::MakeNOP(addr_ShellCasing + 0x6, 4);
    MemoryHelper::MakeNOP(addr_DecalSaving + 0xF, 13);
    MemoryHelper::WriteMemory<uint8_t>(addr_Decal + 0x5, 0xEB);
    HookHelper::ApplyHook((void*)addr_Shatter, &GetShatterLifetime_Hook, (LPVOID*)&GetShatterLifetime);
    HookHelper::ApplyHook((void*)addr_FX, &CreateFX_Hook, (LPVOID*)&CreateFX);
}

static void ApplyInfiniteFlashlightClientPatch()
{
    if (!InfiniteFlashlight) return;

    DWORD addr_Update = ScanModuleSignature(g_State.GameClient, "8B 51 10 8A 42 18 84 C0 8A 86 04 01 00 00", "InfiniteFlashlight_Update");
    DWORD addr_UpdateBar = ScanModuleSignature(g_State.GameClient, "A1 ?? ?? ?? ?? 85 C0 56 8B F1 74 71 D9 86 1C 04 00 00", "InfiniteFlashlight_UpdateBar");
    DWORD addr_UpdateLayout = ScanModuleSignature(g_State.GameClient, "68 ?? ?? ?? ?? 6A 00 68 ?? ?? ?? ?? 50 FF 57 58 8B 0D ?? ?? ?? ?? 50 FF 97 84 00 00 00 8B 0D ?? ?? ?? ?? 8B 11 50 8D 44 24 10 50 FF 52 04 8B 4C 24 0C 8D BE C4 01 00 00", "InfiniteFlashlight_UpdateLayout");
    DWORD addr_Battery = ScanModuleSignature(g_State.GameClient, "D8 4C 24 04 DC AE 88 03 00 00 DD 96 88 03 00 00", "InfiniteFlashlight_Battery");

    if (addr_Update == 0 ||
        addr_UpdateBar == 0 ||
        addr_UpdateLayout == 0 ||
        addr_Battery == 0) {
        return;
    }

    MemoryHelper::WriteMemory<uint8_t>(addr_Update - 0x31, 0xC3);
    MemoryHelper::WriteMemory<uint8_t>(addr_UpdateLayout - 0x36, 0xC3);
    MemoryHelper::WriteMemory<uint8_t>(addr_UpdateBar, 0xC3);
    MemoryHelper::MakeNOP(addr_Battery + 0xA, 6);
}

static void ApplyControllerClientPatch()
{
    if (!SDLGamepadSupport) return;

    DWORD addr_pGameClientShell = ScanModuleSignature(g_State.GameClient, "C1 F8 02 C1 E0 05 2B C2 8B CB BA 01 00 00 00 D3 E2 8B CD 03 C3 50 85 11", "pGameClientShell");
    if (addr_pGameClientShell == 0) return;

    DWORD addr_OnCommandOn = addr_pGameClientShell + MemoryHelper::ReadMemory<int>(addr_pGameClientShell + 0x21) + 0x25;
    DWORD addr_OnCommandOff = addr_pGameClientShell + MemoryHelper::ReadMemory<int>(addr_pGameClientShell + 0x28) + 0x2C;
    DWORD addr_GetExtremalCommandValue = ScanModuleSignature(g_State.GameClient, "83 EC 08 56 57 8B F9 8B 77 04 3B 77 08 C7 44 24 08 00 00 00 00", "GetExtremalCommandValue");
    DWORD addr_IsCommandOn = ScanModuleSignature(g_State.GameClient, "8B D1 8A 42 4C 84 C0 56 74 58", "IsCommandOn");
    DWORD addr_HUDActivateObjectSetObject = ScanModuleSignature(g_State.GameClient, "8B 86 D4 02 00 00 3B C3 8D BE C8 02 00 00 74 0F", "HUDActivateObjectSetObject", 1);
    DWORD addr_HUDSwapUpdate = ScanModuleSignature(g_State.GameClient, "55 8B EC 83 E4 F8 81 EC 84 01", "HUDSwapUpdate");
    DWORD addr_SetOperatingTurret = ScanModuleSignature(g_State.GameClient, "8B 44 24 04 89 81 F4 05 00 00 8B 0D ?? ?? ?? ?? 8B 11 FF 52 3C C2 04 00", "SetOperatingTurret");
    DWORD addr_GetTriggerNameFromCommandID = ScanModuleSignature(g_State.GameClient, "81 EC 44 08 00 00", "GetTriggerNameFromCommandID");
    DWORD addr_SwitchToScreen = ScanModuleSignature(g_State.GameClient, "53 55 56 8B F1 8B 6E 60 33 DB 3B EB 57 8B 7C 24 14", "SwitchToScreen");
    DWORD addr_SetCurrentType = ScanModuleSignature(g_State.GameClient, "53 8B 5C 24 08 85 DB 56 57 8B F1 7C 1C 8B BE E4", "SetCurrentType");
    DWORD addr_HUDSwapUpdateTriggerName = ScanModuleSignature(g_State.GameClient, "8B 0D ?? ?? ?? ?? 6A 57 E8 ?? ?? ?? ?? 50 B9", "HUDSwapUpdateTriggerName");
    DWORD addr_GetZoomMag = ScanModuleSignature(g_State.GameClient, "C7 44 24 30 00 00 00 00 8B 4D 28 57 E8", "GetZoomMag");
	DWORD addr_CycleCtrlSetSelIndex = ScanModuleSignature(g_State.GameClient, "53 8A 5C 24 08 80 FB FE 77 26 8B 91 E4 00 00 00", "CycleCtrlSetSelIndex");
    DWORD addr_DEditLoadModule = ScanModuleSignature(g_State.GameClient, "83 C4 04 84 C0 75 17 8B 4C 24 04", "DEditLoadModule");
    DWORD addr_PerformanceScreenId = ScanModuleSignature(g_State.GameClient, "8B C8 E8 ?? ?? ?? ?? 8B 4E 0C 8B 01 6A ?? FF 50 6C 85 C0 74 0A 8B 10 8B C8 FF 92 88 00 00 00 8B 4E 0C 8B 01 6A", "PerformanceScreenId");
    addr_GetZoomMag = MemoryHelper::ResolveRelativeAddress(addr_GetZoomMag, 0xD);

    if (addr_OnCommandOn == 0 ||
        addr_OnCommandOff == 0 ||
        addr_GetExtremalCommandValue == 0 ||
        addr_IsCommandOn == 0 ||
        addr_HUDActivateObjectSetObject == 0 ||
        addr_HUDSwapUpdate == 0 ||
        addr_SetOperatingTurret == 0 ||
        addr_GetTriggerNameFromCommandID == 0 ||
        addr_SwitchToScreen == 0 ||
        addr_SetCurrentType == 0 ||
        addr_HUDSwapUpdateTriggerName == 0 ||
        addr_GetZoomMag == 0 ||
		addr_CycleCtrlSetSelIndex == 0 ||
        addr_DEditLoadModule == 0 ||
        addr_PerformanceScreenId == 0) {
        return;
    }

    g_State.pGameClientShell = MemoryHelper::ReadMemory<int>(MemoryHelper::ReadMemory<int>(addr_pGameClientShell + 0x1A));

    HookHelper::ApplyHook((void*)(addr_GetExtremalCommandValue), &GetExtremalCommandValue_Hook, (LPVOID*)&GetExtremalCommandValue);
    HookHelper::ApplyHook((void*)addr_IsCommandOn, &IsCommandOn_Hook, (LPVOID*)&IsCommandOn);
    HookHelper::ApplyHook((void*)addr_SetOperatingTurret, &SetOperatingTurret_Hook, (LPVOID*)&SetOperatingTurret);
    HookHelper::ApplyHook((void*)addr_GetTriggerNameFromCommandID, &GetTriggerNameFromCommandID_Hook, (LPVOID*)&GetTriggerNameFromCommandID);
    HookHelper::ApplyHook((void*)addr_HUDActivateObjectSetObject, &HUDActivateObjectSetObject_Hook, (LPVOID*)&HUDActivateObjectSetObject);
    HookHelper::ApplyHook((void*)addr_HUDSwapUpdate, &HUDSwapUpdate_Hook, (LPVOID*)&HUDSwapUpdate);
    HookHelper::ApplyHook((void*)addr_SwitchToScreen, &SwitchToScreen_Hook, (LPVOID*)&SwitchToScreen);
    HookHelper::ApplyHook((void*)addr_SetCurrentType, &SetCurrentType_Hook, (LPVOID*)&SetCurrentType);
    HookHelper::ApplyHook((void*)addr_GetZoomMag, &GetZoomMag_Hook, (LPVOID*)&GetZoomMag);
	HookHelper::ApplyHook((void*)addr_CycleCtrlSetSelIndex, &CycleCtrlSetSelIndex_Hook, (LPVOID*)&CycleCtrlSetSelIndex);
    HookHelper::ApplyHook((void*)(addr_DEditLoadModule - 0xA), &DEditLoadModule_Hook, (LPVOID*)&DEditLoadModule);
	OnCommandOn = reinterpret_cast<decltype(OnCommandOn)>(addr_OnCommandOn);
	OnCommandOff = reinterpret_cast<decltype(OnCommandOff)>(addr_OnCommandOff);
	HUDSwapUpdateTriggerName = reinterpret_cast<decltype(HUDSwapUpdateTriggerName)>(addr_HUDSwapUpdateTriggerName);

    g_State.screenPerformanceCPU = MemoryHelper::ReadMemory<uint8_t>(addr_PerformanceScreenId + 0xD);
    g_State.screenPerformanceGPU = MemoryHelper::ReadMemory<uint8_t>(addr_PerformanceScreenId + 0x25);

	// Gyro
	DWORD addr_ApplyLocalRotationOffset = ScanModuleSignature(g_State.GameClient, "DA E9 DF E0 F6 C4 44 7A 24 D9 ?? 04 D9", "ApplyLocalRotationOffset", 2);
	DWORD addr_UpdatePlayerMovement = ScanModuleSignature(g_State.GameClient, "83 EC 0C 56 8B F1 E8 ?? ?? ?? ?? 8A 88 9E 05 00 00", "UpdatePlayerMovement");
	DWORD addr_BeginAim = ScanModuleSignature(g_State.GameClient, "A1 ?? ?? ?? ?? 56 8B F1 8B 48 28 8B 81 5C 01 00 00", "BeginAim");
	DWORD addr_EndAim = ScanModuleSignature(g_State.GameClient, "A1 ?? ?? ?? ?? 56 8B F1 8B 88 F4 05 00 00 85 C9", "EndAim");

	if (addr_ApplyLocalRotationOffset != 0 && addr_UpdatePlayerMovement != 0 && addr_BeginAim != 0 && addr_EndAim != 0)
	{
		HookHelper::ApplyHook((void*)addr_ApplyLocalRotationOffset, &ApplyLocalRotationOffset_Hook, (LPVOID*)&ApplyLocalRotationOffset);
		HookHelper::ApplyHook((void*)addr_UpdatePlayerMovement, &UpdatePlayerMovement_Hook, (LPVOID*)&UpdatePlayerMovement);
		HookHelper::ApplyHook((void*)addr_BeginAim, &BeginAim_Hook, (LPVOID*)&BeginAim);
		HookHelper::ApplyHook((void*)addr_EndAim, &EndAim_Hook, (LPVOID*)&EndAim);
	}

	// Rumble
	DWORD addr_CClientWeaponFire = ScanModuleSignature(g_State.GameClient, "50 FF 57 7C 83 F8 02", "CClientWeaponFire", 4);
	DWORD addr_HandleMsgPlayerDamage = ScanModuleSignature(g_State.GameClient, "81 EC 4C 01 00 00 55 56 8B B4 24 58 01 00 00", "HandleMsgPlayerDamage");
	DWORD addr_UpdateHealth = ScanModuleSignature(g_State.GameClient, "8B 41 0C 8B 54 24 04 3B D0 76 02 8B D0 8B 41 04", "UpdateHealth");
	DWORD addr_UpdateArmor = ScanModuleSignature(g_State.GameClient, "8B 51 10 8B 44 24 04 3B C2 76 02 8B C2 39 41 08", "UpdateArmor");
	DWORD addr_CHUDMgr_StartFlicker = ScanModuleSignature(g_State.GameClient, "FF 50 70 8B B7 7C 04 00 00 3B B7 80 04 00 00", "CHUDMgr_StartFlicker");
	DWORD addr_CClientWeapon_WeaponPath_OnImpactCB = ScanModuleSignature(g_State.GameClient, "8B 4C 24 08 85 C9 75 03 32 C0 C3 8B 44 24 04", "CClientWeapon_WeaponPath_OnImpactCB");
	DWORD addr_HandleFallLand = ScanModuleSignature(g_State.GameClient, "81 EC 38 02 00 00 ?? 8B ?? ?? ?? C0 00 00 00", "HandleFallLand");
	DWORD addr_CTurretFX_SetDamageState = ScanModuleSignature(g_State.GameClient, "81 EC C8 00 00 00 53 56 57 8B F1 8B 46 4C 8B 0D", "CTurretFX_SetDamageState");

	if (addr_CClientWeaponFire != 0 && addr_HandleMsgPlayerDamage != 0 && addr_UpdateHealth != 0 && addr_UpdateArmor != 0 && addr_CHUDMgr_StartFlicker != 0 && addr_CClientWeapon_WeaponPath_OnImpactCB != 0 && addr_HandleFallLand != 0 && addr_CTurretFX_SetDamageState != 0)
	{
		HookHelper::ApplyHook((void*)addr_CClientWeaponFire, &CClientWeaponFire_Hook, (LPVOID*)&CClientWeaponFire);
		HookHelper::ApplyHook((void*)addr_HandleMsgPlayerDamage, &HandleMsgPlayerDamage_Hook, (LPVOID*)&HandleMsgPlayerDamage);
		HookHelper::ApplyHook((void*)addr_UpdateHealth, &UpdateHealth_Hook, (LPVOID*)&UpdateHealth);
		HookHelper::ApplyHook((void*)addr_UpdateArmor, &UpdateArmor_Hook, (LPVOID*)&UpdateArmor);
		HookHelper::ApplyHook((void*)(addr_CHUDMgr_StartFlicker - 0x32), &CHUDMgr_StartFlicker_Hook, (LPVOID*)&CHUDMgr_StartFlicker);
		HookHelper::ApplyHook((void*)addr_CClientWeapon_WeaponPath_OnImpactCB, &CClientWeapon_WeaponPath_OnImpactCB_Hook, (LPVOID*)&CClientWeapon_WeaponPath_OnImpactCB);
		HookHelper::ApplyHook((void*)addr_HandleFallLand, &HandleFallLand_Hook, (LPVOID*)&HandleFallLand);
		HookHelper::ApplyHook((void*)addr_CTurretFX_SetDamageState, &CTurretFX_SetDamageState_Hook, (LPVOID*)&CTurretFX_SetDamageState);
	}

	if (HideMouseCursor)
	{
		DWORD addr_UseCursor = ScanModuleSignature(g_State.GameClient, "8A 44 24 04 84 C0 56 8B F1 88 46 01 74", "UseCursor");
		DWORD addr_OnMouseMove = ScanModuleSignature(g_State.GameClient, "56 8B F1 8A 86 ?? ?? 00 00 84 C0 0F 84 B3", "OnMouseMove");
		DWORD addr_ForceMouseUpdate = ScanModuleSignature(g_State.GameClient, "8B 90 ?? 37 00 00 8B 80 ?? 37 00 00 56", "ForceMouseUpdate", 3);

		if (addr_UseCursor != 0 && addr_OnMouseMove != 0 && addr_ForceMouseUpdate != 0)
		{
			HookHelper::ApplyHook((void*)addr_OnMouseMove, &OnMouseMove_Hook, (LPVOID*)&OnMouseMove);
			HookHelper::ApplyHook((void*)addr_UseCursor, &UseCursor_Hook, (LPVOID*)&UseCursor);
			HookHelper::ApplyHook((void*)addr_ForceMouseUpdate, &ForceMouseUpdate_Hook, (LPVOID*)&ForceMouseUpdate);
		}
	}

	ScreenJoystickHook::InstallHook();
}

static void ApplyHUDScalingClientPatch()
{
    if (!HUDScaling) return;

    DWORD addr_HUDTerminate = ScanModuleSignature(g_State.GameClient, "53 56 8B D9 8B B3 7C 04 00 00 8B 83 80 04 00 00 57 33 FF 3B F0", "HUDTerminate");
    DWORD addr_HUDInit = ScanModuleSignature(g_State.GameClient, "8B ?? ?? 8D ?? 78 04 00 00", "HUDInit", 1);
    DWORD addr_HUDRender = ScanModuleSignature(g_State.GameClient, "53 8B D9 8A 43 08 84 C0 74", "HUDRender");
    DWORD addr_LayoutDBGetPosition = ScanModuleSignature(g_State.GameClient, "83 EC 10 8B 54 24 20 8B 0D", "LayoutDBGetPosition");
    DWORD addr_GetRectF = ScanModuleSignature(g_State.GameClient, "14 8B 44 24 28 8B 4C 24 18 D9 18", "GetRectF");
    DWORD addr_HUDWeaponListReset = ScanModuleSignature(g_State.GameClient, "51 53 55 8B E9 8B 0D", "HUDWeaponListReset");
    DWORD addr_InitAdditionalTextureData = ScanModuleSignature(g_State.GameClient, "8B 54 24 04 8B 01 83 EC 20 57", "InitAdditionalTextureData");
    DWORD addr_HUDPausedInit = ScanModuleSignature(g_State.GameClient, "56 8B F1 8B 06 57 FF 50 20", "HUDPausedInit");

    if (addr_HUDTerminate == 0 ||
        addr_HUDInit == 0 ||
        addr_HUDRender == 0 ||
        addr_LayoutDBGetPosition == 0 ||
        addr_GetRectF == 0 ||
        addr_HUDWeaponListReset == 0 ||
        addr_InitAdditionalTextureData == 0 ||
        addr_HUDPausedInit == 0) {
        return;
    }

    HookHelper::ApplyHook((void*)addr_HUDInit, &HUDInit_Hook, (LPVOID*)&HUDInit);
    HookHelper::ApplyHook((void*)addr_HUDRender, &HUDRender_Hook, (LPVOID*)&HUDRender);
    HookHelper::ApplyHook((void*)addr_LayoutDBGetPosition, &LayoutDBGetPosition_Hook, (LPVOID*)&LayoutDBGetPosition);
    HookHelper::ApplyHook((void*)(addr_GetRectF - 0x58), &GetRectF_Hook, (LPVOID*)&GetRectF);
    HookHelper::ApplyHook((void*)(addr_InitAdditionalTextureData - 6), &InitAdditionalTextureData_Hook, (LPVOID*)&InitAdditionalTextureData);
    HookHelper::ApplyHook((void*)addr_HUDPausedInit, &HUDPausedInit_Hook, (LPVOID*)&HUDPausedInit);
	HUDTerminate = reinterpret_cast<decltype(HUDTerminate)>(addr_HUDTerminate);
	HUDWeaponListReset = reinterpret_cast<decltype(HUDWeaponListReset)>(addr_HUDWeaponListReset);
}

static void ApplySSAAScaleClientPatch()
{
	if (SSAAScale == 1.0f) return;

	DWORD addr_Camera_UpdateRenderTarget = ScanModuleSignature(g_State.GameClient, "83 EC 08 56 8B F1 8B 0D ?? ?? ?? ?? 85 C9 0F 84 E7", "Camera_UpdateRenderTarget");

	if (addr_Camera_UpdateRenderTarget == 0) return;

	HookHelper::ApplyHook((void*)addr_Camera_UpdateRenderTarget, &Camera_UpdateRenderTarget_Hook, (LPVOID*)&Camera_UpdateRenderTarget);
	MemoryHelper::MakeNOP(addr_Camera_UpdateRenderTarget + 0x32, 0x6);
}

static void ApplySetWeaponCapacityClientPatch()
{
    if (!EnableCustomMaxWeaponCapacity) return;

    DWORD addr_GetWeaponCapacity = ScanModuleSignature(g_State.GameClient, "CC 8B 41 48 8B 0D", "GetWeaponCapacity");

    if (addr_GetWeaponCapacity != 0)
    {
        HookHelper::ApplyHook((void*)(addr_GetWeaponCapacity + 0x1), &GetWeaponCapacity_Hook, (LPVOID*)&GetWeaponCapacity);
        g_State.appliedCustomMaxWeaponCapacity = true;
    }
}

static void ApplyHighResolutionReflectionsClientPatch()
{
    if (!HighResolutionReflections) return;

    DWORD addr = ScanModuleSignature(g_State.GameClient, "8B 47 08 89 46 4C 8A 4F 24 88 4E 68 8A 57 25", "RenderTargetGroupFXInit");

    if (addr != 0)
    {
        HookHelper::ApplyHook((void*)(addr - 0x31), &RenderTargetGroupFXInit_Hook, (LPVOID*)&RenderTargetGroupFXInit);
    }
}

static void ApplyAutoResolutionClientPatch()
{
    if (!AutoResolution) return;

    DWORD addr_AutoDetectPerformanceSettings = ScanModuleSignature(g_State.GameClient, "83 C4 10 83 F8 01 75 37", "AutoDetectPerformanceSettings", 2);
    DWORD addr_SetQueuedConsoleVariable = ScanModuleSignature(g_State.GameClient, "83 EC 10 56 8B F1 8B 46 ?? 8B 4E ?? 8D 54 24 18", "SetQueuedConsoleVariable");
    DWORD addr_SetOption = ScanModuleSignature(g_State.GameClient, "51 8B 44 24 14 85 C0 89 0C 24", "SetOption");

    if (addr_AutoDetectPerformanceSettings == 0 ||
        addr_SetQueuedConsoleVariable == 0 ||
        addr_SetOption == 0) {
        return;
    }

    HookHelper::ApplyHook((void*)addr_AutoDetectPerformanceSettings, &AutoDetectPerformanceSettings_Hook, (LPVOID*)&AutoDetectPerformanceSettings);
    HookHelper::ApplyHook((void*)addr_SetQueuedConsoleVariable, &SetQueuedConsoleVariable_Hook, (LPVOID*)&SetQueuedConsoleVariable);
    HookHelper::ApplyHook((void*)addr_SetOption, &SetOption_Hook, (LPVOID*)&SetOption);
}

static void ApplyKeyboardInputLanguageClientCheck()
{
    if (!FixKeyboardInputLanguage) return;

    DWORD addr_LoadUserProfile = ScanModuleSignature(g_State.GameClient, "53 8B 5C 24 08 84 DB 55 56 57 8B F9", "LoadUserProfile");
    DWORD addr_RestoreDefaults = ScanModuleSignature(g_State.GameClient, "57 8B F9 8B 0D ?? ?? ?? ?? 8B 01 FF 50 4C 8B 10", "RestoreDefaults");

    if (addr_LoadUserProfile == 0 ||
        addr_RestoreDefaults == 0) {
        return;
    }

    HookHelper::ApplyHook((void*)addr_LoadUserProfile, &LoadUserProfile_Hook, (LPVOID*)&LoadUserProfile);
    HookHelper::ApplyHook((void*)addr_RestoreDefaults, &RestoreDefaults_Hook, (LPVOID*)&RestoreDefaults);
}

static void ApplyWeaponFixesClientPatch()
{
    if (!WeaponFixes) return;

    DWORD addr_AimMgrCtor = ScanModuleSignature(g_State.GameClient, "8B C1 C6 00 00 C6 40 01 01 C3", "AimMgrCtor");
    DWORD addr_UpdateWeaponModel = ScanModuleSignature(g_State.GameClient, "83 EC 44 56 8B F1 57 8B 7E 08 85 FF", "UpdateWeaponModel");
    DWORD addr_AnimationClearLock = ScanModuleSignature(g_State.GameClient, "E8 BB FF FF FF C7 41 34 FF FF FF FF C7 81 58 01", "AnimationClearLock");
    DWORD addr_SetAnimProp = ScanModuleSignature(g_State.GameClient, "8B 44 24 04 83 F8 FF 74 ?? 83 F8 12 7D ?? 8B 54 24 08 89 04", "SetAnimProp");
    DWORD addr_InitAnimations = ScanModuleSignature(g_State.GameClient, "6A 08 6A 7A 8B CF FF ?? 24 8B ?? 6A 08", "InitAnimations", 3);
    DWORD addr_GetWeaponSlot = ScanModuleSignature(g_State.GameClient, "8A 51 40 32 C0 84 D2 76 23 56 8B B1 B4 00 00 00 57", "GetWeaponSlot");
    DWORD addr_NextWeapon = ScanModuleSignature(g_State.GameClient, "84 C0 0F 84 ?? 00 00 00 8B CE E8", "NextWeapon");
    DWORD addr_PreviousWeapon = ScanModuleSignature(g_State.GameClient, "8D BE ?? 57 00 00 8B CF E8 ?? ?? ?? ?? 84 C0 74 1F 8B CE E8", "PreviousWeapon");
    DWORD addr_kAP_ACT_Fire_Id = ScanModuleSignature(g_State.GameClient, "84 C0 75 1E 6A 00 68 ?? 00 00 00 6A 00 8B CF", "kAP_ACT_Fire_Id");
    addr_NextWeapon = MemoryHelper::ResolveRelativeAddress(addr_NextWeapon, 0xB);
    addr_PreviousWeapon = MemoryHelper::ResolveRelativeAddress(addr_PreviousWeapon, 0x14);

    if (addr_AimMgrCtor == 0 ||
        addr_UpdateWeaponModel == 0 ||
        addr_AnimationClearLock == 0 ||
        addr_SetAnimProp == 0 ||
        addr_InitAnimations == 0 ||
        addr_GetWeaponSlot == 0 ||
        addr_NextWeapon == 0 ||
        addr_PreviousWeapon == 0 ||
        addr_kAP_ACT_Fire_Id == 0) {
        return;
    }

    HookHelper::ApplyHook((void*)addr_AimMgrCtor, &AimMgrCtor_Hook, (LPVOID*)&AimMgrCtor);
    HookHelper::ApplyHook((void*)addr_UpdateWeaponModel, &UpdateWeaponModel_Hook, (LPVOID*)&UpdateWeaponModel);
    HookHelper::ApplyHook((void*)addr_SetAnimProp, &SetAnimProp_Hook, (LPVOID*)&SetAnimProp);
    HookHelper::ApplyHook((void*)addr_InitAnimations, &InitAnimations_Hook, (LPVOID*)&InitAnimations);
    HookHelper::ApplyHook((void*)addr_GetWeaponSlot, &GetWeaponSlot_Hook, (LPVOID*)&GetWeaponSlot);
    HookHelper::ApplyHook((void*)addr_NextWeapon, &NextWeapon_Hook, (LPVOID*)&NextWeapon);
    HookHelper::ApplyHook((void*)addr_PreviousWeapon, &PreviousWeapon_Hook, (LPVOID*)&PreviousWeapon);
	AnimationClearLock = reinterpret_cast<decltype(AnimationClearLock)>(addr_AnimationClearLock);
    g_State.kAP_ACT_Fire_Id = MemoryHelper::ReadMemory<int>(addr_kAP_ACT_Fire_Id + 0x7);
}

static void ApplyConsoleClientPatch()
{
	if (!ConsoleEnabled) return;

	DWORD addr = ScanModuleSignature(g_State.GameClient, "E8 ?? ?? ?? ?? 8A 4C 24 04 88 48 4C C2 04 00 CC", "SetInputState");

	if (addr != 0)
	{
		HookHelper::ApplyHook((void*)addr, &SetInputState_Hook, (LPVOID*)&SetInputState);
	}
}

static void ApplyClientFXHook()
{
	if (!HighFPSFixes && !SDLGamepadSupport) return;

    DWORD addr = ScanModuleSignature(g_State.GameClient, "83 EC 20 56 57 8B F1 E8 ?? ?? ?? ?? 8A 44 24 30", "LoadFxDll");

    if (addr != 0)
    {
        HookHelper::ApplyHook((void*)addr, &LoadFxDll_Hook, (LPVOID*)&LoadFxDll);
    }
}

static void ApplyDisableHipFireAccuracyPenalty()
{
    if (!DisableHipFireAccuracyPenalty) return;

    DWORD addr = ScanModuleSignature(g_State.GameClient, "83 EC ?? A1 ?? ?? ?? ?? 8B 40 28 56 57 6A 00 8B F1", "DisableHipFireAccuracyPenalty");

    if (addr != 0)
    {
        HookHelper::ApplyHook((void*)addr, &AccuracyMgrUpdate_Hook, (LPVOID*)&AccuracyMgrUpdate);
    }
}

static void ApplyGameDatabaseHook()
{
    if (!HUDScaling) return;

    DWORD addr_GameDatabase = ScanModuleSignature(g_State.GameClient, "8B 5E 08 55 E8 ?? ?? ?? FF 8B 0D ?? ?? ?? ?? 8B 39 68 ?? ?? ?? ?? 6A 00 68 ?? ?? ?? ?? 53 FF 57", "HUDScaling_GameDatabase");

    if (addr_GameDatabase != 0)
    {
        int pDB = MemoryHelper::ReadMemory<int>(addr_GameDatabase + 0xB);
        int pGameDatabase = MemoryHelper::ReadMemory<int>(pDB);
        int pLayoutDB = MemoryHelper::ReadMemory<int>(pGameDatabase);

        HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x58), &DBGetRecord_Hook, (LPVOID*)&DBGetRecord);
        HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x7C), &DBGetInt32_Hook, (LPVOID*)&DBGetInt32);
        HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x80), &DBGetFloat_Hook, (LPVOID*)&DBGetFloat);
        //HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x84), &DBGetString_Hook, (LPVOID*)&DBGetString);
    }
}

static void ApplyClientPatchSet1()
{
	if (!HUDScaling && !SDLGamepadSupport && SSAAScale == 1.0f) return;

	DWORD addr_HUDWeaponListUpdateTriggerNames = ScanModuleSignature(g_State.GameClient, "56 32 DB 89 44 24 0C BE 1E 00 00 00", "HUDWeaponListUpdateTriggerNames");
	DWORD addr_HUDGrenadeListUpdateTriggerNames = ScanModuleSignature(g_State.GameClient, "56 32 DB 89 44 24 0C BE 28 00 00 00", "HUDGrenadeListUpdateTriggerNames");
	DWORD addr_HUDWeaponListInit = ScanModuleSignature(g_State.GameClient, "51 53 55 57 8B F9 8B 07 FF 50 20 8B 0D", "HUDWeaponListInit");
	DWORD addr_HUDGrenadeListInit = ScanModuleSignature(g_State.GameClient, "83 EC 08 53 55 57 8B F9 8B 07 FF 50 20 8B 0D", "HUDGrenadeListInit");
	DWORD addr_ScreenDimsChanged = ScanModuleSignature(g_State.GameClient, "A1 ?? ?? ?? ?? 81 EC 98 00 00 00 85 C0 56 8B F1", "ScreenDimsChanged");
	DWORD addr_SliderSetSliderPos = ScanModuleSignature(g_State.GameClient, "56 8B F1 8B 4C 24 08 8B 86 7C 01 00 00 3B C8 89 8E 80 01 00 00", "SliderSetSliderPos");

	if (addr_HUDWeaponListUpdateTriggerNames == 0 ||
		addr_HUDGrenadeListUpdateTriggerNames == 0 ||
		addr_HUDWeaponListInit == 0 ||
		addr_HUDGrenadeListInit == 0 ||
		addr_ScreenDimsChanged == 0 ||
		addr_SliderSetSliderPos == 0) {
		return;
	}

	HUDWeaponListUpdateTriggerNames = reinterpret_cast<decltype(HUDWeaponListUpdateTriggerNames)>(addr_HUDWeaponListUpdateTriggerNames - 0x10);
	HUDGrenadeListUpdateTriggerNames = reinterpret_cast<decltype(HUDGrenadeListUpdateTriggerNames)>(addr_HUDGrenadeListUpdateTriggerNames - 0x10);
	HookHelper::ApplyHook((void*)addr_HUDWeaponListInit, &HUDWeaponListInit_Hook, (LPVOID*)&HUDWeaponListInit);
	HookHelper::ApplyHook((void*)addr_HUDGrenadeListInit, &HUDGrenadeListInit_Hook, (LPVOID*)&HUDGrenadeListInit);
	HookHelper::ApplyHook((void*)addr_ScreenDimsChanged, &ScreenDimsChanged_Hook, (LPVOID*)&ScreenDimsChanged);
	HookHelper::ApplyHook((void*)addr_SliderSetSliderPos, &SliderSetSliderPos_Hook, (LPVOID*)&SliderSetSliderPos);
}

static void ApplyClientPatchSet2()
{
    if (!WeaponFixes && !EnableCustomMaxWeaponCapacity) return;

    DWORD addr_OnEnterWorld = ScanModuleSignature(g_State.GameClient, "8B F1 E8 ?? ?? ?? ?? DD 05 ?? ?? ?? ?? 8B 96", "OnEnterWorld", 1);

    if (addr_OnEnterWorld != 0)
    {
        HookHelper::ApplyHook((void*)addr_OnEnterWorld, &OnEnterWorld_Hook, (LPVOID*)&OnEnterWorld);
    }
}

static void ApplyClientPatchSet3()
{
	if (!SDLGamepadSupport && !ConsoleEnabled) return;

	DWORD addr_HookedWindowProc = ScanModuleSignature(g_State.GameClient, "56 8B 74 24 0C 81 FE 03 02 00 00 57 8B 7C 24 14", "HookedWindowProc");
	DWORD addr_ChangeState = ScanModuleSignature(g_State.GameClient, "8B 44 24 0C 53 8B 5C 24 0C 57 8B 7E 08", "ChangeState");
	DWORD addr_MsgBoxShow = ScanModuleSignature(g_State.GameClient, "83 EC 70 56 8B F1 8A 86 78 05 00 00 84 C0 0F 85", "MsgBoxShow");
	DWORD addr_MsgBoxHide = ScanModuleSignature(g_State.GameClient, "56 8B F1 8A 86 78 05 00 00 84 C0 0F 84", "MsgBoxHide");

	if (addr_HookedWindowProc == 0 ||
		addr_ChangeState == 0 ||
		addr_MsgBoxShow == 0 ||
		addr_MsgBoxHide == 0) {
		return;
	}

	HookHelper::ApplyHook((void*)addr_HookedWindowProc, &HookedWindowProc_Hook, (LPVOID*)&HookedWindowProc);
	HookHelper::ApplyHook((void*)(addr_ChangeState - 0x13), &ChangeState_Hook, (LPVOID*)&ChangeState);
	HookHelper::ApplyHook((void*)addr_MsgBoxShow, &MsgBoxShow_Hook, (LPVOID*)&MsgBoxShow);
	HookHelper::ApplyHook((void*)addr_MsgBoxHide, &MsgBoxHide_Hook, (LPVOID*)&MsgBoxHide);
}


void ApplyClientPatch()
{
	ApplyHighFPSFixesClientPatch();
	ApplyMouseAimMultiplierClientPatch();
	ApplyXPWidescreenClientPatch();
	ApplySkipSplashScreenClientPatch();
	ApplyDisableLetterboxClientPatch();
	ApplyPersistentWorldClientPatch();
	ApplyInfiniteFlashlightClientPatch();
	ApplyControllerClientPatch();
	ApplyHUDScalingClientPatch();
	ApplySSAAScaleClientPatch();
	ApplySetWeaponCapacityClientPatch();
	ApplyHighResolutionReflectionsClientPatch();
	ApplyAutoResolutionClientPatch();
	ApplyKeyboardInputLanguageClientCheck();
	ApplyWeaponFixesClientPatch();
	ApplyConsoleClientPatch();
	ApplyClientFXHook();
	ApplyDisableHipFireAccuracyPenalty();
	ApplyGameDatabaseHook();
	ApplyClientPatchSet1();
	ApplyClientPatchSet2();
	ApplyClientPatchSet3();
}
