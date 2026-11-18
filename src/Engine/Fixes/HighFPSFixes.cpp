#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

struct AnimTrackerEntry
{
    double lastTime;
    float remainder;
};
static std::unordered_map<DWORD, AnimTrackerEntry> animTrackerCache;
static double animTrackerCachePruneTime = 0.0;

void(__thiscall* AnimTrackerUpdate)(DWORD, unsigned int, DWORD, DWORD, unsigned int*) = nullptr;
void(__thiscall* AnimTrackerAdvance)(DWORD, unsigned int, DWORD, DWORD, unsigned int*) = nullptr;

// ========================
// HighFPSFixes
// ========================

static void __fastcall AnimTrackerUpdate_Hook(DWORD thisPtr, int, unsigned int elapsedMs, DWORD keyCallback, DWORD keyUserData, unsigned int* pLoopCount)
{
    bool isPlaying = (*reinterpret_cast<BYTE*>(thisPtr + 0xC) & 1) != 0;
    float rate = *reinterpret_cast<float*>(thisPtr + 0x10);
    bool isScaled = rate > 0.0f && rate < 1000.0f && rate != 1.0f;

    if (!isPlaying || !isScaled || elapsedMs == 0)
    {
        AnimTrackerUpdate(thisPtr, elapsedMs, keyCallback, keyUserData, pLoopCount);
        return;
    }

    double currentTime = g_State.totalRealTime;
    auto entry = animTrackerCache.try_emplace(thisPtr, AnimTrackerEntry{ currentTime, 0.0f }).first;
    double scaledMs = static_cast<double>(elapsedMs) * rate + entry->second.remainder;
    unsigned int advanceMs = static_cast<unsigned int>(scaledMs);
    entry->second.remainder = static_cast<float>(scaledMs - advanceMs);
    entry->second.lastTime = currentTime;

    // Forget animations that stopped updating
    if (currentTime - animTrackerCachePruneTime > 1.0)
    {
        std::erase_if(animTrackerCache, [currentTime](const auto& cached) { return currentTime - cached.second.lastTime > 1.0; });
        animTrackerCachePruneTime = currentTime;
    }

    AnimTrackerAdvance(thisPtr, advanceMs, keyCallback, keyUserData, pLoopCount);
}

static void ApplyHighFPSFixes()
{
    if (!HighFPSFixes) return;

    DWORD addr_AnimTrackerUpdate = GetAddress(Addr::AnimTrackerUpdate);

    AnimTrackerAdvance = reinterpret_cast<decltype(AnimTrackerAdvance)>(MemoryHelper::ResolveRelativeAddress(addr_AnimTrackerUpdate, 0x36));
    HookHelper::ApplyHook((void*)addr_AnimTrackerUpdate, &AnimTrackerUpdate_Hook, (LPVOID*)&AnimTrackerUpdate, g_State.CurrentFEARGame == FEAR);
}
