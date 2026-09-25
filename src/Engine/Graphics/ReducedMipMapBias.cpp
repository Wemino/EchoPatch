#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

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
		if (StartsWithI(path, "CharsXP\\"))
			return true;
		if (StartsWithI(path, "FX-XP\\"))
			return true;
		if (StartsWithI(path, "Guns-XP\\"))
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
int(__stdcall* CreateTextureWrapper)(DWORD*, int, int) = nullptr;
int(__thiscall* DestroyTextureWrapper)(int*, int*) = nullptr;
int(__thiscall* LoadWorld)(BYTE*, int, int) = nullptr;

// ===========================
// ReducedMipMapBias
// ===========================

static int __fastcall LoadWorld_Hook(BYTE* thisPtr, int, int a2, int a3)
{
    g_State.isLoadingWorld = true;
    int result = LoadWorld(thisPtr, a2, a3);
    g_State.isLoadingWorld = false;
    return result;
}

static int __stdcall CreateTextureWrapper_Hook(DWORD* a1, int a2, int a3)
{
    int result = CreateTextureWrapper(a1, a2, a3);

    if (g_State.isLoadingWorld && result && a3)
    {
        const char* path = reinterpret_cast<const char*>(a3);

        if (TextureHelper::ShouldKeepSharp(path))
        {
            void* d3dTexture = *reinterpret_cast<void**>(result);

            if (d3dTexture)
            {
                auto it = std::lower_bound(g_State.sharpTextures.begin(), g_State.sharpTextures.end(), d3dTexture);
                if (it == g_State.sharpTextures.end() || *it != d3dTexture)
                {
                    g_State.sharpTextures.insert(it, d3dTexture);
                }
            }
        }
    }

    return result;
}

static int __fastcall DestroyTextureWrapper_Hook(int* thisPtr, int, int* a2)
{
    if (a2)
    {
        void* d3dTexture = reinterpret_cast<void*>(*a2);

        if (d3dTexture)
        {
            auto it = std::lower_bound(g_State.sharpTextures.begin(), g_State.sharpTextures.end(), d3dTexture);
            if (it != g_State.sharpTextures.end() && *it == d3dTexture)
            {
                g_State.sharpTextures.erase(it);
            }
        }
    }

    return DestroyTextureWrapper(thisPtr, a2);
}

static HRESULT WINAPI SetTexture_Hook(IDirect3DDevice9* device, DWORD Stage, IDirect3DBaseTexture9* pTexture)
{
    HRESULT hr = D3D9_SetTexture(device, Stage, pTexture);

    bool isSharp = pTexture && std::binary_search(g_State.sharpTextures.begin(), g_State.sharpTextures.end(), pTexture);

    if (isSharp)
    {
        if (!g_State.stageIsDirty[Stage])
        {
            float sharpBias = -2.0f;
            device->SetSamplerState(Stage, D3DSAMP_MIPMAPLODBIAS, *reinterpret_cast<DWORD*>(&sharpBias));
            g_State.stageIsDirty[Stage] = true;
        }
    }
    else if (g_State.stageIsDirty[Stage])
    {
        float defaultBias = -0.5f;
        device->SetSamplerState(Stage, D3DSAMP_MIPMAPLODBIAS, *reinterpret_cast<DWORD*>(&defaultBias));
        g_State.stageIsDirty[Stage] = false;
    }

    return hr;
}

static void ApplyReducedMipMapBias()
{
    if (!ReducedMipMapBias) return;

    MemoryHelper::WriteMemory<float>(GetAddress(Addr::MipMapBias), -0.5f, false);
    HookHelper::ApplyHook((void*)GetAddress(Addr::LoadWorld), &LoadWorld_Hook, (LPVOID*)&LoadWorld);
    HookHelper::ApplyHook((void*)GetAddress(Addr::DestroyTextureWrapper), &DestroyTextureWrapper_Hook, (LPVOID*)&DestroyTextureWrapper);
    HookHelper::ApplyHook((void*)GetAddress(Addr::CreateTextureWrapper), &CreateTextureWrapper_Hook, (LPVOID*)&CreateTextureWrapper);
}

