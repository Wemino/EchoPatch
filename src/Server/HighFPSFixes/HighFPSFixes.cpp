#pragma once

#include "../../Globals.cpp"

void(__thiscall* ServerUpdateSlowMo)(int*) = nullptr;
void(__thiscall* ServerExitSlowMo)(int*, bool, float) = nullptr;

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
