#pragma once

#include "../../Globals.cpp"

// ===================
// FixWaterReflections
// ===================

static std::unordered_map<std::string, int> materialRenderTargets;

static float GetRenderTargetDistanceSqr(int renderTarget, const float* position)
{
	float objectPos[3] = {};
	EngineInterface::Call<int>(EngineInterface::GetClient(), EngineInterface::Slot::GetObjectPos, *(DWORD*)(renderTarget + 0x24), (void*)objectPos);

	float dx = objectPos[0] - position[0];
	float dy = objectPos[1] - position[1];
	float dz = objectPos[2] - position[2];
	return dx * dx + dy * dy + dz * dz;
}

static bool __fastcall RenderTargetFXBindToMaterialParam_Hook(int thisPtr, int)
{
	bool wasBound = *(bool*)(thisPtr + 0x50);
	bool result = RenderTargetFXBindToMaterialParam(thisPtr);

	if (!wasBound && result)
	{
		materialRenderTargets[RenderTargetFXMaterialCStr(thisPtr + 0x70)] = thisPtr;
	}

	return result;
}

static bool __fastcall RenderTargetFXUnbindMaterialParam_Hook(int thisPtr, int)
{
	auto it = materialRenderTargets.find(RenderTargetFXMaterialCStr(thisPtr + 0x70));

	// The material shows another render target, keep it
	if (it != materialRenderTargets.end() && it->second != thisPtr)
	{
		*(bool*)(thisPtr + 0x50) = false;
		return true;
	}

	if (it != materialRenderTargets.end())
	{
		materialRenderTargets.erase(it);
	}

	return RenderTargetFXUnbindMaterialParam(thisPtr);
}

static bool __fastcall RenderTargetFXUpdateRenderTarget_Hook(int thisPtr, int, const float* camera, const float* fov)
{
	// Water at different heights can share a material, it shows the render target closest to the camera
	if (*(bool*)(thisPtr + 0x50))
	{
		auto it = materialRenderTargets.find(RenderTargetFXMaterialCStr(thisPtr + 0x70));
		if (it == materialRenderTargets.end() || (it->second != thisPtr && GetRenderTargetDistanceSqr(thisPtr, camera) < GetRenderTargetDistanceSqr(it->second, camera)))
		{
			// Bound again and rendered now, 'm_bBoundToMaterialParam' and 'm_bDirtyRenderTarget'
			*(bool*)(thisPtr + 0x50) = false;
			RenderTargetFXBindToMaterialParam_Hook(thisPtr, 0);
			*(bool*)(thisPtr + 0x51) = true;
		}
	}

	return RenderTargetFXUpdateRenderTarget(thisPtr, camera, fov);
}

static const float* __stdcall GetMirrorInitialSectorPos(const float* objectPos, const float* mirrorPlane, const float* cameraPos)
{
	return std::fabs(mirrorPlane[1]) > 0.99f ? cameraPos : objectPos;
}

// Replaces 'lea ecx, [esp + 3Ch]' and 'push ecx' in UpdateRenderTargetMirror, the mirror plane is pushed before it and the camera transform is in esi
static __declspec(naked) void UpdateRenderTargetMirrorInitialSectorPos_Hook()
{
	__asm
	{
		push eax
		push edx
		lea ecx, [esp + 0x48]
		push esi
		push dword ptr [esp + 0x18]
		push ecx
		call GetMirrorInitialSectorPos
		mov ecx, eax
		pop edx
		pop eax
		xchg ecx, dword ptr [esp]
		jmp ecx
	}
}
