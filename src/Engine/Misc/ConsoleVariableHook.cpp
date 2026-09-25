#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"
#include "../../Controller/Controller.hpp"

int(__stdcall* SetConsoleVariableFloat)(const char*, float) = nullptr;

float GetCameraOffset(float fov)
{
    constexpr float data[][2] = 
    {
        {70.0f,  0.0f},   {75.0f,  2.5f},   {78.0f,  4.5f},
        {80.0f,  5.75f},  {85.0f,  8.5f},   {93.0f,  12.0f},
        {95.0f,  13.0f},  {105.0f, 17.0f},  {115.0f, 19.5f},
        {128.0f, 23.0f}
    };
    constexpr size_t N = sizeof(data) / sizeof(data[0]);

    size_t i = 0;
    if (fov >= data[N - 1][0])
    {
        i = N - 2;
    }
    else if (fov > data[0][0])
    {
        while (fov > data[i + 1][0]) i++;
    }

    float t = (fov - data[i][0]) / (data[i + 1][0] - data[i][0]);
    return data[i][1] + t * (data[i + 1][1] - data[i][1]);
}

static int __stdcall SetConsoleVariableFloat_Hook(const char* pszVarName, float fValue)
{
    switch (pszVarName[0])
    {
        case 'A':  // AntiAliasFSOverSample
        {
            if (SSAAScale == 1.0f) break;

            const uint32_t varHash = HashHelper::FNV1aRuntime(pszVarName);
            if (varHash == HashHelper::CVarHashes::AntiAliasFSOverSample)
            {
                fValue = 0.0f;
            }
        }

        case 'G':  // GPad*
        {
            if (!SDLGamepadSupport) break;

            const uint32_t varHash = HashHelper::FNV1aRuntime(pszVarName);
            switch (varHash)
            {
                case HashHelper::CVarHashes::GPadAimSensitivity:
                    fValue = GPadAimSensitivity; break;
                case HashHelper::CVarHashes::GPadAimEdgeThreshold:
                    fValue = GPadAimEdgeThreshold; break;
                case HashHelper::CVarHashes::GPadAimEdgeAccelTime:
                    fValue = GPadAimEdgeAccelTime; break;
                case HashHelper::CVarHashes::GPadAimEdgeDelayTime:
                    fValue = GPadAimEdgeDelayTime; break;
                case HashHelper::CVarHashes::GPadAimEdgeMultiplier:
                    fValue = GPadAimEdgeMultiplier; break;
                case HashHelper::CVarHashes::GPadAimAspectRatio:
                    fValue = GPadAimAspectRatio; break;
            }
            break;
        }

        case 'M':  // ModelLODDistanceScale, MouseInvertY
        {
            if (!NoLODBias && !(SDLGamepadSupport && GyroEnabled)) break;

            const uint32_t varHash = HashHelper::FNV1aRuntime(pszVarName);
            if (NoLODBias && varHash == HashHelper::CVarHashes::ModelLODDistanceScale)
            {
                if (fValue > 0.0f)
                    fValue = 0.003f;
            }
            else if (SDLGamepadSupport && GyroEnabled && varHash == HashHelper::CVarHashes::MouseInvertY)
            {
                SetGyroInvertY(fValue == 1.0f);
            }
            break;
        }

        case 'C':  // CameraFirstPersonLODBias, CrosshairSize, CheckPointOptimizeVideoMemory, CameraAttachedOffsetZ
        {
            if (!NoLODBias && !HUDScaling && !OptimizeSaveSpeed && CustomFOV == 0.0f) break;

            const uint32_t varHash = HashHelper::FNV1aRuntime(pszVarName);
            if (NoLODBias && varHash == HashHelper::CVarHashes::CameraFirstPersonLODBias)
            {
                fValue = 0.0f;
            }
            else if (HUDScaling && varHash == HashHelper::CVarHashes::CrosshairSize)
            {
                g_State.crosshairSize = fValue;
                fValue *= g_State.scalingFactorCrosshair;
            }
            else if (OptimizeSaveSpeed && varHash == HashHelper::CVarHashes::CheckPointOptimizeVideoMemory)
            {
                fValue = 0.0f;
            }
            else if (CustomFOV != 0.0f && varHash == HashHelper::CVarHashes::CameraAttachedOffsetZ)
            {
                fValue = GetCameraOffset(CustomFOV);
            }
            break;
        }

        case 'F':  // FovY*
        {
            if (CustomFOV == 0.0f) break;

            const uint32_t varHash = HashHelper::FNV1aRuntime(pszVarName);
            if (varHash == HashHelper::CVarHashes::FovY || varHash == HashHelper::CVarHashes::FovYWidescreen)
            {
                fValue = CustomFOV;
            }
            break;
        }

        case 'S':  // StreamResources
        {
            if (!ForceWindowed) break;

            const uint32_t varHash = HashHelper::FNV1aRuntime(pszVarName);
            if (varHash == HashHelper::CVarHashes::StreamResources)
            {
                SetConsoleVariableFloat("Windowed", 1.0f);
                ForceWindowed = false;
            }
            break;
        }

        case 'P':  // Performance_Screen*, PerturbScale
        {
            if (!(g_State.isInAutoDetect && AutoResolution != 0) && !HUDScaling) break;

            const uint32_t varHash = HashHelper::FNV1aRuntime(pszVarName);
            if (g_State.isInAutoDetect && AutoResolution != 0)
            {
                if (varHash == HashHelper::CVarHashes::Performance_ScreenHeight)
                {
                    fValue = static_cast<float>(g_State.screenHeight);
                }
                else if (varHash == HashHelper::CVarHashes::Performance_ScreenWidth)
                {
                    fValue = static_cast<float>(g_State.screenWidth);
                }
            }
            if (HUDScaling && varHash == HashHelper::CVarHashes::PerturbScale)
            {
                fValue *= g_State.scalingFactorCrosshair;
            }
            break;
        }

        case 'U':  // UseTextScaling
        {
            if (!HUDScaling) break;

            const uint32_t varHash = HashHelper::FNV1aRuntime(pszVarName);
            if (varHash == HashHelper::CVarHashes::UseTextScaling)
            {
                fValue = 0.0f;
            }
            break;
        }
    }

    return SetConsoleVariableFloat(pszVarName, fValue);
}

static void ApplyConsoleVariableHook()
{
    HookHelper::ApplyHook((void*)GetAddress(Addr::SetConsoleVariableFloat), &SetConsoleVariableFloat_Hook, (LPVOID*)&SetConsoleVariableFloat, g_State.IsOriginalGame());
}
