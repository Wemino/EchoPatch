#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"
#include "ConsoleMgr.hpp"
#include "../../Controller/Controller.hpp"

int(__thiscall* MainGameLoop)(int) = nullptr;

// ===============================================
// MaxFPS & SDLGamepadSupport & ConsoleEnabled
// ===============================================

static int __fastcall MainGameLoop_Hook(int thisPtr, int)
{
    if (g_State.isUsingFpsLimiter)
    {
        g_State.fpsLimiter.Limit();
    }

    if (SDLGamepadSupport)
    {
        PollController();
    }

    if (ConsoleEnabled)
    {
        g_State.isConsoleOpen = Console::Update();
    }

    if (HighFPSFixes)
    {
        if (auto* pTimingStruct = *reinterpret_cast<void**>(thisPtr + 0x5D0))
        {
            g_State.simulationFrameTime = *reinterpret_cast<float*>(static_cast<char*>(pTimingStruct) + 0x2C);
            g_State.totalGameTime += g_State.simulationFrameTime;
        }
    }

    return MainGameLoop(thisPtr);
}

static void HookMainLoop()
{
    g_State.isUsingFpsLimiter = MaxFPS != 0 && !g_State.useVsyncOverride;
    if (!g_State.isUsingFpsLimiter && !SDLGamepadSupport && !HighFPSFixes && !ConsoleEnabled) return;

    g_State.fpsLimiter.SetTargetFps(MaxFPS);

    HookHelper::ApplyHook((void*)GetAddress(Addr::MainGameLoop), &MainGameLoop_Hook, (LPVOID*)&MainGameLoop, g_State.CurrentFEARGame == FEAR);
}
