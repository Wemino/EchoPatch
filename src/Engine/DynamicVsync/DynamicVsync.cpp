#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

int(__thiscall* InitializePresentationParameters)(DWORD*, DWORD*, unsigned __int8) = nullptr;

// ========================
// DynamicVsync
// ========================

static int __fastcall InitializePresentationParameters_Hook(DWORD* thisPtr, int, DWORD* a2, unsigned __int8 a3)
{
    int res = InitializePresentationParameters(thisPtr, a2, a3);
    thisPtr[101] = g_State.useVsyncOverride != 0 ? 0 : 0x80000000;
    return res;
}

static void HookVSyncOverride()
{
    if (!DynamicVsync) return;

    HookHelper::ApplyHook((void*)GetAddress(Addr::InitializePresentationParameters), &InitializePresentationParameters_Hook, (LPVOID*)&InitializePresentationParameters, g_State.CurrentFEARGame == FEAR);
}
