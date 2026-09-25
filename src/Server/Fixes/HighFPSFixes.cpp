#pragma once

#include "../../Globals.cpp"

void(__thiscall* ServerUpdateSlowMo)(int*) = nullptr;
void(__thiscall* ServerExitSlowMo)(int*, bool, float) = nullptr;
bool(__thiscall* CheckForSever)(int*, int, char, int) = nullptr;
void(__thiscall* AI_SetNextServerUpdate)(DWORD, float) = nullptr;

// ========================
// HighFPSFixes
// ========================

void __fastcall ServerUpdateSlowMo_Hook(int* thisPtr, int)
{
    int hSlowMoRecord = *(int*)((char*)thisPtr + g_State.phSlowMoRecord);

    // New activation, reset observation
    if (hSlowMoRecord != g_State.lastHSlowMoRecord)
    {
        g_State.slowMoChargeObserved = false;
        g_State.lastHSlowMoRecord = hSlowMoRecord;
    }

    // Latch: once we've seen a real charge, we trust future zero readings
    if (g_State.clientSlowMoCharge > 0.01)
    {
        g_State.slowMoChargeObserved = true;
    }

    // Only force-exit if we've confirmed charge was real and it's now depleted
    if (hSlowMoRecord != 0 && g_State.slowMoChargeObserved && g_State.clientSlowMoCharge <= 0.01)
    {
        ServerExitSlowMo(thisPtr, true, 0.0f);
        return;
    }
    ServerUpdateSlowMo(thisPtr);
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

    if (addr_ServerUpdateSlowMo == 0 || addr_ServerExitSlowMo == 0 || addr_CheckForSever == 0 || addr_AI_SetNextServerUpdate == 0)
        return;

    g_State.phSlowMoRecord = MemoryHelper::ReadMemory<int>(addr_ServerUpdateSlowMo + 0x5);
    HookHelper::ApplyHookReplaceable((void*)addr_ServerUpdateSlowMo, &ServerUpdateSlowMo_Hook, (LPVOID*)&ServerUpdateSlowMo);
    ServerExitSlowMo = reinterpret_cast<decltype(ServerExitSlowMo)>((int)addr_ServerExitSlowMo);

    HookHelper::ApplyHookReplaceable((void*)addr_CheckForSever, &CheckForSever_Hook, (LPVOID*)&CheckForSever);
    HookHelper::ApplyHookReplaceable((void*)(addr_AI_SetNextServerUpdate - 0x15), &AI_SetNextServerUpdate_Hook, (LPVOID*)&AI_SetNextServerUpdate);
}
