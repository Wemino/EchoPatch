#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "../../Globals.cpp"
#include "../../Addresses.cpp"

char(__thiscall* LoadShader)(void*, const char*, void*) = nullptr;

static std::unordered_map<std::string, std::vector<bool>> validatedTechniques;
static std::vector<bool>* currentShader = nullptr;
static size_t currentTechnique = 0;

// ===========================
// CacheShaderValidation
// ===========================

static char __fastcall LoadShader_Hook(void* thisPtr, int, const char* pFileName, void* pDevice)
{
    std::vector<bool>* previousShader = currentShader;
    size_t previousTechnique = currentTechnique;

    currentShader = pFileName ? &validatedTechniques[pFileName] : nullptr;
    currentTechnique = 0;

    char result = LoadShader(thisPtr, pFileName, pDevice);

    currentShader = previousShader;
    currentTechnique = previousTechnique;

    return result;
}

static HRESULT __stdcall ValidateTechnique_Hook(DWORD effect, DWORD technique)
{
    size_t index = currentTechnique++;

    if (currentShader && index < currentShader->size() && (*currentShader)[index])
        return D3D_OK;

    // ID3DXEffect::ValidateTechnique
    auto validateTechnique = reinterpret_cast<HRESULT(__stdcall*)(DWORD, DWORD)>(*(DWORD*)(*(DWORD*)effect + 0xF0));
    HRESULT hr = validateTechnique(effect, technique);

    // A failed technique is validated again the next time, like the engine does
    if (currentShader && SUCCEEDED(hr))
    {
        if (currentShader->size() <= index)
        {
            currentShader->resize(index + 1, false);
        }

        (*currentShader)[index] = true;
    }

    return hr;
}

static void ApplyCacheShaderValidation()
{
    if (!CacheShaderValidation) return;

    DWORD addr_ValidateTechnique = GetAddress(Addr::ValidateTechnique);

    HookHelper::ApplyHook((void*)GetAddress(Addr::LoadShader), &LoadShader_Hook, (LPVOID*)&LoadShader);

    MemoryHelper::MakeCALL(addr_ValidateTechnique, reinterpret_cast<uintptr_t>(&ValidateTechnique_Hook));
    MemoryHelper::MakeNOP(addr_ValidateTechnique + 0x5, 1);
}
