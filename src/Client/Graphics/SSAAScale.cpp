#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"
#include "../../Controller/ScreenJoystickHook.hpp"
#include "../../ClientFX/ClientFX.hpp"
#include "../../Server/Server.hpp"

// =====================
// SSAAScale
// =====================

static DWORD cameraRenderTargetCheckAddr = 0;
static uint8_t cameraRenderTargetCheck[6] = {};
static float cameraRenderTargetScale = 1.0f;

static void __fastcall Camera_UpdateRenderTarget_Hook(int thisPtr, int)
{
	// Created again when SSAAScale is changed in the EchoPatch menu
	if (cameraRenderTargetScale != SSAAScale)
	{
		cameraRenderTargetScale = SSAAScale;
		*(int*)(thisPtr + g_State.cameraRenderTargetOffset + 4) = 0;

		// Also used without the screen effects of the game
		if (SSAAScale != 1.0f)
		{
			MemoryHelper::MakeNOP(cameraRenderTargetCheckAddr, 6);
		}
		else
		{
			MemoryHelper::WriteMemoryRaw(cameraRenderTargetCheckAddr, cameraRenderTargetCheck, 6);
		}
	}

	if (SSAAScale == 1.0f)
	{
		Camera_UpdateRenderTarget(thisPtr);
		g_State.ssaaRenderTarget = 0;
		g_State.isRenderingSSAAScene = false;
		return;
	}

	g_State.isInCameraUpdateRenderTarget = true;
	Camera_UpdateRenderTarget(thisPtr);
	g_State.isInCameraUpdateRenderTarget = false;

	// The scene is rendered into this target until it's copied to the screen
	g_State.ssaaRenderTarget = *(int*)(thisPtr + g_State.cameraRenderTargetOffset);
	g_State.isRenderingSSAAScene = g_State.ssaaRenderTarget != 0;
}
