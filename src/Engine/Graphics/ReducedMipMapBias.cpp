#pragma once

#include <cmath>

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

static bool isLoadingWorld = false;

static constexpr float DEFAULT_MIPMAP_BIAS = -0.25f;

static std::vector<uint32_t> sharpTexturePaths;

namespace TextureHelper
{
	inline bool StartsWithI(const char* path, const char* prefix)
	{
		while (*prefix)
		{
			char p = *path++;
			char x = *prefix++;

			if (p >= 'A' && p <= 'Z') p += 32;
			if (x >= 'A' && x <= 'Z') x += 32;

			if (p != x)
				return false;
		}
		return true;
	}

	inline bool ContainsI(const char* path, const char* substr)
	{
		if (!path || !substr)
			return false;

		size_t subLen = strlen(substr);
		size_t pathLen = strlen(path);

		if (subLen > pathLen)
			return false;

		for (size_t i = 0; i <= pathLen - subLen; i++)
		{
			bool match = true;
			for (size_t j = 0; j < subLen; j++)
			{
				char p = path[i + j];
				char s = substr[j];

				if (p >= 'A' && p <= 'Z') p += 32;
				if (s >= 'A' && s <= 'Z') s += 32;

				if (p != s)
				{
					match = false;
					break;
				}
			}
			if (match)
				return true;
		}
		return false;
	}

	inline bool ShouldKeepSharp(const char* path)
	{
		if (!path)
			return false;

		// Prefix (base game)
		if (StartsWithI(path, "attachments\\"))
			return true;
		if (StartsWithI(path, "chars\\skins\\"))
			return true;
		if (StartsWithI(path, "FX\\"))
			return true;
		if (StartsWithI(path, "guns\\"))
			return true;
		if (StartsWithI(path, "Interface\\"))
			return true;

		// Prefix (expansions)
		if (StartsWithI(path, "charsxp\\"))
			return true;
		if (StartsWithI(path, "fx-xp\\"))
			return true;
		if (StartsWithI(path, "guns-xp\\"))
			return true;
		if (StartsWithI(path, "Attachments-XP\\"))
			return true;

		// Prefabs - full folder paths
		if (StartsWithI(path, "Prefabs\\Systemic\\Elevators\\"))
			return true;
		if (StartsWithI(path, "Prefabs\\Systemic\\Security\\"))
			return true;
		if (StartsWithI(path, "Prefabs\\Systemic\\Vehicles\\"))
			return true;
		if (StartsWithI(path, "Prefabs\\Industrial\\Spillway\\"))
			return true;
		if (StartsWithI(path, "Prefabs\\Industrial\\Interactive\\"))
			return true;

		// Materials - Industrial
		if (StartsWithI(path, "Materials\\Industrial\\"))
		{
			if (ContainsI(path, "Cabinet") ||
				ContainsI(path, "Elevator"))
				return true;
		}

		// Prefabs - Industrial
		if (StartsWithI(path, "Prefabs\\Industrial\\"))
		{
			if (ContainsI(path, "Barrel") ||
				ContainsI(path, "box") ||
				ContainsI(path, "Bucket") ||
				ContainsI(path, "Cargo") ||
				ContainsI(path, "control_unit") ||
				ContainsI(path, "Door") ||
				ContainsI(path, "hammer") ||
				ContainsI(path, "generator") ||
				ContainsI(path, "Garage_Gate") ||
				ContainsI(path, "Dumpster") ||
				ContainsI(path, "Fan") ||
				ContainsI(path, "Machine") ||
				ContainsI(path, "Phone") ||
				ContainsI(path, "rack_pallet") ||
				ContainsI(path, "power_") ||
				ContainsI(path, "shelf_metal") ||
				ContainsI(path, "spray_") ||
				ContainsI(path, "tool_chest") ||
				ContainsI(path, "Wallbox") ||
				ContainsI(path, "workbench") ||
				ContainsI(path, "wrench"))
				return true;
		}

		// Prefabs - Office
		if (StartsWithI(path, "Prefabs\\Office\\"))
		{
			if (ContainsI(path, "Book") ||
				ContainsI(path, "Cactus") ||
				ContainsI(path, "Copier") ||
				ContainsI(path, "Kiosk") ||
				ContainsI(path, "mouse_"))
				return true;
		}

		// Prefabs - Tech
		if (StartsWithI(path, "Prefabs\\Tech\\"))
		{
			if (ContainsI(path, "alma") ||
				ContainsI(path, "barrel_tech_"))
				return true;
		}

		// Prefabs - Test
		if (StartsWithI(path, "Prefabs\\Test\\"))
		{
			if (ContainsI(path, "chair02") ||
				ContainsI(path, "desk_") ||
				ContainsI(path, "DeskMonitor"))
				return true;
		}

		// Tex - Office
		if (StartsWithI(path, "Tex\\Office\\"))
		{
			if (ContainsI(path, "CorporateART01") ||
				ContainsI(path, "Painting01") ||
				ContainsI(path, "Painting03"))
				return true;
		}

		// Tex - Industrial
		if (StartsWithI(path, "Tex\\Industrial\\"))
		{
			if (ContainsI(path, "Fence"))
				return true;
		}

		return false;
	}
}

HRESULT(WINAPI* D3D9_SetTexture)(IDirect3DDevice9*, DWORD, IDirect3DBaseTexture9*) = nullptr;
int(__thiscall* LoadWorld)(BYTE*, int, int) = nullptr;

// ===========================
// ReducedMipMapBias
// ===========================

static int __fastcall LoadWorld_Hook(BYTE* thisPtr, int, int a2, int a3)
{
    isLoadingWorld = true;
    int result = LoadWorld(thisPtr, a2, a3);
    isLoadingWorld = false;
    return result;
}

static bool IsSharpTexturePath(const char* path)
{
    return std::binary_search(sharpTexturePaths.begin(), sharpTexturePaths.end(), HashHelper::FNV1aRuntime(path));
}

static void MarkSharpTexture(void* d3dTexture, const char* path)
{
    if (!d3dTexture) return;

    auto it = std::lower_bound(g_State.sharpTextures.begin(), g_State.sharpTextures.end(), d3dTexture);
    if (it == g_State.sharpTextures.end() || *it != d3dTexture)
    {
        g_State.sharpTextures.insert(it, d3dTexture);
    }

    uint32_t pathHash = HashHelper::FNV1aRuntime(path);
    auto itPath = std::lower_bound(sharpTexturePaths.begin(), sharpTexturePaths.end(), pathHash);
    if (itPath == sharpTexturePaths.end() || *itPath != pathHash)
    {
        sharpTexturePaths.insert(itPath, pathHash);
    }
}

static void UnmarkSharpTexture(void* d3dTexture)
{
    if (!d3dTexture) return;

    auto it = std::lower_bound(g_State.sharpTextures.begin(), g_State.sharpTextures.end(), d3dTexture);
    if (it != g_State.sharpTextures.end() && *it == d3dTexture)
    {
        g_State.sharpTextures.erase(it);
    }
}

static HRESULT WINAPI SetTexture_Hook(IDirect3DDevice9* device, DWORD Stage, IDirect3DBaseTexture9* pTexture)
{
    HRESULT hr = D3D9_SetTexture(device, Stage, pTexture);

    if (Stage >= std::size(g_State.stageIsDirty))
        return hr;

    bool isSharp = pTexture && std::binary_search(g_State.sharpTextures.begin(), g_State.sharpTextures.end(), pTexture);

    if (isSharp)
    {
        static float ssaaBiasScale = 0.0f;
        static float ssaaBiasOffset = 0.0f;

        if (ssaaBiasScale != SSAAScale)
        {
            ssaaBiasScale = SSAAScale;
            ssaaBiasOffset = SSAAScale > 1.0f ? std::log2(SSAAScale) : 0.0f;
        }

        float sharpBias = g_State.isRenderingSSAAScene ? std::min(-1.5f + ssaaBiasOffset, 0.0f) : -1.5f;

        if (!g_State.stageIsDirty[Stage] || g_State.stageBias[Stage] != sharpBias)
        {
            device->SetSamplerState(Stage, D3DSAMP_MIPMAPLODBIAS, *reinterpret_cast<DWORD*>(&sharpBias));
            g_State.stageIsDirty[Stage] = true;
            g_State.stageBias[Stage] = sharpBias;
        }
    }
    else if (g_State.stageIsDirty[Stage])
    {
        float defaultBias = DEFAULT_MIPMAP_BIAS;
        device->SetSamplerState(Stage, D3DSAMP_MIPMAPLODBIAS, *reinterpret_cast<DWORD*>(&defaultBias));
        g_State.stageIsDirty[Stage] = false;
    }

    return hr;
}

static void ApplyReducedMipMapBias()
{
    if (!ReducedMipMapBias) return;

    MemoryHelper::WriteMemory<float>(GetAddress(Addr::MipMapBias), DEFAULT_MIPMAP_BIAS, false);
    HookHelper::ApplyHook((void*)GetAddress(Addr::LoadWorld), &LoadWorld_Hook, (LPVOID*)&LoadWorld);
}