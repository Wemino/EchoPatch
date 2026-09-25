#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

// =======================
// FastVRAMDetection
// =======================

static uint64_t __stdcall GetVRAM_Hook()
{
    if (g_State.cachedVRAM != 0)
        return g_State.cachedVRAM;

    // Failsafe
    return 0x20000000; // 512MB
}

static void ApplyFastVRAMDetection()
{
    if (!FastVRAMDetection) return;

    MemoryHelper::MakeJMP(GetAddress(Addr::GetVRAM_1), (uintptr_t)&GetVRAM_Hook);

    uintptr_t secondary = GetAddress(Addr::GetVRAM_2);
    if (secondary)
    {
        MemoryHelper::MakeJMP(secondary, (uintptr_t)&GetVRAM_Hook);
    }
}
