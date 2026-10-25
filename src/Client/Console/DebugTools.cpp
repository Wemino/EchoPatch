#pragma once

#include "../../Globals.cpp"
#include "../../Engine/Console/EngineInterfaces.cpp"
#include "../../Server/Console/DebugLines.cpp"
#include "ConsoleMgr.hpp"

// =======================
// ConsoleDebugTools
// =======================

// Debug commands of the game that the retail build left empty or never registered, rebuilt on what the game still has
namespace DebugTools
{
    using namespace EngineInterface;

    // Messages and cheats the retail server still handles
    constexpr uint8_t kMessageCheat = 107;
    constexpr uint8_t kMessageTeleport = 121;
    constexpr uint8_t kMessageAIDebug = 249;
    constexpr uint32_t kSpectatorClip = 3;

    // The expansion 2 added a cheat before these ones
    inline uint8_t GetCheatClip() { return g_State.CurrentFEARGame == FEARXP2 ? 24 : 23; }
    inline uint8_t GetCheatTeleport() { return g_State.CurrentFEARGame == FEARXP2 ? 25 : 24; }
    inline uint8_t GetCheatRemoveAI() { return g_State.CurrentFEARGame == FEARXP2 ? 27 : 26; }

    // Player camera: +0x0C camera object, +0x10 position, +0x1C local rotation, +0xB8 rotation of the transform
    static DWORD playerMgrAddr = 0;
    static uint8_t* screenShotModeFlag = nullptr;
    static DWORD hudMgr = 0;

    static bool isInstalled = false;
    static bool isScreenShotMode = false;
    static bool isNoclip = false;
    static DWORD noclipObject = 0;
    static int cameraPointCount = 0;

    // Perturb of the weapon, read by the calculations of the accuracy manager
    static DWORD accuracyMgr = 0;
    static float movementPerturb = 0.0f;
    static float firingPerturb = 0.0f;
    static float turningPerturb = 0.0f;
    static bool isPerturbDebug = false;

    // The game counts the frames it renders the world with the player camera, the debug lines are only drawn over those
    static DWORD renderCountAddr = 0;
    static uint32_t drawnRenderCount = 0;
    static DWORD cameraFovMember = 0;

    // Camera of the frame, projected like the engine does unless the check found a difference
    static DWORD viewCamera = 0;
    static Vector3 viewPosition;
    static Vector3 viewRight, viewUp, viewForward;
    static float viewScaleX = 1.0f, viewScaleY = 1.0f;
    static DWORD checkedCamera = 0;
    static bool useEngineProjection = false;

    inline float GetClientFloat(const char* name, float defaultValue = 0.0f)
    {
        return GetVariableFloat(GetClient(), name, defaultValue);
    }

    inline DWORD GetClientObject()
    {
        return CallMember<DWORD>(GetClient(), Client_GetClientObject);
    }

    inline bool RequireGame(const char* command)
    {
        if (!IsServerRunning() || !GetClientObject())
        {
            Console::Print(std::string(command) + " needs a game in progress", ConsoleLineType::Error);
            return false;
        }

        return true;
    }

    inline bool GetCamera(Vector3& position, Rotation& rotation)
    {
        DWORD playerMgr = playerMgrAddr ? *(DWORD*)playerMgrAddr : 0;
        DWORD camera = playerMgr ? *(DWORD*)(playerMgr + 0x28) : 0;
        if (!camera)
            return false;

        position = *(Vector3*)(camera + 0x10);
        rotation = *(Rotation*)(camera + 0xB8) * *(Rotation*)(camera + 0x1C);
        return true;
    }

    inline DWORD GetPlayerCameraObject()
    {
        DWORD playerMgr = playerMgrAddr ? *(DWORD*)playerMgrAddr : 0;
        DWORD camera = playerMgr ? *(DWORD*)(playerMgr + 0x28) : 0;
        return camera ? *(DWORD*)(camera + 0x0C) : 0;
    }

    // Positions are typed and shown in level coordinates when ApplyWorldOffset is on, like the game does
    inline Vector3 GetLevelOffset()
    {
        Vector3 offset;
        if (GetClientFloat("ApplyWorldOffset", 1.0f) != 0.0f)
        {
            Call<int>(GetClient(), Slot::GetSourceWorldOffset, &offset);
        }

        return offset;
    }

    inline bool SendCheat(uint8_t cheat, uint32_t data)
    {
        ServerMessage message(kMessageCheat);
        message.WriteUint8(cheat);
        message.WriteUint32(data);
        return message.Send();
    }

    inline std::string FormatAngles(const Rotation& rotation)
    {
        float pitch, yaw, roll;
        rotation.ToEuler(pitch, yaw, roll);

        char text[96];
        snprintf(text, sizeof(text), "%.1f %.1f %.1f", pitch * kRadiansToDegrees, yaw * kRadiansToDegrees, roll * kRadiansToDegrees);
        return text;
    }

    // AIDebug <command>, the server routes it to its AI systems
    static void __cdecl AIDebugProgram(int argc, char** argv)
    {
        if (argc < 1)
        {
            Console::PrintHeader("AIDebug <command>");
            Console::PrintRow("ListGoals", "List the goals of every AI", 34);
            Console::PrintRow("ListUnusedGoals", "List the goals no AI uses", 34);
            Console::PrintRow("ListUnusedActions", "List the actions no AI uses", 34);
            Console::PrintRow("AIStimulusMgr Stats", "Count the stimuli by type", 34);
            Console::PrintRow("AIStimulusMgr List", "List the stimuli", 34);
            Console::PrintRow("AIWorkingMemoryCentral List", "List the facts of the central memory", 34);
            return;
        }

        if (!RequireGame("AIDebug"))
            return;

        // Each argument is quoted so the server reads them back the same
        std::string command;
        for (int i = 0; i < argc; i++)
        {
            command += "\"" + std::string(argv[i]) + "\" ";
        }

        if (command.size() >= 256)
        {
            Console::Print("AIDebug: the command is too long", ConsoleLineType::Error);
            return;
        }

        ServerMessage message(kMessageAIDebug);
        message.WriteString(command.c_str());
        message.Send();
    }

    // Teleport <x> <y> <z>, the player keeps the direction of the camera
    static void __cdecl TeleportProgram(int argc, char** argv)
    {
        if (argc < 3)
        {
            Console::Print("Teleport <x> <y> <z>, in level coordinates", ConsoleLineType::Warning);
            return;
        }

        if (!RequireGame("Teleport"))
            return;

        Vector3 position = { (float)atof(argv[0]), (float)atof(argv[1]), (float)atof(argv[2]) };
        position = position - GetLevelOffset();

        Vector3 cameraPosition;
        Rotation rotation;
        GetCamera(cameraPosition, rotation);

        ServerMessage message(kMessageTeleport);
        message.WriteFloats(&position.x, 3);
        message.WriteFloats(&rotation.x, 4);
        message.Send();
    }

    // Noclip [0|1], flies through the world
    static void __cdecl NoclipProgram(int argc, char** argv)
    {
        if (!RequireGame("Noclip"))
            return;

        // A new world puts the player back on the ground
        if (noclipObject != GetClientObject())
        {
            isNoclip = false;
        }

        bool enable = argc > 0 ? atoi(argv[0]) != 0 : !isNoclip;
        if (SendCheat(GetCheatClip(), enable ? kSpectatorClip : 0))
        {
            isNoclip = enable;
            noclipObject = GetClientObject();
            Console::Print(enable ? "Noclip on" : "Noclip off");
        }
    }

    static void __cdecl TeleportToStartProgram(int, char**)
    {
        if (RequireGame("TeleportToStart") && SendCheat(GetCheatTeleport(), 1))
        {
            Console::Print("Back to the start of the level");
        }
    }

    static void __cdecl RemoveAIProgram(int, char**)
    {
        if (RequireGame("RemoveAI") && SendCheat(GetCheatRemoveAI(), 1))
        {
            Console::Print("Every AI of the level is removed");
        }
    }

    // CamPos [save [name]], prints the camera or adds it to CameraPoints.txt in the WorldEdit format
    static void __cdecl CamPosProgram(int argc, char** argv)
    {
        Vector3 position;
        Rotation rotation;
        if (!GetClientObject() || !GetCamera(position, rotation))
        {
            Console::Print("CamPos needs a game in progress", ConsoleLineType::Error);
            return;
        }

        position = position + GetLevelOffset();

        float pitch, yaw, roll;
        rotation.ToEuler(pitch, yaw, roll);

        char text[160];
        snprintf(text, sizeof(text), "%.0f %.0f %.0f", position.x, position.y, position.z);
        Console::PrintRow("Position", text, 11);
        Console::PrintRow("Rotation", FormatAngles(rotation) + " (pitch yaw roll)", 11);

        if (argc > 0 && _stricmp(argv[0], "save") == 0)
        {
            std::string name = argc > 1 ? argv[1] : "CameraPoint" + std::to_string(++cameraPointCount);
            FILE* file = nullptr;
            if (fopen_s(&file, "CameraPoints.txt", "a") != 0 || !file)
            {
                Console::Print("Unable to write CameraPoints.txt", ConsoleLineType::Error);
                return;
            }

            fprintf(file, "[%s]\nPos = <%.3f, %.3f, %.3f>\nRotation = <%.5f, %.5f, %.5f>\n\n", name.c_str(), position.x, position.y, position.z, pitch, yaw, roll);
            fclose(file);
            Console::Print("Saved " + name + " to CameraPoints.txt");
        }
    }

    // MakeCubicEnvMap [size] [file], renders the six sides around the camera
    static void __cdecl MakeCubicEnvMapProgram(int argc, char** argv)
    {
        Vector3 position;
        Rotation rotation;
        if (!GetClientObject() || !GetCamera(position, rotation))
        {
            Console::Print("MakeCubicEnvMap needs a game in progress", ConsoleLineType::Error);
            return;
        }

        uint32_t size = argc > 0 ? (uint32_t)atoi(argv[0]) : (uint32_t)(GetClientFloat("MakeCubicEnvMapSize", 256.0f) + 0.5f);
        std::string file = argc > 1 ? argv[1] : GetVariableString(GetClient(), "MakeCubicEnvMapName", "CubicEnvMap.dds");
        if (size < 16 || size > 4096 || (size & (size - 1)) != 0)
        {
            Console::Print("MakeCubicEnvMap: the size must be a power of two from 16 to 4096", ConsoleLineType::Error);
            return;
        }

        // The engine prints the result
        DWORD renderer = Call<DWORD>(GetClient(), Slot::GetRenderer);
        if (renderer)
        {
            CallOffset<int>(renderer, Renderer_MakeCubicEnvMap, &position, size, file.c_str());
        }
    }

    // ScreenShotMode hides the HUD, the game still checks the flag
    inline void UpdateScreenShotMode()
    {
        bool enable = GetClientFloat("ScreenShotMode") > 0.0f;
        if (enable == isScreenShotMode || !screenShotModeFlag)
            return;

        isScreenShotMode = enable;
        *screenShotModeFlag = enable ? 1 : 0;
        if (hudMgr && UpdateRenderLevel)
        {
            UpdateRenderLevel(hudMgr);
        }

        Console::Print(enable ? "Screen shot mode on, the HUD is hidden" : "Screen shot mode off");
    }

    // Perturb panel
    inline bool IsPerturbPanelActive()
    {
        return isPerturbDebug && accuracyMgr && !isScreenShotMode && GetClientObject();
    }

    inline void PerturbBar(const char* label, float value, float scale)
    {
        const float fontSize = ImGui::GetFontSize();
        ImGui::TextDisabled("%s", label);
        ImGui::SameLine(fontSize * 5.0f);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, g_accentColor);
        ImGui::ProgressBar(scale > 0.0f ? std::clamp(value / scale, 0.0f, 1.0f) : 0.0f, ImVec2(fontSize * 8.0f, fontSize), "");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::Text("%.2f", value);
    }

    inline void RenderPerturbPanel()
    {
        if (!BeginOverlayPanel("##PerturbPanel", 2, "Weapon perturb"))
            return;

        float current = *(float*)accuracyMgr;
        float scale = std::max({ 1.0f, current, movementPerturb, firingPerturb, turningPerturb });
        PerturbBar("Current", current, scale);
        PerturbBar("Move", movementPerturb, scale);
        PerturbBar("Fire", firingPerturb, scale);
        PerturbBar("Turn", turningPerturb, scale);

        EndOverlayPanel();
    }

    // Debug lines, projected with the camera the world was rendered with
    static bool ProjectPoint(const Vector3& point, float& x, float& y, float& depth)
    {
        Vector3 offset = point - viewPosition;
        depth = offset.Dot(viewForward);
        if (depth <= 0.001f)
        {
            // Behind the camera only the depth is used
            x = y = 0.0f;
            return true;
        }

        x = (offset.Dot(viewRight) / depth * viewScaleX + 1.0f) * 0.5f;
        y = 0.5f - offset.Dot(viewUp) / depth * viewScaleY * 0.5f;
        return true;
    }

    static bool ProjectPointEngine(const Vector3& point, float& x, float& y, float& depth)
    {
        DWORD renderer = Call<DWORD>(GetClient(), Slot::GetRenderer);
        float screen[3] = {};
        if (!renderer || CallOffset<int>(renderer, Renderer_WorldToScreen, viewCamera, &point, screen) != 0)
            return false;

        x = screen[0];
        y = screen[1];
        depth = screen[2];
        return true;
    }

    // The projection is compared to the one of the engine once per camera, the engine one is used if they differ
    inline void CheckProjection()
    {
        Vector3 point = viewPosition + viewForward * 500.0f + viewRight * 120.0f + viewUp * 60.0f;
        float x, y, depth, engineX, engineY, engineDepth;
        ProjectPoint(point, x, y, depth);
        if (ProjectPointEngine(point, engineX, engineY, engineDepth))
        {
            useEngineProjection = fabsf(x - engineX) > 0.002f || fabsf(y - engineY) > 0.002f || fabsf(depth - engineDepth) > depth * 0.01f;
        }
    }

    inline bool GetLineView(DebugLines::View& view)
    {
        DWORD client = GetClient();
        DWORD hCamera = GetPlayerCameraObject();
        if (!client || !hCamera || !cameraFovMember)
            return false;

        Rotation rotation;
        float fovX = 0.0f, fovY = 0.0f;
        Call<int>(client, Slot::GetObjectPos, hCamera, &viewPosition);
        Call<int>(client, Slot::GetObjectRotation, hCamera, &rotation);
        CallMember<int>(client, cameraFovMember, hCamera, &fovX, &fovY);
        if (fovX <= 0.0f || fovY <= 0.0f)
            return false;

        viewCamera = hCamera;
        viewRight = rotation.Right();
        viewUp = rotation.Up();
        viewForward = rotation.Forward();
        viewScaleX = 1.0f / tanf(fovX * 0.5f);
        viewScaleY = 1.0f / tanf(fovY * 0.5f);

        if (hCamera != checkedCamera)
        {
            CheckProjection();
            checkedCamera = hCamera;
        }

        view.position = viewPosition;
        view.range = std::max(GetClientFloat("MaxAIDebugRange", 2000.0f), 100.0f);
        view.project = useEngineProjection ? &ProjectPointEngine : &ProjectPoint;
        return true;
    }

    // Drawn when the world was rendered since the last frame, not over the menus
    inline bool IsLineViewActive()
    {
        return renderCountAddr && DebugLines::HasLines() && *(uint32_t*)renderCountAddr != drawnRenderCount && GetPlayerCameraObject();
    }

    inline void RenderLineView()
    {
        drawnRenderCount = *(uint32_t*)renderCountAddr;

        DebugLines::View view;
        if (GetLineView(view))
        {
            DebugLines::Render(view);
        }
    }

    // Programs are registered once the game registered its own
    inline void Install()
    {
        DWORD client = GetClient();
        if (!client || !Console::ProgramExists("Teleport"))
            return;

        isInstalled = true;

        Console::ReplaceProgram("AIDebug", reinterpret_cast<DWORD>(&AIDebugProgram));
        Console::ReplaceProgram("Teleport", reinterpret_cast<DWORD>(&TeleportProgram));

        struct Program { const char* name; void(__cdecl* function)(int, char**); };
        const Program programs[] =
        {
            { "Noclip", &NoclipProgram },
            { "TeleportToStart", &TeleportToStartProgram },
            { "RemoveAI", &RemoveAIProgram },
            { "CamPos", &CamPosProgram },
            { "MakeCubicEnvMap", &MakeCubicEnvMapProgram },
        };

        for (const auto& program : programs)
        {
            if (!Console::ProgramExists(program.name))
            {
                Call<int>(client, Slot::RegisterConsoleProgram, program.name, program.function);
            }
        }

        // The game reads it without making it
        CreateVariableFloat(client, "MaxAIDebugRange", 2000.0f);

        Console::RefreshEntries();
    }

    // Called once per frame from the main loop
    static void Update()
    {
        if (!isInstalled)
        {
            Install();
            if (!isInstalled)
                return;
        }

        UpdateScreenShotMode();
        isPerturbDebug = GetClientFloat("DebugPerturb") > 0.0f;

        DebugLines::Update();
    }
}

// The HUD manager calls it when the HUD should change, it checks the screenshot flag first
static void __fastcall UpdateRenderLevel_Hook(DWORD thisPtr, int)
{
    DebugTools::hudMgr = thisPtr;
    UpdateRenderLevel(thisPtr);
}

// Impacts are marked with the debug effect while DebugPerturb is on
static const char* __fastcall GetMarkFXName_Hook(DWORD thisPtr, int, DWORD hRecord, const char* attribute, uint32_t index, const char* defaultValue)
{
    const char* name = GetMarkFXName(thisPtr, hRecord, attribute, index, defaultValue);
    if (!name || !name[0] || !DebugTools::isPerturbDebug)
        return name;

    return "Impact_Debug";
}

static float __fastcall CalculateMovementPerturb_Hook(DWORD thisPtr, int)
{
    DebugTools::accuracyMgr = thisPtr;
    DebugTools::movementPerturb = CalculateMovementPerturb(thisPtr);
    return DebugTools::movementPerturb;
}

static float __fastcall CalculateFiringPerturb_Hook(DWORD thisPtr, int)
{
    DebugTools::firingPerturb = CalculateFiringPerturb(thisPtr);
    return DebugTools::firingPerturb;
}

static float __fastcall CalculateTurningPerturb_Hook(DWORD thisPtr, int)
{
    DebugTools::turningPerturb = CalculateTurningPerturb(thisPtr);
    return DebugTools::turningPerturb;
}
