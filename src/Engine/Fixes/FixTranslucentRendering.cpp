#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

enum class GlassPass { None, Main, Deferred };

// Technique indices of the materials
constexpr int TechniqueAmbient = 0;
constexpr int TechniqueTranslucent = 1;
constexpr int TechniquePoint = 2;
constexpr int TechniqueDirectional = 6;

struct TechniqueDesc
{
    const char* name;
    uint32_t passes;
    uint32_t annotations;
};

char(__thiscall* BeginTechnique)(int, int, int, uint32_t*) = nullptr;
int(__thiscall* LightPasses)(int, int, int, int) = nullptr;
int(__thiscall* BlackLightPass)(int, int, int, int) = nullptr;
bool(__thiscall* RenderListIsEmpty)(int) = nullptr;
void(__thiscall* RenderListClear)(int) = nullptr;
int(__thiscall* ComputeSortKeys)(int, float*, int, int) = nullptr;

static GlassPass glassPass = GlassPass::None;
static bool lightOnlySkipped = false;
static int lightRenderer = 0;
static int lightVariant = 0;

// ===========================
// FixTranslucentRendering
// ===========================

static DWORD GetTechnique(int material, int technique, int variant)
{
    return *(DWORD*)(material + 0xB0 + (technique * 3 + variant) * 4);
}

// Glass like materials have no Ambient pass and only add the light on top of what is behind
static bool IsLightOnlyMaterial(int material, int variant)
{
    int ambientMaterial = *(int*)(material + 4 + variant * 4);
    if (!ambientMaterial) return true;

    DWORD effect = *(DWORD*)(ambientMaterial + 0xAC);
    DWORD technique = GetTechnique(ambientMaterial, TechniqueAmbient, variant);
    if (!effect || !technique) return true;

    // ID3DXEffect::GetTechniqueDesc
    TechniqueDesc desc{};
    auto getTechniqueDesc = reinterpret_cast<HRESULT(__stdcall*)(DWORD, DWORD, TechniqueDesc*)>(*(DWORD*)(*(DWORD*)effect + 0x14));
    return FAILED(getTechniqueDesc(effect, technique, &desc)) || desc.passes == 0;
}

static bool HasLightTechnique(int material, int variant)
{
    for (int technique = TechniquePoint; technique <= TechniqueDirectional; technique++)
    {
        int techniqueMaterial = *(int*)(material + 4 + (technique * 3 + variant) * 4);
        if (techniqueMaterial && GetTechnique(techniqueMaterial, technique, variant))
            return true;
    }

    return false;
}

static char __fastcall BeginTechnique_Hook(int material, int, int technique, int variant, uint32_t* passes)
{
    if (glassPass != GlassPass::None && technique >= TechniquePoint && technique <= TechniqueDirectional && GetTechnique(material, technique, variant))
    {
        bool lightOnly = IsLightOnlyMaterial(material, variant);

        // Drawn again after the translucent pass
        if (glassPass == GlassPass::Main && lightOnly)
        {
            lightOnlySkipped = true;
            return 0;
        }

        if (glassPass == GlassPass::Deferred && !lightOnly)
            return 0;
    }

    return BeginTechnique(material, technique, variant, passes);
}

static int __fastcall LightPasses_Hook(int thisPtr, int, int scene, int visibleObjects, int variant)
{
    glassPass = GlassPass::Main;
    lightOnlySkipped = false;
    lightRenderer = thisPtr;
    lightVariant = variant;

    int result = LightPasses(thisPtr, scene, visibleObjects, variant);

    glassPass = GlassPass::None;
    return result;
}

// Called right after the translucent pass
static int __fastcall BlackLightPass_Hook(int thisPtr, int, int scene, int visibleObjects, int variant)
{
    if (lightOnlySkipped)
    {
        lightOnlySkipped = false;
        glassPass = GlassPass::Deferred;
        lightRenderer = thisPtr;
        lightVariant = variant;

        LightPasses(thisPtr, scene, visibleObjects, variant);

        glassPass = GlassPass::None;
    }

    return BlackLightPass(thisPtr, scene, visibleObjects, variant);
}

static bool __fastcall RenderListIsEmpty_Hook(int list, int)
{
    bool result = RenderListIsEmpty(list);

    // Only the lit objects list of a light during the deferred pass
    if (result || glassPass != GlassPass::Deferred || list != lightRenderer + 0x60)
        return result;

    int entrySize = g_State.IsExpansion() ? 0x2C : 0x28;
    for (int entry = *(int*)(list + 4); entry != *(int*)(list + 8); entry += entrySize)
    {
        int material = **(int**)(entry + 4);
        if (material && HasLightTechnique(material, lightVariant) && IsLightOnlyMaterial(material, lightVariant))
            return false;
    }

    // Skips the light and its shadows when it does not reach any glass
    RenderListClear(list);
    return true;
}

static float DistanceToBoundsSqr(const float* point, const float* min, const float* max)
{
    float distance = 0.0f;
    for (int i = 0; i < 3; i++)
    {
        float delta = point[i] - std::clamp(point[i], min[i], max[i]);
        distance += delta * delta;
    }

    return distance;
}

// Translucent objects are sorted by the distance to their position only
static int __fastcall ComputeSortKeys_Hook(int list, int, float* cameraPos, int technique, int variant)
{
    int result = ComputeSortKeys(list, cameraPos, technique, variant);
    if (technique != TechniqueTranslucent) return result;

    int entrySize = g_State.IsExpansion() ? 0x2C : 0x28;
    for (int entry = *(int*)(list + 4); entry != *(int*)(list + 8); entry += entrySize)
    {
        int object = *(int*)(entry + 8);
        if (!object) continue;

        // First person effects are drawn after everything else, the farthest first
        uint8_t renderLayer = *(uint8_t*)(*(int*)(object + 0xD4) + 8);
        if (renderLayer == 1)
        {
            *(float*)(entry + 0x18) = -1.0f / (1.0f + *(float*)(entry + 0x18));
            continue;
        }

        // Custom render objects not drawn in view space
        if (*(uint8_t*)(object + 0x57) != 6 || *(uint8_t*)(object + 0xF4) == 2)
            continue;

        // Visibility box or sphere, relative to the position
        const float* pos = (float*)(object + 0x9C);
        float min[3], max[3];
        bool hasBox = *(bool*)(object + 0xF5);

        for (int i = 0; i < 3; i++)
        {
            if (hasBox)
            {
                min[i] = pos[i] + *(float*)(object + 0x108 + i * 4);
                max[i] = pos[i] + *(float*)(object + 0x114 + i * 4);
            }
            else
            {
                float center = pos[i] + *(float*)(object + 0xF8 + i * 4);
                float radius = *(float*)(object + 0x104);
                min[i] = center - radius;
                max[i] = center + radius;
            }
        }

        // Decals use their center, the rest their closest point so the smoke around the camera is drawn last
        if (renderLayer >= 2 && renderLayer <= 4)
        {
            float center[3] = { (min[0] + max[0]) * 0.5f, (min[1] + max[1]) * 0.5f, (min[2] + max[2]) * 0.5f };
            *(float*)(entry + 0x18) = DistanceToBoundsSqr(cameraPos, center, center);
        }
        else
        {
            *(float*)(entry + 0x18) = DistanceToBoundsSqr(cameraPos, min, max);
        }
    }

    return result;
}

static void ApplyFixTranslucentRendering()
{
    if (!FixTranslucentRendering) return;

    bool checkRegion = g_State.CurrentFEARGame == FEAR;

    RenderListClear = reinterpret_cast<decltype(RenderListClear)>(GetAddress(Addr::RenderListClear));

    HookHelper::ApplyHook((void*)GetAddress(Addr::BeginTechnique), &BeginTechnique_Hook, (LPVOID*)&BeginTechnique, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::LightPasses), &LightPasses_Hook, (LPVOID*)&LightPasses, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::BlackLightPass), &BlackLightPass_Hook, (LPVOID*)&BlackLightPass, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::RenderListIsEmpty), &RenderListIsEmpty_Hook, (LPVOID*)&RenderListIsEmpty, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::ComputeSortKeys), &ComputeSortKeys_Hook, (LPVOID*)&ComputeSortKeys, checkRegion);
}
