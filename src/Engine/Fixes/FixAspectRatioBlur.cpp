#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"
#include "../../Shaders/ShadowBlur_BlurBuffer_fxo.hpp"
#include "../../Shaders/blur_fxo.hpp"
#include "../../Shaders/mirror_fxo.hpp"
#include "../../Shaders/screeneffect_fxo.hpp"
#include "../../Shaders/screeneffect_bleach_fxo.hpp"
#include "../../Shaders/refract_fxo.hpp"
#include "../../Shaders/refract_additive_fxo.hpp"
#include "../../Shaders/refract_blur_fxo.hpp"
#include "../../Shaders/refract_dx9_fxo.hpp"
#include "../../Shaders/refract_thick_fxo.hpp"
#include "../../Shaders/feedback_fxo.hpp"
#include "../../Shaders/feedbackblur_fxo.hpp"
#include "../../Shaders/depthoffield_fxo.hpp"
#include "../../Shaders/neon_outside_fxo.hpp"

bool(__thiscall* GetShaderFile)(int, char*, DWORD*, size_t*) = nullptr;

// =======================
// FixAspectRatioBlur
// =======================

struct ShaderOverride
{
    const char* path;
    const uint8_t* data;
    size_t size;
    // Extraction Point and Perseus Mandate ship their own build of some effects
    const uint8_t* expansionData = nullptr;
    size_t expansionSize = 0;
};

static const ShaderOverride ShaderOverrides[] =
{
    { "ShadowBlur_BlurBuffer.fxo", ShadowBlur_fxo, ShadowBlur_fxo_len },
    { "rigid\\Translucent\\Effect\\blur.fxo", Blur_fxo, Blur_fxo_len },
    { "skeletal\\Translucent\\Effect\\blur.fxo", BlurSkeletal_fxo, BlurSkeletal_fxo_len },
    { "rigid\\Solid\\Effect\\mirror.fxo", Mirror_fxo, Mirror_fxo_len },
    { "rigid\\Translucent\\Effect\\screeneffect.fxo", ScreenEffect_fxo, ScreenEffect_fxo_len },
    { "rigid\\Translucent\\Effect\\screeneffect_bleach.fxo", ScreenEffectBleach_fxo, ScreenEffectBleach_fxo_len },
    { "rigid\\Translucent\\Effect\\refract.fxo", Refract_fxo, Refract_fxo_len },
    { "skeletal\\Translucent\\Effect\\refract.fxo", RefractSkeletal_fxo, RefractSkeletal_fxo_len },
    { "rigid\\Translucent\\Effect\\refract_additive.fxo", RefractAdditive_fxo, RefractAdditive_fxo_len, RefractAdditiveExpansion_fxo, RefractAdditiveExpansion_fxo_len },
    { "skeletal\\Translucent\\Effect\\refract_additive.fxo", RefractAdditiveSkeletal_fxo, RefractAdditiveSkeletal_fxo_len },
    { "rigid\\Translucent\\Effect\\refract_blur.fxo", RefractBlur_fxo, RefractBlur_fxo_len },
    { "skeletal\\Translucent\\Effect\\refract_blur.fxo", RefractBlurSkeletal_fxo, RefractBlurSkeletal_fxo_len },
    { "rigid\\Translucent\\Effect\\refract_dx9.fxo", RefractDX9_fxo, RefractDX9_fxo_len, RefractDX9Expansion_fxo, RefractDX9Expansion_fxo_len },
    { "rigid\\Translucent\\Effect\\refract_thick.fxo", RefractThick_fxo, RefractThick_fxo_len },
    { "skeletal\\Translucent\\Effect\\refract_thick.fxo", RefractThickSkeletal_fxo, RefractThickSkeletal_fxo_len },
    { "rigid\\Translucent\\Effect\\feedback.fxo", Feedback_fxo, Feedback_fxo_len },
    { "skeletal\\Translucent\\Effect\\feedback.fxo", FeedbackSkeletal_fxo, FeedbackSkeletal_fxo_len },
    { "rigid\\Translucent\\Effect\\feedbackblur.fxo", FeedbackBlur_fxo, FeedbackBlur_fxo_len },
    { "skeletal\\Translucent\\Effect\\feedbackblur.fxo", FeedbackBlurSkeletal_fxo, FeedbackBlurSkeletal_fxo_len },
    { "rigid\\Solid\\Effect\\depthoffield.fxo", DepthOfField_fxo, DepthOfField_fxo_len },
    { "skeletal\\Solid\\Effect\\depthoffield.fxo", DepthOfFieldSkeletal_fxo, DepthOfFieldSkeletal_fxo_len },
    { "rigid\\Translucent\\neon_outside.fxo", NeonOutside_fxo, NeonOutside_fxo_len },
    { "skeletal\\Translucent\\neon_outside.fxo", NeonOutsideSkeletal_fxo, NeonOutsideSkeletal_fxo_len },
};

static bool __fastcall GetShaderFile_Hook(int thisptr, int, char* String1, DWORD* a3, size_t* a4)
{
    if (String1)
    {
        for (const ShaderOverride& shader : ShaderOverrides)
        {
            if (!StrStrIA(String1, shader.path)) continue;

            bool useExpansion = shader.expansionData && g_State.IsExpansion();
            *a3 = (DWORD)(uintptr_t)(useExpansion ? shader.expansionData : shader.data);
            *a4 = useExpansion ? shader.expansionSize : shader.size;
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
