#pragma once

void ApplyClientPatch();

// Subset of client function pointers used by controller handling
extern int (__thiscall* HUDWeaponListUpdateTriggerNames)(int);
extern int (__thiscall* HUDGrenadeListUpdateTriggerNames)(int);
extern void (__cdecl* HUDSwapUpdateTriggerName)();
extern bool (__thiscall* OnCommandOn)(int, int);
extern bool (__thiscall* OnCommandOff)(int, int);
extern void (__thiscall* SetCurrentType)(int, int);

// From core
extern int(__stdcall* SetConsoleVariableFloat)(const char*, float);
