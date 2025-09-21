#include "../Core/Core.hpp"
#include "MinHook.hpp"
#include "../helper.hpp"

#pragma region ClientFX Patches

void ApplyClientFXPatch()
{
    if (!HighFPSFixes) return;

    DWORD targetMemoryLocation_ParticleUpdateThreshold = ScanModuleSignature(g_State.GameClientFX, "D9 44 24 04 ?? D8 1D", "FixParticleUpdateThreshold");
    DWORD targetMemoryLocation_AddParticleBatchMarker = ScanModuleSignature(g_State.GameClientFX, "C7 40 18 00 00 80 BF C2 08 00", "FixParticleLifetime_AddParticleBatchMarker");
    DWORD targetMemoryLocation_EmitParticleBatch = ScanModuleSignature(g_State.GameClientFX, "C7 40 18 00 00 80 BF ?? ?? ?? 81 C4 C0", "FixParticleLifetime_EmitParticleBatch");

    if (targetMemoryLocation_ParticleUpdateThreshold == 0 ||
        targetMemoryLocation_AddParticleBatchMarker == 0 ||
        targetMemoryLocation_EmitParticleBatch == 0) {
        return;
    }

    // Disable frametime check for FX updates
    MemoryHelper::MakeNOP(targetMemoryLocation_ParticleUpdateThreshold, 4);
    MemoryHelper::MakeNOP(targetMemoryLocation_ParticleUpdateThreshold + 0x5, 6);
    MemoryHelper::MakeNOP(targetMemoryLocation_ParticleUpdateThreshold + 0xD, 7);

    // m_fLifetime: -1 -> 0 (prevent issue inside GetParticleSizeAndColor)
    MemoryHelper::WriteMemory<float>(targetMemoryLocation_AddParticleBatchMarker + 0x3, 0.0f);
    MemoryHelper::WriteMemory<float>(targetMemoryLocation_EmitParticleBatch + 0x3, 0.0f);
}

#pragma endregion