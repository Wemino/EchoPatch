#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

int(__thiscall* InitializePresentationParameters)(DWORD*, DWORD*, unsigned __int8) = nullptr;

// ====================================
// DynamicVsync & WindowStylePerMode
// ====================================

static int __fastcall InitializePresentationParameters_Hook(DWORD* thisPtr, int, DWORD* a2, unsigned __int8 a3)
{
    // Called before the device is created or reset, a2 = resolution and a3 = windowed mode
    if (WindowStylePerMode)
    {
        UpdateWindowStyle(g_State.hWnd, a2[0], a2[1], a3 != 0);
    }

    int res = InitializePresentationParameters(thisPtr, a2, a3);

    if (DynamicVsync)
    {
        thisPtr[101] = g_State.useVsyncOverride != 0 ? 0 : 0x80000000;
    }

    return res;
}

static void HookVSyncOverride()
{
    if (!DynamicVsync && !WindowStylePerMode) return;

    HookHelper::ApplyHook((void*)GetAddress(Addr::InitializePresentationParameters), &InitializePresentationParameters_Hook, (LPVOID*)&InitializePresentationParameters, g_State.CurrentFEARGame == FEAR);
}
