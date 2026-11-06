#pragma once

#include <algorithm>
#include <climits>
#include <vector>
#include "../../Globals.cpp"
#include "../../Addresses.cpp"
#include "SSAAResolve.cpp"

int(__thiscall* InitializePresentationParameters)(DWORD*, DWORD*, unsigned __int8) = nullptr;

static IDirect3D9* const* s_direct3D = nullptr;
static FpsLimiter fpsLimiter;
static bool isUsingFpsLimiter = false;

static constexpr UINT DEFAULT_MAX_FPS = 240;

static constexpr UINT MIN_MODE_WIDTH = 640;
static constexpr UINT MIN_MODE_HEIGHT = 480;
static constexpr UINT MIN_REFRESH_RATE = 60;

// ====================================
// MaxFPS
// ====================================

static std::vector<UINT> GetSupportedRefreshRates(UINT width, UINT height)
{
    std::vector<UINT> refreshRates;

    IDirect3D9* d3d = *s_direct3D;
    if (!d3d) return refreshRates;

    UINT modeCount = d3d->GetAdapterModeCount(g_State.displayAdapter, D3DFMT_X8R8G8B8);
    for (UINT i = 0; i < modeCount; i++)
    {
        D3DDISPLAYMODE mode{};
        if (FAILED(d3d->EnumAdapterModes(g_State.displayAdapter, D3DFMT_X8R8G8B8, i, &mode)))
            continue;

        if (mode.Width < MIN_MODE_WIDTH || mode.Height < MIN_MODE_HEIGHT || mode.RefreshRate < MIN_REFRESH_RATE)
            continue;

        if (width != 0 && (mode.Width != width || mode.Height != height))
            continue;

        if (std::find(refreshRates.begin(), refreshRates.end(), mode.RefreshRate) == refreshRates.end())
        {
            refreshRates.push_back(mode.RefreshRate);
        }
    }

    std::sort(refreshRates.begin(), refreshRates.end());
    return refreshRates;
}

static std::vector<UINT> GetDisplayRefreshRates()
{
    std::vector<UINT> refreshRates = GetSupportedRefreshRates(g_State.displayWidth, g_State.displayHeight);

    if (refreshRates.empty())
    {
        refreshRates = GetSupportedRefreshRates(0, 0);
    }

    if (refreshRates.empty())
    {
        UINT displayFrequency = SystemHelper::GetCurrentDisplayFrequency();
        refreshRates.push_back(displayFrequency >= MIN_REFRESH_RATE ? displayFrequency : DEFAULT_MAX_FPS);
    }

    return refreshRates;
}

// MaxFPS is -1 until a limit is chosen, it is also treated as -1 if it isn't a refresh rate of the display
static UINT GetMaxFPS()
{
    std::vector<UINT> refreshRates = GetDisplayRefreshRates();

    UINT maxFPS = refreshRates.front();
    UINT maxFPSDistance = UINT_MAX;
    for (UINT refreshRate : refreshRates)
    {
        if (static_cast<float>(refreshRate) == MaxFPS)
            return refreshRate;

        UINT distance = refreshRate > DEFAULT_MAX_FPS ? refreshRate - DEFAULT_MAX_FPS : DEFAULT_MAX_FPS - refreshRate;
        if (distance < maxFPSDistance)
        {
            maxFPS = refreshRate;
            maxFPSDistance = distance;
        }
    }

    return maxFPS;
}

static UINT GetFullscreenRefreshRate()
{
    if (g_State.isWindowed || !g_State.isVsyncEnabled) return D3DPRESENT_RATE_DEFAULT;

    UINT maxFPS = GetMaxFPS();
    std::vector<UINT> refreshRates = GetSupportedRefreshRates(g_State.displayWidth, g_State.displayHeight);

    return std::find(refreshRates.begin(), refreshRates.end(), maxFPS) != refreshRates.end() ? maxFPS : D3DPRESENT_RATE_DEFAULT;
}

static bool IsRefreshRateOutdated()
{
    return GetFullscreenRefreshRate() != g_State.refreshRate;
}

static void UpdateFpsLimiter()
{
    if (!g_State.isVsyncEnabled)
    {
        isUsingFpsLimiter = false;
        return;
    }

    UINT maxFPS = GetMaxFPS();

    UINT refreshRate = g_State.refreshRate != D3DPRESENT_RATE_DEFAULT ? g_State.refreshRate : SystemHelper::GetCurrentDisplayFrequency();
    isUsingFpsLimiter = refreshRate < MIN_REFRESH_RATE || maxFPS < refreshRate;
    fpsLimiter.SetTargetFps(maxFPS);
}

// ====================================
// MaxFPS & WindowStylePerMode
// ====================================

static int __fastcall InitializePresentationParameters_Hook(DWORD* thisPtr, int, DWORD* a2, unsigned __int8 a3)
{
    // Called before the device is created or reset, a2 = resolution and a3 = windowed mode
    if (WindowStylePerMode)
    {
        UpdateWindowStyle(g_State.hWnd, a2[0], a2[1], a3 != 0);
    }

    // The SSAA resolve resources can't outlive the device or survive a reset
    if (SSAAScale != 1.0f)
    {
        ReleaseSSAAResolve();
    }

    int res = InitializePresentationParameters(thisPtr, a2, a3);

    // thisPtr[81] = adapter of the device, thisPtr[100] = refresh rate and thisPtr[101] = presentation interval
    if (thisPtr[81])
    {
        g_State.displayAdapter = *reinterpret_cast<UINT*>(thisPtr[81]);
    }

    g_State.displayWidth = a2[0];
    g_State.displayHeight = a2[1];
    g_State.isWindowed = a3 != 0;
    g_State.isVsyncEnabled = thisPtr[101] != D3DPRESENT_INTERVAL_IMMEDIATE;
    g_State.refreshRate = GetFullscreenRefreshRate();
    thisPtr[100] = g_State.refreshRate;

    UpdateFpsLimiter();

    return res;
}

static void HookPresentationParameters()
{
    s_direct3D = reinterpret_cast<IDirect3D9* const*>(GetAddress(Addr::Direct3D));

    HookHelper::ApplyHook((void*)GetAddress(Addr::InitializePresentationParameters), &InitializePresentationParameters_Hook, (LPVOID*)&InitializePresentationParameters, g_State.CurrentFEARGame == FEAR);
}
