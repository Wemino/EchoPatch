#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"
#include "../../Controller/ScreenJoystickHook.hpp"
#include "../../ClientFX/ClientFX.hpp"
#include "../../Server/Server.hpp"

// =========================
// HighResolutionReflections
// =========================

static bool __fastcall RenderTargetGroupFXInit_Hook(int thisPtr, int, DWORD* psfxCreateStruct)
{
	// Low
	psfxCreateStruct[3] *= 4;
	psfxCreateStruct[4] *= 4;
	// Medium
	psfxCreateStruct[5] *= 4;
	psfxCreateStruct[6] *= 4;
	// High
	psfxCreateStruct[7] *= 4;
	psfxCreateStruct[8] *= 4;
	return RenderTargetGroupFXInit(thisPtr, psfxCreateStruct);
}
