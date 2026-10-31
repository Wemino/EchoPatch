#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

// ===========================
// WindowStylePerMode
// ===========================

static void UpdateWindowStyle(HWND hWnd, UINT width, UINT height, bool windowed)
{
    if (!hWnd || width == 0 || height == 0) return;

    LONG visible = GetWindowLongA(hWnd, GWL_STYLE) & WS_VISIBLE;

    // Borderless in fullscreen
    if (!windowed)
    {
        SetWindowLongA(hWnd, GWL_STYLE, WS_POPUP | visible);
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
        ShowWindow(hWnd, SW_SHOW);
        return;
    }

    LONG windowedStyle = FixWindowStyle ? (WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX) : (WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);

    // Restore the title bar when coming back from fullscreen
    bool styleChanged = false;
    if ((GetWindowLongA(hWnd, GWL_STYLE) & windowedStyle) != windowedStyle)
    {
        SetWindowLongA(hWnd, GWL_STYLE, windowedStyle | visible);
        styleChanged = true;
    }

    RECT client{};
    GetClientRect(hWnd, &client);
    LONG clientWidth = client.right - client.left;
    LONG clientHeight = client.bottom - client.top;

    if (styleChanged || clientWidth != static_cast<LONG>(width) || clientHeight != static_cast<LONG>(height))
    {
        // Resize around the current center so the client area matches the resolution
        POINT center{ clientWidth / 2, clientHeight / 2 };
        ClientToScreen(hWnd, &center);

        RECT rc{};
        rc.left = center.x - static_cast<LONG>(width / 2);
        rc.top = center.y - static_cast<LONG>(height / 2);
        rc.right = rc.left + static_cast<LONG>(width);
        rc.bottom = rc.top + static_cast<LONG>(height);
        AdjustWindowRectEx(&rc, static_cast<DWORD>(GetWindowLongA(hWnd, GWL_STYLE)), FALSE, static_cast<DWORD>(GetWindowLongA(hWnd, GWL_EXSTYLE)));

        // Keep the title bar on screen
        MONITORINFO mi{};
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfoA(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &mi))
        {
            if (rc.left < mi.rcWork.left) { rc.right += mi.rcWork.left - rc.left; rc.left = mi.rcWork.left; }
            if (rc.top < mi.rcWork.top) { rc.bottom += mi.rcWork.top - rc.top; rc.top = mi.rcWork.top; }
        }

        // No longer topmost after leaving fullscreen
        SetWindowPos(hWnd, HWND_NOTOPMOST, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }

    ShowWindow(hWnd, SW_SHOW);
}
