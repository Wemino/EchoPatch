#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"
#include "../../Controller/ScreenJoystickHook.hpp"
#include "../../ClientFX/ClientFX.hpp"
#include "../../Server/Server.hpp"

// ========================
// FixKeyboardInputLanguage
// ========================

static void __fastcall LoadUserProfile_Hook(int thisPtr, int, bool bLoadDefaults, bool bLoadDisplaySettings)
{
	g_State.isLoadingDefault = bLoadDefaults;
	g_State.currentKeyIndex = 0;
	LoadUserProfile(thisPtr, bLoadDefaults, bLoadDisplaySettings);
	g_State.isLoadingDefault = false;
}

static bool __fastcall RestoreDefaults_Hook(int thisPtr, int, uint8_t nFlags)
{
	g_State.isLoadingDefault = true;
	g_State.currentKeyIndex = 0;
	bool res = RestoreDefaults(thisPtr, nFlags);
	g_State.isLoadingDefault = false;
	return res;
}
