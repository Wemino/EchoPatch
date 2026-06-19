#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"

bool(__thiscall* CPlayerInventory_UseGear)(int*, int, int) = nullptr;
void(__thiscall* DetonateRemoteCharges)(DWORD*) = nullptr;

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
