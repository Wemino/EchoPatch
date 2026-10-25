#pragma once

#include "../../Globals.cpp"
#include "../../Engine/Console/EngineInterfaces.cpp"
#include "ConsoleMgr.hpp"

using EngineInterface::Vector3;

// Line drawn by a line system: start, end, then the color in the engine order
struct DebugLineData
{
    Vector3 start;
    Vector3 end;
    uint8_t b, g, r, a;
};

DWORD(__cdecl* GetLineSystem)(DWORD, const char*) = nullptr;
void(__thiscall* ClearLineSystem)(DWORD) = nullptr;
void(__thiscall* AddLineToSystem)(DWORD, const DebugLineData*) = nullptr;
void(__cdecl* ClearAllLineSystems)() = nullptr;
DWORD(__thiscall* GetNodeList)(DWORD, int) = nullptr;
void(__thiscall* UpdateNodeDebugStatus)(DWORD, DWORD) = nullptr;
void(__thiscall* RedrawNavMeshPoly)(DWORD, int) = nullptr;
void(__thiscall* ConstructString)(void*) = nullptr;
void(__thiscall* DestroyString)(void*) = nullptr;
void*(__thiscall* AssignString)(void*, const char*) = nullptr;
const char*(__thiscall* StringCStr)(const void*) = nullptr;

// =======================
// ConsoleDebugLines
// =======================

namespace DebugLines
{
    constexpr DWORD kSystemHeader = 0x10;
    constexpr DWORD kSystemObject = 0x08;
    constexpr DWORD kSystemFirstLine = 0x14;
    constexpr DWORD kSystemLastLine = 0x18;
    constexpr DWORD kSystemLabel = 0x24;
    constexpr DWORD kSystemLabelPos = 0x40;
    constexpr DWORD kSystemSize = 0x4C;

    // No object is behind the line systems kept here, any value but 0 works
    constexpr DWORD kNoObject = 1;

    // Entry of the list of the game: +0x00 object reference, +0x0C object, +0x10 line system
    constexpr DWORD kEntrySystem = 0x10;

    // Nav mesh: polys and edges are 72 bytes
    constexpr DWORD kNavMeshPolyCount = 0x24;
    constexpr DWORD kNavMeshPolys = 0x28;
    constexpr DWORD kNavMeshRegionsBegin = 0x58;
    constexpr DWORD kNavMeshRegionsEnd = 0x5C;
    constexpr DWORD kNavMeshEdgeCount = 0x7C;
    constexpr DWORD kNavMeshEdges = 0x80;
    constexpr DWORD kNavMeshElementSize = 72;
    constexpr DWORD kPolyId = 0x00;
    constexpr DWORD kPolyCenter = 0x14;
    constexpr DWORD kPolyEdgeCount = 0x38;
    constexpr DWORD kPolyEdgeList = 0x3C;
    constexpr DWORD kEdgeType = 0x04;
    constexpr DWORD kEdgeStart = 0x08;
    constexpr DWORD kEdgeEnd = 0x18;
    constexpr DWORD kEdgePolyA = 0x34;
    constexpr DWORD kEdgePolyB = 0x38;
    constexpr DWORD kRegionName = 0x20;

    // Lines closer than this to the camera plane are cut
    constexpr float kNearDepth = 1.0f;

    struct LineSystem
    {
        alignas(4) uint8_t data[kSystemSize];
        DWORD entry;
    };

    // Camera the lines are drawn from, given by the client: project gives the position on the screen from 0 to 1 and the depth
    struct View
    {
        Vector3 position;
        float range = 2000.0f;
        bool (*project)(const Vector3& point, float& x, float& y, float& depth) = nullptr;
    };

    static DWORD nullSystem = 0;
    static DWORD nodeMgrAddr = 0;
    static DWORD navMeshAddr = 0;
    static DWORD nodeDrawOffset = 0;
    static DWORD nodeHideOffset = 0;
    static int nodeTypeCount = 0;
    static const char* const* nodeTypeNames = nullptr;
    static DWORD regionPolyCountOffset = 0;
    static DWORD regionPolyListOffset = 0;
    static DWORD serverModuleBase = 0;
    static bool isServerPatched = false;

    // Line systems are only made while a view is on, the retail behavior stays otherwise
    static std::unordered_map<DWORD, LineSystem*> systems;
    static bool canCreateSystems = false;
    static bool hasPlayer = false;

    // State of the views for the current world
    static DWORD worldKey = 0;
    static int drawnNodeType = -1;
    static bool isNavMeshDrawn = false;
    static bool isRegionsDrawn = false;
    static size_t nextStatusNode = 0;
    static std::string lastNodesValue;

    // The addresses found in the server module are only kept while it stays loaded
    inline bool IsServerModuleLoaded()
    {
        HMODULE module = nullptr;
        return serverModuleBase && GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)GetLineSystem, &module) && (DWORD)module == serverModuleBase;
    }

    inline bool IsKeptSystem(DWORD system)
    {
        return system && systems.find(system) != systems.end();
    }

    inline DWORD CreateSystem(DWORD entry)
    {
        LineSystem* system = new LineSystem();
        memset(system->data, 0, sizeof(system->data));

        // Same class table as the null system, the lines and the label start empty
        memcpy(system->data, (const void*)nullSystem, kSystemHeader);
        DWORD address = reinterpret_cast<DWORD>(system->data);
        *(DWORD*)(address + kSystemObject) = kNoObject;
        ConstructString((void*)(address + kSystemLabel));
        system->entry = entry;
        systems[address] = system;
        return address;
    }

    // The entries of the game point to the null system again, the lines go with the allocator of the game while its module is loaded
    inline void ReleaseSystems(bool moduleLoaded)
    {
        if (systems.empty())
            return;

        for (const auto& [address, system] : systems)
        {
            if (moduleLoaded)
            {
                if (*(DWORD*)(system->entry + kEntrySystem) == address)
                {
                    *(DWORD*)(system->entry + kEntrySystem) = nullSystem;
                }

                ClearLineSystem(address);
                DestroyString((void*)(address + kSystemLabel));
            }

            delete system;
        }

        systems.clear();
    }

    // ClearLines empties every line system
    inline void ClearSystems()
    {
        for (const auto& [address, system] : systems)
        {
            ClearLineSystem(address);
            AssignString((void*)(address + kSystemLabel), "");
        }
    }

    inline DWORD GetSystem(DWORD owner, const char* name)
    {
        DWORD system = GetLineSystem ? GetLineSystem(owner, name) : 0;
        return IsKeptSystem(system) ? system : 0;
    }

    inline void ClearSystem(DWORD system)
    {
        if (system)
        {
            ClearLineSystem(system);
        }
    }

    inline void SetLabel(DWORD system, const char* text, const Vector3& position)
    {
        if (system)
        {
            AssignString((void*)(system + kSystemLabel), text);
            *(Vector3*)(system + kSystemLabelPos) = position;
        }
    }

    inline void AddLine(DWORD system, const Vector3& start, const Vector3& end, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
    {
        if (system)
        {
            DebugLineData line = { start, end, b, g, r, a };
            AddLineToSystem(system, &line);
        }
    }

    inline std::string ReadGameString(DWORD address)
    {
        const char* text = StringCStr ? StringCStr((const void*)address) : nullptr;
        return text ? text : "";
    }

    // AI nodes
    inline DWORD GetNodeManager()
    {
        return nodeMgrAddr ? *(DWORD*)nodeMgrAddr : 0;
    }

    template <typename Fn>
    inline void ForEachNode(int type, Fn&& fn)
    {
        DWORD nodeMgr = GetNodeManager();
        if (!nodeMgr || type < 0 || type >= nodeTypeCount)
            return;

        DWORD list = GetNodeList(nodeMgr, type);
        if (!list)
            return;

        // Vector: allocator, first, last, end
        DWORD* first = *(DWORD**)(list + 0x04);
        DWORD* last = *(DWORD**)(list + 0x08);
        for (DWORD* it = first; it && it < last; it++)
        {
            if (*it)
            {
                fn(*it);
            }
        }
    }

    inline void CallNodeVirtual(DWORD node, DWORD offset)
    {
        reinterpret_cast<int(__thiscall*)(DWORD)>(EngineInterface::GetVirtual(node, offset))(node);
    }

    // A number or a node type name, 0 or an unknown name is off
    inline int ParseNodeType(const std::string& value)
    {
        if (value.empty())
            return -1;

        char* end = nullptr;
        double number = strtod(value.c_str(), &end);
        if (end && end != value.c_str() && *end == '\0')
            return number >= 1.0 && number < nodeTypeCount ? (int)number : -1;

        for (int i = 0; i < nodeTypeCount; i++)
        {
            if (nodeTypeNames[i] && _stricmp(nodeTypeNames[i], value.c_str()) == 0)
                return i;
        }

        return -1;
    }

    inline const char* GetNodeTypeName(int type)
    {
        return type >= 0 && type < nodeTypeCount && nodeTypeNames[type] ? nodeTypeNames[type] : "?";
    }

    inline bool IsStatusNodeType(int type)
    {
        const char* name = GetNodeTypeName(type);
        return _stricmp(name, "Cover") == 0 || _stricmp(name, "Armored") == 0 || _stricmp(name, "Ambush") == 0 || _stricmp(name, "Safety") == 0 || _stricmp(name, "DarkChant") == 0;
    }

    inline void ShowNodes(int type, bool show)
    {
        ForEachNode(type, [&](DWORD node)
        {
            CallNodeVirtual(node, show ? nodeDrawOffset : nodeHideOffset);
        });
    }

    // Nodes that show if they can be used are drawn again when it changes, a few per frame
    inline void UpdateNodeStatus(int type)
    {
        if (!UpdateNodeDebugStatus || !IsStatusNodeType(type))
            return;

        DWORD hPlayer = EngineInterface::GetPlayerObject();
        if (!hPlayer)
            return;

        std::vector<DWORD> nodes;
        ForEachNode(type, [&](DWORD node) { nodes.push_back(node); });
        if (nodes.empty())
            return;

        for (int i = 0; i < 5 && i < (int)nodes.size(); i++)
        {
            nextStatusNode = (nextStatusNode + 1) % nodes.size();
            UpdateNodeDebugStatus(nodes[nextStatusNode], hPlayer);
        }
    }

    // Nav mesh
    inline DWORD GetNavMesh()
    {
        return navMeshAddr ? *(DWORD*)navMeshAddr : 0;
    }

    inline void ShowNavMesh(bool show)
    {
        DWORD navMesh = GetNavMesh();
        if (!navMesh)
            return;

        int polyCount = *(int*)(navMesh + kNavMeshPolyCount);
        DWORD polys = *(DWORD*)(navMesh + kNavMeshPolys);

        for (int i = 0; i < polyCount && polys; i++)
        {
            if (show)
            {
                RedrawNavMeshPoly(navMesh, i);
            }
            else
            {
                DWORD poly = polys + i * kNavMeshElementSize;
                DWORD system = GetSystem(poly, "ShowNavMesh");
                SetLabel(system, "", Vector3());
                ClearSystem(system);
            }
        }
    }

    // AI regions, drawn as the outline of their nav mesh polys
    inline void DrawRegion(DWORD navMesh, DWORD region, bool show)
    {
        DWORD system = GetSystem(region, "ShowAIRegions");
        if (!system)
            return;

        ClearSystem(system);
        SetLabel(system, "", Vector3());
        if (!show)
            return;

        const int polyCount = *(int*)(navMesh + kNavMeshPolyCount);
        const DWORD polys = *(DWORD*)(navMesh + kNavMeshPolys);
        const int edgeCount = *(int*)(navMesh + kNavMeshEdgeCount);
        const DWORD edges = *(DWORD*)(navMesh + kNavMeshEdges);

        const int regionPolyCount = *(int*)(region + regionPolyCountOffset);
        const int* regionPolys = *(const int**)(region + regionPolyListOffset);
        if (!polys || !edges || !regionPolys || regionPolyCount <= 0)
            return;

        auto containsPoly = [&](int polyId)
        {
            for (int i = 0; i < regionPolyCount; i++)
            {
                if (regionPolys[i] == polyId)
                    return true;
            }
            return false;
        };

        bool labeled = false;
        for (int i = 0; i < regionPolyCount; i++)
        {
            int polyId = regionPolys[i];
            if (polyId < 0 || polyId >= polyCount)
                continue;

            DWORD poly = polys + polyId * kNavMeshElementSize;
            int polyEdgeCount = *(int*)(poly + kPolyEdgeCount);
            const int* polyEdges = *(const int**)(poly + kPolyEdgeList);

            for (int e = 0; polyEdges && e < polyEdgeCount; e++)
            {
                int edgeId = polyEdges[e];
                if (edgeId < 0 || edgeId >= edgeCount)
                    continue;

                DWORD edge = edges + edgeId * kNavMeshElementSize;

                // A border of the nav mesh, or an edge shared with a poly outside of the region
                bool outline = *(int*)(edge + kEdgeType) == 0;
                if (!outline)
                {
                    int polyA = *(int*)(edge + kEdgePolyA);
                    int neighbor = polyA != *(int*)(poly + kPolyId) ? polyA : *(int*)(edge + kEdgePolyB);
                    outline = !containsPoly(neighbor);
                }

                if (outline)
                {
                    Vector3 start = *(Vector3*)(edge + kEdgeStart);
                    Vector3 end = *(Vector3*)(edge + kEdgeEnd);
                    start.y += 10.0f;
                    end.y += 10.0f;
                    AddLine(system, start, end, 0, 255, 0);
                }
            }

            if (!labeled)
            {
                Vector3 center = *(Vector3*)(poly + kPolyCenter);
                center.y += 16.0f;
                SetLabel(system, ReadGameString(region + kRegionName).c_str(), center);
                labeled = true;
            }
        }
    }

    inline void ShowRegions(bool show)
    {
        DWORD navMesh = GetNavMesh();
        if (!navMesh || !regionPolyCountOffset)
            return;

        DWORD* first = *(DWORD**)(navMesh + kNavMeshRegionsBegin);
        DWORD* last = *(DWORD**)(navMesh + kNavMeshRegionsEnd);

        for (DWORD* it = first; it && it < last; it++)
        {
            if (*it)
            {
                DrawRegion(navMesh, *it, show);
            }
        }
    }

    // A new world, or the same one loaded again, is drawn again
    inline DWORD GetWorldKey()
    {
        DWORD navMesh = GetNavMesh();
        DWORD polys = navMesh ? *(DWORD*)(navMesh + kNavMeshPolys) : 0;
        return EngineInterface::GetPlayerObject() ^ (polys << 1);
    }

    inline float GetServerFloat(const char* name)
    {
        return EngineInterface::GetVariableFloat(EngineInterface::GetServer(), name);
    }

    inline std::string GetServerString(const char* name)
    {
        return EngineInterface::GetVariableString(EngineInterface::GetServer(), name);
    }

    inline void ResetViews()
    {
        drawnNodeType = -1;
        isNavMeshDrawn = false;
        isRegionsDrawn = false;
        nextStatusNode = 0;
    }

    // Called once per frame
    inline void Update()
    {
        if (!isServerPatched)
            return;

        if (!EngineInterface::IsServerRunning() || !IsServerModuleLoaded())
        {
            canCreateSystems = false;
            hasPlayer = false;
            worldKey = 0;
            ResetViews();
            ReleaseSystems(IsServerModuleLoaded());
            return;
        }

        std::string nodesValue = GetServerString("ShowAINodes");
        bool showNavMesh = GetServerFloat("ShowAINavMesh") > 0.0f;
        bool showRegions = GetServerFloat("ShowAIRegions") > 0.0f;
        bool showPaths = GetServerFloat("ShowAIPath") > 0.0f;

        int nodeType = ParseNodeType(nodesValue);
        if (nodesValue != lastNodesValue)
        {
            if (nodeType < 0 && !nodesValue.empty() && nodesValue != "0")
            {
                Console::Print("ShowAINodes: unknown node type '" + nodesValue + "', use a number from 1 to " + std::to_string(nodeTypeCount - 1) + " or a type name", ConsoleLineType::Warning);
            }
            lastNodesValue = nodesValue;
        }

        canCreateSystems = nodeType >= 0 || showNavMesh || showRegions || showPaths;

        bool ok = EngineInterface::SafeCall([&]()
        {
            // Nothing is drawn while a world loads or ends, the player is there once it is ready
            hasPlayer = EngineInterface::GetPlayerObject() != 0;
            if (!hasPlayer)
            {
                worldKey = 0;
                ResetViews();
                return;
            }

            DWORD key = GetWorldKey();
            if (key != worldKey)
            {
                // The objects of the previous world are gone, their line systems too
                ReleaseSystems(true);
                worldKey = key;
                ResetViews();
            }

            if (nodeType != drawnNodeType)
            {
                if (drawnNodeType >= 0)
                {
                    ShowNodes(drawnNodeType, false);
                }

                if (nodeType >= 0)
                {
                    ShowNodes(nodeType, true);
                }

                drawnNodeType = nodeType;
                nextStatusNode = 0;
            }
            else if (nodeType >= 0)
            {
                UpdateNodeStatus(nodeType);
            }

            if (showNavMesh != isNavMeshDrawn)
            {
                ShowNavMesh(showNavMesh);
                isNavMeshDrawn = showNavMesh;
            }

            if (showRegions != isRegionsDrawn)
            {
                ShowRegions(showRegions);
                isRegionsDrawn = showRegions;
            }

            // Nothing is kept while every view is off
            if (!canCreateSystems)
            {
                ReleaseSystems(true);
            }
        });

        if (!ok)
        {
            // Turned off until the next world
            Console::Print("The AI debug views failed in this world, they are turned off until the next one", ConsoleLineType::Error);
            drawnNodeType = nodeType;
            isNavMeshDrawn = showNavMesh;
            isRegionsDrawn = showRegions;
        }
    }

    // Drawing, the lines stay over the world: they are full up to the range and fade out at twice the range, the labels end at the range like in the game
    inline bool HasLines()
    {
        return isServerPatched && canCreateSystems && hasPlayer && !systems.empty();
    }

    inline float GetFade(float distance, float range)
    {
        return distance <= range ? 1.0f : std::clamp(2.0f - distance / range, 0.0f, 1.0f);
    }

    // Distance from a point to the closest point of a line
    inline float GetLineDistance(const Vector3& point, const Vector3& start, const Vector3& end)
    {
        Vector3 direction = end - start;
        float lengthSq = direction.Dot(direction);
        float t = lengthSq > 0.0f ? std::clamp((point - start).Dot(direction) / lengthSq, 0.0f, 1.0f) : 0.0f;
        return (start + direction * t - point).Length();
    }

    inline void DrawLine(ImDrawList* drawList, const View& view, const ImVec2& size, const DebugLineData& line, float thickness)
    {
        float fade = GetFade(GetLineDistance(view.position, line.start, line.end), view.range);
        if (fade <= 0.0f)
            return;

        float startX, startY, startDepth, endX, endY, endDepth;
        if (!view.project(line.start, startX, startY, startDepth) || !view.project(line.end, endX, endY, endDepth))
            return;

        if (startDepth < kNearDepth && endDepth < kNearDepth)
            return;

        // The part behind the camera is cut, the depth changes linearly along the line
        if (startDepth < kNearDepth || endDepth < kNearDepth)
        {
            float t = (kNearDepth - startDepth) / (endDepth - startDepth);
            Vector3 cut = line.start + (line.end - line.start) * t;
            bool projected = startDepth < kNearDepth ? view.project(cut, startX, startY, startDepth) : view.project(cut, endX, endY, endDepth);
            if (!projected)
                return;
        }

        ImVec2 a(startX * size.x, startY * size.y);
        ImVec2 b(endX * size.x, endY * size.y);
        if ((a.x < 0.0f && b.x < 0.0f) || (a.x > size.x && b.x > size.x) || (a.y < 0.0f && b.y < 0.0f) || (a.y > size.y && b.y > size.y))
            return;

        drawList->AddLine(a, b, IM_COL32(line.r, line.g, line.b, (int)(line.a * fade)), thickness);
    }

    inline void DrawLabel(ImDrawList* drawList, const View& view, const ImVec2& size, DWORD system, const DebugLineData* firstLine)
    {
        const char* text = StringCStr((const void*)(system + kSystemLabel));
        if (!text || !text[0])
            return;

        const Vector3 position = *(const Vector3*)(system + kSystemLabelPos);
        float distance = (position - view.position).Length();
        if (distance > view.range)
            return;

        float x, y, depth;
        if (!view.project(position, x, y, depth) || depth < kNearDepth || x < 0.0f || x > 1.0f || y < 0.0f || y > 1.0f)
            return;

        // Smaller with the distance like the labels of the game, in the color of the lines
        ImFont* font = ImGui::GetFont();
        float fontSize = ImGui::GetFontSize() * (1.0f - 0.4f * distance / view.range);
        ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);
        ImVec2 pos(floorf(x * size.x - textSize.x * 0.5f), floorf(y * size.y - textSize.y));
        ImU32 color = firstLine ? IM_COL32(firstLine->r, firstLine->g, firstLine->b, 255) : IM_COL32(255, 255, 255, 255);

        drawList->AddText(font, fontSize, ImVec2(pos.x + 1.0f, pos.y + 1.0f), IM_COL32(0, 0, 0, 200), text);
        drawList->AddText(font, fontSize, pos, color, text);
    }

    inline void Render(const View& view)
    {
        if (!view.project || !StringCStr)
            return;

        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        const ImVec2 size = ImGui::GetIO().DisplaySize;
        const float thickness = std::max(1.0f, Console::GetUIScale());

        // A line system the game freed in the meantime stops the drawing for this frame
        EngineInterface::SafeCall([&]()
        {
            for (const auto& [address, system] : systems)
            {
                const DebugLineData* first = *(const DebugLineData**)(address + kSystemFirstLine);
                const DebugLineData* last = *(const DebugLineData**)(address + kSystemLastLine);
                for (const DebugLineData* line = first; line && line < last; line++)
                {
                    DrawLine(drawList, view, size, *line, thickness);
                }

                DrawLabel(drawList, view, size, address, first && first < last ? first : nullptr);
            }
        });
    }
}

// Line system of an entry of the list of the game, called by the game for each view that draws
static DWORD __stdcall GetKeptLineSystem(DWORD entry)
{
    DWORD system = *(DWORD*)(entry + DebugLines::kEntrySystem);
    if (DebugLines::IsKeptSystem(system))
        return system;

    system = DebugLines::canCreateSystems ? DebugLines::CreateSystem(entry) : DebugLines::nullSystem;
    *(DWORD*)(entry + DebugLines::kEntrySystem) = system;
    return system;
}

// Replaces the end of the function that returns the line system of a name, esi is the entry
static __declspec(naked) void GetLineSystemEntry_Hook()
{
    __asm
    {
        push esi
        call GetKeptLineSystem
        pop esi
        add esp, 0x1C
        ret
    }
}

// ClearLines also clears the line systems kept here
static void __cdecl ClearAllLineSystems_Hook()
{
    ClearAllLineSystems();
    DebugLines::ClearSystems();
}

static void ApplyConsoleDebugLinesServerPatch()
{
    if (!ConsoleEnabled || g_State.CurrentFEARGame == FEARMP) return;

    DebugLines::isServerPatched = false;

    DWORD addr_GetSystemEntry = ScanModuleSignature(g_State.GameServer, "8B 46 0C 85 C0 8B 46 10 75 1F 85 C0 74 20 57 8B 78 08 85 FF 74 0F 8B CE E8 ?? ?? ?? ?? 57 8B CE E8 ?? ?? ?? ?? 8B 46 10 5F 5E 83 C4 1C C3 C7 46 10", "DebugLines_GetSystemEntry");
    DWORD addr_GetSystem = ScanModuleSignature(g_State.GameServer, "8B 44 24 08 8B 4C 24 04 81 EC 00 01 00 00 50 51 68 ?? ?? ?? ?? 8D 54 24 0C 68 00 01 00 00 52 E8 ?? ?? ?? ?? 8D 44 24 14 50 68 ?? ?? ?? ?? E8", "DebugLines_GetSystem");
    DWORD addr_Clear = ScanModuleSignature(g_State.GameServer, "56 8B F1 8B 46 14 85 C0 74 09 50 E8 ?? ?? ?? ?? 83 C4 04 C7 46 14 00 00 00 00 C7 46 18 00 00 00 00 C7 46 1C 00 00 00 00 C6 46 20 01 5E C3", "DebugLines_Clear");
    DWORD addr_AddLine = ScanModuleSignature(g_State.GameServer, "8B 41 08 85 C0 74 08 83 C1 10 E9 ?? ?? ?? ?? C2 04 00", "DebugLines_AddLine");
    DWORD addr_ClearLines = ScanModuleSignature(g_State.GameServer, "F6 C4 41 75 20 E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 85 C9 74 11 8B 15 ?? ?? ?? ?? 8B 01 6A 00 52 FF 90", "DebugLines_ClearLines");
    DWORD addr_NodeStatus = ScanModuleSignature(g_State.GameServer, "56 6A FF 8B F1 8B 4C 24 0C 8B 06 6A 00 51 8D 56 ?? 52 6A 00 8B CE FF 90 ?? ?? 00 00 38 86 ?? ?? 00 00 74 1A 88 86 ?? ?? 00 00 8B 06 8B CE FF 90 ?? ?? 00 00 8B 16 8B CE FF 92", "DebugLines_NodeStatus");
    DWORD addr_NodeTypeName = ScanModuleSignature(g_State.GameServer, "8B 44 24 04 83 F8 FF 75 06 B8 ?? ?? ?? ?? C3 85 C0 7C 0D 83 F8 ?? 7D 08 8B 04 85 ?? ?? ?? ?? C3", "DebugLines_NodeTypeName");
    DWORD addr_NodeList = ScanModuleSignature(g_State.GameServer, "8B 46 08 8B 48 0C 57 51 8B 0D ?? ?? ?? ?? E8", "DebugLines_NodeList");
    DWORD addr_NavMeshPoly = ScanModuleSignature(g_State.GameServer, "84 C0 74 0F 8B 4E 40 51 8B 0D ?? ?? ?? ?? E8", "DebugLines_NavMeshPoly");
    DWORD addr_RegionPolys = ScanModuleSignature(g_State.GameServer, "8B 51 ?? 56 33 C0 85 D2 7E 13 8B 49 ?? 8B 74 24 08 39 31 74 0E 40 83 C1 04 3B C2 7C F4 32 C0", "DebugLines_RegionPolys");

    // The line systems of the game use the strings of msvcp71
    HMODULE msvcp = GetModuleHandleA("msvcp71.dll");
    ConstructString = msvcp ? reinterpret_cast<decltype(ConstructString)>(GetProcAddress(msvcp, "??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@XZ")) : nullptr;
    DestroyString = msvcp ? reinterpret_cast<decltype(DestroyString)>(GetProcAddress(msvcp, "??1?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@XZ")) : nullptr;
    AssignString = msvcp ? reinterpret_cast<decltype(AssignString)>(GetProcAddress(msvcp, "??4?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV01@PBD@Z")) : nullptr;
    StringCStr = msvcp ? reinterpret_cast<decltype(StringCStr)>(GetProcAddress(msvcp, "?c_str@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPBDXZ")) : nullptr;

    if (addr_GetSystemEntry == 0 ||
        addr_GetSystem == 0 ||
        addr_Clear == 0 ||
        addr_AddLine == 0 ||
        !ConstructString || !DestroyString || !AssignString || !StringCStr) {
        return;
    }

    // Systems of a previous load of the module are left as they are
    for (const auto& [address, system] : DebugLines::systems)
    {
        delete system;
    }
    DebugLines::systems.clear();

    GetLineSystem = reinterpret_cast<decltype(GetLineSystem)>(addr_GetSystem);
    ClearLineSystem = reinterpret_cast<decltype(ClearLineSystem)>(addr_Clear);
    AddLineToSystem = reinterpret_cast<decltype(AddLineToSystem)>(addr_AddLine);
    DebugLines::nullSystem = MemoryHelper::ReadMemory<DWORD>(addr_GetSystemEntry + 0x31);
    MemoryHelper::MakeJMP(addr_GetSystemEntry, reinterpret_cast<uintptr_t>(&GetLineSystemEntry_Hook));

    if (addr_ClearLines != 0)
    {
        ClearAllLineSystems = reinterpret_cast<decltype(ClearAllLineSystems)>(MemoryHelper::ResolveRelativeAddress(addr_ClearLines, 0x6));
        MemoryHelper::MakeCALL(addr_ClearLines + 0x5, reinterpret_cast<uintptr_t>(&ClearAllLineSystems_Hook));
    }

    if (addr_NodeStatus != 0 && addr_NodeTypeName != 0 && addr_NodeList != 0)
    {
        UpdateNodeDebugStatus = reinterpret_cast<decltype(UpdateNodeDebugStatus)>(addr_NodeStatus);
        DebugLines::nodeHideOffset = MemoryHelper::ReadMemory<DWORD>(addr_NodeStatus + 0x30);
        DebugLines::nodeDrawOffset = MemoryHelper::ReadMemory<DWORD>(addr_NodeStatus + 0x3A);
        DebugLines::nodeTypeCount = MemoryHelper::ReadMemory<uint8_t>(addr_NodeTypeName + 0x15);
        DebugLines::nodeTypeNames = reinterpret_cast<const char* const*>(MemoryHelper::ReadMemory<DWORD>(addr_NodeTypeName + 0x1B));
        DebugLines::nodeMgrAddr = MemoryHelper::ReadMemory<DWORD>(addr_NodeList + 0xA);
        GetNodeList = reinterpret_cast<decltype(GetNodeList)>(MemoryHelper::ResolveRelativeAddress(addr_NodeList, 0xF));
    }

    if (addr_NavMeshPoly != 0)
    {
        DebugLines::navMeshAddr = MemoryHelper::ReadMemory<DWORD>(addr_NavMeshPoly + 0xA);
        RedrawNavMeshPoly = reinterpret_cast<decltype(RedrawNavMeshPoly)>(MemoryHelper::ResolveRelativeAddress(addr_NavMeshPoly, 0xF));
    }

    if (addr_RegionPolys != 0)
    {
        DebugLines::regionPolyCountOffset = MemoryHelper::ReadMemory<uint8_t>(addr_RegionPolys + 0x2);
        DebugLines::regionPolyListOffset = MemoryHelper::ReadMemory<uint8_t>(addr_RegionPolys + 0xC);
    }

    DebugLines::serverModuleBase = (DWORD)g_State.GameServer;
    DebugLines::worldKey = 0;
    DebugLines::lastNodesValue.clear();
    DebugLines::ResetViews();
    DebugLines::isServerPatched = true;
}
