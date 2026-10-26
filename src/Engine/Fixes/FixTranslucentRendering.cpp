#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

enum class GlassPass { None, Main, Deferred };

// Technique indices of the materials
constexpr int TechniqueAmbient = 0;
constexpr int TechniqueTranslucent = 1;
constexpr int TechniquePoint = 2;
constexpr int TechniqueDirectional = 6;

// Each group of glass costs a light pass, more groups are lit together
constexpr size_t MaxGlassGroups = 4;

// Sort key offsets, overlapping decals by render layer and decals in front of the glass they are on
constexpr float DecalLayerOffset = 1e-6f;
constexpr float GlassDecalOffset = 1e-5f;

struct TechniqueDesc
{
    const char* name;
    uint32_t passes;
    uint32_t annotations;
};

struct DecalSort
{
    int entry;
    int layer;
    float center[3];
    float radius;
    float maxKey;
    int cluster;
};

struct MovedDecal
{
    int object;
    float pos[3];
};

struct GlassPosition
{
    int object;
    float pos[3];
    float min[3];
    float max[3];
    float distance;
};

int(__thiscall* LightPasses)(int, int, int, int) = nullptr;
int(__thiscall* BlackLightPass)(int, int, int, int) = nullptr;
bool(__thiscall* RenderListIsEmpty)(int) = nullptr;
void(__thiscall* RenderListClear)(int) = nullptr;
int(__thiscall* ComputeSortKeys)(int, float*, int, int) = nullptr;
char(__thiscall* RenderSortedList)(int, int, int, int, void*, int) = nullptr;
int(__stdcall* LightTranslucentObjects)(const float*, float, const float*, int) = nullptr;
int(__stdcall* LightTranslucentDirectional)(const float*, int) = nullptr;

static GlassPass glassPass = GlassPass::None;
static bool lightOnlySkipped = false;
static int lightRenderer = 0;
static int lightScene = 0;
static int lightObjects = 0;
static int lightVariant = 0;
static IDirect3DDevice9* const* rendererDevice = nullptr;

// Glass lit after the translucent objects behind it, by groups at the same distance
static std::unordered_map<int, int> glassGroups;
static size_t glassGroupCount = 0;
static int glassGroup = -1;
static float translucentMin = -FLT_MAX;
static float translucentMax = FLT_MAX;
static std::vector<float> translucentKeys;
static std::vector<float> glassGroupDistances;
static std::vector<GlassPosition> glassPositions;
static int spareLightList[3] = {};

static std::vector<DecalSort> decalSorts;
static std::vector<float> decalClusterKeys;
static std::vector<MovedDecal> movedDecals;

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

// Visibility box of a custom render object, its box or sphere is relative to the position
static void GetCustomRenderBox(int object, float* min, float* max)
{
    const float* pos = (float*)(object + 0x9C);
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
}

// Visibility sphere of a custom render object, returns its radius
static float GetCustomRenderSphere(int object, float* center)
{
    const float* pos = (float*)(object + 0x9C);

    if (!*(bool*)(object + 0xF5))
    {
        for (int i = 0; i < 3; i++)
            center[i] = pos[i] + *(float*)(object + 0xF8 + i * 4);

        return *(float*)(object + 0x104);
    }

    float radius = 0.0f;
    for (int i = 0; i < 3; i++)
    {
        float min = *(float*)(object + 0x108 + i * 4);
        float max = *(float*)(object + 0x114 + i * 4);
        center[i] = pos[i] + (min + max) * 0.5f;
        radius += (max - min) * (max - min) * 0.25f;
    }

    return sqrtf(radius);
}

// Decals projected on the world and on the world models
static bool IsDecal(int object)
{
    uint8_t renderLayer = *(uint8_t*)(*(int*)(object + 0xD4) + 8);
    return *(uint8_t*)(object + 0x57) == 6 && *(uint8_t*)(object + 0xF4) != 2 && renderLayer >= 2 && renderLayer <= 4;
}

// Visibility box of models, world models and custom render objects
static bool GetObjectBounds(int object, float* min, float* max)
{
    uint8_t type = *(uint8_t*)(object + 0x57);
    if (type != 1 && type != 2 && type != 6) return false;

    // 0 for a sphere and 1 for a box
    float bounds[6] = {};
    int shape = reinterpret_cast<int(__thiscall*)(int, float*)>((*(DWORD**)object)[2])(object, bounds);

    for (int i = 0; i < 3; i++)
    {
        min[i] = shape == 1 ? bounds[i] : bounds[i] - bounds[3];
        max[i] = shape == 1 ? bounds[i + 3] : bounds[i] + bounds[3];
    }

    return shape == 0 || shape == 1;
}

// Closest distance from the camera to an object
static float GetObjectDistanceSqr(int object, const float* cameraPos)
{
    const float* pos = (float*)(object + 0x9C);
    float min[3], max[3];

    return GetObjectBounds(object, min, max) ? DistanceToBoundsSqr(cameraPos, min, max) : DistanceToBoundsSqr(cameraPos, pos, pos);
}

// Decals projected on a glass follow its world model, they share its position and touch it
static bool GetGlassDistance(int object, const DecalSort& decal, float& distance)
{
    const float* pos = (float*)(object + 0x9C);

    for (const auto& glass : glassPositions)
    {
        if (glass.pos[0] == pos[0] && glass.pos[1] == pos[1] && glass.pos[2] == pos[2] && DistanceToBoundsSqr(decal.center, glass.min, glass.max) <= decal.radius * decal.radius)
        {
            distance = glass.distance;
            return true;
        }
    }

    return false;
}

static size_t CountTranslucentKeys(float min, float max)
{
    return std::upper_bound(translucentKeys.begin(), translucentKeys.end(), max) - std::upper_bound(translucentKeys.begin(), translucentKeys.end(), min);
}

static bool IsGlassInGroup(int object)
{
    if (glassGroup < 0) return true;

    // Not seen before the translucent pass, lit with the nearest glass
    auto it = glassGroups.find(object);
    return it != glassGroups.end() ? it->second == glassGroup : glassGroup == (int)glassGroupCount - 1;
}

// Groups the glass from the farthest, a new group starts when something translucent is between them
static bool PlanGlassGroups(const float* cameraPos, std::vector<float>& groupDistances)
{
    // Main world glass has no object, lit after all the world and before the first person effects
    std::vector<std::pair<float, int>> glass;
    for (const auto& [object, group] : glassGroups)
    {
        glass.push_back({ object ? GetObjectDistanceSqr(object, cameraPos) : -FLT_MIN, object });
    }

    std::sort(glass.begin(), glass.end(), [](const auto& a, const auto& b) { return a.first > b.first; });

    groupDistances.clear();
    for (const auto& [distance, object] : glass)
    {
        if (groupDistances.empty() || CountTranslucentKeys(distance, groupDistances.back()) > 0)
        {
            groupDistances.push_back(distance);
        }
        else
        {
            groupDistances.back() = distance;
        }

        glassGroups[object] = (int)groupDistances.size() - 1;
    }

    // Too many groups, the ones with the least translucent objects between them are lit together
    bool isMerged = groupDistances.size() > MaxGlassGroups;

    while (groupDistances.size() > MaxGlassGroups)
    {
        size_t merged = 0;
        for (size_t i = 1; i + 1 < groupDistances.size(); i++)
        {
            if (CountTranslucentKeys(groupDistances[i + 1], groupDistances[i]) < CountTranslucentKeys(groupDistances[merged + 1], groupDistances[merged]))
                merged = i;
        }

        groupDistances.erase(groupDistances.begin() + merged);

        for (auto& [object, group] : glassGroups)
        {
            if (group > (int)merged)
                group--;
        }
    }

    glassGroupCount = groupDistances.size();
    return isMerged;
}

// Lights the glass skipped by the light passes
static void DrawDeferredGlass(int group, bool isTranslucentPass)
{
    IDirect3DDevice9* device = isTranslucentPass ? *rendererDevice : nullptr;
    int lightList = lightRenderer + 0x60;
    DWORD states[4][2] = { { D3DRS_ZENABLE }, { D3DRS_ZWRITEENABLE }, { D3DRS_ALPHABLENDENABLE }, { D3DRS_STENCILENABLE } };

    // The light passes use the list of the translucent pass, it's kept aside meanwhile
    if (isTranslucentPass)
    {
        std::swap_ranges((int*)(lightList + 4), (int*)(lightList + 0x10), spareLightList);

        for (auto& state : states)
        {
            device->GetRenderState((D3DRENDERSTATETYPE)state[0], &state[1]);
        }
    }

    glassPass = GlassPass::Deferred;
    glassGroup = group;

    LightPasses(lightRenderer, lightScene, lightObjects, lightVariant);

    glassPass = GlassPass::None;
    glassGroup = -1;

    if (isTranslucentPass)
    {
        if (*(int*)(lightList + 4) != *(int*)(lightList + 8))
        {
            RenderListClear(lightList);
        }

        std::swap_ranges((int*)(lightList + 4), (int*)(lightList + 0x10), spareLightList);

        for (const auto& state : states)
        {
            device->SetRenderState((D3DRENDERSTATETYPE)state[0], state[1]);
        }
    }
}

static int FindDecalCluster(int index)
{
    while (decalSorts[index].cluster != index)
    {
        decalSorts[index].cluster = decalSorts[decalSorts[index].cluster].cluster;
        index = decalSorts[index].cluster;
    }

    return index;
}

// Overlapping decals are drawn together at the distance of the farthest one, then by render layer whatever the camera position
static void SortOverlappingDecals()
{
    size_t count = decalSorts.size();
    if (count == 0) return;

    // Swept along the axis they are the most spread on, only the ones close enough are tested
    int axis = 0;
    float spread[3] = {};

    for (int k = 0; k < 3; k++)
    {
        auto [low, high] = std::minmax_element(decalSorts.begin(), decalSorts.end(), [k](const DecalSort& a, const DecalSort& b) { return a.center[k] < b.center[k]; });
        spread[k] = high->center[k] - low->center[k];
        axis = spread[k] > spread[axis] ? k : axis;
    }

    std::sort(decalSorts.begin(), decalSorts.end(), [axis](const DecalSort& a, const DecalSort& b) { return a.center[axis] - a.radius < b.center[axis] - b.radius; });

    for (size_t i = 0; i < count; i++)
    {
        decalSorts[i].cluster = (int)i;
    }

    for (size_t i = 0; i < count; i++)
    {
        for (size_t j = i + 1; j < count && decalSorts[j].center[axis] - decalSorts[j].radius <= decalSorts[i].center[axis] + decalSorts[i].radius; j++)
        {
            float distance = 0.0f;
            for (int k = 0; k < 3; k++)
            {
                distance += (decalSorts[i].center[k] - decalSorts[j].center[k]) * (decalSorts[i].center[k] - decalSorts[j].center[k]);
            }

            float reach = decalSorts[i].radius + decalSorts[j].radius;
            if (distance <= reach * reach)
            {
                decalSorts[FindDecalCluster((int)j)].cluster = FindDecalCluster((int)i);
            }
        }
    }

    decalClusterKeys.assign(count, 0.0f);
    for (size_t i = 0; i < count; i++)
    {
        float& key = decalClusterKeys[FindDecalCluster((int)i)];
        key = std::max(key, *(float*)(decalSorts[i].entry + 0x18));
    }

    // Still in front of the glass they are on
    for (size_t i = 0; i < count; i++)
    {
        *(float*)(decalSorts[i].entry + 0x18) = std::min(decalClusterKeys[FindDecalCluster((int)i)], decalSorts[i].maxKey) * (1.0f - decalSorts[i].layer * DecalLayerOffset);
    }
}

static int __fastcall LightPasses_Hook(int thisPtr, int, int scene, int visibleObjects, int variant)
{
    glassPass = GlassPass::Main;
    lightOnlySkipped = false;
    lightRenderer = thisPtr;
    lightScene = scene;
    lightObjects = visibleObjects;
    lightVariant = variant;
    glassGroups.clear();
    glassGroupCount = 0;

    int result = LightPasses(thisPtr, scene, visibleObjects, variant);

    glassPass = GlassPass::None;
    return result;
}

// Called right after the translucent pass, the glass is lit here when the translucent pass did not
static int __fastcall BlackLightPass_Hook(int thisPtr, int, int scene, int visibleObjects, int variant)
{
    if (lightOnlySkipped)
    {
        lightOnlySkipped = false;
        lightRenderer = thisPtr;
        lightScene = scene;
        lightObjects = visibleObjects;
        lightVariant = variant;

        DrawDeferredGlass(-1, false);
    }

    return BlackLightPass(thisPtr, scene, visibleObjects, variant);
}

static bool __fastcall RenderListIsEmpty_Hook(int list, int)
{
    bool result = RenderListIsEmpty(list);

    // The glass is drawn without stencil, its shadow casters are skipped
    if (!result && glassPass == GlassPass::Deferred && list == lightRenderer + 0x70)
    {
        RenderListClear(list);
        return true;
    }

    // Only the lit objects list of a light during the light passes
    if (result || glassPass == GlassPass::None || list != lightRenderer + 0x60)
        return result;

    bool hasLightOnly = false;
    bool hasLit = false;

    int entrySize = g_State.IsExpansion() ? 0x2C : 0x28;
    for (int entry = *(int*)(list + 4); entry != *(int*)(list + 8); entry += entrySize)
    {
        int material = **(int**)(entry + 4);
        if (!material || !HasLightTechnique(material, lightVariant)) continue;

        if (!IsLightOnlyMaterial(material, lightVariant))
        {
            hasLit = true;
        }
        else if (glassPass == GlassPass::Main)
        {
            glassGroups.emplace(*(int*)(entry + 8), 0);
            hasLightOnly = true;
        }
        else if (IsGlassInGroup(*(int*)(entry + 8)))
        {
            hasLightOnly = true;
        }
    }

    if (glassPass == GlassPass::Main && hasLightOnly)
        lightOnlySkipped = true;

    if (glassPass == GlassPass::Main ? hasLit : hasLightOnly)
        return false;

    // Skips the light and its shadows when it has nothing to light in this pass
    RenderListClear(list);
    return true;
}

static int __fastcall ComputeSortKeys_Hook(int list, int, float* cameraPos, int technique, int variant)
{
    int result = ComputeSortKeys(list, cameraPos, technique, variant);
    int entrySize = g_State.IsExpansion() ? 0x2C : 0x28;

    // Each object is checked from its own material, the light technique can come from a parent, fallback or detail level material
    if (glassPass != GlassPass::None && technique >= TechniquePoint && technique <= TechniqueDirectional)
    {
        for (int entry = *(int*)(list + 4); entry != *(int*)(list + 8); entry += entrySize)
        {
            if (!*(int*)entry) continue;

            int object = *(int*)(entry + 8);
            bool lightOnly = IsLightOnlyMaterial(**(int**)(entry + 4), variant);

            // Left without technique like the materials missing it, the glass is drawn again during the translucent pass
            if (glassPass == GlassPass::Main && lightOnly)
            {
                *(int*)entry = 0;
                lightOnlySkipped = true;
                glassGroups.emplace(object, 0);
            }
            else if (glassPass == GlassPass::Deferred && (!lightOnly || !IsGlassInGroup(object)))
            {
                *(int*)entry = 0;
            }
        }

        return result;
    }

    // Translucent objects are sorted by the distance to their position only
    if (technique != TechniqueTranslucent) return result;

    decalSorts.clear();

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

        // Decals use their center, the rest their closest point so the smoke around the camera is drawn last
        if (renderLayer >= 2 && renderLayer <= 4)
        {
            DecalSort decal{ entry, renderLayer - 2 };
            decal.radius = GetCustomRenderSphere(object, decal.center);
            decal.maxKey = FLT_MAX;
            *(float*)(entry + 0x18) = DistanceToBoundsSqr(cameraPos, decal.center, decal.center);

            // On a glass, drawn over its light
            float glassDistance = 0.0f;
            if (*(uint8_t*)(object + 0xF4) == 0 && GetGlassDistance(object, decal, glassDistance))
                decal.maxKey = glassDistance * (1.0f - GlassDecalOffset);

            decalSorts.push_back(decal);
        }
        else
        {
            float min[3], max[3];
            GetCustomRenderBox(object, min, max);
            *(float*)(entry + 0x18) = DistanceToBoundsSqr(cameraPos, min, max);
        }
    }

    SortOverlappingDecals();

    bool isSplit = translucentMin != -FLT_MAX || translucentMax != FLT_MAX;

    for (int entry = *(int*)(list + 4); entry != *(int*)(list + 8); entry += entrySize)
    {
        // Drawn first, a key that can't be compared would break the sorting
        float& key = *(float*)(entry + 0x18);
        if (!std::isfinite(key))
            key = FLT_MAX;

        // Only the objects between two groups of glass
        if (isSplit && (key <= translucentMin || key > translucentMax))
            *(int*)entry = 0;
    }

    return result;
}

// Sort keys of the translucent objects, kept sorted to count the ones between two distances
static void ComputeTranslucentKeys(int list, float* cameraPos, int variant)
{
    ComputeSortKeys_Hook(list, 0, cameraPos, TechniqueTranslucent, variant);

    translucentKeys.clear();

    int entrySize = g_State.IsExpansion() ? 0x2C : 0x28;
    for (int entry = *(int*)(list + 4); entry != *(int*)(list + 8); entry += entrySize)
    {
        if (*(int*)entry)
            translucentKeys.push_back(*(float*)(entry + 0x18));
    }

    std::sort(translucentKeys.begin(), translucentKeys.end());
}

// The translucent objects behind the glass are drawn before its light, the ones in front after
static char __fastcall RenderSortedList_Hook(int list, int, int scene, int technique, int variant, void* callback, int userData)
{
    if (technique != TechniqueTranslucent || !lightOnlySkipped || list != lightRenderer + 0x60 || scene != lightScene || !*rendererDevice)
        return RenderSortedList(list, scene, technique, variant, callback, userData);

    lightOnlySkipped = false;

    float* cameraPos = (float*)(scene + 0x288);

    glassPositions.clear();
    for (const auto& [object, group] : glassGroups)
    {
        GlassPosition glass{ object };
        if (!object || !GetObjectBounds(object, glass.min, glass.max)) continue;

        memcpy(glass.pos, (float*)(object + 0x9C), sizeof(glass.pos));
        glass.distance = DistanceToBoundsSqr(cameraPos, glass.min, glass.max);
        glassPositions.push_back(glass);
    }

    // Sort keys of everything drawn by this pass
    ComputeTranslucentKeys(list, cameraPos, variant);

    // A glass lit with a nearer group, the decals on it are drawn after that group
    if (PlanGlassGroups(cameraPos, glassGroupDistances))
    {
        for (auto& glass : glassPositions)
        {
            glass.distance = std::min(glass.distance, glassGroupDistances[glassGroups[glass.object]]);
        }

        ComputeTranslucentKeys(list, cameraPos, variant);
    }

    char result = 1;
    float max = FLT_MAX;

    for (size_t group = 0; group <= glassGroupDistances.size(); group++)
    {
        float min = group < glassGroupDistances.size() ? glassGroupDistances[group] : -FLT_MAX;

        if (CountTranslucentKeys(min, max) > 0)
        {
            translucentMin = min;
            translucentMax = max;
            result = RenderSortedList(list, scene, technique, variant, callback, userData);
            translucentMin = -FLT_MAX;
            translucentMax = FLT_MAX;
        }

        if (group < glassGroupDistances.size())
            DrawDeferredGlass((int)group, true);

        max = min;
    }

    glassPositions.clear();
    return result;
}

// Translucent objects are lit from their position, the decals from their center like when they are sorted
static int __stdcall LightTranslucentObjects_Hook(const float* lightPos, float radius, const float* color, int objects)
{
    // Already lit by the light passes
    if (glassPass == GlassPass::Deferred)
        return *(int*)(objects + 8);

    movedDecals.clear();

    for (int* it = *(int**)(objects + 4); it != *(int**)(objects + 8); it++)
    {
        int object = *it;
        float* pos = (float*)(object + 0x9C);
        float center[3];

        if (!(*(uint16_t*)(object + 0x64) & 0x400) || !IsDecal(object)) continue;

        GetCustomRenderSphere(object, center);
        if (center[0] == pos[0] && center[1] == pos[1] && center[2] == pos[2]) continue;

        movedDecals.push_back({ object, { pos[0], pos[1], pos[2] } });
        memcpy(pos, center, sizeof(center));
    }

    int result = LightTranslucentObjects(lightPos, radius, color, objects);

    for (auto it = movedDecals.rbegin(); it != movedDecals.rend(); it++)
        memcpy((float*)(it->object + 0x9C), it->pos, sizeof(it->pos));

    return result;
}

static int __stdcall LightTranslucentDirectional_Hook(const float* color, int objects)
{
    // Already lit by the light passes
    if (glassPass == GlassPass::Deferred)
        return *(int*)(objects + 8);

    return LightTranslucentDirectional(color, objects);
}

static void ApplyFixTranslucentRendering()
{
    if (!FixTranslucentRendering) return;

    bool checkRegion = g_State.CurrentFEARGame == FEAR;

    RenderListClear = reinterpret_cast<decltype(RenderListClear)>(GetAddress(Addr::RenderListClear));
    rendererDevice = reinterpret_cast<IDirect3DDevice9* const*>(GetAddress(Addr::RendererDevice));

    HookHelper::ApplyHook((void*)GetAddress(Addr::LightPasses), &LightPasses_Hook, (LPVOID*)&LightPasses, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::BlackLightPass), &BlackLightPass_Hook, (LPVOID*)&BlackLightPass, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::RenderListIsEmpty), &RenderListIsEmpty_Hook, (LPVOID*)&RenderListIsEmpty, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::ComputeSortKeys), &ComputeSortKeys_Hook, (LPVOID*)&ComputeSortKeys, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::LightTranslucentObjects), &LightTranslucentObjects_Hook, (LPVOID*)&LightTranslucentObjects, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::LightTranslucentDirectional), &LightTranslucentDirectional_Hook, (LPVOID*)&LightTranslucentDirectional, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::RenderSortedList), &RenderSortedList_Hook, (LPVOID*)&RenderSortedList, checkRegion);
}
