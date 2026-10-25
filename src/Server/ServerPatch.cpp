#pragma once

#include "../Globals.cpp"

void ApplyServerPatch()
{
    ApplyPersistentWorldServerPatch();
    ApplySavePersistentWorldStateServerPatch();
    ApplySetWeaponCapacityServerPatch();
    ApplyHighFPSFixesServerPatch();
    ApplyControllerServerPatch();
    ApplyConsoleDebugLinesServerPatch();
}
