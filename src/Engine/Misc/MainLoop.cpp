#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"
#include "ConsoleMgr.hpp"
#include "../../Controller/Controller.hpp"

static FpsLimiter fpsLimiter{ 300.0f };
static bool isUsingFpsLimiter = false;
static LARGE_INTEGER performanceFrequency{};
static LARGE_INTEGER lastFrameCounter{};

int(__thiscall* MainGameLoop)(int) = nullptr;

// ===============================================
// MaxFPS & SDLGamepadSupport & ConsoleEnabled
// ===============================================

static int __fastcall MainGameLoop_Hook(int thisPtr, int)
{
    if (isUsingFpsLimiter)
    {
        fpsLimiter.Limit();
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

        // The simulation frame time is scaled by slow-mo, things done once per frame follow the real time
        LARGE_INTEGER frameCounter;
        QueryPerformanceCounter(&frameCounter);
        if (lastFrameCounter.QuadPart != 0)
        {
            g_State.realFrameTime = static_cast<double>(frameCounter.QuadPart - lastFrameCounter.QuadPart) / performanceFrequency.QuadPart;
            g_State.totalRealTime += g_State.realFrameTime;
        }
        lastFrameCounter = frameCounter;
        g_State.frameCount++;
    }

    return MainGameLoop(thisPtr);
}

static void HookMainLoop()
{
    isUsingFpsLimiter = MaxFPS != 0 && !g_State.useVsyncOverride;
    if (!isUsingFpsLimiter && !SDLGamepadSupport && !HighFPSFixes && !ConsoleEnabled) return;

    fpsLimiter.SetTargetFps(MaxFPS);
    QueryPerformanceFrequency(&performanceFrequency);

    HookHelper::ApplyHook((void*)GetAddress(Addr::MainGameLoop), &MainGameLoop_Hook, (LPVOID*)&MainGameLoop, g_State.CurrentFEARGame == FEAR);
}
