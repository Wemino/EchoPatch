#pragma once

#include "../../Globals.cpp"

static int phSlowMoRecord = 0;
static int lastHSlowMoRecord = 0;
static bool slowMoChargeObserved = false;
static DWORD pPlayerObjListBegin = 0;
static DWORD pPlayerObjListEnd = 0;

void(__thiscall* ServerUpdateSlowMo)(int*) = nullptr;
void(__thiscall* ServerExitSlowMo)(int*, bool, float) = nullptr;
void(__thiscall* PlayerInventoryUpdate)(int) = nullptr;
bool(__thiscall* CheckForSever)(int*, int, char, int) = nullptr;
void(__thiscall* AI_SetNextServerUpdate)(DWORD, float) = nullptr;

// ========================
// HighFPSFixes
// ========================

// The local client is in this process, so a lone player on the server is the local player
static bool IsLocalPlayerAlone()
{
    DWORD playerObjListBegin = *(DWORD*)pPlayerObjListBegin;
    DWORD playerObjListEnd = *(DWORD*)pPlayerObjListEnd;
    return playerObjListEnd - playerObjListBegin == sizeof(DWORD);
}

void __fastcall ServerUpdateSlowMo_Hook(int* thisPtr, int)
{
    if (!IsLocalPlayerAlone())
    {
        ServerUpdateSlowMo(thisPtr);
        return;
    }

    int hSlowMoRecord = *(int*)((char*)thisPtr + phSlowMoRecord);

    // New activation, reset observation
    if (hSlowMoRecord != lastHSlowMoRecord)
    {
        slowMoChargeObserved = false;
        lastHSlowMoRecord = hSlowMoRecord;
    }

    // Latch: once we've seen a real charge, we trust future zero readings
    if (g_State.clientSlowMoCharge > 0.01)
    {
        slowMoChargeObserved = true;
    }

    // Only force-exit if we've confirmed charge was real and it's now depleted
    if (hSlowMoRecord != 0 && slowMoChargeObserved && g_State.clientSlowMoCharge <= 0.01)
    {
        ServerExitSlowMo(thisPtr, true, 0.0f);
        return;
    }
    ServerUpdateSlowMo(thisPtr);
}

// The server charge follows the charge displayed by the client, only with a single player as the client also drains its charge when another player uses slowmo
void __fastcall PlayerInventoryUpdate_Hook(int thisPtr, int)
{
    PlayerInventoryUpdate(thisPtr);

    if (!IsLocalPlayerAlone())
        return;

    float fMaxCharge = *(float*)(thisPtr + 0x64);
    *(float*)(thisPtr + 0x5C) = std::min(static_cast<float>(g_State.clientSlowMoCharge), fMaxCharge);
}

bool __fastcall CheckForSever_Hook(int* thisPtr, int, int a2, char a3, int a4)
{
    bool isSevered = CheckForSever(thisPtr, a2, a3, a4);

    if (isSevered)
    {
        DWORD pCharacter = *reinterpret_cast<DWORD*>(reinterpret_cast<DWORD>(thisPtr) + 0x4);
        if (pCharacter)
        {
            AI_SetNextServerUpdate(pCharacter, 0.01f); // prevent erratic ragdoll
        }
    }

    return isSevered;
}

void __fastcall AI_SetNextServerUpdate_Hook(DWORD thisPtr, int, float a2)
{
    AI_SetNextServerUpdate(thisPtr, 0.00000001f);
}

static void ApplyHighFPSFixesServerPatch()
{
    if (!HighFPSFixes) return;

    DWORD addr_ServerUpdateSlowMo = ScanModuleSignature(g_State.GameServer, "56 8B F1 8B 86 ?? ?? ?? ?? 85 C0 74 6B", "ServerUpdateSlowMo");
    DWORD addr_ServerExitSlowMo = ScanModuleSignature(g_State.GameServer, "51 53 8B D9 8B 83 ?? ?? ?? ?? 85 C0 0F 84 ?? ?? ?? ?? DD 05", "ServerExitSlowMo");
    DWORD addr_CheckForSever = ScanModuleSignature(g_State.GameServer, "83 EC 18 8B 44 24 1C 89 4C 24 08", "CheckForSever");
    DWORD addr_AI_SetNextServerUpdate = ScanModuleSignature(g_State.GameServer, "8B 44 24 10 83 C4 08 89 86", "AI_SetNextServerUpdate");
    DWORD addr_PlayerInventoryUpdate = ScanModuleSignature(g_State.GameServer, "51 A1 ?? ?? ?? ?? 56 8B F1 8B 88 ?? ?? 00 00 85 C9 74 ?? 8A 46 ?? 84 C0 74 ?? 8A 46 ?? 84 C0 74 ?? E8", "PlayerInventoryUpdate");
    DWORD addr_PlayerObjList = ScanModuleSignature(g_State.GameServer, "8B 35 ?? ?? ?? ?? 3B 35 ?? ?? ?? ?? 74 25 8B 87 ?? ?? 00 00 8B 0E 33 D2", "PlayerObjList");

    if (addr_ServerUpdateSlowMo == 0 || addr_ServerExitSlowMo == 0 || addr_CheckForSever == 0 || addr_AI_SetNextServerUpdate == 0 || addr_PlayerInventoryUpdate == 0 || addr_PlayerObjList == 0)
        return;

    // Needs the client charge, not available on a dedicated server
    if (g_State.isClientSlowMoTracked)
    {
        pPlayerObjListBegin = MemoryHelper::ReadMemory<DWORD>(addr_PlayerObjList + 0x2);
        pPlayerObjListEnd = MemoryHelper::ReadMemory<DWORD>(addr_PlayerObjList + 0x8);
        phSlowMoRecord = MemoryHelper::ReadMemory<int>(addr_ServerUpdateSlowMo + 0x5);
        HookHelper::ApplyHookReplaceable((void*)addr_ServerUpdateSlowMo, &ServerUpdateSlowMo_Hook, (LPVOID*)&ServerUpdateSlowMo);
        HookHelper::ApplyHookReplaceable((void*)addr_PlayerInventoryUpdate, &PlayerInventoryUpdate_Hook, (LPVOID*)&PlayerInventoryUpdate);
        ServerExitSlowMo = reinterpret_cast<decltype(ServerExitSlowMo)>((int)addr_ServerExitSlowMo);
    }

    HookHelper::ApplyHookReplaceable((void*)addr_CheckForSever, &CheckForSever_Hook, (LPVOID*)&CheckForSever);
    HookHelper::ApplyHookReplaceable((void*)(addr_AI_SetNextServerUpdate - 0x15), &AI_SetNextServerUpdate_Hook, (LPVOID*)&AI_SetNextServerUpdate);
}
