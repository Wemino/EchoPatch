#pragma once

#include "../Globals.cpp"

void ApplyServerPatch()
{
    HookHelper::BeginBatch();
    ApplyPersistentWorldServerPatch();
    ApplySavePersistentWorldStateServerPatch();
    ApplySetWeaponCapacityServerPatch();
    ApplyHighFPSFixesServerPatch();
    ApplyControllerServerPatch();
    ApplyConsoleDebugLinesServerPatch();
    HookHelper::EndBatch();
}
