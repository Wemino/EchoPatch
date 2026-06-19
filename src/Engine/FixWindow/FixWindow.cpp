#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

BOOL(WINAPI* ori_AdjustWindowRect)(LPRECT, DWORD, BOOL);

static BOOL WINAPI AdjustWindowRect_Hook(LPRECT lpRect, DWORD dwStyle, BOOL bMenu)
{
    if (dwStyle == 0xCF0000)
    {
        dwStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    }

    return ori_AdjustWindowRect(lpRect, dwStyle, bMenu);
}

static void ApplyFixWindow()
{
    if (!FixWindowStyle) return;

    HookHelper::ApplyHookAPI(L"user32.dll", "AdjustWindowRect", &AdjustWindowRect_Hook, (LPVOID*)&ori_AdjustWindowRect);

    // Remove 'DISCL_NOWINKEY'
    MemoryHelper::WriteMemory<uint8_t>(GetAddress(Addr::DisableNoWinKey), 6);
}
