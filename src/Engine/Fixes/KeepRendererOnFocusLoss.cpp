#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

int(__cdecl* TermRenderer)() = nullptr;
int(__cdecl* InitRenderer)() = nullptr;

static IDirect3DDevice9* const* s_focusDevice = nullptr;
static const volatile uint8_t* s_rendererActive = nullptr;
static bool isRestorePending = false;

// ===========================
// KeepRendererOnFocusLoss
// ===========================

static HRESULT GetDeviceState()
{
    IDirect3DDevice9* device = *s_focusDevice;
    return device ? device->TestCooperativeLevel() : D3DERR_INVALIDCALL;
}

static int __cdecl FocusLostTermRenderer_Hook()
{
    // Only shut down the renderer if the window is minimized or the device is lost
    if (!IsIconic(g_State.hWnd) && GetDeviceState() == D3D_OK)
        return 0;

    return TermRenderer();
}

static int __cdecl FocusGainedInitRenderer_Hook()
{
    isRestorePending = false;

    // The device was kept, nothing to restore
    if (*s_rendererActive && GetDeviceState() == D3D_OK)
        return 0;

    return InitRenderer();
}

// The device can still be lost without a focus change (session lock, display change)
static void RecoverLostDevice()
{
    if (!*s_focusDevice) return;

    HRESULT hr = GetDeviceState();

    if (*s_rendererActive)
    {
        if (hr != D3DERR_DEVICENOTRESET) return;

        // Same sequence as a focus change
        TermRenderer();
        isRestorePending = true;
    }

    if (isRestorePending && hr != D3DERR_DEVICELOST && GetForegroundWindow() == g_State.hWnd)
    {
        if (InitRenderer() == 0)
        {
            isRestorePending = false;
        }
    }
}

static void ApplyKeepRendererOnFocusLoss()
{
    if (!KeepRendererOnFocusLoss) return;

    DWORD addr_FocusLost = GetAddress(Addr::FocusLostTermRenderer);
    DWORD addr_FocusGained = GetAddress(Addr::FocusGainedInitRenderer);

    s_focusDevice = reinterpret_cast<IDirect3DDevice9* const*>(GetAddress(Addr::RendererDevice));
    s_rendererActive = reinterpret_cast<const volatile uint8_t*>(GetAddress(Addr::RendererActive));

    TermRenderer = reinterpret_cast<decltype(TermRenderer)>(MemoryHelper::ResolveRelativeAddress(addr_FocusLost, 0x1));
    InitRenderer = reinterpret_cast<decltype(InitRenderer)>(MemoryHelper::ResolveRelativeAddress(addr_FocusGained, 0x1));

    MemoryHelper::MakeCALL(addr_FocusLost, reinterpret_cast<uintptr_t>(&FocusLostTermRenderer_Hook));
    MemoryHelper::MakeCALL(addr_FocusGained, reinterpret_cast<uintptr_t>(&FocusGainedInitRenderer_Hook));
}
