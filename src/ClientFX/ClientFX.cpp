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

    if (!result || *pIntensity <= 0.01f)
        return result;

    int props = *(int*)(thisPtr + 20);
    bool useRadius = *(bool*)(props + 52);

    if (!useRadius && *pIntensity < 0.1f)
        return result;

    float intensity = *pIntensity;
    if (intensity > 1.0f) intensity = 1.0f;

    uint16_t lowFreq, highFreq;
    uint32_t duration;

    if (fUnitLifetime < 0.05f)
    {
        // Initial impact
        uint16_t baseIntensity = static_cast<uint16_t>(35000 + intensity * 30000);
        lowFreq = baseIntensity;
        highFreq = static_cast<uint16_t>(baseIntensity * 0.6f);
        duration = 250;
    }
    else
    {
        // Sustained rumble, follows intensity curve
        uint16_t baseIntensity = static_cast<uint16_t>(10000 + intensity * 20000);
        lowFreq = baseIntensity;
        highFreq = static_cast<uint16_t>(baseIntensity * 0.5f);
        duration = 150;  // Overlaps with 50ms refresh
    }

    uint32_t currentTime = GetTickCount64();
    uint16_t maxIntensity = (lowFreq > highFreq) ? lowFreq : highFreq;

    // Apply if: timer expired or stronger shake overrides weaker one
    if (currentTime > g_State.lastShakeRumbleTime + 50 || maxIntensity > g_State.lastShakeRumbleIntensity + 2000)
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
    if (!SDLGamepadSupport || !RumbleEnabled) return;

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