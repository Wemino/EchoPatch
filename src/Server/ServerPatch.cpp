#pragma once

#include "../Globals.cpp"

void ApplyServerPatch()
{
    ApplyPersistentWorldServerPatch();
    ApplySetWeaponCapacityServerPatch();
    ApplyHighFPSFixesServerPatch();
    ApplyControllerServerPatch();
}
