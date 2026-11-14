#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"
#include "../../Controller/ScreenJoystickHook.hpp"
#include "../../ClientFX/ClientFX.hpp"
#include "../../Server/Server.hpp"

// =========================
// HighResolutionReflections
// =========================

// Render target groups whose render target was created in high resolution
static std::unordered_set<int> highResolutionRenderTargetGroups;

static bool __fastcall RenderTargetGroupFXCreateRenderTarget_Hook(int thisPtr, int, uint32_t nLOD)
{
	if (!HighResolutionReflections)
	{
		highResolutionRenderTargetGroups.erase(thisPtr);
		return RenderTargetGroupFXCreateRenderTarget(thisPtr, nLOD);
	}

	highResolutionRenderTargetGroups.insert(thisPtr);

	// 'm_nDimensions' of the LOD
	DWORD* dimensions = (DWORD*)(thisPtr + 0x50) + nLOD * 2;
	dimensions[0] *= 4;
	dimensions[1] *= 4;
	bool result = RenderTargetGroupFXCreateRenderTarget(thisPtr, nLOD);
	dimensions[0] /= 4;
	dimensions[1] /= 4;
	return result;
}

static bool __fastcall RenderTargetGroupFXUpdate_Hook(int thisPtr, int)
{
	// Created again like when RenderTargetLOD changes
	if (highResolutionRenderTargetGroups.contains(thisPtr) != HighResolutionReflections)
	{
		*(DWORD*)(thisPtr + 0x44) = -1;
	}

	return RenderTargetGroupFXUpdate(thisPtr);
}
