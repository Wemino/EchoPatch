#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

static void ApplyDisablePunkBuster()
{
    if (!DisablePunkBuster) return;

    MemoryHelper::WriteMemory<uint8_t>(GetAddress(Addr::PunkBusterRet), 0xC3);
    MemoryHelper::WriteMemory<uint32_t>(GetAddress(Addr::PunkBusterZero), 0x0);
}
