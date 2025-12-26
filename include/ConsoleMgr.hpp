#pragma once

#include <d3d9.h>
#include <Windows.h>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include "../helper.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

// Types
struct ConsoleCommand
{
    std::string name;
    std::string value;
};

struct ConsoleVariable
{
    std::string name;
    std::string value;
};

struct ConsoleProgram
{
    std::string name;
};

enum class CvarType { String, Float };

struct DynamicCvar
{
    std::string value;
    int managerInstance;
    CvarType type;
};

enum class EditTargetType { None, Command, EngineVar, ConfigVar };

struct ConsoleAddresses
{
    DWORD cursorLockAddr;
    DWORD cvarListHead;
    DWORD cvarArrayStart;
    DWORD cvarArrayEnd;
    DWORD cmdArrayStart;
    DWORD cmdArrayEnd;
    DWORD cvarVtableFloat;
    DWORD cvarVtableInt;
};

// Function Pointers 
char(__cdecl* SetCvarString)(int, const char*, char*) = nullptr;
char(__cdecl* SetCvarFloat)(int, const char*, int) = nullptr;
void(__stdcall* RunConsoleCommand)(const char*) = nullptr;

// State
inline ConsoleAddresses g_addresses = {};
inline std::map<std::string, DynamicCvar> g_dynamicCvars;
inline std::set<std::string, std::less<>> g_consolePrograms;

inline DWORD g_devicePtrAddr = 0;
inline IDirect3DDevice9* g_pDevice = nullptr;
inline IDirect3DDevice9* g_pLastKnownDevice = nullptr;
inline HWND g_hWnd = nullptr;
inline std::string g_title = "Console";

inline bool g_imguiInitialized = false;
inline bool g_visible = false;
inline bool g_inReset = false;
inline bool g_focusInput = false;
inline bool g_justOpened = false;

inline bool g_isPlaying = false;
inline bool g_isMsgBoxVisible = false;
inline bool g_cursorShownByUs = false;

inline char g_inputBuffer[512] = {};
inline std::vector<std::string> g_outputLines;
inline bool g_scrollToBottom = false;

inline std::vector<std::string> g_commandHistory;
inline int g_historyPos = -1;

inline std::vector<ConsoleCommand> g_browserCommands;
inline std::vector<ConsoleVariable> g_browserStaticVars;
inline std::vector<ConsoleVariable> g_browserDynamicVars;
inline std::vector<ConsoleProgram> g_browserPrograms;
inline bool g_showBrowserWindow = false;
inline char g_browserFilter[128] = {};
inline int g_browserTab = 0;
inline bool g_browserNeedsReset = false;

inline bool g_showEditModal = false;
inline EditTargetType g_editTargetType = EditTargetType::None;
inline std::string g_editVarName;
inline char g_editValueBuffer[256] = {};

inline int g_lastWidth = 0;
inline int g_lastHeight = 0;

inline float g_uiScale = 1.0f;
inline float g_lastScaleHeight = 0.0f;
inline bool g_needsScaleRebuild = false;
inline float g_pendingScaleHeight = 0.0f;
inline bool g_highResScalingEnabled = true;

inline bool g_loggingEnabled = false;
inline FILE* g_logFile = nullptr;

// Helpers
inline bool ContainsCaseInsensitive(const std::string& haystack, const char* needle)
{
    if (!needle || !needle[0])
        return true;

    std::string lowerHaystack = haystack;
    std::string lowerNeedle = needle;

    for (auto& c : lowerHaystack) c = tolower(c);
    for (auto& c : lowerNeedle) c = tolower(c);

    return lowerHaystack.find(lowerNeedle) != std::string::npos;
}

inline auto FindCvarCaseInsensitive(const std::string& name) -> decltype(g_dynamicCvars.end())
{
    for (auto it = g_dynamicCvars.begin(); it != g_dynamicCvars.end(); it++)
    {
        if (_stricmp(it->first.c_str(), name.c_str()) == 0)
            return it;
    }

    return g_dynamicCvars.end();
}

inline std::string GenerateLogFilename(const std::string& title)
{
    std::string filename;
    for (char c : title)
    {
        if (c == ' ')
        {
            filename += '_';
        }
        else if (isalnum(c))
        {
            filename += tolower(c);
        }
    }

    char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "_%lld.txt", (long long)time(nullptr));

    return filename + timestamp;
}

inline IDirect3DDevice9* GetDevice()
{
    if (g_devicePtrAddr == 0)
        return nullptr;

    return *(IDirect3DDevice9**)g_devicePtrAddr;
}

// Cursor Management
inline bool ShouldShowCursorWhenClosed()
{
    return !g_isPlaying || g_isMsgBoxVisible;
}

inline void ShowConsoleCursor()
{
    int count = ShowCursor(TRUE);
    while (count > 1) count = ShowCursor(FALSE);
    while (count < 0) count = ShowCursor(TRUE);
    g_cursorShownByUs = true;
}

inline void HideConsoleCursor()
{
    if (g_cursorShownByUs)
    {
        int count = ShowCursor(FALSE);
        while (count >= 0) count = ShowCursor(FALSE);
        while (count < -1) count = ShowCursor(TRUE);
        g_cursorShownByUs = false;
    }
}

inline void OnConsoleOpened()
{
    ShowConsoleCursor();

    if (g_addresses.cursorLockAddr)
    {
        MemoryHelper::WriteMemory<DWORD>(g_addresses.cursorLockAddr, 0);
    }
}

inline void OnConsoleClosed()
{
    SetCursor(LoadCursor(nullptr, IDC_ARROW));

    if (ShouldShowCursorWhenClosed())
    {
        g_cursorShownByUs = false;
    }
    else
    {
        HideConsoleCursor();

        if (g_addresses.cursorLockAddr)
        {
            MemoryHelper::WriteMemory<DWORD>(g_addresses.cursorLockAddr, 1);
        }
    }
}

// UI Scaling
inline float CalculateUIScale(float displayHeight)
{
    if (!g_highResScalingEnabled)
        return 1.0f;
    if (displayHeight <= 1080.0f)
        return 1.0f;

    return displayHeight / 1080.0f;
}

// Console Variable Registration, links unregistered cvars from the array into the linked list
inline void RegisterMissingConsoleVars()
{
    if (g_addresses.cvarListHead == 0 || g_addresses.cvarArrayStart == 0 || g_addresses.cvarArrayEnd == 0)
        return;

    DWORD* pHead = (DWORD*)g_addresses.cvarListHead;

    std::set<std::string> registeredNames;
    DWORD current = *pHead;

    while (current)
    {
        const char* name = *(const char**)(current + 0x08);
        if (name && name[0])
        {
            registeredNames.insert(name);
        }

        current = *(DWORD*)(current + 0x10);
    }

    const DWORD cvarStructSize = 0x18;
    for (DWORD addr = g_addresses.cvarArrayStart; addr < g_addresses.cvarArrayEnd; addr += cvarStructSize)
    {
        const char* name = *(const char**)(addr + 0x08);
        if (!name || !name[0])
            continue;

        if (registeredNames.find(name) == registeredNames.end())
        {
            *(DWORD*)(addr + 0x10) = *pHead;
            *pHead = addr;
        }
    }
}

// Populate Functions
inline std::vector<ConsoleCommand> PopulateCommands()
{
    std::vector<ConsoleCommand> commands;
    if (g_addresses.cmdArrayStart == 0 || g_addresses.cmdArrayEnd == 0)
        return commands;

    const DWORD cmdStructSize = 0x18;
    for (DWORD addr = g_addresses.cmdArrayStart; addr < g_addresses.cmdArrayEnd; addr += cmdStructSize)
    {
        const char* name = *(const char**)(addr + 0x00);
        if (!name || !name[0])
            continue;

        ConsoleCommand cmd;
        cmd.name = name;

        DWORD ptrFloat = *(DWORD*)(addr + 0x04);
        DWORD ptrInt = *(DWORD*)(addr + 0x08);
        DWORD ptrStr = *(DWORD*)(addr + 0x10);

        if (ptrStr != 0)
        {
            const char* strVal = *(const char**)ptrStr;
            if (strVal)
            {
                cmd.value = std::string("\"") + strVal + "\"";
            }
        }
        else if (ptrFloat != 0)
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.2f", *(float*)ptrFloat);
            cmd.value = buf;
        }
        else if (ptrInt != 0)
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", *(int*)ptrInt);
            cmd.value = buf;
        }

        commands.push_back(cmd);
    }

    std::sort(commands.begin(), commands.end(), [](const ConsoleCommand& a, const ConsoleCommand& b) 
    {
        return _stricmp(a.name.c_str(), b.name.c_str()) < 0;
    });

    return commands;
}

inline std::vector<ConsoleVariable> PopulateStaticVariables()
{
    std::vector<ConsoleVariable> variables;
    if (g_addresses.cvarListHead == 0)
        return variables;

    DWORD current = *(DWORD*)g_addresses.cvarListHead;
    while (current)
    {
        const char* name = *(const char**)(current + 0x08);
        if (name && name[0])
        {
            ConsoleVariable var;
            var.name = name;

            DWORD vtable = *(DWORD*)(current + 0x00);
            char valueBuf[32];
            if (vtable == g_addresses.cvarVtableFloat)
                snprintf(valueBuf, sizeof(valueBuf), "%.2f", *(float*)(current + 0x14));
            else
                snprintf(valueBuf, sizeof(valueBuf), "%d", *(int*)(current + 0x14));

            var.value = valueBuf;
            variables.push_back(var);
        }

        current = *(DWORD*)(current + 0x10);
    }

    std::sort(variables.begin(), variables.end(), [](const ConsoleVariable& a, const ConsoleVariable& b) 
    {
        return _stricmp(a.name.c_str(), b.name.c_str()) < 0;
    });

    return variables;
}

inline std::vector<ConsoleVariable> PopulateDynamicVariables()
{
    std::vector<ConsoleVariable> variables;
    for (const auto& pair : g_dynamicCvars)
    {
        ConsoleVariable var;
        var.name = pair.first;
        var.value = pair.second.value;
        variables.push_back(var);
    }

    std::sort(variables.begin(), variables.end(), [](const ConsoleVariable& a, const ConsoleVariable& b) 
    {
        return _stricmp(a.name.c_str(), b.name.c_str()) < 0;
    });

    return variables;
}

inline std::vector<ConsoleProgram> PopulatePrograms()
{
    std::vector<ConsoleProgram> programs;
    for (const auto& name : g_consolePrograms)
    {
        ConsoleProgram prog;
        prog.name = name;
        programs.push_back(prog);
    }

    std::sort(programs.begin(), programs.end(), [](const ConsoleProgram& a, const ConsoleProgram& b) 
    {
        return _stricmp(a.name.c_str(), b.name.c_str()) < 0;
    });

    return programs;
}

// Edit Modal
inline void OpenEditModal(EditTargetType type, const std::string& name, const std::string& currentValue)
{
    g_editTargetType = type;
    g_editVarName = name;

    std::string cleanValue = currentValue;
    if (cleanValue.size() >= 2 && cleanValue.front() == '"' && cleanValue.back() == '"')
    {
        cleanValue = cleanValue.substr(1, cleanValue.size() - 2);
    }

    strncpy(g_editValueBuffer, cleanValue.c_str(), sizeof(g_editValueBuffer) - 1);
    g_editValueBuffer[sizeof(g_editValueBuffer) - 1] = '\0';
    g_showEditModal = true;
}

inline void ExecuteCommandInternal(const char* command);

inline void RenderEditModal()
{
    if (!g_showEditModal)
        return;

    ImGui::OpenPopup("Edit Value");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(350 * g_uiScale, 0), ImGuiCond_Always);

    if (ImGui::BeginPopupModal("Edit Value", &g_showEditModal, ImGuiWindowFlags_AlwaysAutoResize))
    {
        const char* typeStr = "";
        switch (g_editTargetType)
        {
            case EditTargetType::Command:   typeStr = "Command"; break;
            case EditTargetType::EngineVar: typeStr = "Engine Variable"; break;
            case EditTargetType::ConfigVar: typeStr = "Config Variable"; break;
            default: break;
        }

        ImGui::Text("%s:", typeStr);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.50f, 0.15f, 1.0f));
        ImGui::Text("%s", g_editVarName.c_str());
        ImGui::PopStyleColor();

        ImGui::Separator();
        ImGui::Text("New Value:");
        ImGui::PushItemWidth(-1);

        static bool focusInput = false;
        if (ImGui::IsWindowAppearing())
        {
            focusInput = true;
        }

        if (focusInput)
        {
            ImGui::SetKeyboardFocusHere();
            focusInput = false;
        }

        bool enterPressed = ImGui::InputText("##EditValue", g_editValueBuffer, sizeof(g_editValueBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::PopItemWidth();
        ImGui::Separator();

        bool apply = ImGui::Button("Apply", ImVec2(100 * g_uiScale, 0)) || enterPressed;
        ImGui::SameLine();
        bool cancel = ImGui::Button("Cancel", ImVec2(100 * g_uiScale, 0));

        if (apply && g_editValueBuffer[0] != '\0')
        {
            switch (g_editTargetType)
            {
                case EditTargetType::Command:
                case EditTargetType::EngineVar:
                    ExecuteCommandInternal((g_editVarName + " " + g_editValueBuffer).c_str());
                    break;
                case EditTargetType::ConfigVar:
                    ExecuteCommandInternal(("seta " + g_editVarName + " " + g_editValueBuffer).c_str());
                    break;
                default: break;
            }

            g_showEditModal = false;
            ImGui::CloseCurrentPopup();
        }

        if (cancel)
        {
            g_showEditModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// Command Handler
inline void HandleConsoleCommand(const char* command)
{
    if (!command || !command[0])
        return;

    if (_stricmp(command, "clear") == 0)
    {
        g_outputLines.clear();
        return;
    }

    if (_strnicmp(command, "seta ", 5) == 0)
    {
        const char* args = command + 5;
        while (*args && isspace(*args)) args++;

        std::string varName;
        while (*args && !isspace(*args))
        {
            varName += *args++;
        }

        while (*args && isspace(*args)) args++;
        std::string varValue = args;

        if (varName.empty() || varValue.empty())
        {
            g_outputLines.push_back("[ERROR] Usage: seta <variable> <value>");
            g_scrollToBottom = true;
            return;
        }

        auto it = FindCvarCaseInsensitive(varName);
        if (it == g_dynamicCvars.end())
        {
            g_outputLines.push_back("[ERROR] Unknown config variable: " + varName);
            g_scrollToBottom = true;
            return;
        }

        int managerInstance = it->second.managerInstance;
        std::string actualName = it->first;

        if (it->second.type == CvarType::Float)
        {
            if (SetCvarFloat == nullptr)
            {
                g_outputLines.push_back("[ERROR] CVar system not initialized");
                g_scrollToBottom = true;
                return;
            }

            float floatVal = (float)atof(varValue.c_str());
            int asInt;
            memcpy(&asInt, &floatVal, sizeof(float));
            SetCvarFloat(managerInstance, actualName.c_str(), asInt);

            char buf[32];
            snprintf(buf, sizeof(buf), "%.2f", floatVal);
            g_dynamicCvars[actualName] = { buf, managerInstance, CvarType::Float };
        }
        else
        {
            if (SetCvarString == nullptr)
            {
                g_outputLines.push_back("[ERROR] CVar system not initialized");
                g_scrollToBottom = true;
                return;
            }

            SetCvarString(managerInstance, actualName.c_str(), (char*)varValue.c_str());
            g_dynamicCvars[actualName] = { varValue, managerInstance, CvarType::String };
        }

        g_outputLines.push_back(actualName + " = " + varValue);
        g_scrollToBottom = true;
        return;
    }

    if (RunConsoleCommand)
    {
        RunConsoleCommand(command);
    }
}

// Theme
inline void ApplyConsoleStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 2.0f;
    style.FrameRounding = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 2.0f;

    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(6.0f, 4.0f);
    style.ScrollbarSize = 14.0f;

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;

    ImVec4* colors = style.Colors;

    ImVec4 bgDark = ImVec4(0.05f, 0.05f, 0.06f, 0.95f);
    ImVec4 bgMid = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    ImVec4 bgLight = ImVec4(0.15f, 0.15f, 0.16f, 1.00f);
    ImVec4 border = ImVec4(0.35f, 0.30f, 0.25f, 0.70f);

    ImVec4 accent = ImVec4(0.85f, 0.50f, 0.15f, 1.00f);
    ImVec4 accentHover = ImVec4(0.95f, 0.60f, 0.20f, 1.00f);
    ImVec4 accentDim = ImVec4(0.60f, 0.35f, 0.10f, 1.00f);

    ImVec4 textBright = ImVec4(0.95f, 0.92f, 0.88f, 1.00f);
    ImVec4 textDim = ImVec4(0.60f, 0.58f, 0.55f, 1.00f);

    colors[ImGuiCol_WindowBg] = bgDark;
    colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.06f, 0.06f, 0.07f, 0.98f);
    colors[ImGuiCol_Border] = border;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);

    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.11f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.06f, 0.06f, 0.07f, 0.80f);
    colors[ImGuiCol_MenuBarBg] = bgMid;

    colors[ImGuiCol_Text] = textBright;
    colors[ImGuiCol_TextDisabled] = textDim;

    colors[ImGuiCol_FrameBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = bgLight;
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.17f, 0.16f, 1.00f);

    colors[ImGuiCol_Button] = ImVec4(0.14f, 0.13f, 0.12f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.20f, 0.18f, 1.00f);
    colors[ImGuiCol_ButtonActive] = accent;

    colors[ImGuiCol_Header] = ImVec4(0.16f, 0.15f, 0.14f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.22f, 0.20f, 1.00f);
    colors[ImGuiCol_HeaderActive] = accent;

    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.03f, 0.03f, 0.03f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab] = accentDim;
    colors[ImGuiCol_ScrollbarGrabHovered] = accent;
    colors[ImGuiCol_ScrollbarGrabActive] = accentHover;

    colors[ImGuiCol_Separator] = ImVec4(0.40f, 0.35f, 0.28f, 0.60f);
    colors[ImGuiCol_SeparatorHovered] = accentHover;
    colors[ImGuiCol_SeparatorActive] = accent;

    colors[ImGuiCol_ResizeGrip] = accentDim;
    colors[ImGuiCol_ResizeGripHovered] = accent;
    colors[ImGuiCol_ResizeGripActive] = accentHover;

    colors[ImGuiCol_Tab] = bgMid;
    colors[ImGuiCol_TabHovered] = accentHover;
    colors[ImGuiCol_TabActive] = accent;
    colors[ImGuiCol_TabUnfocused] = bgMid;
    colors[ImGuiCol_TabUnfocusedActive] = accentDim;

    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = accentHover;
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.85f, 0.50f, 0.15f, 0.40f);
    colors[ImGuiCol_NavHighlight] = accent;
}

// ImGui Lifecycle
inline void InitImGui(IDirect3DDevice9* pDevice, HWND hWnd)
{
    if (g_imguiInitialized || !pDevice || !hWnd)
        return;

    // Get actual render resolution from backbuffer, not display mode
    float renderHeight = 1080.0f;
    IDirect3DSurface9* pBackBuffer = nullptr;
    if (SUCCEEDED(pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer)))
    {
        D3DSURFACE_DESC desc;
        if (SUCCEEDED(pBackBuffer->GetDesc(&desc)))
        {
            renderHeight = (float)desc.Height;
        }

        pBackBuffer->Release();
    }

    g_uiScale = CalculateUIScale(renderHeight);
    g_lastScaleHeight = renderHeight;

    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    float fontSize = 13.0f * g_uiScale;
    ImFontConfig fontConfig;
    fontConfig.SizePixels = fontSize;
    io.Fonts->AddFontDefault(&fontConfig);

    ApplyConsoleStyle();
    ImGui::GetStyle().ScaleAllSizes(g_uiScale);

    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX9_Init(pDevice);

    g_imguiInitialized = true;
    g_pDevice = pDevice;
}

inline void RebuildImGuiForScale()
{
    if (!g_imguiInitialized || g_pendingScaleHeight <= 0)
        return;

    g_uiScale = CalculateUIScale(g_pendingScaleHeight);
    g_lastScaleHeight = g_pendingScaleHeight;
    g_needsScaleRebuild = false;
    g_pendingScaleHeight = 0.0f;

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    float fontSize = 13.0f * g_uiScale;
    ImFontConfig fontConfig;
    fontConfig.SizePixels = fontSize;
    io.Fonts->AddFontDefault(&fontConfig);

    ImGui_ImplDX9_InvalidateDeviceObjects();
    ImGui_ImplDX9_CreateDeviceObjects();

    ImGui::GetStyle() = ImGuiStyle();
    ApplyConsoleStyle();
    ImGui::GetStyle().ScaleAllSizes(g_uiScale);
}

inline void RequestScaleRebuild(float newHeight)
{
    if (fabs(newHeight - g_lastScaleHeight) < 50.0f)
        return;

    g_needsScaleRebuild = true;
    g_pendingScaleHeight = newHeight;
}

inline void ShutdownImGui()
{
    if (g_imguiInitialized)
    {
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        g_imguiInitialized = false;
        g_pDevice = nullptr;
    }
}

inline void CheckDevice()
{
    IDirect3DDevice9* pDevice = GetDevice();
    if (!pDevice)
        return;

    if (pDevice != g_pLastKnownDevice)
    {
        ShutdownImGui();
        InitImGui(pDevice, g_hWnd);
        g_pLastKnownDevice = pDevice;
    }
}

// Input
inline void UpdateMouseInput()
{
    if (!g_imguiInitialized)
        return;

    ImGuiIO& io = ImGui::GetIO();
    POINT pt;
    if (GetCursorPos(&pt) && ScreenToClient(g_hWnd, &pt))
    {
        io.MousePos = ImVec2((float)pt.x, (float)pt.y);
    }

    io.MouseDown[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    io.MouseDown[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    io.MouseDown[2] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
}

inline int InputCallbackStub(ImGuiInputTextCallbackData* data)
{
    if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
    {
        if (g_commandHistory.empty())
            return 0;

        if (data->EventKey == ImGuiKey_UpArrow)
        {
            if (g_historyPos == -1)
                g_historyPos = (int)g_commandHistory.size() - 1;
            else if (g_historyPos > 0)
                g_historyPos--;
        }
        else if (data->EventKey == ImGuiKey_DownArrow)
        {
            if (g_historyPos != -1)
            {
                g_historyPos++;
                if (g_historyPos >= (int)g_commandHistory.size())
                    g_historyPos = -1;
            }
        }

        if (g_historyPos >= 0 && g_historyPos < (int)g_commandHistory.size())
        {
            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, g_commandHistory[g_historyPos].c_str());
        }
        else
        {
            data->DeleteChars(0, data->BufTextLen);
        }
    }

    return 0;
}

inline void ExecuteCommandInternal(const char* command)
{
    g_outputLines.push_back(std::string("> ") + command);
    g_scrollToBottom = true;

    g_commandHistory.push_back(command);
    if (g_commandHistory.size() > 50)
    {
        g_commandHistory.erase(g_commandHistory.begin());
    }

    g_historyPos = -1;

    HandleConsoleCommand(command);
}

// Rendering
inline float GetScaledMargin(float baseMargin, float currentSize, float referenceSize = 1080.0f)
{
    return baseMargin * (currentSize / referenceSize);
}

inline void RenderBrowserWindow()
{
    ImGuiIO& io = ImGui::GetIO();
    float width = io.DisplaySize.x;
    float height = io.DisplaySize.y;

    ImGuiCond posCond = g_browserNeedsReset ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
    ImGui::SetNextWindowPos(ImVec2(width * 0.15f, height * 0.1f), posCond);
    ImGui::SetNextWindowSize(ImVec2(556 * g_uiScale, 400 * g_uiScale), posCond);

    if (g_browserNeedsReset)
    {
        g_browserNeedsReset = false;
    }

    if (ImGui::Begin("Console Browser", &g_showBrowserWindow))
    {
        ImGui::Text("Filter:");
        ImGui::SameLine();
        ImGui::PushItemWidth(150 * g_uiScale);
        ImGui::InputText("##BrowserFilter", g_browserFilter, sizeof(g_browserFilter));
        ImGui::PopItemWidth();

        ImGui::SameLine();
        if (ImGui::Button("Commands"))
        {
            g_browserTab = 0;
            g_browserCommands = PopulateCommands();
        }
        ImGui::SameLine();
        if (ImGui::Button("Engine Vars"))
        {
            g_browserTab = 1;
            g_browserStaticVars = PopulateStaticVariables();
        }
        ImGui::SameLine();
        if (ImGui::Button("Config Vars"))
        {
            g_browserTab = 2;
            g_browserDynamicVars = PopulateDynamicVariables();
        }
        ImGui::SameLine();
        if (ImGui::Button("Programs"))
        {
            g_browserTab = 3;
            g_browserPrograms = PopulatePrograms();
        }

        ImGui::Separator();
        ImGui::BeginChild("BrowserList", ImVec2(0, 0), true);

        float contentWidth = ImGui::GetWindowContentRegionMax().x;

        if (g_browserTab == 0)
        {
            if (g_browserCommands.empty())
            {
                ImGui::TextDisabled("Click Commands to load.");
            }
            else
            {
                for (const auto& cmd : g_browserCommands)
                {
                    if (!ContainsCaseInsensitive(cmd.name, g_browserFilter))
                        continue;

                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.75f, 0.80f, 1.0f));
                    ImGui::Selectable(cmd.name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick);
                    ImGui::PopStyleColor();

                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                    {
                        OpenEditModal(EditTargetType::Command, cmd.name, cmd.value);
                    }

                    if (!cmd.value.empty())
                    {
                        std::string valueText = "= " + cmd.value;
                        float textWidth = ImGui::CalcTextSize(valueText.c_str()).x;
                        ImGui::SameLine(contentWidth - textWidth);
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.82f, 0.78f, 1.0f));
                        ImGui::Text("%s", valueText.c_str());
                        ImGui::PopStyleColor();
                    }
                }
            }
        }

        if (g_browserTab == 1)
        {
            if (g_browserStaticVars.empty())
            {
                ImGui::TextDisabled("Click Engine Vars to load.");
            }
            else
            {
                for (const auto& var : g_browserStaticVars)
                {
                    if (!ContainsCaseInsensitive(var.name, g_browserFilter))
                        continue;

                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.78f, 0.50f, 1.0f));
                    ImGui::Selectable(var.name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick);
                    ImGui::PopStyleColor();

                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                    {
                        OpenEditModal(EditTargetType::EngineVar, var.name, var.value);
                    }

                    std::string valueText = "= " + var.value;
                    float textWidth = ImGui::CalcTextSize(valueText.c_str()).x;
                    ImGui::SameLine(contentWidth - textWidth);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.82f, 0.78f, 1.0f));
                    ImGui::Text("%s", valueText.c_str());
                    ImGui::PopStyleColor();
                }
            }
        }

        if (g_browserTab == 2)
        {
            if (g_browserDynamicVars.empty())
            {
                ImGui::TextDisabled("Click Config Vars to load.");
            }
            else
            {
                for (const auto& var : g_browserDynamicVars)
                {
                    if (!ContainsCaseInsensitive(var.name, g_browserFilter))
                        continue;

                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.60f, 0.90f, 1.0f));
                    ImGui::Selectable(var.name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick);
                    ImGui::PopStyleColor();

                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                    {
                        OpenEditModal(EditTargetType::ConfigVar, var.name, var.value);
                    }

                    std::string valueText = "= " + var.value;
                    float textWidth = ImGui::CalcTextSize(valueText.c_str()).x;
                    ImGui::SameLine(contentWidth - textWidth);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.82f, 0.78f, 1.0f));
                    ImGui::Text("%s", valueText.c_str());
                    ImGui::PopStyleColor();
                }
            }
        }

        if (g_browserTab == 3)
        {
            if (g_browserPrograms.empty())
            {
                ImGui::TextDisabled("Click Programs to load.");
            }
            else
            {
                for (const auto& prog : g_browserPrograms)
                {
                    if (!ContainsCaseInsensitive(prog.name, g_browserFilter))
                        continue;

                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.70f, 0.40f, 1.0f));
                    ImGui::Text("%s", prog.name.c_str());
                    ImGui::PopStyleColor();
                }
            }
        }

        ImGui::EndChild();
        RenderEditModal();
    }

    ImGui::End();
}

inline void RenderConsoleWindow()
{
    ImGuiIO& io = ImGui::GetIO();
    float width = io.DisplaySize.x;
    float height = io.DisplaySize.y;

    bool resolutionChanged = ((int)width != g_lastWidth || (int)height != g_lastHeight) && (g_lastWidth != 0 || g_lastHeight != 0);
    g_lastWidth = (int)width;
    g_lastHeight = (int)height;

    if (resolutionChanged)
    {
        RequestScaleRebuild(height);
        g_browserNeedsReset = true;
    }

    float marginX = GetScaledMargin(10.0f, height);
    float marginY = GetScaledMargin(10.0f, height);

    ImGuiCond posCondition = resolutionChanged ? ImGuiCond_Always : ImGuiCond_FirstUseEver;

    float textAreaHeight = height * 0.5f;
    float headerHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
    float footerHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
    float consoleHeight = headerHeight + textAreaHeight + footerHeight + (ImGui::GetStyle().WindowPadding.y * 2) + 20.0f;

    ImGui::SetNextWindowPos(ImVec2(marginX, marginY), posCondition);
    ImGui::SetNextWindowSize(ImVec2(width - (marginX * 2), consoleHeight), posCondition);
    ImGui::SetNextWindowSizeConstraints(ImVec2(400 * g_uiScale, 200 * g_uiScale), ImVec2(width - (marginX * 2), height * 0.95f));

    if (g_justOpened)
    {
        ImGui::SetNextWindowFocus();
    }

    bool windowOpen = true;
    if (ImGui::Begin(g_title.c_str(), &windowOpen, ImGuiWindowFlags_None))
    {
        if (ImGui::Button("Browse"))
        {
            g_showBrowserWindow = !g_showBrowserWindow;
            if (g_showBrowserWindow)
            {
                switch (g_browserTab)
                {
                    case 0: g_browserCommands = PopulateCommands(); break;
                    case 1: g_browserStaticVars = PopulateStaticVariables(); break;
                    case 2: g_browserDynamicVars = PopulateDynamicVariables(); break;
                    case 3: g_browserPrograms = PopulatePrograms(); break;
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear"))
            g_outputLines.clear();

        ImGui::Separator();

        float inputFooterHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
        ImGui::BeginChild("OutputRegion", ImVec2(0, -inputFooterHeight), true, ImGuiWindowFlags_HorizontalScrollbar);

        for (const auto& line : g_outputLines)
        {
            if (line.find("[ERROR]") != std::string::npos)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
                ImGui::TextUnformatted(line.c_str());
                ImGui::PopStyleColor();
            }
            else if (line.rfind(">", 0) == 0)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.45f, 0.15f, 1.0f));
                ImGui::TextUnformatted(line.c_str());
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::TextUnformatted(line.c_str());
            }
        }

        if (g_scrollToBottom)
        {
            ImGui::SetScrollHereY(1.0f);
            g_scrollToBottom = false;
        }

        ImGui::EndChild();
        ImGui::Separator();
        ImGui::PushItemWidth(-1);

        if (g_focusInput)
        {
            ImGui::SetKeyboardFocusHere();
            g_focusInput = false;
        }

        if (ImGui::InputText("##Input", g_inputBuffer, sizeof(g_inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory, InputCallbackStub, nullptr))
        {
            if (g_inputBuffer[0] != '\0')
            {
                ExecuteCommandInternal(g_inputBuffer);
                g_inputBuffer[0] = '\0';
            }
            g_focusInput = true;
        }

        ImGui::PopItemWidth();

        if (g_justOpened)
        {
            g_focusInput = true;
            g_justOpened = false;
        }
    }

    ImGui::End();

    if (!windowOpen)
    {
        g_visible = false;
        g_showBrowserWindow = false;
        OnConsoleClosed();
    }
}

inline void Render(IDirect3DDevice9* pDevice)
{
    if (g_needsScaleRebuild)
    {
        RebuildImGuiForScale();
    }

    ShowConsoleCursor();
    UpdateMouseInput();

    if (g_addresses.cursorLockAddr)
    {
        MemoryHelper::WriteMemory<DWORD>(g_addresses.cursorLockAddr, 0);
    }

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    RenderConsoleWindow();
    if (g_showBrowserWindow)
    {
        RenderBrowserWindow();
    }

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

// Public API
namespace Console
{
    inline void Init(DWORD devicePtrAddr, HWND hWnd, const char* title, bool enableHighResScaling = true, bool enableLogging = false)
    {
        g_devicePtrAddr = devicePtrAddr;
        g_hWnd = hWnd;
        g_title = std::string(title) + " Console";
        g_highResScalingEnabled = enableHighResScaling;
        g_loggingEnabled = enableLogging;

        if (g_loggingEnabled)
        {
            std::string filename = GenerateLogFilename(g_title);
            g_logFile = fopen(filename.c_str(), "w");
        }
    }

    inline void InitAddresses(const ConsoleAddresses& addresses)
    {
        g_addresses = addresses;
        RegisterMissingConsoleVars();
    }

    inline void Shutdown()
    {
        if (g_cursorShownByUs)
        {
            HideConsoleCursor();
        }

        ShutdownImGui();

        if (g_logFile)
        {
            fclose(g_logFile);
            g_logFile = nullptr;
        }

        g_devicePtrAddr = 0;
        g_hWnd = nullptr;
        g_pDevice = nullptr;
        g_pLastKnownDevice = nullptr;
        g_visible = false;
        g_inReset = false;
        g_outputLines.clear();
        g_commandHistory.clear();
        g_browserCommands.clear();
        g_browserStaticVars.clear();
        g_browserDynamicVars.clear();
        g_browserPrograms.clear();
        g_inputBuffer[0] = '\0';
    }

    inline bool Update(bool isPlaying, bool isMsgBoxVisible)
    {
        g_isPlaying = isPlaying;
        g_isMsgBoxVisible = isMsgBoxVisible;

        if (GetAsyncKeyState(VK_HOME) & 1)
        {
            if (!g_visible)
            {
                g_visible = true;
                g_justOpened = true;
                g_focusInput = true;
                OnConsoleOpened();
            }
            else
            {
                g_visible = false;
                g_showBrowserWindow = false;
                OnConsoleClosed();
            }
        }

        CheckDevice();
        return g_visible;
    }

    inline void OnWindowReactivated()
    {
        if (g_visible)
        {
            ShowCursor(TRUE);
            g_cursorShownByUs = true;
        }
    }

    inline void OnEndScene()
    {
        if (g_visible && g_imguiInitialized && !g_inReset)
        {
            IDirect3DDevice9* pDevice = GetDevice();
            if (pDevice)
            {
                Render(pDevice);
            }
        }
    }

    inline void OnBeforeReset()
    {
        g_inReset = true;
        if (g_imguiInitialized)
        {
            ImGui_ImplDX9_InvalidateDeviceObjects();
        }
    }

    inline void OnAfterReset(bool success)
    {
        if (success && g_imguiInitialized)
        {
            ImGui_ImplDX9_CreateDeviceObjects();
        }

        g_inReset = false;
    }

    inline void AddOutput(const char* format, ...)
    {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        g_outputLines.push_back(buffer);
        if (g_outputLines.size() > 500)
        {
            g_outputLines.erase(g_outputLines.begin());
        }

        g_scrollToBottom = true;

        if (g_loggingEnabled && g_logFile)
        {
            time_t now = time(nullptr);
            struct tm* t = localtime(&now);
            fprintf(g_logFile, "[%02d:%02d:%02d] %s\n", t->tm_hour, t->tm_min, t->tm_sec, buffer);
            fflush(g_logFile);
        }
    }

    inline void ClearOutput() { g_outputLines.clear(); }
    inline bool IsInitialized() { return g_imguiInitialized; }
}