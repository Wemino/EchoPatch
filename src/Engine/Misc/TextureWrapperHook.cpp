#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"
#include "../Graphics/ReducedMipMapBias.cpp"
#include "../Fixes/ReduceTextureMemory.cpp"

int(__stdcall* CreateTextureWrapper)(DWORD*, int, int) = nullptr;
int(__thiscall* DestroyTextureWrapper)(int*, int*) = nullptr;

// Only called for the textures loaded from a file, a3 = path of the texture
static int __stdcall CreateTextureWrapper_Hook(DWORD* a1, int a2, int a3)
{
    if (ReduceTextureMemory)
    {
        BeginFileTexture();
    }

    int result = CreateTextureWrapper(a1, a2, a3);

    if (ReduceTextureMemory)
    {
        EndFileTexture(reinterpret_cast<int*>(result));
    }

    if (ReducedMipMapBias && result && a3)
    {
        const char* path = reinterpret_cast<const char*>(a3);

        if (isReloadingTextures ? IsSharpTexturePath(path) : isLoadingWorld && TextureHelper::ShouldKeepSharp(path))
        {
            MarkSharpTexture(*reinterpret_cast<void**>(result), path);
        }
    }

    return result;
}

static int __fastcall DestroyTextureWrapper_Hook(int* thisPtr, int, int* a2)
{
    if (a2)
    {
        UnmarkSharpTexture(reinterpret_cast<void*>(*a2));
        ForgetVideoMemoryTexture(a2);
    }

    return DestroyTextureWrapper(thisPtr, a2);
}

static void ApplyTextureWrapperHook()
{
    if (!ReducedMipMapBias && !ReduceTextureMemory) return;

    HookHelper::ApplyHook((void*)GetAddress(Addr::DestroyTextureWrapper), &DestroyTextureWrapper_Hook, (LPVOID*)&DestroyTextureWrapper);
    HookHelper::ApplyHook((void*)GetAddress(Addr::CreateTextureWrapper), &CreateTextureWrapper_Hook, (LPVOID*)&CreateTextureWrapper);
}
