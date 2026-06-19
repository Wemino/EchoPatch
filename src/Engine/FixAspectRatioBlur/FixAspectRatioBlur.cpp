#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"
#include "../../Shaders/ShadowBlur_BlurBuffer_fxo.hpp"
#include "../../Shaders/blur_fxo.hpp"

bool(__thiscall* GetShaderFile)(int, char*, DWORD*, size_t*) = nullptr;

// =======================
// FixAspectRatioBlur
// =======================

static bool __fastcall GetShaderFile_Hook(int thisptr, int, char* String1, DWORD* a3, size_t* a4)
{
    if (String1)
    {
        if (StrStrIA(String1, "ShadowBlur_BlurBuffer.fxo"))
        {
            *a3 = (DWORD)(uintptr_t)ShadowBlur_fxo;
            *a4 = ShadowBlur_fxo_len;
            return true;
        }
        else if (StrStrIA(String1, "rigid\\Translucent\\Effect\\blur.fxo"))
        {
            *a3 = (DWORD)(uintptr_t)Blur_fxo;
            *a4 = Blur_fxo_len;
            return true;
        }
    }

    return GetShaderFile(thisptr, String1, a3, a4);
}

static void ApplyFixAspectRatioBlur()
{
    if (!FixAspectRatioBlur) return;

    HookHelper::ApplyHook((void*)GetAddress(Addr::GetShaderFile), &GetShaderFile_Hook, (LPVOID*)&GetShaderFile, g_State.CurrentFEARGame == FEAR);
}
