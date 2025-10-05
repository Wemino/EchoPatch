#include <Windows.h>

#include "../Core/Core.hpp"
#include "MinHook.hpp"
#include "../helper.hpp"

void(__thiscall* SetWeaponCapacityServer)(int, uint8_t) = nullptr;
void(__thiscall* PlayerInventoryInit)(int, int) = nullptr;

#pragma region Server Hooks

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

#pragma endregion

#pragma region Server Patches

// Helper to track and store hooked addresses
static void ApplyTrackedHook(DWORD address, LPVOID hookFunc, LPVOID* originalPtr)
{
    HookHelper::ApplyHook((void*)address, hookFunc, originalPtr);
    g_State.hookedServerFunctionAddresses.push_back(address);
}

static void ApplyPersistentWorldServerPatch()
{
    if (!EnablePersistentWorldState) return;

    DWORD targetMemoryLocation_BodyFading = ScanModuleSignature(g_State.GameServer, "8A 86 ?? ?? 00 00 84 C0 74 A1 8D 8E", "BodyFading");

    if (targetMemoryLocation_BodyFading != 0)
    {
        MemoryHelper::WriteMemory<uint8_t>(targetMemoryLocation_BodyFading + 0x22, 0x75);
    }
}

static void ApplySetWeaponCapacityServerPatch()
{
    if (!EnableCustomMaxWeaponCapacity || !g_State.appliedCustomMaxWeaponCapacity) return;

    DWORD targetMemoryLocation_SetWeaponCapacityServer = ScanModuleSignature(g_State.GameServer, "56 8B F1 8B 56 18 85 D2 8D 4E 14 57 75", "SetWeaponCapacityServer");
    DWORD targetMemoryLocation_PlayerInventoryInit = ScanModuleSignature(g_State.GameServer, "33 DB 3B CB 89 ?? 0C 74", "PlayerInventoryInit", 2);

    if (targetMemoryLocation_SetWeaponCapacityServer == 0 ||
        targetMemoryLocation_PlayerInventoryInit == 0) {
        return;
    }

    ApplyTrackedHook(targetMemoryLocation_SetWeaponCapacityServer, &SetWeaponCapacityServer_Hook, (LPVOID*)&SetWeaponCapacityServer);
    ApplyTrackedHook(targetMemoryLocation_PlayerInventoryInit, &PlayerInventoryInit_Hook, (LPVOID*)&PlayerInventoryInit);
}

void ApplyServerPatch()
{
    if (g_State.hookedServerFunctionAddresses.size() != 0) return;

    ApplyPersistentWorldServerPatch();
    ApplySetWeaponCapacityServerPatch();
}

#pragma endregion