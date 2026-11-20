#pragma once

#include "../../Globals.cpp"

uint32_t(__thiscall* GetSeverFrequencyCap)(int) = nullptr;
uint32_t(__thiscall* GetSeverTotalCap)(int) = nullptr;
uint32_t(__thiscall* GetGibFrequencyCap)(int) = nullptr;

// ====================
// UncapDismemberment
// ====================

static uint32_t __fastcall GetSeverFrequencyCap_Hook(int, int)
{
    return 0;
}

// Same value as the BodySeverTest debug variable, which also forces every sever chance to 100%
static uint32_t __fastcall GetSeverTotalCap_Hook(int, int)
{
    return 9999;
}

static uint32_t __fastcall GetGibFrequencyCap_Hook(int, int)
{
    return static_cast<uint32_t>(GibFrequencyCap);
}

static void ApplyUncapDismembermentServerPatch()
{
    if (!UncapDismemberment) return;

    DWORD addr_GetSeverTotalCap = ScanModuleSignature(g_State.GameServer, "56 8B F1 8B 0D ?? ?? ?? ?? 85 C9 74 23 8B 15 ?? ?? ?? ?? 8B 01 52 FF 90 ?? 01 00 00 D8 1D ?? ?? ?? ?? DF E0 F6 C4 41 75 07 B8 0F 27 00 00 5E C3", "GetSeverTotalCap");

    if (addr_GetSeverTotalCap == 0)
        return;

    HookHelper::ApplyHookReplaceable((void*)addr_GetSeverTotalCap, &GetSeverTotalCap_Hook, (LPVOID*)&GetSeverTotalCap);
    HookHelper::ApplyHookReplaceable((void*)(addr_GetSeverTotalCap - 0x30), &GetSeverFrequencyCap_Hook, (LPVOID*)&GetSeverFrequencyCap);
    HookHelper::ApplyHookReplaceable((void*)(addr_GetSeverTotalCap - 0x60), &GetGibFrequencyCap_Hook, (LPVOID*)&GetGibFrequencyCap);
}