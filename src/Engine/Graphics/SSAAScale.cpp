#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"
#include "SSAAResolve.cpp"

int(__stdcall* CreateRenderTarget)(int, int, int, DWORD*) = nullptr;
int(__stdcall* StretchRect)(int, const float*, int, const float*) = nullptr;

static IDirect3DDevice9* const* ssaaDevice = nullptr;

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

        D3DCAPS9 caps;
        IDirect3DDevice9* device = *ssaaDevice;
        if (device && SUCCEEDED(device->GetDeviceCaps(&caps)) && (width > (int)caps.MaxTextureWidth || height > (int)caps.MaxTextureHeight))
        {
            float scale = std::min((float)caps.MaxTextureWidth / width, (float)caps.MaxTextureHeight / height);
            width = std::min((int)(width * scale), (int)caps.MaxTextureWidth);
            height = std::min((int)(height * scale), (int)caps.MaxTextureHeight);
        }

        g_State.ssaaWidth = width;
        g_State.ssaaHeight = height;
    }

    return CreateRenderTarget(width, height, flags, target);
}

static bool ResolveSSAAToScreen()
{
    IDirect3DDevice9* device = *ssaaDevice;
    if (!device) return false;

    IDirect3DSurface9* sourceSurface = nullptr;
    IDirect3DTexture9* sourceTexture = nullptr;
    IDirect3DSurface9* backBuffer = nullptr;
    D3DSURFACE_DESC desc;
    bool result = false;

    // The scene target is still bound when its copy to the screen is requested
    if (SUCCEEDED(device->GetRenderTarget(0, &sourceSurface)) &&
        SUCCEEDED(sourceSurface->GetDesc(&desc)) &&
        desc.Width == (UINT)g_State.ssaaWidth && desc.Height == (UINT)g_State.ssaaHeight &&
        SUCCEEDED(sourceSurface->GetContainer(__uuidof(IDirect3DTexture9), reinterpret_cast<void**>(&sourceTexture))) &&
        SUCCEEDED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer)))
    {
        result = ResolveSSAA(device, sourceTexture, backBuffer);
    }

    if (backBuffer) backBuffer->Release();
    if (sourceTexture) sourceTexture->Release();
    if (sourceSurface) sourceSurface->Release();
    return result;
}

static int __stdcall StretchRect_Hook(int sourceTarget, const float* sourceRect, int destTarget, const float* destRect)
{
    // CPlayerCamera::RenderCamera copying the supersampled scene to the screen
    if (sourceTarget != 0 && sourceTarget == g_State.ssaaRenderTarget && destTarget == 0)
    {
        g_State.isRenderingSSAAScene = false;

        if (ResolveSSAAToScreen())
            return 0;
    }

    return StretchRect(sourceTarget, sourceRect, destTarget, destRect);
}

static void ApplySSAAScale()
{
    if (SSAAScale == 1.0f) return;

    ssaaDevice = reinterpret_cast<IDirect3DDevice9* const*>(GetAddress(Addr::RendererDevice));

    HookHelper::ApplyHook((void*)GetAddress(Addr::CreateRenderTarget), &CreateRenderTarget_Hook, (LPVOID*)&CreateRenderTarget);
    HookHelper::ApplyHook((void*)GetAddress(Addr::StretchRect), &StretchRect_Hook, (LPVOID*)&StretchRect);
}
