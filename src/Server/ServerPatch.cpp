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
    ApplyDoorBashingExtrasServerPatch();
    ApplyDoorBashSparksServerPatch();
    ApplyUncapDismembermentServerPatch();
    ApplyHighFPSFixesServerPatch();
    ApplyFixWaterReflectionsServerPatch();
    ApplyDoorFixesServerPatch();
    ApplyControllerServerPatch();
    ApplyConsoleDebugLinesServerPatch();
    HookHelper::EndBatch();
}
