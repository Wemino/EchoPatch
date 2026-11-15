#pragma once

#include "../Globals.cpp"

void ApplyServerPatch()
{
    HookHelper::BeginBatch();
    ApplyLocalPlayerServerPatch();
    ApplyPersistentWorldServerPatch();
    ApplySavePersistentWorldStateServerPatch();
    ApplySetWeaponCapacityServerPatch();
    ApplyHighFPSFixesServerPatch();
    ApplyControllerServerPatch();
    ApplyConsoleDebugLinesServerPatch();
    HookHelper::EndBatch();
}
