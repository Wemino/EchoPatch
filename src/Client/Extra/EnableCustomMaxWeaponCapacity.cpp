#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"
#include "../../Controller/ScreenJoystickHook.hpp"
#include "../../ClientFX/ClientFX.hpp"
#include "../../Server/Server.hpp"

// =============================
// EnableCustomMaxWeaponCapacity
// =============================

static uint8_t __fastcall GetWeaponCapacity_Hook(int thisPtr, int)
{
	return MaxWeaponCapacity;
}

// ============================================
//  EnableCustomMaxWeaponCapacity & WeaponFixes
// ============================================

static void __fastcall OnEnterWorld_Hook(int thisPtr, int)
{
	OnEnterWorld(thisPtr);

	// Set the flag to allow aiming (can be 0 if loading a save during a cutscene)
	if (WeaponFixes && g_State.pAimMgr != 0)
	{
		g_State.isEnteringWorld = true;
		g_State.pAimMgr[1] = 1;
	}

	// Update the weapon capacity
	if (EnableCustomMaxWeaponCapacity)
	{
		SetWeaponCapacityServer(g_State.CPlayerInventory, MaxWeaponCapacity);
	}
}
