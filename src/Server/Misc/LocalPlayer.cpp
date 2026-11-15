#pragma once

#include "../../Globals.cpp"

static DWORD pPlayerObjListBegin = 0;
static DWORD pPlayerObjListEnd = 0;
static DWORD pLTServerLocalPlayerAddr = 0;
static int playerClientOffset = 0;
static int getClientInfoFlagsOffset = 0;

// ===============
//  LocalPlayer
// ===============

static bool IsLocalPlayer(DWORD character)
{
    int ltServer = pPlayerObjListBegin ? MemoryHelper::ReadMemory<int>(pLTServerLocalPlayerAddr) : 0;
    if (!ltServer)
        return false;

    for (DWORD* player = *(DWORD**)pPlayerObjListBegin; player != *(DWORD**)pPlayerObjListEnd; player++)
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

static bool IsLocalPlayerAlone()
{
    if (!pPlayerObjListBegin)
        return false;

    DWORD* playerObjListBegin = *(DWORD**)pPlayerObjListBegin;
    DWORD* playerObjListEnd = *(DWORD**)pPlayerObjListEnd;
    return playerObjListEnd - playerObjListBegin == 1 && IsLocalPlayer(*playerObjListBegin);
}

static void ApplyLocalPlayerServerPatch()
{
    pPlayerObjListBegin = 0;
    pPlayerObjListEnd = 0;

    if (!(HighFPSFixes && g_State.isClientSlowMoTracked) && !SDLGamepadSupport) return;

    DWORD addr_PlayerObjList = ScanModuleSignature(g_State.GameServer, "8B 35 ?? ?? ?? ?? 3B 35 ?? ?? ?? ?? 74 2D 8B 06 85 C0 74 1B 8B 88 ?? ?? 00 00 85 C9 74 11", "PlayerObjList");
    DWORD addr_UpdateInContainerState = ScanModuleSignature(g_State.GameServer, "51 56 8B F1 8B 86 ?? ?? 00 00 50 A1 ?? ?? ?? ?? FF 50 ?? 83 C4 04 A8 01", "UpdateInContainerState");

    if (addr_PlayerObjList == 0 || addr_UpdateInContainerState == 0)
        return;

    playerClientOffset = MemoryHelper::ReadMemory<int>(addr_UpdateInContainerState + 0x6);
    pLTServerLocalPlayerAddr = MemoryHelper::ReadMemory<DWORD>(addr_UpdateInContainerState + 0xC);
    getClientInfoFlagsOffset = MemoryHelper::ReadMemory<uint8_t>(addr_UpdateInContainerState + 0x12);
    pPlayerObjListEnd = MemoryHelper::ReadMemory<DWORD>(addr_PlayerObjList + 0x8);
    pPlayerObjListBegin = MemoryHelper::ReadMemory<DWORD>(addr_PlayerObjList + 0x2);
}
