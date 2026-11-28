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
    ApplyUncapDismembermentServerPatch();
    ApplyHighFPSFixesServerPatch();
    ApplyFixWaterReflectionsServerPatch();
    ApplyFixDoorCollisionServerPatch();
    ApplyControllerServerPatch();
    ApplyConsoleDebugLinesServerPatch();
    HookHelper::EndBatch();
}
