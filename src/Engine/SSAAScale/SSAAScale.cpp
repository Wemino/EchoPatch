#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

int(__stdcall* CreateRenderTarget)(int, int, int, DWORD*) = nullptr;

// =====================
// SSAAScale
// =====================

static int __stdcall CreateRenderTarget_Hook(int width, int height, int flags, DWORD* target)
{
    if (g_State.isInCameraUpdateRenderTarget)
    {
        width = (int)(g_State.currentWidth * SSAAScale + 0.5f);
        height = (int)(g_State.currentHeight * SSAAScale + 0.5f);
        flags |= 0x300; // eRTO_DepthBufferTexture | eRTO_ShadowBuffer
    }

    return CreateRenderTarget(width, height, flags, target);
}

static void ApplySSAAScale()
{
    if (SSAAScale == 1.0f) return;

    static const uint8_t SSAA_LinearFilter[10] = { 0x6A, 0x02, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

    MemoryHelper::WriteMemoryRaw(GetAddress(Addr::SSAALinearFilter), SSAA_LinearFilter, sizeof(SSAA_LinearFilter));
    HookHelper::ApplyHook((void*)GetAddress(Addr::CreateRenderTarget), &CreateRenderTarget_Hook, (LPVOID*)&CreateRenderTarget);
}
