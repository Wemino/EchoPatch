#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"
#include "../../Controller/ScreenJoystickHook.hpp"
#include "../../ClientFX/ClientFX.hpp"
#include "../../Server/Server.hpp"

// =======================
// ConsoleEnabled
// =======================

static void __stdcall SetInputState_Hook(bool bAllowInput)
{
	g_State.wasInputDisabled = !bAllowInput;

	if (g_State.isConsoleOpen)
	{
		bAllowInput = false;
	}

	SetInputState(bAllowInput);
}
