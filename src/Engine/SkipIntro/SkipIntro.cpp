#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

int(__stdcall* CreateVideoTexture)(char*, int) = nullptr;

// =======================
// SkipIntro
// =======================

static int __stdcall CreateVideoTexture_Hook(char* video_path, int a2)
{
    auto DisableHook = []()
    {
        MH_DisableHook((void*)GetAddress(Addr::CreateVideoTexture));
    };

    if (SkipAllIntro)
    {
        // Skip all movies while keeping the sound of the menu
        DisableHook();
        SystemHelper::SimulateSpacebarPress(g_State.hWnd);
    }

    static const struct { bool flag; const char* names[2]; } skips[] =
    {
        { SkipSierraIntro,   { "sierralogo.bik",       nullptr } },
        { SkipMonolithIntro, { "MonolithLogo.bik",     nullptr } },
        { SkipWBGamesIntro,  { "WBGames.bik",          nullptr } },
        { SkipNvidiaIntro,   { "TWIMTBP_640x480.bik",  "Nvidia_LogoXP2.bik" } },
        { SkipTimegateIntro, { "timegate.bik",         "TimeGate.bik" } },
        { SkipDellIntro,     { "dell_xps.bik",         "Dell_LogoXP2.bik" } },
    };

    for (const auto& s : skips)
    {
        for (const char* name : s.names)
        {
            if (name && strstr(video_path, name))
            {
                if (SkipAllIntro || s.flag)
                {
                    video_path[0] = '\0';
                }

                return CreateVideoTexture(video_path, a2);
            }
        }
    }

    if (!SkipAllIntro)
    {
        DisableHook();
    }

    return CreateVideoTexture(video_path, a2);
}

static void ApplySkipIntroHook()
{
    HookHelper::ApplyHook((void*)GetAddress(Addr::CreateVideoTexture), &CreateVideoTexture_Hook, (LPVOID*)&CreateVideoTexture, g_State.CurrentFEARGame == FEAR);
}
