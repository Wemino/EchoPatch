#pragma once

#include "../../Globals.cpp"

void(__thiscall* SetWeaponCapacityServer)(int, uint8_t) = nullptr;
void(__thiscall* PlayerInventoryInit)(int, int) = nullptr;

// ==============================
//  EnableCustomMaxWeaponCapacity
// ==============================

static void __fastcall SetWeaponCapacityServer_Hook(int thisPtr, int, uint8_t nCap)
{
    SetWeaponCapacityServer(thisPtr, MaxWeaponCapacity);
}

static void __fastcall PlayerInventoryInit_Hook(int thisPtr, int, int nCap)
{
    g_State.CPlayerInventory = thisPtr;
    PlayerInventoryInit(thisPtr, nCap);
}

static void ApplySetWeaponCapacityServerPatch()
{
    if (!EnableCustomMaxWeaponCapacity || !g_State.appliedCustomMaxWeaponCapacity) return;

    DWORD addr_SetWeaponCapacityServer = ScanModuleSignature(g_State.GameServer, "56 8B F1 8B 56 18 85 D2 8D 4E 14 57 75", "SetWeaponCapacityServer");
    DWORD addr_PlayerInventoryInit = ScanModuleSignature(g_State.GameServer, "33 DB 3B CB 89 ?? 0C 74", "PlayerInventoryInit", 2);

    if (addr_SetWeaponCapacityServer == 0 || addr_PlayerInventoryInit == 0)       
        return;

    HookHelper::ApplyHookReplaceable((void*)addr_SetWeaponCapacityServer, &SetWeaponCapacityServer_Hook, (LPVOID*)&SetWeaponCapacityServer);
    HookHelper::ApplyHookReplaceable((void*)addr_PlayerInventoryInit, &PlayerInventoryInit_Hook, (LPVOID*)&PlayerInventoryInit);
}
