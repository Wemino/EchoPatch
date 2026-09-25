#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

static void ApplyDisableJoystick()
{
    if (!SDLGamepadSupport && !DisableRedundantHIDInit) return;

    MemoryHelper::MakeNOP(GetAddress(Addr::DisableJoystickInit), 25);
}
