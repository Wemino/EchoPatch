#include "../Core/Core.hpp"
#include "MinHook.hpp"
#include "../helper.hpp"
#include "../Controller/Controller.hpp"

bool(__thiscall* CCameraShakeFX_GetShakeIntensity)(int, float, float*, float*) = nullptr;

#pragma region ClientFX Hooks

// ===============
//  Rumble
// ===============

static bool __fastcall CCameraShakeFX_GetShakeIntensity_Hook(int thisPtr, int, float fUnitLifetime, float* pCameraPos, float* pIntensity)
{
    bool result = CCameraShakeFX_GetShakeIntensity(thisPtr, fUnitLifetime, pCameraPos, pIntensity);

    if (!result)
        return result;

    float intensity = *pIntensity;
    if (intensity < 0.005f)
        return result;

    int props = *(int*)(thisPtr + 20);
    bool useRadius = *(bool*)(props + 52);
    int falloffType = *(int*)(props + 56);
    int intensityCurve = *(int*)(props + 68);

    if (!useRadius || falloffType == 1)
        return result;

    if (intensity > 1.0f) intensity = 1.0f;

    float baseDesign = *(float*)(intensityCurve + 8);

    uint16_t lowFreq = 0;
    uint16_t highFreq = 0;
    uint32_t duration = 150;

    if (baseDesign < 0.2f)
    {
        float normalized = (intensity - 0.005f) / 0.045f;
        if (normalized < 0.0f) normalized = 0.0f;
        if (normalized > 1.0f) normalized = 1.0f;

        lowFreq = static_cast<uint16_t>(5000 + normalized * 11000);
        highFreq = 0;
        duration = 120;
    }
    else
    {
        float scaled = intensity * intensity;

        if (fUnitLifetime < 0.05f)
        {
            lowFreq = static_cast<uint16_t>(scaled * 65535.0f);
            highFreq = static_cast<uint16_t>(scaled * 55000.0f);
            duration = 200;
        }
        else
        {
            lowFreq = static_cast<uint16_t>(scaled * 50000.0f);
            highFreq = static_cast<uint16_t>(scaled * 30000.0f);
            duration = 150;
        }
    }

    uint64_t currentTime = GetTickCount64();
    uint16_t maxIntensity = (lowFreq > highFreq) ? lowFreq : highFreq;

    if (currentTime > g_State.lastShakeRumbleTime + 50 || maxIntensity > g_State.lastShakeRumbleIntensity + 500)
    {
        SetGamepadRumble(lowFreq, highFreq, duration);
        g_State.lastShakeRumbleTime = currentTime;
        g_State.lastShakeRumbleIntensity = maxIntensity;
    }

    return result;
}

#pragma endregion

// Helper to track and store hooked addresses
static void ApplyTrackedHook(DWORD address, LPVOID hookFunc, LPVOID* originalPtr)
{
    HookHelper::ApplyHook((void*)address, hookFunc, originalPtr);
    g_State.hookedClientFXFunctionAddresses.push_back(address);
}

static void ApplyHighFPSFixesClientFXPatch()
{
    if (!HighFPSFixes) return;

    DWORD addr_ParticleUpdateThreshold = ScanModuleSignature(g_State.GameClientFX, "D9 44 24 04 ?? D8 1D", "FixParticleUpdateThreshold");
    DWORD addr_AddParticleBatchMarker = ScanModuleSignature(g_State.GameClientFX, "C7 40 18 00 00 80 BF C2 08 00", "FixParticleLifetime_AddParticleBatchMarker");
    DWORD addr_EmitParticleBatch = ScanModuleSignature(g_State.GameClientFX, "C7 40 18 00 00 80 BF ?? ?? ?? 81 C4 C0", "FixParticleLifetime_EmitParticleBatch");

    if (addr_ParticleUpdateThreshold == 0 ||
        addr_AddParticleBatchMarker == 0 ||
        addr_EmitParticleBatch == 0) {
        return;
    }

    // Disable frametime check for FX updates
    MemoryHelper::MakeNOP(addr_ParticleUpdateThreshold, 4);
    MemoryHelper::MakeNOP(addr_ParticleUpdateThreshold + 0x5, 6);
    MemoryHelper::MakeNOP(addr_ParticleUpdateThreshold + 0xD, 7);

    // m_fLifetime: -1 -> 0 (prevent issue inside GetParticleSizeAndColor)
    MemoryHelper::WriteMemory<float>(addr_AddParticleBatchMarker + 0x3, 0.0f);
    MemoryHelper::WriteMemory<float>(addr_EmitParticleBatch + 0x3, 0.0f);
}

static void ApplyControllerClientFXPatch()
{
    if (!RumbleEnabled) return;

    DWORD addr_GetShakeIntensity = ScanModuleSignature(g_State.GameClientFX, "51 52 8B CF C7 44 24 14 00 00 80 3F E8", "GetShakeIntensity");
    addr_GetShakeIntensity = MemoryHelper::ResolveRelativeAddress(addr_GetShakeIntensity, 0xD);

    if (addr_GetShakeIntensity)
    {
        ApplyTrackedHook(addr_GetShakeIntensity, &CCameraShakeFX_GetShakeIntensity_Hook, (LPVOID*)&CCameraShakeFX_GetShakeIntensity);
    }
}

#pragma region ClientFX Patches

void ApplyClientFXPatch()
{
    if (g_State.hookedClientFXFunctionAddresses.size() != 0)
    {
        // ClientFX has been unloaded, remove all previously installed hooks
        for (DWORD address : g_State.hookedClientFXFunctionAddresses)
        {
            MH_RemoveHook((void*)address);
        }

        g_State.hookedClientFXFunctionAddresses.clear();
    }

    ApplyHighFPSFixesClientFXPatch();
    ApplyControllerClientFXPatch();
}

#pragma endregion