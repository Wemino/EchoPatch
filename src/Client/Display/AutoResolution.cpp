#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"
#include "../../Controller/ScreenJoystickHook.hpp"
#include "../../ClientFX/ClientFX.hpp"
#include "../../Server/Server.hpp"

// ======================
// AutoResolution
// ======================

static void __cdecl AutoDetectPerformanceSettings_Hook()
{
	g_State.isInAutoDetect = true;
	AutoDetectPerformanceSettings();
	g_State.isInAutoDetect = false;
}

static void __fastcall SetOption_Hook(int thisPtr, int, int a2, int a3, int a4, int a5)
{
	g_State.isSettingOption = true;
	SetOption(thisPtr, a2, a3, a4, a5);
	g_State.isSettingOption = false;
}

static bool __fastcall SetQueuedConsoleVariable_Hook(int thisPtr, int, const char* pszVar, float a3, int a4)
{
	// Don't update the resolution if we change the presets
	if (g_State.isSettingOption && (strcmp(pszVar, "Performance_ScreenHeight") == 0 || strcmp(pszVar, "Performance_ScreenWidth") == 0))
	{
		return false;
	}

	return SetQueuedConsoleVariable(thisPtr, pszVar, a3, a4);
}
