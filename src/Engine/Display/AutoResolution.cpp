#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

int(__cdecl* SetRenderMode)(int) = nullptr;

// =======================
// AutoResolution
// =======================

static int __cdecl SetRenderMode_Hook(int rMode)
{
    *(DWORD*)(rMode + 0x84) = g_State.screenWidth;
    *(DWORD*)(rMode + 0x88) = g_State.screenHeight;
    return SetRenderMode(rMode);
}

static void ApplyAutoResolution()
{
    if (AutoResolution == 0) return;

    MemoryHelper::WriteMemory<int>(GetAddress(Addr::ScreenWidth), g_State.screenWidth, false);
    MemoryHelper::WriteMemory<int>(GetAddress(Addr::ScreenHeight), g_State.screenHeight, false);
}

static void ApplyForceRenderMode()
{
    if (AutoResolution != 2) return;

    HookHelper::ApplyHook((void*)GetAddress(Addr::SetRenderMode), &SetRenderMode_Hook, (LPVOID*)&SetRenderMode, g_State.IsOriginalGame());
}
