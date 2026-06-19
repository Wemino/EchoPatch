#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

char(__thiscall* GetSocketTransform)(DWORD*, unsigned int, DWORD*, char) = nullptr;

// ===========================
// FixScriptedAnimationCrash
// ===========================

static char __fastcall GetSocketTransform_Hook(DWORD* thisp, int, unsigned int nodeIndex, DWORD* outTransform, char a4)
{
    int modelData = *(thisp + 68);

    // Attempts to retrieve bone transforms for a character whose model data has been freed
    if (modelData == 0)
    {
        // Return identity transform and return success
        outTransform[0] = 0;
        outTransform[1] = 0;
        outTransform[2] = 0;
        outTransform[3] = 0;
        outTransform[4] = 0;
        outTransform[5] = 0;
        outTransform[6] = 0x3F800000;
        outTransform[7] = 0x3F800000;

        return 1;
    }

    return GetSocketTransform(thisp, nodeIndex, outTransform, a4);
}

static void ApplyFixScriptedAnimationCrash()
{
    if (!FixScriptedAnimationCrash) return;

    // Only happen on the first game
    if (g_State.CurrentFEARGame == FEAR)
    {
        HookHelper::ApplyHook((void*)GetAddress(Addr::GetSocketTransform), &GetSocketTransform_Hook, (LPVOID*)&GetSocketTransform, true);
    }
}
