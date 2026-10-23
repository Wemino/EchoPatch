#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"
#include "../../Controller/ScreenJoystickHook.hpp"
#include "../../ClientFX/ClientFX.hpp"
#include "../../Server/Server.hpp"

// ========================
// FixKeyboardInputLanguage
// ========================

// Command ID to DirectInput key, from ProfileDatabase/Defaults.Gamdb00p (same keys in FEAR, XP and XP2)
static const std::unordered_map<unsigned int, uint8_t> defaultKeyMap =
{
	{0, 0x11},   // Forward: W
	{1, 0x1F},   // Backward: S
	{3, 0x1E},   // Strafe Left: A
	{4, 0x20},   // Strafe Right: D
	{10, 0xCB},  // Turn Left: Left
	{9, 0xCD},   // Turn Right: Right
	{6, 0x9D},   // Strafe: Right Ctrl
	{15, 0x39},  // Jump: Space
	{14, 0x2E},  // Crouch: C
	{20, 0x10},  // Lean Left: Q
	{21, 0x12},  // Lean Right: E
	{81, 0x22},  // Throw Grenade: G
	{87, 0x21},  // Activate: F
	{88, 0x13},  // Reload: R
	{71, 0x2A},  // Zoom: Shift
	{106, 0x1D}, // Slow-Mo: Ctrl
	{114, 0x2D}, // Flashlight: X
	{79, 0x0F},  // Status: Tab
	{78, 0x32},  // Mission: M (not in XP2)
	{84, 0x14},  // Say: T
	{104, 0x15}, // Team Say: Y
	{115, 0x2F}, // Radio: V
	{8, 0xC8},   // Look Up: Up
	{7, 0xD0},   // Look Down: Down
	{82, 0xCF},  // Center View: End
	{30, 0x02},  // Weapon 1: 1
	{31, 0x03},  // Weapon 2: 2
	{32, 0x04},  // Weapon 3: 3
	{102, 0x30}, // Drop Weapon: B
	{40, 0x05},  // Grenade 1: 4
	{41, 0x06},  // Grenade 2: 5
	{42, 0x07},  // Grenade 3: 6
	{43, 0x08},  // Grenade 4: 7 (XP and XP2)
	{70, 0x2C},  // Medkit: Z
	{50, 0x23}   // Holster: H
};

static unsigned int lastDefaultKeyCommand = UINT_MAX;

static void __fastcall LoadUserProfile_Hook(int thisPtr, int, bool bLoadDefaults, bool bLoadDisplaySettings)
{
	g_State.isLoadingDefault = bLoadDefaults;
	lastDefaultKeyCommand = UINT_MAX;
	LoadUserProfile(thisPtr, bLoadDefaults, bLoadDisplaySettings);
	g_State.isLoadingDefault = false;
	g_State.defaultKeyInputMgr = 0;
	g_State.isDefaultKeyUnresolved = false;
}

static bool __fastcall RestoreDefaults_Hook(int thisPtr, int, uint8_t nFlags)
{
	g_State.isLoadingDefault = true;
	lastDefaultKeyCommand = UINT_MAX;
	bool res = RestoreDefaults(thisPtr, nFlags);
	g_State.isLoadingDefault = false;
	g_State.defaultKeyInputMgr = 0;
	g_State.isDefaultKeyUnresolved = false;
	return res;
}

// pBinding is a CBindMgr::SBinding: device, object, command, then 8 floats
static void __fastcall SetBinding_Hook(int thisPtr, int, const DWORD* pBinding, bool bOverwrite)
{
	int inputMgr = g_State.defaultKeyInputMgr;
	if (inputMgr == 0)
	{
		SetBinding(thisPtr, pBinding, bOverwrite);
		return;
	}

	// Keyboard binding from the default profile, see GetDeviceObjectDesc_Hook
	bool isUnresolved = g_State.isDefaultKeyUnresolved;
	g_State.defaultKeyInputMgr = 0;
	g_State.isDefaultKeyUnresolved = false;

	// Only the command's first keyboard binding takes the default key
	unsigned int command = pBinding[2];
	bool isFirstBinding = command != lastDefaultKeyCommand;
	lastDefaultKeyCommand = command;

	auto it = defaultKeyMap.find(command);
	uintptr_t keyboard = GetKeyboardDevice(inputMgr, pBinding[0]);
	unsigned int object = 0;

	if (isFirstBinding && it != defaultKeyMap.end() && keyboard != 0 && FindDIKInTable(keyboard, it->second, &object))
	{
		DWORD binding[11];
		memcpy(binding, pBinding, sizeof(binding));
		binding[1] = object;
		SetBinding(thisPtr, binding, bOverwrite);
		return;
	}

	// The key name wasn't found, the game skips this binding
	if (isUnresolved)
		return;

	SetBinding(thisPtr, pBinding, bOverwrite);
}
