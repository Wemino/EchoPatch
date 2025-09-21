#pragma once

void ApplyServerPatch();

// Expose function pointer used by client OnEnterWorld_Hook
extern void(__thiscall* SetWeaponCapacityServer)(int, uint8_t);
