#pragma once

#include "../../Globals.cpp"

static void ApplyPersistentWorldServerPatch()
{
    if (!EnablePersistentWorldState) return;

    DWORD addr_BodyFading = ScanModuleSignature(g_State.GameServer, "8A 86 ?? ?? 00 00 84 C0 74 A1 8D 8E", "BodyFading");

    if (addr_BodyFading != 0)
    {
        MemoryHelper::WriteMemory<uint8_t>(addr_BodyFading + 0x22, 0xEB);
    }
}
