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

	// The scene is rendered into this target until it's copied to the screen
	g_State.ssaaRenderTarget = *(int*)(thisPtr + g_State.cameraRenderTargetOffset);
	g_State.isRenderingSSAAScene = g_State.ssaaRenderTarget != 0;
}
