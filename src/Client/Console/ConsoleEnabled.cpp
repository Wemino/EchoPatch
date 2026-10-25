#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"
#include "../../Controller/ScreenJoystickHook.hpp"
#include "../../ClientFX/ClientFX.hpp"
#include "../../Server/Server.hpp"
#include "ConsoleMgr.hpp"

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

// Commands still held are released, like the game does when its chat box opens and closes
static void ClearHeldCommands()
{
	if (GetBindMgr && BindMgrClearAllCommands)
	{
		BindMgrClearAllCommands(GetBindMgr());
	}
}

// Called from the main loop when the console opens or closes
static bool OnConsoleStateChanged(bool open)
{
	if (!SetInputState)
		return false;

	if (open)
	{
		// Cleared while the input is still enabled so the player stops moving
		ClearHeldCommands();
		SetInputState(false);
	}
	else
	{
		// Keys pressed while typing don't trigger commands until they are released
		SetInputState(!g_State.wasInputDisabled);
		ClearHeldCommands();
	}

	return true;
}
