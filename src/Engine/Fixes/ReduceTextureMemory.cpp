#pragma once

#include <algorithm>
#include <vector>
#include "../../Globals.cpp"
#include "../../Addresses.cpp"
#include "../Graphics/ReducedMipMapBias.cpp"

HRESULT(WINAPI* D3D9_Reset)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*) = nullptr;
HRESULT(WINAPI* D3D9_CreateTexture)(IDirect3DDevice9*, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DTexture9**, HANDLE*) = nullptr;
HRESULT(WINAPI* D3D9_CreateCubeTexture)(IDirect3DDevice9*, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DCubeTexture9**, HANDLE*) = nullptr;
void(__cdecl* RebindTextures)() = nullptr;

// Texture wrappers holding a default pool texture, sorted
static std::vector<int*> videoMemoryTextures;

static bool isTextureDeviceHooked = false;
static bool isCreatingFileTexture = false;
static bool isReloadingTextures = false;
static bool hasReleasedTextures = false;

// ===========================
// ReduceTextureMemory
// ===========================

// Managed textures keep a copy in the memory of the game, the engine fills a system memory texture instead
static HRESULT WINAPI CreateTexture_Hook(IDirect3DDevice9* device, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle)
{
    if (isCreatingFileTexture && Pool == D3DPOOL_MANAGED && Usage == 0)
    {
        Pool = D3DPOOL_SYSTEMMEM;
    }

    return D3D9_CreateTexture(device, Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle);
}

static HRESULT WINAPI CreateCubeTexture_Hook(IDirect3DDevice9* device, UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle)
{
    if (isCreatingFileTexture && Pool == D3DPOOL_MANAGED && Usage == 0)
    {
        Pool = D3DPOOL_SYSTEMMEM;
    }

    return D3D9_CreateCubeTexture(device, EdgeLength, Levels, Usage, Format, Pool, ppCubeTexture, pSharedHandle);
}

static HRESULT GetTextureSurface(IDirect3DBaseTexture9* texture, bool isCube, UINT face, UINT level, IDirect3DSurface9** surface)
{
    if (isCube)
        return static_cast<IDirect3DCubeTexture9*>(texture)->GetCubeMapSurface(static_cast<D3DCUBEMAP_FACES>(face), level, surface);

    return static_cast<IDirect3DTexture9*>(texture)->GetSurfaceLevel(level, surface);
}

static bool CopySurface(IDirect3DSurface9* source, IDirect3DSurface9* target)
{
    D3DSURFACE_DESC desc;
    D3DLOCKED_RECT from, to;

    if (FAILED(source->GetDesc(&desc)) || FAILED(source->LockRect(&from, nullptr, D3DLOCK_READONLY)))
        return false;

    if (FAILED(target->LockRect(&to, nullptr, 0)))
    {
        source->UnlockRect();
        return false;
    }

    bool isBlockCompressed = desc.Format == D3DFMT_DXT1 || desc.Format == D3DFMT_DXT2 || desc.Format == D3DFMT_DXT3 || desc.Format == D3DFMT_DXT4 || desc.Format == D3DFMT_DXT5;
    UINT rows = isBlockCompressed ? (desc.Height + 3) / 4 : desc.Height;
    size_t rowSize = static_cast<size_t>(std::min(from.Pitch, to.Pitch));

    for (UINT y = 0; y < rows; y++)
    {
        memcpy(static_cast<BYTE*>(to.pBits) + y * to.Pitch, static_cast<BYTE*>(from.pBits) + y * from.Pitch, rowSize);
    }

    target->UnlockRect();
    source->UnlockRect();
    return true;
}

// Copy of a system memory texture in the default or in the managed pool
static IDirect3DBaseTexture9* CopyStagingTexture(IDirect3DDevice9* device, IDirect3DBaseTexture9* staging, D3DPOOL pool)
{
    D3DRESOURCETYPE type = staging->GetType();
    if (type != D3DRTYPE_TEXTURE && type != D3DRTYPE_CUBETEXTURE) return nullptr;

    bool isCube = type == D3DRTYPE_CUBETEXTURE;
    DWORD levels = staging->GetLevelCount();
    D3DSURFACE_DESC desc;
    IDirect3DBaseTexture9* texture = nullptr;
    HRESULT hr;

    if (isCube)
    {
        IDirect3DCubeTexture9* source = static_cast<IDirect3DCubeTexture9*>(staging);
        if (FAILED(source->GetLevelDesc(0, &desc)) || desc.Pool != D3DPOOL_SYSTEMMEM) return nullptr;

        hr = device->CreateCubeTexture(desc.Width, levels, 0, desc.Format, pool, reinterpret_cast<IDirect3DCubeTexture9**>(&texture), nullptr);
    }
    else
    {
        IDirect3DTexture9* source = static_cast<IDirect3DTexture9*>(staging);
        if (FAILED(source->GetLevelDesc(0, &desc)) || desc.Pool != D3DPOOL_SYSTEMMEM) return nullptr;

        hr = device->CreateTexture(desc.Width, desc.Height, levels, 0, desc.Format, pool, reinterpret_cast<IDirect3DTexture9**>(&texture), nullptr);
    }

    if (FAILED(hr) || !texture) return nullptr;

    bool isCopied = true;

    if (pool == D3DPOOL_DEFAULT)
    {
        if (isCube)
        {
            for (int face = 0; face < 6; face++)
            {
                static_cast<IDirect3DCubeTexture9*>(staging)->AddDirtyRect(static_cast<D3DCUBEMAP_FACES>(face), nullptr);
            }
        }
        else
        {
            static_cast<IDirect3DTexture9*>(staging)->AddDirtyRect(nullptr);
        }

        isCopied = SUCCEEDED(device->UpdateTexture(staging, texture));
    }
    else
    {
        for (UINT face = 0; face < (isCube ? 6u : 1u) && isCopied; face++)
        {
            for (UINT level = 0; level < levels && isCopied; level++)
            {
                IDirect3DSurface9* from = nullptr;
                IDirect3DSurface9* to = nullptr;

                isCopied = SUCCEEDED(GetTextureSurface(staging, isCube, face, level, &from)) && SUCCEEDED(GetTextureSurface(texture, isCube, face, level, &to)) && CopySurface(from, to);

                if (from) from->Release();
                if (to) to->Release();
            }
        }
    }

    if (!isCopied)
    {
        texture->Release();
        return nullptr;
    }

    return texture;
}

static void BeginFileTexture()
{
    isCreatingFileTexture = isTextureDeviceHooked;
}

// Upload the system memory texture to a default pool texture that only exists in video memory
static void EndFileTexture(int* wrapper)
{
    if (!isCreatingFileTexture) return;

    isCreatingFileTexture = false;

    if (!wrapper || !wrapper[0]) return;

    IDirect3DBaseTexture9* staging = reinterpret_cast<IDirect3DBaseTexture9*>(wrapper[0]);
    IDirect3DDevice9* device = nullptr;

    if (FAILED(staging->GetDevice(&device))) return;

    bool isVideoMemoryOnly = true;
    IDirect3DBaseTexture9* texture = CopyStagingTexture(device, staging, D3DPOOL_DEFAULT);

    // Out of video memory, a system memory texture can't be rendered so create the managed texture the engine asked for
    if (!texture)
    {
        isVideoMemoryOnly = false;
        texture = CopyStagingTexture(device, staging, D3DPOOL_MANAGED);
    }

    device->Release();

    if (!texture) return;

    wrapper[0] = reinterpret_cast<int>(texture);
    staging->Release();

    if (isVideoMemoryOnly)
    {
        auto it = std::lower_bound(videoMemoryTextures.begin(), videoMemoryTextures.end(), wrapper);
        if (it == videoMemoryTextures.end() || *it != wrapper)
        {
            videoMemoryTextures.insert(it, wrapper);
        }
    }
}

static void ForgetVideoMemoryTexture(int* wrapper)
{
    auto it = std::lower_bound(videoMemoryTextures.begin(), videoMemoryTextures.end(), wrapper);
    if (it != videoMemoryTextures.end() && *it == wrapper)
    {
        videoMemoryTextures.erase(it);
    }
}

static void ReleaseVideoMemoryTextures(IDirect3DDevice9* device)
{
    if (videoMemoryTextures.empty()) return;

    // The device holds a reference to the textures that are bound
    for (DWORD stage = 0; stage < 16; stage++)
    {
        device->SetTexture(stage, nullptr);
    }

    for (int* wrapper : videoMemoryTextures)
    {
        IUnknown* texture = reinterpret_cast<IUnknown*>(wrapper[0]);
        if (!texture) continue;

        UnmarkSharpTexture(texture);
        texture->Release();

        // The wrapper is destroyed when its texture is loaded again, 'DestroyTextureWrapper' only releases a texture that is set
        wrapper[0] = 0;
        hasReleasedTextures = true;
    }
}

static void ReloadReleasedTextures()
{
    if (!hasReleasedTextures || isReloadingTextures) return;

    hasReleasedTextures = false;

    // Console command of the engine, every texture is destroyed and created again from its file
    isReloadingTextures = true;
    RebindTextures();
    isReloadingTextures = false;
}

// Default pool textures don't survive a reset, they are released before it and loaded again after it
static HRESULT WINAPI Reset_Hook(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    ReleaseVideoMemoryTextures(device);

    HRESULT hr = D3D9_Reset(device, pPresentationParameters);

    if (SUCCEEDED(hr))
    {
        // The sampler states are back to their default values
        std::fill(std::begin(g_State.stageIsDirty), std::end(g_State.stageIsDirty), false);

        ReloadReleasedTextures();
    }

    return hr;
}

static void HookTextureDeviceMethods(void** vtable)
{
    isTextureDeviceHooked = HookHelper::ApplyHookReplaceable(vtable[16], &Reset_Hook, (LPVOID*)&D3D9_Reset)
        && HookHelper::ApplyHookReplaceable(vtable[23], &CreateTexture_Hook, (LPVOID*)&D3D9_CreateTexture)
        && HookHelper::ApplyHookReplaceable(vtable[25], &CreateCubeTexture_Hook, (LPVOID*)&D3D9_CreateCubeTexture);
}

static void ApplyReduceTextureMemory()
{
    if (!ReduceTextureMemory) return;

    RebindTextures = reinterpret_cast<decltype(RebindTextures)>(GetAddress(Addr::RebindTextures));
}
