#pragma once

#include "../../Globals.cpp"

struct FlickerEntry
{
    double lastTime;
    int value;
};
static std::unordered_map<int, FlickerEntry> flickerCache;
static double flickerCachePruneTime = 0.0;

int(__cdecl* DynaLightRand)() = nullptr;

// ========================
// HighFPSFixes
// ========================

static int __cdecl DynaLightFlicker(int dynaLight)
{
    // Rolls a new flicker intensity once per 60 FPS frame, each light keeps its own roll
    auto entry = flickerCache.try_emplace(dynaLight, FlickerEntry{ -1.0, 0 }).first;
    if (IsTargetFrameDue(entry->second.lastTime))
    {
        entry->second.value = DynaLightRand();
    }
    int value = entry->second.value;

    // Forget lights that stopped updating
    double currentTime = g_State.totalRealTime;
    if (currentTime - flickerCachePruneTime > 1.0)
    {
        std::erase_if(flickerCache, [currentTime](const auto& cached) { return currentTime - cached.second.lastTime > 1.0; });
        flickerCachePruneTime = currentTime;
    }

    return value;
}

// Replaces the rand call of the dynamic light flicker, the light is in esi
static __declspec(naked) void DynaLightRand_Hook()
{
    __asm
    {
        push esi
        call DynaLightFlicker
        add esp, 4
        ret
    }
}

static void ApplyHighFPSFixesClientFXPatch()
{
    if (!HighFPSFixes) return;

    DWORD addr_ParticleUpdateThreshold = ScanModuleSignature(g_State.GameClientFX, "D9 44 24 04 ?? D8 1D", "FixParticleUpdateThreshold");
    DWORD addr_AddParticleBatchMarker = ScanModuleSignature(g_State.GameClientFX, "C7 40 18 00 00 80 BF C2 08 00", "FixParticleLifetime_AddParticleBatchMarker");
    DWORD addr_EmitParticleBatch = ScanModuleSignature(g_State.GameClientFX, "C7 40 18 00 00 80 BF ?? ?? ?? 81 C4 C0", "FixParticleLifetime_EmitParticleBatch");
    DWORD addr_DynaLightRand = ScanModuleSignature(g_State.GameClientFX, "FF 15 ?? ?? ?? ?? 89 44 24 ?? DB 44 24 ?? D8 0D ?? ?? ?? ?? D9 05 ?? ?? ?? ?? D8 64 24", "DynaLightRand");

    if (addr_ParticleUpdateThreshold == 0 ||
        addr_AddParticleBatchMarker == 0 ||
        addr_EmitParticleBatch == 0 ||
        addr_DynaLightRand == 0) {
        return;
    }

    // Disable frametime check for FX updates
    MemoryHelper::MakeNOP(addr_ParticleUpdateThreshold, 4);
    MemoryHelper::MakeNOP(addr_ParticleUpdateThreshold + 0x5, 6);
    MemoryHelper::MakeNOP(addr_ParticleUpdateThreshold + 0xD, 7);

    // m_fLifetime: -1 -> 0 (prevent issue inside GetParticleSizeAndColor)
    MemoryHelper::WriteMemory<float>(addr_AddParticleBatchMarker + 0x3, 0.0f);
    MemoryHelper::WriteMemory<float>(addr_EmitParticleBatch + 0x3, 0.0f);

    // Dynamic light flicker rolls once per 60 FPS frame instead of every frame
    DynaLightRand = reinterpret_cast<decltype(DynaLightRand)>(MemoryHelper::ReadMemory<DWORD>(MemoryHelper::ReadMemory<DWORD>(addr_DynaLightRand + 0x2)));
    MemoryHelper::MakeCALL(addr_DynaLightRand, reinterpret_cast<uintptr_t>(&DynaLightRand_Hook));
    MemoryHelper::MakeNOP(addr_DynaLightRand + 0x5, 1);
}
