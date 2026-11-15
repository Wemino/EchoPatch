#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

static void* hookedSetTextureAddr = nullptr;

bool(__thiscall* CreateAndInitializeDevice)(DWORD*, DWORD*, DWORD*, int, char) = nullptr;

static IDXGIAdapter* FindMonitorAdapter(IDXGIFactory1* pFactory, HMONITOR hMonitor)
{
    IDXGIAdapter* pAdapter = nullptr;
    for (UINT i = 0; pFactory->EnumAdapters(i, &pAdapter) == S_OK; i++)
    {
        IDXGIOutput* pOutput = nullptr;
        for (UINT j = 0; pAdapter->EnumOutputs(j, &pOutput) == S_OK; j++)
        {
            DXGI_OUTPUT_DESC desc;
            bool isMonitorOutput = pOutput->GetDesc(&desc) == S_OK && desc.Monitor == hMonitor;
            pOutput->Release();

            if (isMonitorOutput)
                return pAdapter;
        }

        pAdapter->Release();
    }

    return nullptr;
}

static bool __fastcall CreateAndInitializeDevice_Hook(DWORD* thisp, int, DWORD* a2, DWORD* a3, int a4, char a5)
{
    if (FastVRAMDetection)
    {
        IDXGIFactory1* pFactory = nullptr;
        if (CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory) == S_OK)
        {
            IDirect3D9* d3d = *reinterpret_cast<IDirect3D9**>(GetAddress(Addr::Direct3D));
            HMONITOR hMonitor = d3d ? d3d->GetAdapterMonitor(a2[0]) : nullptr;
            IDXGIAdapter* pAdapter = hMonitor ? FindMonitorAdapter(pFactory, hMonitor) : nullptr;
            if (pAdapter)
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
            if (hookedSetTextureAddr)
            {
                MH_RemoveHook(hookedSetTextureAddr);
                D3D9_SetTexture = nullptr;
            }

            g_State.sharpTextures.clear();
            std::fill(std::begin(g_State.stageIsDirty), std::end(g_State.stageIsDirty), false);
            HookHelper::ApplyHook(newSetTextureAddr, &SetTexture_Hook, (LPVOID*)&D3D9_SetTexture);
            hookedSetTextureAddr = newSetTextureAddr;
        }

        if (ReduceTextureMemory)
        {
            HookTextureDeviceMethods(vtable);
        }
    }

    return result;
}

static void ApplyDeviceCreationHook()
{
    if (!ReducedMipMapBias && !FastVRAMDetection && !ReduceTextureMemory) return;

    HookHelper::ApplyHook((void*)GetAddress(Addr::CreateAndInitializeDevice), &CreateAndInitializeDevice_Hook, (LPVOID*)&CreateAndInitializeDevice, g_State.CurrentFEARGame == FEAR);
}
