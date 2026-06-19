#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"
#include "../../Controller/ScreenJoystickHook.hpp"
#include "../../ClientFX/ClientFX.hpp"
#include "../../Server/Server.hpp"

// =============================
// DisableHipFireAccuracyPenalty
// =============================

static void __fastcall AccuracyMgrUpdate_Hook(float* thisPtr, int)
{
	AccuracyMgrUpdate(thisPtr);
	*thisPtr = 0.0f;
}
