#pragma once

#include "../Globals.cpp"

void ApplyServerPatch()
{
    HookHelper::BeginBatch();
    ApplyLocalPlayerServerPatch();
    ApplyPersistentWorldServerPatch();
    ApplySavePersistentWorldStateServerPatch();
    ApplySetWeaponCapacityServerPatch();
    ApplyDoorBashingServerPatch();
    ApplyHighFPSFixesServerPatch();
    ApplyFixWaterReflectionsServerPatch();
    ApplyControllerServerPatch();
    ApplyConsoleDebugLinesServerPatch();
    HookHelper::EndBatch();
}
