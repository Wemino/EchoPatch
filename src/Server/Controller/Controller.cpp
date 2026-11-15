#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"

static int detonatorListHead = 0;
static int playerClientOffset = 0;
static int getClientInfoFlagsOffset = 0;
static DWORD pRumblePlayerListBegin = 0;
static DWORD pRumblePlayerListEnd = 0;
static DWORD pLTServerRumbleAddr = 0;

bool(__thiscall* CPlayerInventory_UseGear)(int*, int, int) = nullptr;
void(__thiscall* DetonateRemoteCharges)(DWORD*) = nullptr;

// ===============
//  Rumble
// ===============

static bool IsLocalPlayer(DWORD character)
{
    int ltServer = MemoryHelper::ReadMemory<int>(pLTServerRumbleAddr);
    if (!ltServer)
        return false;

    for (DWORD* player = *(DWORD**)pRumblePlayerListBegin; player != *(DWORD**)pRumblePlayerListEnd; player++)
    {
        if (*player != character)
            continue;

        DWORD hClient = *(DWORD*)(character + playerClientOffset);
        auto GetClientInfoFlags = *(uint32_t(__cdecl**)(DWORD))(ltServer + getClientInfoFlagsOffset);

        // CIF_LOCAL
        return hClient && (GetClientInfoFlags(hClient) & 0x1);
    }

    return false;
}

static bool __fastcall CPlayerInventory_UseGear_Hook(int* thisPtr, int, int hGear, bool a3)
{
    bool bUsed = CPlayerInventory_UseGear(thisPtr, hGear, a3);

    // m_pPlayer
    if (bUsed && RumbleEnabled && IsLocalPlayer(thisPtr[3]))
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
    if (g_State.isUsingRemoteDetonator && RumbleEnabled && IsLocalPlayer((DWORD)thisPtr))
    {
        DWORD* listHead = (DWORD*)thisPtr[detonatorListHead];
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
    DWORD addr_UpdateInContainerState = ScanModuleSignature(g_State.GameServer, "51 56 8B F1 8B 86 ?? ?? 00 00 50 A1 ?? ?? ?? ?? FF 50 ?? 83 C4 04 A8 01", "UpdateInContainerState");
    DWORD addr_PlayerObjList = ScanModuleSignature(g_State.GameServer, "8B 35 ?? ?? ?? ?? 3B 35 ?? ?? ?? ?? 74 2D 8B 06 85 C0 74 1B 8B 88 ?? ?? 00 00 85 C9 74 11", "PlayerObjList");

    if (addr_UseGear == 0 || addr_DetonateRemoteCharges == 0 || addr_UpdateInContainerState == 0 || addr_PlayerObjList == 0)
        return;

    detonatorListHead = MemoryHelper::ReadMemory<int>(addr_DetonateRemoteCharges + 0x6) / 4;
    playerClientOffset = MemoryHelper::ReadMemory<int>(addr_UpdateInContainerState + 0x6);
    pLTServerRumbleAddr = MemoryHelper::ReadMemory<DWORD>(addr_UpdateInContainerState + 0xC);
    getClientInfoFlagsOffset = MemoryHelper::ReadMemory<uint8_t>(addr_UpdateInContainerState + 0x12);
    pRumblePlayerListBegin = MemoryHelper::ReadMemory<DWORD>(addr_PlayerObjList + 0x2);
    pRumblePlayerListEnd = MemoryHelper::ReadMemory<DWORD>(addr_PlayerObjList + 0x8);
    HookHelper::ApplyHookReplaceable((void*)addr_UseGear, &CPlayerInventory_UseGear_Hook, (LPVOID*)&CPlayerInventory_UseGear);
    HookHelper::ApplyHookReplaceable((void*)addr_DetonateRemoteCharges, &DetonateRemoteCharges_Hook, (LPVOID*)&DetonateRemoteCharges);
}
