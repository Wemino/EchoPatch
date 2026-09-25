#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"
#include "../../Client/Client.hpp"
#include "../../Server/Server.hpp"

intptr_t(__cdecl* LoadGameDLL)(char*, char, DWORD*) = nullptr;

// When the game is loading the Client or Server
static intptr_t __cdecl LoadGameDLL_Hook(char* FileName, char a2, DWORD* a3)
{
    intptr_t result = LoadGameDLL(FileName, a2, a3);

    // Convert FileName to wide string
    wchar_t wFileName[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, FileName, -1, wFileName, MAX_PATH);

    // Use FileName to get the module handle
    HMODULE ApiDLL = GetModuleHandleW(wFileName);
    if (ApiDLL)
    {
        if (!g_State.isClientLoaded) // First time is client
        {
            g_State.GameClient = ApiDLL;
            g_State.isClientLoaded = true;
            ApplyClientPatch();
        }
        else // Otherwise server
        {
            g_State.GameServer = ApiDLL;
            ApplyServerPatch();
        }
    }

    return result;
}

static void ApplyClientHook()
{
    HookHelper::ApplyHook((void*)GetAddress(Addr::LoadGameDLL), &LoadGameDLL_Hook, (LPVOID*)&LoadGameDLL, g_State.CurrentFEARGame == FEAR);
}
