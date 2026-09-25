#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

static void ApplyFixDirectInputFps()
{
    // Root cause documented by Methanhydrat: https://community.pcgamingwiki.com/files/file/789-directinput-fps-fix/
    // Fix SetWindowsHookExA input lag from: https://github.com/Vityacv/fearservmod
    if (!DisableRedundantHIDInit) return;

    MemoryHelper::MakeNOP(GetAddress(Addr::DirectInputFps_HID1), 22);
    MemoryHelper::MakeNOP(GetAddress(Addr::DirectInputFps_HID2), 29);
}
