#pragma once

#include "../../Globals.cpp"

void(__thiscall* PolyGridReadProp)(int, int) = nullptr;

// ===================
// FixWaterReflections
// ===================

static void __fastcall PolyGridReadProp_Hook(int thisPtr, int, int pProps)
{
    PolyGridReadProp(thisPtr, pProps);

    // Some levels turn off 'm_bReflectionMap' but keep 'm_sReflectionRTGroup' (length at 0x1A4), the water then has nothing to reflect
    if (*(DWORD*)(thisPtr + 0x1A4) != 0)
    {
        *(bool*)(thisPtr + 0x170) = true;
    }
}

static void ApplyFixWaterReflectionsServerPatch()
{
    if (!FixWaterReflections || g_State.CurrentFEARGame != FEARXP) return;

    DWORD addr_PolyGridReadProp = ScanModuleSignature(g_State.GameServer, "83 EC 58 56 57 8B 7C 24 64 85 FF 8B F1 0F 84", "PolyGridReadProp");

    if (addr_PolyGridReadProp == 0)
        return;

    HookHelper::ApplyHookReplaceable((void*)addr_PolyGridReadProp, &PolyGridReadProp_Hook, (LPVOID*)&PolyGridReadProp);
}
