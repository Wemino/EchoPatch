#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

int(__cdecl* SetRenderMode)(int) = nullptr;
int(__thiscall* FindRenderMode)(int, int, int, int, int, char, char) = nullptr;

// =======================
// AutoResolution
// =======================

static int __cdecl SetRenderMode_Hook(int rMode)
{
    *(DWORD*)(rMode + 0x84) = g_State.screenWidth;
    *(DWORD*)(rMode + 0x88) = g_State.screenHeight;
    return SetRenderMode(rMode);
}

static int __fastcall FindRenderMode_Hook(int thisPtr, int, int deviceType, int width, int height, int bitDepth, char hardwareTnL, char windowed)
{
    int mode = FindRenderMode(thisPtr, deviceType, width, height, bitDepth, hardwareTnL, windowed);
    if (mode) return mode;

    // The saved resolution is not supported by the display, use the desktop one instead of falling back to 640x480
    return FindRenderMode(thisPtr, deviceType, *(int*)(thisPtr + 0x4), *(int*)(thisPtr + 0x8), bitDepth, hardwareTnL, windowed);
}

static void ApplyAutoResolution()
{
    if (AutoResolution == 0) return;

    MemoryHelper::WriteMemory<int>(GetAddress(Addr::ScreenWidth), g_State.screenWidth, false);
    MemoryHelper::WriteMemory<int>(GetAddress(Addr::ScreenHeight), g_State.screenHeight, false);

    HookHelper::ApplyHook((void*)GetAddress(Addr::FindRenderMode), &FindRenderMode_Hook, (LPVOID*)&FindRenderMode, g_State.IsOriginalGame());
}

static void ApplyForceRenderMode()
{
    if (AutoResolution != 2) return;

    HookHelper::ApplyHook((void*)GetAddress(Addr::SetRenderMode), &SetRenderMode_Hook, (LPVOID*)&SetRenderMode, g_State.IsOriginalGame());
}
