#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

bool(__cdecl* LoadWorldShadows)(int*) = nullptr;

static void SetVertexBufferPool(uint8_t pool)
{
    uintptr_t address = GetAddress(Addr::VertexBufferPool);
    if (!address) return;

    MemoryHelper::WriteMemory<uint8_t>(address, pool);
}

// ===========================
// FixNvidiaShadowCorruption
// ===========================

static bool __cdecl LoadWorldShadows_Hook(int* a1)
{
    if (g_State.isUsingNvidiaDevice)
        SetVertexBufferPool(D3DPOOL_SYSTEMMEM);

    bool result = LoadWorldShadows(a1);

    if (g_State.isUsingNvidiaDevice)
        SetVertexBufferPool(D3DPOOL_MANAGED);

    return result;
}

static void ApplyFixNvidiaShadowCorruption()
{
    if (!FixNvidiaShadowCorruption) return;

    HookHelper::ApplyHook((void*)GetAddress(Addr::LoadWorldShadows), &LoadWorldShadows_Hook, (LPVOID*)&LoadWorldShadows, g_State.CurrentFEARGame == FEAR);
}
