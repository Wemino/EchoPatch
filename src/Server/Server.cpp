#define NOMINMAX

#include "../Core/Core.hpp"
#include "../helper.hpp"
#include "../Controller/Controller.hpp"

void(__thiscall* SetWeaponCapacityServer)(int, uint8_t) = nullptr;
void(__thiscall* PlayerInventoryInit)(int, int) = nullptr;
void(__thiscall* ServerUpdateSlowMo)(int*) = nullptr;
void(__thiscall* ServerExitSlowMo)(int*, bool, float) = nullptr;
bool(__thiscall* CPlayerInventory_UseGear)(int*, int, int) = nullptr;
void(__thiscall* DetonateRemoteCharges)(DWORD*) = nullptr;

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

// ===============
//  Rumble
// ===============

static bool __fastcall CPlayerInventory_UseGear_Hook(int* thisPtr, int, int hGear, bool a3)
{
    bool bUsed = CPlayerInventory_UseGear(thisPtr, hGear, a3);

    if (bUsed && RumbleEnabled)
    {
        const char* gearName = *(const char**)hGear;

        if (gearName)
        {
            uint32_t hash = HashHelper::FNV1aRuntime(gearName);

            switch (hash)
            {
                case HashHelper::GearHashes::ArmorLightSP:
                case HashHelper::GearHashes::Medkit:
                    SetGamepadRumble(20000, 22000, 150, 2);
                    break;

                case HashHelper::GearHashes::HealthMax:
                case HashHelper::GearHashes::SlowMoMax:
                    SetGamepadRumble(55000, 45000, 300, 2);
                    break;

                default:
                    SetGamepadRumble(15000, 22000, 150, 2);
                    break;
            }
        }
    }

    return bUsed;
}

static void __fastcall DetonateRemoteCharges_Hook(DWORD* thisPtr, int)
{
    if (g_State.isUsingRemoteDetonator && RumbleEnabled)
    {
        DWORD* listHead = (DWORD*)thisPtr[g_State.detonatorListHead];
        DWORD* current = (DWORD*)*listHead;
        int count = 0;

        while (current != listHead)
        {
            if (current[5] != 0)
                count++;

            current = (DWORD*)*current;
        }

        g_State.isUsingRemoteDetonator = false;

        if (count > 0)
        {
            Uint32 duration = std::min(250 + (count * 100), 2000);
            Uint16 low = std::min(40000 + count * 5000, 65535);
            Uint16 high = std::min(30000 + count * 7000, 65535);
            SetGamepadRumble(low, high, duration, 6);
        }
    }

    DetonateRemoteCharges(thisPtr);
}

#pragma endregion

#pragma region Server Patches

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

    HookHelper::ApplyHookReplaceable((void*)addr_SetWeaponCapacityServer, &SetWeaponCapacityServer_Hook, (LPVOID*)&SetWeaponCapacityServer);
    HookHelper::ApplyHookReplaceable((void*)addr_PlayerInventoryInit, &PlayerInventoryInit_Hook, (LPVOID*)&PlayerInventoryInit);
}

static void ApplyHighFPSFixesServerPatch()
{
    if (!HighFPSFixes) return;

    DWORD addr_ServerUpdateSlowMo = ScanModuleSignature(g_State.GameServer, "56 8B F1 8B 86 ?? ?? ?? ?? 85 C0 74 6B", "ServerUpdateSlowMo");
    DWORD addr_ServerExitSlowMo = ScanModuleSignature(g_State.GameServer, "51 53 8B D9 8B 83 ?? ?? ?? ?? 85 C0 0F 84 ?? ?? ?? ?? DD 05", "ServerExitSlowMo");

    if (addr_ServerUpdateSlowMo == 0 || addr_ServerExitSlowMo == 0)
        return;

    g_State.phSlowMoRecord = MemoryHelper::ReadMemory<int>(addr_ServerUpdateSlowMo + 0x5);
    HookHelper::ApplyHookReplaceable((void*)addr_ServerUpdateSlowMo, &ServerUpdateSlowMo_Hook, (LPVOID*)&ServerUpdateSlowMo);
    ServerExitSlowMo = reinterpret_cast<decltype(ServerExitSlowMo)>((int)addr_ServerExitSlowMo);
}

static void ApplyControllerServerPatch()
{
    if (!SDLGamepadSupport) return;

    DWORD addr_UseGear = ScanModuleSignature(g_State.GameServer, "83 EC 18 57 8B F9 8B 4F 0C 8B 01 FF", "UseGear");
    DWORD addr_DetonateRemoteCharges = ScanModuleSignature(g_State.GameServer, "56 57 8B F9 8B 87 ?? ?? 00 00 8B 30 3B F0 74 42", "DetonateRemoteCharges");

    if (addr_UseGear == 0 || addr_DetonateRemoteCharges == 0)
        return;

    g_State.detonatorListHead = MemoryHelper::ReadMemory<int>(addr_DetonateRemoteCharges + 0x6) / 4;
    HookHelper::ApplyHookReplaceable((void*)addr_UseGear, &CPlayerInventory_UseGear_Hook, (LPVOID*)&CPlayerInventory_UseGear);
    HookHelper::ApplyHookReplaceable((void*)addr_DetonateRemoteCharges, &DetonateRemoteCharges_Hook, (LPVOID*)&DetonateRemoteCharges);
}

void ApplyServerPatch()
{
    ApplyPersistentWorldServerPatch();
    ApplySetWeaponCapacityServerPatch();
    ApplyHighFPSFixesServerPatch();
    ApplyControllerServerPatch();
}

#pragma endregion