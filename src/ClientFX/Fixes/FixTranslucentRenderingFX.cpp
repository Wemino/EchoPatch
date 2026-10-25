#pragma once

#include "../../Globals.cpp"

static DWORD pLTClientFXAddr = 0;

bool(__thiscall* ProjectDecal)(int) = nullptr;

// ========================
// FixTranslucentRendering
// ========================

static bool __fastcall ProjectDecal_Hook(int thisPtr, int)
{
    bool result = ProjectDecal(thisPtr);

    DWORD client = *(DWORD*)pLTClientFXAddr;
    if (!client) return result;

    DWORD customRender = EngineInterface::Call<DWORD>(client, EngineInterface::Slot::GetCustomRender);
    uint32_t decalCount = std::min(*(uint32_t*)(thisPtr + 0x1EC), 4u);

    for (uint32_t i = 0; i < decalCount; i++)
    {
        // SWorldModelDecal
        int decal = thisPtr + 0x7C + i * 0x5C;
        DWORD hObject = *(DWORD*)(decal + 0x10);

        // Only the main world, its decals are in world space and left at the origin
        if (!hObject || !*(bool*)(decal + 0x54)) continue;

        // Translucent objects are sorted and lit from their position, move it to 'm_vVisCenterWS'
        EngineInterface::Vector3 center = *(EngineInterface::Vector3*)(decal + 0x24);
        EngineInterface::Vector3 sphereCenter{};

        EngineInterface::Call<int>(client, EngineInterface::Slot::SetObjectPos, hObject, &center);
        EngineInterface::CallOffset<int>(customRender, EngineInterface::CustomRender_SetVisBoundingSphere, hObject, &sphereCenter, *(float*)(decal + 0x14));
    }

    return result;
}

static void ApplyFixTranslucentRenderingClientFXPatch()
{
    if (!FixTranslucentRendering) return;

    DWORD addr_ProjectDecal = ScanModuleSignature(g_State.GameClientFX, "81 EC A8 01 00 00 56 57 8B F1 E8 ?? ?? ?? ?? ?? 46 20", "ProjectDecal");
    DWORD addr_DecalRenderLayer = ScanModuleSignature(g_State.GameClientFX, "0F B6 40 5D 83 F8 02 BF 02 00 00 00 7F 02 8B F8 8B 0D ?? ?? ?? ?? 8B 11", "DecalRenderLayer");

    if (addr_ProjectDecal == 0 || addr_DecalRenderLayer == 0)
        return;

    pLTClientFXAddr = MemoryHelper::ReadMemory<DWORD>(addr_DecalRenderLayer + 0x12);

    HookHelper::ApplyHookReplaceable((void*)addr_ProjectDecal, &ProjectDecal_Hook, (LPVOID*)&ProjectDecal);
}
