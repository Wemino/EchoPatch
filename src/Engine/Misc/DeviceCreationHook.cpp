#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

bool(__thiscall* CreateAndInitializeDevice)(DWORD*, DWORD*, DWORD*, int, char) = nullptr;

static bool __fastcall CreateAndInitializeDevice_Hook(DWORD* thisp, int, DWORD* a2, DWORD* a3, int a4, char a5)
{
    if (FastVRAMDetection)
    {
        IDXGIFactory1* pFactory = nullptr;
        if (CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory) == S_OK)
        {
            IDXGIAdapter* pAdapter = nullptr;
            if (pFactory->EnumAdapters(a2[0], &pAdapter) == S_OK)
            {
                DXGI_ADAPTER_DESC desc;
                if (pAdapter->GetDesc(&desc) == S_OK)
                {
                    g_State.cachedVRAM = desc.DedicatedVideoMemory;
                }
                pAdapter->Release();
            }
            pFactory->Release();
        }

        if (g_State.cachedVRAM < (512u << 20))
        {
            g_State.cachedVRAM = 0;
        }

        if (g_State.cachedVRAM > 0xFFF00000)
        {
            g_State.cachedVRAM = 0xFFF00000;
        }
    }

    if (FixNvidiaShadowCorruption)
    {
        DWORD VendorId = a2[267];

        // NVIDIA device?
        g_State.isUsingNvidiaDevice = (VendorId == 0x10DE);
    }

    bool result = CreateAndInitializeDevice(thisp, a2, a3, a4, a5);

    if (result)
    {
        IDirect3DDevice9* device = reinterpret_cast<IDirect3DDevice9*>(thisp[0]);
        if (!device) return result;

        if (FastVRAMDetection && g_State.cachedVRAM == 0)
        {
            UINT availMem = device->GetAvailableTextureMem();
            g_State.cachedVRAM = (availMem > 0xFFF00000) ? 0xFFF00000 : availMem;
        }

        void** vtable = *reinterpret_cast<void***>(device);

        if (ReducedMipMapBias)
        {
            void* newSetTextureAddr = vtable[65];
            if (g_State.hookedSetTextureAddr)
            {
                MH_RemoveHook(g_State.hookedSetTextureAddr);
                D3D9_SetTexture = nullptr;
            }

            g_State.sharpTextures.clear();
            std::fill(std::begin(g_State.stageIsDirty), std::end(g_State.stageIsDirty), false);
            HookHelper::ApplyHook(newSetTextureAddr, &SetTexture_Hook, (LPVOID*)&D3D9_SetTexture);
            g_State.hookedSetTextureAddr = newSetTextureAddr;
            g_State.hookedDevice = device;
        }
    }

    return result;
}

static void ApplyDeviceCreationHook()
{
    if (!FixNvidiaShadowCorruption && !ReducedMipMapBias && !FastVRAMDetection) return;

    HookHelper::ApplyHook((void*)GetAddress(Addr::CreateAndInitializeDevice), &CreateAndInitializeDevice_Hook, (LPVOID*)&CreateAndInitializeDevice, g_State.CurrentFEARGame == FEAR);
}
