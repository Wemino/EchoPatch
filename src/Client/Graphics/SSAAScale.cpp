#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"
#include "../../Controller/ScreenJoystickHook.hpp"
#include "../../ClientFX/ClientFX.hpp"
#include "../../Server/Server.hpp"

// =====================
// SSAAScale
// =====================

static void __fastcall Camera_UpdateRenderTarget_Hook(int thisPtr, int)
{
	g_State.isInCameraUpdateRenderTarget = true;
	Camera_UpdateRenderTarget(thisPtr);
	g_State.isInCameraUpdateRenderTarget = false;
}
