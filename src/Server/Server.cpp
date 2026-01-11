#include "../Core/Core.hpp"
#include "MinHook.hpp"
#include "../helper.hpp"

void(__thiscall* SetWeaponCapacityServer)(int, uint8_t) = nullptr;
void(__thiscall* PlayerInventoryInit)(int, int) = nullptr;
void(__thiscall* ServerUpdateSlowMo)(int*) = nullptr;
void(__thiscall* ServerExitSlowMo)(int*, bool, float) = nullptr;

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

// ========================
// HighFPSFixes
// ========================

void __fastcall ServerUpdateSlowMo_Hook(int* thisPtr, int)
{
    int hSlowMoRecord = *(int*)((char*)thisPtr + g_State.phSlowMoRecord);

    // Force server exit when client charge depleted
    if (hSlowMoRecord != 0 && g_State.clientSlowMoCharge <= 0.01)
    {
        ServerExitSlowMo(thisPtr, true, 0.0f);
        return;
    }

    ServerUpdateSlowMo(thisPtr);
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

    DWORD addr_BodyFading = ScanModuleSignature(g_State.GameServer, "8A 86 ?? ?? 00 00 84 C0 74 A1 8D 8E", "BodyFading");

    if (addr_BodyFading != 0)
    {
        MemoryHelper::WriteMemory<uint8_t>(addr_BodyFading + 0x22, 0xEB);
    }
}

static void ApplySetWeaponCapacityServerPatch()
{
    if (!EnableCustomMaxWeaponCapacity || !g_State.appliedCustomMaxWeaponCapacity) return;

    DWORD addr_SetWeaponCapacityServer = ScanModuleSignature(g_State.GameServer, "56 8B F1 8B 56 18 85 D2 8D 4E 14 57 75", "SetWeaponCapacityServer");
    DWORD addr_PlayerInventoryInit = ScanModuleSignature(g_State.GameServer, "33 DB 3B CB 89 ?? 0C 74", "PlayerInventoryInit", 2);

    if (addr_SetWeaponCapacityServer == 0 || addr_PlayerInventoryInit == 0)       
        return;

    ApplyTrackedHook(addr_SetWeaponCapacityServer, &SetWeaponCapacityServer_Hook, (LPVOID*)&SetWeaponCapacityServer);
    ApplyTrackedHook(addr_PlayerInventoryInit, &PlayerInventoryInit_Hook, (LPVOID*)&PlayerInventoryInit);
}

static void ApplyHighFPSFixesServerPatch()
{
    if (!HighFPSFixes) return;

    DWORD addr_ServerUpdateSlowMo = ScanModuleSignature(g_State.GameServer, "56 8B F1 8B 86 ?? ?? ?? ?? 85 C0 74 6B", "ServerUpdateSlowMo");
    DWORD addr_ServerExitSlowMo = ScanModuleSignature(g_State.GameServer, "51 53 8B D9 8B 83 ?? ?? ?? ?? 85 C0 0F 84 ?? ?? ?? ?? DD 05", "ServerExitSlowMo");

    if (addr_ServerUpdateSlowMo == 0 || addr_ServerExitSlowMo == 0)
        return;

    g_State.phSlowMoRecord = MemoryHelper::ReadMemory<int>(addr_ServerUpdateSlowMo + 0x5);
    ApplyTrackedHook(addr_ServerUpdateSlowMo, &ServerUpdateSlowMo_Hook, (LPVOID*)&ServerUpdateSlowMo);
    ServerExitSlowMo = reinterpret_cast<decltype(ServerExitSlowMo)>((int)addr_ServerExitSlowMo);
}

void ApplyServerPatch()
{
    if (g_State.hookedServerFunctionAddresses.size() != 0)
    {
        // Server has been unloaded, remove all previously installed hooks
        for (DWORD address : g_State.hookedServerFunctionAddresses)
        {
            MH_RemoveHook((void*)address);
        }

        g_State.hookedServerFunctionAddresses.clear();
    }

    ApplyPersistentWorldServerPatch();
    ApplySetWeaponCapacityServerPatch();
    ApplyHighFPSFixesServerPatch();
}

#pragma endregion