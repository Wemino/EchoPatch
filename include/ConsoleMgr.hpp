#pragma once

#include <d3d9.h>
#include <Windows.h>
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <string>
#include <set>
#include <vector>
#include <deque>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include "../src/helper.cpp"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Types
enum class ConsoleEntryType { EngineCommand, Program, ServerCommand, EngineVar, RenderVar, ConfigVar, ServerVar };

struct ConsoleEntry
{
    std::string name;
    std::string value;
    std::string defaultValue;
    ConsoleEntryType type = ConsoleEntryType::ConfigVar;
};

enum class ConsoleLineType { Output, Command, Info, Warning, Error, Header };

struct ConsoleLine
{
    std::string text;
    ImU32 color = 0;
    ConsoleLineType type = ConsoleLineType::Output;
    uint64_t id = 0;
    uint32_t time = 0;

    // Help lines are drawn in two columns, the second one starting at this offset
    uint16_t split = 0;
    bool dimFirst = false;

    // Height of the line when word wrap is on, measured again when the width or the font size changes
    float wrapWidth = -1.0f;
    float wrapFontSize = 0.0f;
    float wrapHeight = 0.0f;
};

struct PendingCommand
{
    std::string text;
    bool server = false;
};

enum class NavState { Normal, History, Complete };
enum class CompletionRequest { None, Next, Previous, List };

enum BrowserTab { BrowserTab_Commands, BrowserTab_EngineVars, BrowserTab_ConfigVars, BrowserTab_ServerVars, BrowserTab_Count };

// Severity filters of the output
enum OutputFilter { OutputFilter_Log = 1 << 0, OutputFilter_Warnings = 1 << 1, OutputFilter_Errors = 1 << 2, OutputFilter_All = 7 };

// Where a typed name is executed
enum class NameTarget { Unknown, ClientCommand, ClientVar, EngineVar, ServerCommand, ServerVar };

struct ConsoleAddresses
{
    DWORD cursorLockAddr;
    DWORD cvarListHead;
    DWORD cvarArrayStart;
    DWORD cvarArrayEnd;
    DWORD cvarVtableFloat;
    DWORD cvarVtableInt;
    DWORD clientManager;
    DWORD serverManager;
    DWORD engineVarTable;
    int engineVarCount;
};

struct ConsoleSettings
{
    bool highResScaling = true;
    bool logging = false;
    int toggleKey = VK_HOME;
    int bufferLen = 500;
    int historyLen = 20;
    float alpha = 1.0f;
    std::string position = "Top";
};

// Engine variables of the console, read every frame like the original console did
struct ConsoleEngineVars
{
    float* alpha = nullptr;
    int* bufferLen = nullptr;
    int* historyLen = nullptr;
    float* rect[4] = {};
};

// Text field that had the keyboard during the last frame
struct TextFieldState
{
    bool active = false;
    int cursor = 0;
    int length = 0;
    bool selection = false;
};

// Function Pointers
char(__cdecl* ExecuteConsoleCommand)(DWORD, const char*) = nullptr;

// State
inline ConsoleAddresses g_addresses = {};
inline ConsoleSettings g_settings;
inline ConsoleEngineVars g_engineVars;
inline bool g_engineReady = false;

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
inline bool g_inputActive = false;
inline bool g_moveCursorToEnd = false;
inline bool g_consoleHasKeyboard = false;
inline ULONGLONG g_openTime = 0;

// Called from the main loop when the console opens or closes, returns false to be called again later
inline bool (*g_stateCallback)(bool open) = nullptr;
inline bool g_notifiedOpen = false;

inline bool g_cursorShownByUs = false;
inline bool g_mouseMessagesSeen = false;

inline UINT g_toggleKey = VK_HOME;
inline bool g_closeOnEscapeRelease = false;
inline bool g_consumedKeys[256] = {};

inline TextFieldState g_textField;
inline TextFieldState g_textFieldFrame;

inline char g_inputBuffer[512] = {};
inline std::deque<ConsoleLine> g_outputLines;
inline SRWLOCK g_outputLock = SRWLOCK_INIT;
inline uint64_t g_nextLineId = 1;
inline uint64_t g_outputGeneration = 0;
inline char g_outputFilter[128] = {};
inline int g_outputMask = OutputFilter_All;
inline bool g_showTimestamps = false;
inline bool g_wordWrap = false;
inline bool g_scrollToBottom = false;
inline int g_scrollLines = 0;
inline int g_scrollPages = 0;
inline int g_scrollEdge = 0;

// Lines shown with the current filters, rebuilt when the output or the filters change
inline std::vector<int> g_visibleRows;
inline std::vector<float> g_wrapOffsets;
inline uint64_t g_visibleGeneration = UINT64_MAX;
inline std::string g_visibleFilter;
inline int g_visibleMask = -1;
inline int g_countLog = 0;
inline int g_countWarnings = 0;
inline int g_countErrors = 0;

// Selected output lines, a range of line ids so it stays valid when old lines are dropped
inline uint64_t g_selectionAnchor = 0;
inline uint64_t g_selectionActive = 0;
inline bool g_selectionDragging = false;
inline bool g_copySelectionRequest = false;

inline std::vector<PendingCommand> g_pendingCommands;

inline std::deque<std::string> g_commandHistory;
inline std::string g_historyFilename;

inline NavState g_navState = NavState::Normal;
inline std::string g_navCommand;
inline std::string g_navAppliedText;
inline int g_historyPos = -1;

inline CompletionRequest g_completionRequest = CompletionRequest::None;
inline std::string g_completionBase;
inline std::string g_completionPrefix;
inline std::vector<ConsoleEntry> g_completionMatches;
inline int g_completionIndex = -1;

inline std::vector<ConsoleEntry> g_candidates;
inline bool g_candidatesDirty = true;
inline ULONGLONG g_candidatesTime = 0;
inline std::vector<ConsoleEntry> g_suggestions;
inline std::string g_suggestionPrefix;

inline std::vector<ConsoleEntry> g_browserEntries[BrowserTab_Count];
inline bool g_showBrowserWindow = false;
inline bool g_browserFocused = false;
inline char g_browserFilter[128] = {};
inline bool g_browserNeedsReset = false;
inline bool g_browserNeedsRefresh = true;
inline ULONGLONG g_browserRefreshTime = 0;

inline bool g_showEditModal = false;
inline ConsoleEntry g_editEntry;
inline char g_editValueBuffer[256] = {};
inline bool g_editModalFocusInput = false;

// Values of the engine variables before the config files run, keyed by lowercase name
inline std::unordered_map<std::string, std::string> g_engineVarDefaults;

inline int g_lastWidth = 0;
inline int g_lastHeight = 0;
inline float g_appliedRect[4] = { -1.0f, -1.0f, -1.0f, -1.0f };

inline float g_uiScale = 1.0f;
inline float g_lastScaleHeight = 0.0f;
inline bool g_needsScaleRebuild = false;
inline float g_pendingScaleHeight = 0.0f;
inline bool g_highResScalingEnabled = true;
inline ImFont* g_fontBitmap = nullptr;
inline ImFont* g_fontVector = nullptr;

inline bool g_loggingEnabled = false;
inline FILE* g_logFile = nullptr;

// Output lock, an SRWLOCK rather than std::mutex which depends on the msvcp140.dll loaded in the game process
struct OutputLock
{
    OutputLock() { AcquireSRWLockExclusive(&g_outputLock); }
    ~OutputLock() { ReleaseSRWLockExclusive(&g_outputLock); }
    OutputLock(const OutputLock&) = delete;
    OutputLock& operator=(const OutputLock&) = delete;
};

// Colors
inline const ImVec4 g_accentColor = ImVec4(0.85f, 0.50f, 0.15f, 1.0f);
inline const ImVec4 g_commandLineColor = ImVec4(0.80f, 0.45f, 0.15f, 1.0f);
inline const ImVec4 g_infoLineColor = ImVec4(0.70f, 0.72f, 0.95f, 1.0f);
inline const ImVec4 g_warningLineColor = ImVec4(1.0f, 0.78f, 0.30f, 1.0f);
inline const ImVec4 g_errorLineColor = ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
inline const ImVec4 g_valueColor = ImVec4(0.85f, 0.82f, 0.78f, 1.0f);
inline const ImVec4 g_timestampColor = ImVec4(0.47f, 0.45f, 0.42f, 1.0f);
inline const ImVec4 g_logDotColor = ImVec4(0.62f, 0.60f, 0.57f, 1.0f);
inline const ImVec4 g_serverOnColor = ImVec4(0.42f, 0.80f, 0.45f, 1.0f);
inline const ImVec4 g_helpTextColor = ImVec4(0.64f, 0.66f, 0.76f, 1.0f);

// Helpers
inline bool ContainsCaseInsensitive(const std::string& haystack, const char* needle)
{
    if (!needle || !needle[0])
        return true;

    auto it =
    std::search(haystack.begin(), haystack.end(), needle, needle + strlen(needle),
    [](char a, char b)
    {
        return tolower((unsigned char)a) == tolower((unsigned char)b);
    });

    return it != haystack.end();
}

inline bool StartsWithCaseInsensitive(const std::string& text, const std::string& prefix)
{
    return text.size() >= prefix.size() && _strnicmp(text.c_str(), prefix.c_str(), prefix.size()) == 0;
}

inline std::string ToLower(std::string text)
{
    for (auto& c : text) c = (char)tolower((unsigned char)c);
    return text;
}

inline std::string FormatFloat(float value)
{
    if (value == 0.0f)
        return std::signbit(value) ? "-0.0" : "0.0";

    char full[64];
    snprintf(full, sizeof(full), "%.15f", (double)value);

    const char* dot = strchr(full, '.');
    if (!dot)
        return std::string(full) + ".0";

    int dotIdx = (int)(dot - full);
    int fullLen = (int)strlen(full);
    char candidate[64];

    for (int decimals = 1; decimals <= 6; decimals++)
    {
        int truncLen = dotIdx + 1 + decimals;
        if (truncLen <= fullLen)
        {
            memcpy(candidate, full, truncLen);
            candidate[truncLen] = '\0';
            if (strtof(candidate, nullptr) == value)
                return candidate;
        }

        snprintf(candidate, sizeof(candidate), "%.*f", decimals, (double)value);
        if (strtof(candidate, nullptr) == value)
            return candidate;
    }

    snprintf(candidate, sizeof(candidate), "%.6f", (double)value);
    std::string str(candidate);
    size_t dotPos = str.find('.');
    if (dotPos != std::string::npos)
    {
        size_t lastNonZero = str.find_last_not_of('0');
        if (lastNonZero != std::string::npos && lastNonZero > dotPos)
        {
            str.erase(lastNonZero + 1);
        }
        else
        {
            str.erase(dotPos + 2);
        }
    }
    return str;
}

inline std::string GenerateFilename(const std::string& title, const char* suffix)
{
    std::string filename;
    for (char c : title)
    {
        if (c == ' ')
        {
            filename += '_';
        }
        else if (isalnum((unsigned char)c))
        {
            filename += (char)tolower((unsigned char)c);
        }
    }

    return filename + suffix;
}

inline std::string GenerateLogFilename(const std::string& title)
{
    char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "_%lld.txt", (long long)time(nullptr));

    return GenerateFilename(title, timestamp);
}

inline std::string TrimString(const std::string& text)
{
    size_t start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";

    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

// The engine only keeps the first argument of "name value", quoting preserves the spaces
inline std::string QuoteValue(std::string value)
{
    value.erase(std::remove(value.begin(), value.end(), '"'), value.end());
    return "\"" + value + "\"";
}

inline std::string StripQuotes(const std::string& value)
{
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
        return value.substr(1, value.size() - 2);

    return value;
}

// Splits a line on ';' outside of quotes, the same separator the engine parser uses
inline std::vector<std::string> SplitCommandLine(const char* line)
{
    std::vector<std::string> commands;
    std::string current;
    bool inQuotes = false;

    for (const char* p = line; *p; p++)
    {
        if (*p == '"')
        {
            inQuotes = !inQuotes;
        }
        else if (*p == ';' && !inQuotes)
        {
            commands.push_back(current);
            current.clear();
            continue;
        }

        current += *p;
    }

    commands.push_back(current);
    return commands;
}

inline std::vector<std::string> TokenizeCommand(const std::string& command)
{
    std::vector<std::string> tokens;
    size_t i = 0;

    while (i < command.size())
    {
        while (i < command.size() && isspace((unsigned char)command[i])) i++;
        if (i >= command.size())
            break;

        std::string token;
        if (command[i] == '"')
        {
            size_t end = command.find('"', i + 1);
            if (end == std::string::npos)
                end = command.size();

            token = command.substr(i + 1, end - i - 1);
            i = end + 1;
        }
        else
        {
            while (i < command.size() && !isspace((unsigned char)command[i])) token += command[i++];
        }

        tokens.push_back(token);
    }

    return tokens;
}

// Text encoding: the engine uses the ANSI code page, ImGui UTF-8
inline bool IsAscii(const char* text, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        if ((unsigned char)text[i] >= 0x80)
            return false;
    }

    return true;
}

inline std::string ConvertCodePage(const char* text, size_t length, UINT from, UINT to)
{
    if (length == 0)
        return std::string();

    int wideLength = MultiByteToWideChar(from, 0, text, (int)length, nullptr, 0);
    if (wideLength <= 0)
        return std::string(text, length);

    std::wstring wide(wideLength, L'\0');
    MultiByteToWideChar(from, 0, text, (int)length, &wide[0], wideLength);

    int outLength = WideCharToMultiByte(to, 0, wide.c_str(), wideLength, nullptr, 0, nullptr, nullptr);
    if (outLength <= 0)
        return std::string(text, length);

    std::string out(outLength, '\0');
    WideCharToMultiByte(to, 0, wide.c_str(), wideLength, &out[0], outLength, nullptr, nullptr);
    return out;
}

inline std::string AnsiToUtf8(const char* text, size_t length)
{
    if (IsAscii(text, length))
        return std::string(text, length);

    return ConvertCodePage(text, length, CP_ACP, CP_UTF8);
}

inline std::string AnsiToUtf8(const char* text)
{
    return text ? AnsiToUtf8(text, strlen(text)) : std::string();
}

inline std::string Utf8ToAnsi(const std::string& text)
{
    if (IsAscii(text.c_str(), text.size()))
        return text;

    return ConvertCodePage(text.c_str(), text.size(), CP_UTF8, CP_ACP);
}

// Case-insensitive edit distance, stops early above the limit
inline int EditDistance(const std::string& a, const std::string& b, int limit)
{
    if ((int)a.size() - (int)b.size() > limit || (int)b.size() - (int)a.size() > limit)
        return limit + 1;

    std::vector<int> previous(b.size() + 1), current(b.size() + 1);
    for (size_t j = 0; j <= b.size(); j++) previous[j] = (int)j;

    for (size_t i = 1; i <= a.size(); i++)
    {
        current[0] = (int)i;
        int rowMin = current[0];

        for (size_t j = 1; j <= b.size(); j++)
        {
            int cost = tolower((unsigned char)a[i - 1]) == tolower((unsigned char)b[j - 1]) ? 0 : 1;
            current[j] = std::min({ previous[j] + 1, current[j - 1] + 1, previous[j - 1] + cost });
            rowMin = std::min(rowMin, current[j]);
        }

        if (rowMin > limit)
            return limit + 1;

        std::swap(previous, current);
    }

    return previous[b.size()];
}

inline IDirect3DDevice9* GetDevice()
{
    if (g_devicePtrAddr == 0)
        return nullptr;

    return *(IDirect3DDevice9**)g_devicePtrAddr;
}

inline ImVec4 GetEntryColor(ConsoleEntryType type)
{
    switch (type)
    {
        case ConsoleEntryType::EngineCommand: return ImVec4(0.40f, 0.75f, 0.80f, 1.0f);
        case ConsoleEntryType::Program:       return ImVec4(0.90f, 0.70f, 0.40f, 1.0f);
        case ConsoleEntryType::ServerCommand: return ImVec4(0.80f, 0.55f, 0.85f, 1.0f);
        case ConsoleEntryType::EngineVar:     return ImVec4(0.50f, 0.78f, 0.50f, 1.0f);
        case ConsoleEntryType::RenderVar:     return ImVec4(0.45f, 0.80f, 0.70f, 1.0f);
        case ConsoleEntryType::ConfigVar:     return ImVec4(0.50f, 0.60f, 0.90f, 1.0f);
        case ConsoleEntryType::ServerVar:     return ImVec4(0.85f, 0.60f, 0.60f, 1.0f);
    }

    return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
}

inline const char* GetEntryTypeName(ConsoleEntryType type)
{
    switch (type)
    {
        case ConsoleEntryType::EngineCommand: return "Command";
        case ConsoleEntryType::Program:       return "Program";
        case ConsoleEntryType::ServerCommand: return "Server Command";
        case ConsoleEntryType::EngineVar:     return "Engine Variable";
        case ConsoleEntryType::RenderVar:     return "Render Variable";
        case ConsoleEntryType::ConfigVar:     return "Config Variable";
        case ConsoleEntryType::ServerVar:     return "Server Variable";
    }

    return "";
}

// Short label of the type tags
inline const char* GetEntryTag(ConsoleEntryType type)
{
    switch (type)
    {
        case ConsoleEntryType::EngineCommand: return "CMD";
        case ConsoleEntryType::Program:       return "PROG";
        case ConsoleEntryType::ServerCommand: return "SV CMD";
        case ConsoleEntryType::EngineVar:     return "ENGINE";
        case ConsoleEntryType::RenderVar:     return "RENDER";
        case ConsoleEntryType::ConfigVar:     return "CONFIG";
        case ConsoleEntryType::ServerVar:     return "SV VAR";
    }

    return "";
}

inline bool IsCommandEntry(ConsoleEntryType type)
{
    return type == ConsoleEntryType::EngineCommand || type == ConsoleEntryType::Program || type == ConsoleEntryType::ServerCommand;
}

inline bool IsServerEntry(ConsoleEntryType type)
{
    return type == ConsoleEntryType::ServerCommand || type == ConsoleEntryType::ServerVar;
}

// Engine commands that set the font of the original console, they have no effect on this one
inline bool IsHiddenEngineCommand(const char* name)
{
    return _stricmp(name, "ConsoleFont") == 0 || _stricmp(name, "ConsoleFontSize") == 0;
}

// Console Managers
// Manager: +0x00 engine variable table, +0x04 engine variable count, +0x10 command list, +0x2C variable hash table
inline DWORD GetClientManager()
{
    DWORD state = g_addresses.clientManager;
    if (state == 0 || *(DWORD*)(state + 0x2C) == 0)
        return 0;

    return state;
}

inline DWORD GetServerManager()
{
    if (g_addresses.serverManager == 0)
        return 0;

    DWORD serverConsole = *(DWORD*)g_addresses.serverManager;
    if (serverConsole == 0)
        return 0;

    // Second virtual function of the server console interface returns its manager
    auto GetState = reinterpret_cast<DWORD(__thiscall*)(DWORD)>(*(DWORD*)(*(DWORD*)serverConsole + 0x04));
    DWORD state = GetState(serverConsole);
    if (state == 0 || *(DWORD*)(state + 0x2C) == 0)
        return 0;

    return state;
}

// Walks the command list: the built-in engine commands, then the console programs registered by the game (flag 1)
template <typename Fn>
inline void ForEachConsoleCommand(DWORD state, Fn&& fn)
{
    DWORD head = state + 0x10;
    for (DWORD link = *(DWORD*)head; link != 0 && link != head; link = *(DWORD*)link)
    {
        DWORD command = *(DWORD*)(link + 0x08);
        const char* name = command ? *(const char**)command : nullptr;
        if (name && name[0])
        {
            fn(name, *(DWORD*)(command + 0x14));
        }
    }
}

// Walks every bucket of the variable hash table, the engine iterator skips the newest entry of the first bucket
template <typename Fn>
inline void ForEachConsoleVar(DWORD state, Fn&& fn)
{
    DWORD table = *(DWORD*)(state + 0x2C);
    if (table == 0)
        return;

    DWORD bucketCount = *(DWORD*)(table + 0x08);
    for (DWORD i = 0; i < bucketCount; i++)
    {
        DWORD head = table + 0x0C + i * 0x14 + 0x0C;
        for (DWORD element = *(DWORD*)head; element != 0 && element != head; element = *(DWORD*)element)
        {
            DWORD var = *(DWORD*)(element + 0x10);
            const char* name = var ? *(const char**)(var + 0x14) : nullptr;
            if (name && name[0])
            {
                const char* value = *(const char**)(var + 0x18);
                fn(name, value ? value : "");
            }
        }
    }
}

// Names are in the engine encoding
inline bool ConsoleCommandExists(DWORD state, const std::string& name)
{
    bool found = false;
    ForEachConsoleCommand(state, [&](const char* commandName, DWORD)
    {
        if (!found && _stricmp(commandName, name.c_str()) == 0)
            found = true;
    });

    return found;
}

inline bool ConsoleVarExists(DWORD state, const std::string& name, std::string* actualName = nullptr, std::string* value = nullptr)
{
    bool found = false;
    ForEachConsoleVar(state, [&](const char* varName, const char* varValue)
    {
        if (!found && _stricmp(varName, name.c_str()) == 0)
        {
            found = true;
            if (actualName)
                *actualName = varName;
            if (value)
                *value = varValue;
        }
    });

    return found;
}

inline DWORD FindEngineVar(DWORD state, const char* name)
{
    DWORD table = *(DWORD*)state;
    int count = *(int*)(state + 0x04);

    for (int i = 0; table && i < count; i++)
    {
        DWORD engineVar = table + i * 0x18;
        const char* varName = *(const char**)engineVar;
        if (varName && _stricmp(varName, name) == 0)
            return engineVar;
    }

    return 0;
}

// Engine variable: +0x00 name, +0x04 float*, +0x08 int*, +0x0C variable**, +0x10 char**
inline std::string GetEngineVarValue(DWORD engineVar)
{
    float* pFloat = *(float**)(engineVar + 0x04);
    int* pInt = *(int**)(engineVar + 0x08);
    DWORD* pCommandVar = *(DWORD**)(engineVar + 0x0C);
    char** pString = *(char***)(engineVar + 0x10);

    if (pString)
        return std::string("\"") + (*pString ? *pString : "") + "\"";

    if (pFloat)
        return FormatFloat(*pFloat);

    if (pInt)
        return std::to_string(*pInt);

    if (pCommandVar && *pCommandVar)
    {
        const char* value = *(const char**)(*pCommandVar + 0x18);
        return value ? value : "";
    }

    return "";
}

// The engine variables still have their built-in values when the game window is created, the config files run later
inline void CaptureEngineVarDefaults()
{
    g_engineVarDefaults.clear();

    DWORD table = g_addresses.engineVarTable;
    int count = g_addresses.engineVarCount;
    if (table == 0 || count <= 0 || count > 4096)
        return;

    for (int i = 0; i < count; i++)
    {
        DWORD engineVar = table + i * 0x18;
        const char* name = *(const char**)engineVar;
        if (!name || !name[0])
            continue;

        // A variable that only points to a console variable has no value of its own
        if (!*(float**)(engineVar + 0x04) && !*(int**)(engineVar + 0x08) && !*(char***)(engineVar + 0x10))
            continue;

        g_engineVarDefaults[ToLower(name)] = AnsiToUtf8(GetEngineVarValue(engineVar).c_str());
    }
}

inline std::string GetEngineVarDefault(const char* name)
{
    auto it = g_engineVarDefaults.find(ToLower(name));
    return it != g_engineVarDefaults.end() ? it->second : std::string();
}

inline void ResolveEngineVars(DWORD state)
{
    auto FloatVar = [state](const char* name) -> float*
    {
        DWORD engineVar = FindEngineVar(state, name);
        return engineVar ? *(float**)(engineVar + 0x04) : nullptr;
    };

    auto IntVar = [state](const char* name) -> int*
    {
        DWORD engineVar = FindEngineVar(state, name);
        return engineVar ? *(int**)(engineVar + 0x08) : nullptr;
    };

    g_engineVars.alpha = FloatVar("ConsoleAlpha");
    g_engineVars.bufferLen = IntVar("ConsoleBufferLen");
    g_engineVars.historyLen = IntVar("ConsoleHistoryLen");
    g_engineVars.rect[0] = FloatVar("ConsoleLeft");
    g_engineVars.rect[1] = FloatVar("ConsoleTop");
    g_engineVars.rect[2] = FloatVar("ConsoleRight");
    g_engineVars.rect[3] = FloatVar("ConsoleBottom");
}

inline int GetBufferLength()
{
    return std::max(g_engineVars.bufferLen ? *g_engineVars.bufferLen : g_settings.bufferLen, 10);
}

inline int GetHistoryLength()
{
    return std::max(g_engineVars.historyLen ? *g_engineVars.historyLen : g_settings.historyLen, 5);
}

inline float GetBackgroundAlpha()
{
    float alpha = g_engineVars.alpha ? *g_engineVars.alpha : g_settings.alpha;
    return std::isfinite(alpha) ? std::clamp(alpha, 0.0f, 1.0f) : 1.0f;
}

// Same presets as the MoveConsole command
inline void ParseConsolePosition(const std::string& position, float rect[4])
{
    const float presets[][4] =
    {
        { 0.0f, 0.0f, 1.0f, 0.5f },
        { 0.0f, 0.5f, 1.0f, 1.0f },
        { 0.25f, 0.25f, 0.75f, 0.75f },
        { 0.0f, 0.0f, 1.0f, 1.0f }
    };

    int preset = 0;
    if (_stricmp(position.c_str(), "Bottom") == 0) preset = 1;
    else if (_stricmp(position.c_str(), "Middle") == 0) preset = 2;
    else if (_stricmp(position.c_str(), "Full") == 0) preset = 3;

    memcpy(rect, presets[preset], sizeof(float) * 4);

    float coords[4];
    if (sscanf_s(position.c_str(), "%f %f %f %f", &coords[0], &coords[1], &coords[2], &coords[3]) == 4)
    {
        memcpy(rect, coords, sizeof(coords));
    }
}

inline void GetConsoleRect(float rect[4])
{
    ParseConsolePosition(g_settings.position, rect);

    for (int i = 0; i < 4; i++)
    {
        if (g_engineVars.rect[i])
            rect[i] = *g_engineVars.rect[i];

        rect[i] = std::isfinite(rect[i]) ? std::clamp(rect[i], 0.0f, 1.0f) : 0.0f;
    }
}

// Output
inline ImU32 ConvertEngineColor(unsigned int color)
{
    // White (CPrint) and yellow (engine messages) are the default colors, those keep the theme color
    unsigned int rgb = color & 0x00FFFFFF;
    if (rgb == 0x00FFFFFF || rgb == 0x00FFFF00)
        return 0;

    return IM_COL32((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, 0xFF);
}

// Prefix followed by a separator, so "Error: ..." matches but not "ErrorLog"
inline bool StartsWithWord(const std::string& text, const std::string& word)
{
    return StartsWithCaseInsensitive(text, word) && (text.size() == word.size() || !isalnum((unsigned char)text[word.size()]));
}

inline ConsoleLineType ClassifyLine(const std::string& text)
{
    if (text.find("[ERROR]") != std::string::npos || StartsWithWord(text, "Error") ||
        StartsWithWord(text, "LT ERROR") || StartsWithWord(text, "LOAD ERROR"))
        return ConsoleLineType::Error;

    if (StartsWithWord(text, "Warning") || text.find("Warning!") != std::string::npos)
        return ConsoleLineType::Warning;

    return ConsoleLineType::Output;
}

inline void FormatTimestamp(uint32_t time, char* buffer, size_t size)
{
    snprintf(buffer, size, "%02u:%02u:%02u", (unsigned)(time / 3600), (unsigned)((time / 60) % 60), (unsigned)(time % 60));
}

inline void ClearLineSelection()
{
    g_selectionAnchor = 0;
    g_selectionActive = 0;
    g_selectionDragging = false;
}

inline bool HasLineSelection()
{
    return g_selectionAnchor != 0;
}

inline bool IsLineSelected(uint64_t id)
{
    if (g_selectionAnchor == 0)
        return false;

    return id >= std::min(g_selectionAnchor, g_selectionActive) && id <= std::max(g_selectionAnchor, g_selectionActive);
}

inline void AddLine(std::string text, ConsoleLineType type, ImU32 color = 0, size_t split = 0, bool dimFirst = false)
{
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());

    SYSTEMTIME now;
    GetLocalTime(&now);

    OutputLock lock;

    if (g_loggingEnabled && g_logFile)
    {
        fprintf(g_logFile, "[%02d:%02d:%02d] %s\n", now.wHour, now.wMinute, now.wSecond, text.c_str());
        fflush(g_logFile);
    }

    ConsoleLine line;
    line.text = std::move(text);
    line.color = color;
    line.type = type;
    line.id = g_nextLineId++;
    line.time = now.wHour * 3600u + now.wMinute * 60u + now.wSecond;
    line.split = (uint16_t)std::min<size_t>(split, line.text.size());
    line.dimFirst = dimFirst;
    g_outputLines.push_back(std::move(line));

    size_t maxLines = (size_t)GetBufferLength();
    while (g_outputLines.size() > maxLines)
    {
        g_outputLines.pop_front();
    }

    g_outputGeneration++;
}

inline void PushOutputLine(const std::string& line, ConsoleLineType type = ConsoleLineType::Output)
{
    if (type == ConsoleLineType::Output)
        type = ClassifyLine(line);

    AddLine(line, type);
}

inline void ClearOutputLines()
{
    OutputLock lock;
    g_outputLines.clear();
    g_outputGeneration++;
    ClearLineSelection();
}

inline int GetLineFilterBit(ConsoleLineType type)
{
    switch (type)
    {
        case ConsoleLineType::Warning: return OutputFilter_Warnings;
        case ConsoleLineType::Error:   return OutputFilter_Errors;
        default:                       return OutputFilter_Log;
    }
}

// Requires the output lock
inline void RebuildVisibleRows()
{
    if (g_visibleGeneration == g_outputGeneration && g_visibleMask == g_outputMask && g_visibleFilter == g_outputFilter)
        return;

    g_visibleRows.clear();
    g_countLog = g_countWarnings = g_countErrors = 0;

    for (int i = 0; i < (int)g_outputLines.size(); i++)
    {
        const ConsoleLine& line = g_outputLines[i];
        int bit = GetLineFilterBit(line.type);

        if (bit == OutputFilter_Warnings) g_countWarnings++;
        else if (bit == OutputFilter_Errors) g_countErrors++;
        else g_countLog++;

        if ((g_outputMask & bit) && ContainsCaseInsensitive(line.text, g_outputFilter))
        {
            g_visibleRows.push_back(i);
        }
    }

    g_visibleGeneration = g_outputGeneration;
    g_visibleMask = g_outputMask;
    g_visibleFilter = g_outputFilter;
}

inline std::string FormatLineForCopy(const ConsoleLine& line)
{
    if (!g_showTimestamps)
        return line.text;

    char timestamp[16];
    FormatTimestamp(line.time, timestamp, sizeof(timestamp));
    return std::string(timestamp) + "  " + line.text;
}

// Requires the output lock, copies the visible lines, only the selected ones if asked
inline int CopyRowsLocked(bool selectedOnly)
{
    RebuildVisibleRows();

    std::string text;
    int count = 0;
    for (int row : g_visibleRows)
    {
        const ConsoleLine& line = g_outputLines[row];
        if (selectedOnly && !IsLineSelected(line.id))
            continue;

        text += FormatLineForCopy(line);
        text += '\n';
        count++;
    }

    if (count > 0)
        ImGui::SetClipboardText(text.c_str());

    return count;
}

// Copy button: the selected lines, or every visible line
inline void CopyOutputToClipboard()
{
    OutputLock lock;
    if (!HasLineSelection() || CopyRowsLocked(true) == 0)
    {
        CopyRowsLocked(false);
    }
}

// Cursor Management
inline bool ShouldShowCursorWhenClosed()
{
    return !g_State.isPlaying || g_State.isMsgBoxVisible;
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

inline float GetFontPixelSize()
{
    return 13.0f * g_uiScale;
}

// The pixel font only looks right at its native size, the vector version is used when the console is scaled
inline ImFont* GetConsoleFont()
{
    return (fabs(GetFontPixelSize() - 13.0f) < 0.01f) ? g_fontBitmap : g_fontVector;
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

// Populate Functions, the names and values are converted to UTF-8 for the interface
inline void SortEntries(std::vector<ConsoleEntry>& entries)
{
    std::sort(entries.begin(), entries.end(), [](const ConsoleEntry& a, const ConsoleEntry& b)
    {
        return _stricmp(a.name.c_str(), b.name.c_str()) < 0;
    });
}

inline std::vector<ConsoleEntry> PopulateServerCommands()
{
    std::vector<ConsoleEntry> commands;

    if (DWORD state = GetServerManager())
    {
        ForEachConsoleCommand(state, [&](const char* name, DWORD)
        {
            commands.push_back({ AnsiToUtf8(name), "", "", ConsoleEntryType::ServerCommand });
        });
    }

    return commands;
}

inline std::vector<ConsoleEntry> PopulateCommands(bool includeServer = true)
{
    std::vector<ConsoleEntry> commands;

    if (DWORD state = GetClientManager())
    {
        ForEachConsoleCommand(state, [&](const char* name, DWORD flags)
        {
            if (!(flags & 1) && IsHiddenEngineCommand(name))
                return;

            commands.push_back({ AnsiToUtf8(name), "", "", (flags & 1) ? ConsoleEntryType::Program : ConsoleEntryType::EngineCommand });
        });
    }

    if (includeServer)
    {
        std::vector<ConsoleEntry> serverCommands = PopulateServerCommands();
        commands.insert(commands.end(), serverCommands.begin(), serverCommands.end());
    }

    SortEntries(commands);
    return commands;
}

// Variables compiled in the engine: the engine variable table and the renderer variables
inline std::vector<ConsoleEntry> PopulateEngineVariables()
{
    std::vector<ConsoleEntry> variables;
    std::set<std::string> names;

    if (DWORD state = GetClientManager())
    {
        DWORD table = *(DWORD*)state;
        int count = *(int*)(state + 0x04);

        for (int i = 0; table && i < count; i++)
        {
            DWORD engineVar = table + i * 0x18;
            const char* name = *(const char**)engineVar;
            if (!name || !name[0] || !names.insert(ToLower(name)).second)
                continue;

            variables.push_back({ AnsiToUtf8(name), AnsiToUtf8(GetEngineVarValue(engineVar).c_str()), GetEngineVarDefault(name), ConsoleEntryType::EngineVar });
        }
    }

    // Render variable: +0x00 vtable, +0x04 default value, +0x08 name, +0x10 next, +0x14 value
    DWORD current = g_addresses.cvarListHead ? *(DWORD*)g_addresses.cvarListHead : 0;
    while (current)
    {
        const char* name = *(const char**)(current + 0x08);
        if (name && name[0] && names.insert(ToLower(name)).second)
        {
            ConsoleEntry var;
            var.name = AnsiToUtf8(name);
            var.type = ConsoleEntryType::RenderVar;

            float defaultValue = *(float*)(current + 0x04);
            DWORD vtable = *(DWORD*)(current + 0x00);
            if (vtable == g_addresses.cvarVtableFloat)
            {
                var.value = FormatFloat(*(float*)(current + 0x14));
                var.defaultValue = FormatFloat(defaultValue);
            }
            else
            {
                var.value = std::to_string(*(int*)(current + 0x14));
                var.defaultValue = std::to_string((int)defaultValue);
            }

            variables.push_back(var);
        }

        current = *(DWORD*)(current + 0x10);
    }

    SortEntries(variables);
    return variables;
}

// Variables created at runtime by the game and the config files
inline std::vector<ConsoleEntry> PopulateConfigVariables(const std::vector<ConsoleEntry>& engineVariables)
{
    std::vector<ConsoleEntry> variables;

    DWORD state = GetClientManager();
    if (state == 0)
        return variables;

    std::set<std::string> engineNames;
    for (const auto& var : engineVariables)
    {
        engineNames.insert(ToLower(var.name));
    }

    ForEachConsoleVar(state, [&](const char* name, const char* value)
    {
        std::string utf8Name = AnsiToUtf8(name);
        if (engineNames.find(ToLower(utf8Name)) == engineNames.end())
        {
            variables.push_back({ utf8Name, AnsiToUtf8(value), "", ConsoleEntryType::ConfigVar });
        }
    });

    SortEntries(variables);
    return variables;
}

inline std::vector<ConsoleEntry> PopulateServerVariables()
{
    std::vector<ConsoleEntry> variables;

    if (DWORD state = GetServerManager())
    {
        ForEachConsoleVar(state, [&](const char* name, const char* value)
        {
            variables.push_back({ AnsiToUtf8(name), AnsiToUtf8(value), "", ConsoleEntryType::ServerVar });
        });
    }

    SortEntries(variables);
    return variables;
}

inline void RefreshBrowser()
{
    g_browserEntries[BrowserTab_Commands] = PopulateCommands();
    g_browserEntries[BrowserTab_EngineVars] = PopulateEngineVariables();
    g_browserEntries[BrowserTab_ConfigVars] = PopulateConfigVariables(g_browserEntries[BrowserTab_EngineVars]);
    g_browserEntries[BrowserTab_ServerVars] = PopulateServerVariables();

    g_browserNeedsRefresh = false;
    g_browserRefreshTime = GetTickCount64();
}

// Everything that can be typed in the console
inline void RefreshCandidates(bool force = false)
{
    ULONGLONG now = GetTickCount64();
    if (!force && !g_candidatesDirty && now - g_candidatesTime < 1000)
        return;

    std::vector<ConsoleEntry> engineVariables = PopulateEngineVariables();
    std::vector<ConsoleEntry> candidates = PopulateCommands(false);
    std::vector<ConsoleEntry> configVariables = PopulateConfigVariables(engineVariables);

    candidates.insert(candidates.end(), engineVariables.begin(), engineVariables.end());
    candidates.insert(candidates.end(), configVariables.begin(), configVariables.end());

    std::set<std::string> clientNames;
    for (const auto& entry : candidates)
    {
        clientNames.insert(ToLower(entry.name));
    }

    std::vector<ConsoleEntry> serverEntries = PopulateServerCommands();
    std::vector<ConsoleEntry> serverVariables = PopulateServerVariables();
    serverEntries.insert(serverEntries.end(), serverVariables.begin(), serverVariables.end());

    for (auto& entry : serverEntries)
    {
        if (clientNames.insert(ToLower(entry.name)).second)
        {
            candidates.push_back(std::move(entry));
        }
    }

    SortEntries(candidates);

    g_candidates = std::move(candidates);
    g_candidatesDirty = false;
    g_candidatesTime = now;
    g_suggestionPrefix.clear();
}

// History
inline void LoadHistory()
{
    g_commandHistory.clear();

    FILE* file = nullptr;
    if (fopen_s(&file, g_historyFilename.c_str(), "r") != 0 || !file)
        return;

    char line[512];
    while (fgets(line, sizeof(line), file))
    {
        std::string command = TrimString(line);
        if (!command.empty())
        {
            g_commandHistory.push_back(command);
        }
    }

    fclose(file);

    while ((int)g_commandHistory.size() > g_settings.historyLen && g_commandHistory.size() > 5)
    {
        g_commandHistory.pop_front();
    }
}

inline void SaveHistory()
{
    FILE* file = nullptr;
    if (fopen_s(&file, g_historyFilename.c_str(), "w") != 0 || !file)
        return;

    for (const auto& command : g_commandHistory)
    {
        fprintf(file, "%s\n", command.c_str());
    }

    fclose(file);
}

// Unique entries like the engine history, the most recent one at the end
inline void AddToHistory(const std::string& command)
{
    g_commandHistory.erase(std::remove_if(g_commandHistory.begin(), g_commandHistory.end(), [&](const std::string& entry)
    {
        return _stricmp(entry.c_str(), command.c_str()) == 0;
    }), g_commandHistory.end());

    g_commandHistory.push_back(command);

    while ((int)g_commandHistory.size() > GetHistoryLength())
    {
        g_commandHistory.pop_front();
    }

    SaveHistory();
}

// Navigation (history and completion)
inline void EndNav()
{
    g_navState = NavState::Normal;
    g_historyPos = -1;
    g_completionIndex = -1;
    g_completionMatches.clear();
}

inline void StartNav(NavState state, const char* text)
{
    if (g_navState == state)
        return;

    EndNav();
    g_navCommand = text;
    g_navState = state;
}

inline void SetInputText(ImGuiInputTextCallbackData* data, const std::string& text)
{
    data->DeleteChars(0, data->BufTextLen);
    data->InsertChars(0, text.c_str());
    g_navAppliedText = data->Buf;
}

inline int FindHistory(const std::string& prefix, int start, int step)
{
    for (int i = start; i >= 0 && i < (int)g_commandHistory.size(); i += step)
    {
        if (StartsWithCaseInsensitive(g_commandHistory[i], prefix))
            return i;
    }

    return -1;
}

// Same as the engine history, Up/Down only go through the commands starting with what was typed
inline void CycleHistory(ImGuiInputTextCallbackData* data, int direction)
{
    if (g_commandHistory.empty())
        return;

    if (g_navState != NavState::History)
    {
        std::string prefix = data->Buf;
        int last = (int)g_commandHistory.size() - 1;
        int index = (direction < 0) ? FindHistory(prefix, last, -1) : FindHistory(prefix, 0, 1);
        if (index < 0)
            return;

        StartNav(NavState::History, data->Buf);
        g_historyPos = index;
    }
    else
    {
        int index = FindHistory(g_navCommand, g_historyPos + direction, direction);
        if (index < 0)
        {
            // Back to what was typed
            SetInputText(data, g_navCommand);
            EndNav();
            return;
        }

        g_historyPos = index;
    }

    SetInputText(data, g_commandHistory[g_historyPos]);
}

// The part of the line being completed: after the last ';', while the command name is typed
inline bool GetCompletionSegment(const char* text, std::string& base, std::string& segment)
{
    const char* start = text;
    bool inQuotes = false;
    for (const char* p = text; *p; p++)
    {
        if (*p == '"')
            inQuotes = !inQuotes;
        else if (*p == ';' && !inQuotes)
            start = p + 1;
    }

    while (*start == ' ') start++;

    if (strchr(start, ' '))
        return false;

    base.assign(text, start - text);
    segment = start;
    return true;
}

inline std::vector<ConsoleEntry> FindCompletionMatches(const std::string& prefix)
{
    std::vector<ConsoleEntry> matches;
    for (const auto& entry : g_candidates)
    {
        if (StartsWithCaseInsensitive(entry.name, prefix))
        {
            matches.push_back(entry);
        }
    }

    return matches;
}

// Ctrl+Tab, prints every match like the original console
inline void ListMatches(const char* text)
{
    std::string base, segment;
    if (!GetCompletionSegment(text, base, segment))
        return;

    RefreshCandidates(true);
    std::vector<ConsoleEntry> matches = FindCompletionMatches(segment);
    if (matches.empty())
        return;

    AddLine(">" + segment + "*", ConsoleLineType::Command);
    for (const auto& match : matches)
    {
        std::string line = match.value.empty() ? "  " + match.name : "  " + match.name + " = " + match.value;
        if (IsServerEntry(match.type))
            line += "  (server)";

        AddLine(line, ConsoleLineType::Info);
    }

    g_scrollToBottom = true;
}

// Tab / Shift+Tab, cycles through the matches like the original console
inline void ApplyCompletion(ImGuiInputTextCallbackData* data, CompletionRequest request)
{
    if (request == CompletionRequest::List)
    {
        ListMatches(data->Buf);
        return;
    }

    if (g_navState != NavState::Complete)
    {
        std::string base, segment;
        if (!GetCompletionSegment(data->Buf, base, segment))
            return;

        RefreshCandidates(true);
        std::vector<ConsoleEntry> matches = FindCompletionMatches(segment);
        if (matches.empty())
            return;

        // A single match is completed directly, ready for its arguments
        if (matches.size() == 1)
        {
            EndNav();
            SetInputText(data, base + matches[0].name + " ");
            return;
        }

        StartNav(NavState::Complete, data->Buf);
        g_completionBase = base;
        g_completionPrefix = segment;
        g_completionMatches = std::move(matches);
        g_completionIndex = (request == CompletionRequest::Next) ? 0 : (int)g_completionMatches.size() - 1;
    }
    else
    {
        g_completionIndex += (request == CompletionRequest::Next) ? 1 : -1;
        if (g_completionIndex < 0 || g_completionIndex >= (int)g_completionMatches.size())
        {
            // Back to what was typed, the next press starts over
            SetInputText(data, g_navCommand);
            EndNav();
            return;
        }
    }

    SetInputText(data, g_completionBase + g_completionMatches[g_completionIndex].name);
}

// Edit Modal
inline void OpenEditModal(const ConsoleEntry& entry)
{
    g_editEntry = entry;
    strncpy_s(g_editValueBuffer, sizeof(g_editValueBuffer), StripQuotes(entry.value).c_str(), _TRUNCATE);

    g_showEditModal = true;
    g_editModalFocusInput = true;
}

inline void ExecuteCommandInternal(const char* command, bool server = false);

inline void ApplyVariableValue(const ConsoleEntry& entry, const std::string& value)
{
    ExecuteCommandInternal((entry.name + " " + QuoteValue(StripQuotes(value))).c_str(), entry.type == ConsoleEntryType::ServerVar);
}

// Remembers the caret of the text field that has the keyboard, used by the toggle key
inline void TrackTextField(ImGuiInputTextCallbackData* data)
{
    g_textFieldFrame.active = true;
    g_textFieldFrame.cursor = data->CursorPos;
    g_textFieldFrame.length = data->BufTextLen;
    g_textFieldFrame.selection = data->HasSelection();
}

inline int TrackTextFieldCallback(ImGuiInputTextCallbackData* data)
{
    if (data->EventFlag == ImGuiInputTextFlags_CallbackAlways)
    {
        TrackTextField(data);
    }

    return 0;
}

inline void RenderEditModal()
{
    if (!g_showEditModal)
        return;

    ImGui::OpenPopup("Edit Value");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 28.0f, 0), ImGuiCond_Always);

    if (ImGui::BeginPopupModal("Edit Value", &g_showEditModal, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextDisabled("%s", GetEntryTypeName(g_editEntry.type));
        ImGui::PushStyleColor(ImGuiCol_Text, GetEntryColor(g_editEntry.type));
        ImGui::Text("%s", g_editEntry.name.c_str());
        ImGui::PopStyleColor();

        ImGui::Separator();
        ImGui::TextDisabled("Current:");
        ImGui::SameLine();
        ImGui::TextUnformatted(g_editEntry.value.empty() ? "-" : g_editEntry.value.c_str());

        if (!g_editEntry.defaultValue.empty())
        {
            ImGui::TextDisabled("Default:");
            ImGui::SameLine();
            ImGui::TextUnformatted(g_editEntry.defaultValue.c_str());
        }

        ImGui::Text("New Value:");
        ImGui::PushItemWidth(-1);

        if (g_editModalFocusInput)
        {
            ImGui::SetKeyboardFocusHere();
            g_editModalFocusInput = false;
        }

        bool enterPressed = ImGui::InputText("##EditValue", g_editValueBuffer, sizeof(g_editValueBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackAlways, TrackTextFieldCallback);
        ImGui::PopItemWidth();
        ImGui::Separator();

        float buttonWidth = ImGui::GetFontSize() * 7.0f;
        bool apply = ImGui::Button("Apply", ImVec2(buttonWidth, 0)) || enterPressed;

        bool reset = false;
        if (!g_editEntry.defaultValue.empty())
        {
            ImGui::SameLine();
            ImGui::BeginDisabled(g_editEntry.value == g_editEntry.defaultValue);
            reset = ImGui::Button("Reset", ImVec2(buttonWidth, 0));
            ImGui::EndDisabled();
        }

        ImGui::SameLine();
        bool cancel = ImGui::Button("Cancel", ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false);

        if (apply || reset)
        {
            ApplyVariableValue(g_editEntry, reset ? g_editEntry.defaultValue : g_editValueBuffer);

            g_showEditModal = false;
            ImGui::CloseCurrentPopup();
        }
        else if (cancel)
        {
            g_showEditModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// Command Handler
// The commands are typed in UTF-8, the engine reads them in its own encoding
inline void RunConsoleString(DWORD state, const std::string& command)
{
    if (ExecuteConsoleCommand && state)
    {
        ExecuteConsoleCommand(state, Utf8ToAnsi(command).c_str());
    }
}

// Commands handled by EchoPatch and the engine commands worth explaining
struct CommandHelp
{
    const char* name;
    const char* usage;
    const char* description;
};

inline const CommandHelp g_commandHelp[] =
{
    { "help",            "help [name]",             "Show this help, or details about a command or variable" },
    { "clear",           "clear",                   "Clear the output" },
    { "seta",            "seta <variable> <value>", "Set a variable in the console that has it, client or server" },
    { "serv",            "serv <command>",          "Send a command to the server" },
    { "Exec",            "Exec <file>",             "Run each line of a config file" },
    { "MoveConsole",     "MoveConsole <position>",  "Top, Bottom, Middle, Full, or left top right bottom in [0..1]" },
    { "ConsoleHistory",  "ConsoleHistory",          "List the command history" },
    { "ListCommands",    "ListCommands",            "List the engine commands" },
    { "Set",             "Set",                     "Print every client variable with its value" },
    { "quit",            "quit",                    "Exit the game" },
};

inline const CommandHelp* FindCommandHelp(const std::string& name)
{
    for (const auto& help : g_commandHelp)
    {
        if (_stricmp(help.name, name.c_str()) == 0)
            return &help;
    }

    return nullptr;
}

inline std::string PadRight(const std::string& text, size_t width)
{
    return text.size() >= width ? text + " " : text + std::string(width - text.size(), ' ');
}

inline std::string GetKeyName(UINT vk)
{
    UINT scanCode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
    LONG lParam = (LONG)(scanCode << 16);

    switch (vk)
    {
        case VK_HOME: case VK_END: case VK_INSERT: case VK_DELETE: case VK_PRIOR: case VK_NEXT:
        case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN: case VK_DIVIDE: case VK_NUMLOCK:
            lParam |= 1 << 24;
            break;
    }

    char name[64] = {};
    if (GetKeyNameTextA(lParam, name, sizeof(name)) > 0)
        return AnsiToUtf8(name);

    char code[16];
    snprintf(code, sizeof(code), "Key %u", vk);
    return code;
}

// Help row in two columns, a name or a label then its description or its value
inline void AddHelpRow(const std::string& first, const std::string& second, size_t width, bool dimFirst = false)
{
    std::string column = "  " + PadRight(first, width);
    AddLine(column + second, ConsoleLineType::Info, 0, column.size(), dimFirst);
}

inline void PrintGeneralHelp()
{
    const size_t column = 26;

    AddLine("Console help", ConsoleLineType::Header);
    AddLine("  Type a command or a variable, Tab completes it. 'help <name>' shows the details of one.", ConsoleLineType::Info);

    AddLine("Commands", ConsoleLineType::Header);
    for (const auto& help : g_commandHelp)
    {
        AddHelpRow(help.usage, help.description, column);
    }

    AddLine("Syntax", ConsoleLineType::Header);
    AddHelpRow("Name", "Print the value of a variable", column);
    AddHelpRow("Name value", "Set a variable, \"quotes\" keep the spaces of a value", column);
    AddHelpRow("Name !", "Toggle a variable between 0 and 1", column);
    AddHelpRow("command1; command2", "Run several commands", column);

    AddLine("Keys", ConsoleLineType::Header);
    AddHelpRow(GetKeyName(g_toggleKey), "Open or close the console", column);
    AddHelpRow("Tab, Shift+Tab", "Cycle through the matches, Ctrl+Tab lists them", column);
    AddHelpRow("Up, Down", "Go through the history", column);
    AddHelpRow("PgUp, PgDn", "Scroll, with Ctrl to the top or the bottom", column);
    AddHelpRow("Click, Shift+Click", "Select lines, Ctrl+C copies them", column);
    AddHelpRow("Escape", "Clear the selection, then the line, then close", column);
}

inline std::vector<ConsoleEntry> PopulateAllEntries()
{
    std::vector<ConsoleEntry> entries = PopulateCommands(true);
    std::vector<ConsoleEntry> engineVariables = PopulateEngineVariables();
    std::vector<ConsoleEntry> configVariables = PopulateConfigVariables(engineVariables);
    std::vector<ConsoleEntry> serverVariables = PopulateServerVariables();

    entries.insert(entries.end(), engineVariables.begin(), engineVariables.end());
    entries.insert(entries.end(), configVariables.begin(), configVariables.end());
    entries.insert(entries.end(), serverVariables.begin(), serverVariables.end());
    return entries;
}

// Close names for a typo, from the commands and variables of both consoles
inline void PrintSimilarNames(const std::string& name)
{
    const int limit = name.size() <= 4 ? 1 : 2;
    std::vector<std::pair<int, std::string>> matches;
    std::set<std::string> seen;

    for (const auto& entry : PopulateAllEntries())
    {
        if (!seen.insert(ToLower(entry.name)).second)
            continue;

        int distance = EditDistance(name, entry.name, limit);
        if (distance <= limit)
        {
            matches.push_back({ distance, entry.name });
        }
    }

    if (matches.empty())
        return;

    std::sort(matches.begin(), matches.end(), [](const auto& a, const auto& b)
    {
        return a.first != b.first ? a.first < b.first : _stricmp(a.second.c_str(), b.second.c_str()) < 0;
    });

    std::string text = "  Did you mean ";
    for (size_t i = 0; i < matches.size() && i < 3; i++)
    {
        if (i > 0)
            text += ", ";
        text += matches[i].second;
    }

    AddLine(text + "?", ConsoleLineType::Info);
}

inline void PrintNameHelp(const std::string& name)
{
    std::vector<ConsoleEntry> matches;
    for (const auto& entry : PopulateAllEntries())
    {
        if (_stricmp(entry.name.c_str(), name.c_str()) == 0)
        {
            matches.push_back(entry);
        }
    }

    const CommandHelp* help = FindCommandHelp(name);
    if (matches.empty() && !help)
    {
        AddLine("No command or variable named " + name, ConsoleLineType::Error);
        PrintSimilarNames(name);
        return;
    }

    const size_t column = 11;
    AddLine(matches.empty() ? help->name : matches[0].name, ConsoleLineType::Header);

    if (help)
    {
        AddHelpRow("Usage", help->usage, column, true);
        AddHelpRow("About", help->description, column, true);
    }

    for (const auto& entry : matches)
    {
        AddHelpRow("Type", GetEntryTypeName(entry.type), column, true);

        if (!IsCommandEntry(entry.type))
        {
            AddHelpRow("Value", entry.value.empty() ? "-" : entry.value, column, true);

            if (!entry.defaultValue.empty())
            {
                AddHelpRow("Default", entry.defaultValue, column, true);
            }
        }
    }
}

inline void HandleHelpCommand(const std::vector<std::string>& args)
{
    if (args.size() < 2)
    {
        PrintGeneralHelp();
    }
    else
    {
        PrintNameHelp(args[1]);
    }

    g_scrollToBottom = true;
}

inline void HandleSetaCommand(const std::string& command)
{
    const char* args = command.c_str();
    while (*args && isspace((unsigned char)*args)) args++;
    args += 4;
    while (*args && isspace((unsigned char)*args)) args++;

    std::string varName;
    while (*args && !isspace((unsigned char)*args))
    {
        varName += *args++;
    }

    while (*args && isspace((unsigned char)*args)) args++;
    std::string varValue = StripQuotes(TrimString(args));

    if (varName.empty() || varValue.empty())
    {
        PushOutputLine("[ERROR] Usage: seta <variable> <value>");
        return;
    }

    // Sets the variable in the console that owns it, the server one if the client doesn't have it
    std::string engineName = Utf8ToAnsi(varName);
    std::string actualName;
    DWORD state = GetClientManager();
    if (state && !ConsoleVarExists(state, engineName, &actualName))
    {
        DWORD engineVar = FindEngineVar(state, engineName.c_str());
        if (engineVar)
            actualName = *(const char**)engineVar;
        else
            state = 0;
    }

    if (!state)
    {
        state = GetServerManager();
        if (!state || !ConsoleVarExists(state, engineName, &actualName))
        {
            PushOutputLine("[ERROR] Unknown config variable: " + varName);
            return;
        }
    }

    std::string displayName = AnsiToUtf8(actualName.c_str());
    RunConsoleString(state, displayName + " " + QuoteValue(varValue));
    PushOutputLine(displayName + " = " + varValue);
}

inline void PrintHistory()
{
    for (const auto& command : g_commandHistory)
    {
        PushOutputLine(command);
    }

    PushOutputLine(std::to_string(g_commandHistory.size()) + " commands");
}

// Same order as the engine, the commands first, then the variables, the server only for the names the client doesn't have
inline NameTarget ResolveName(const std::string& engineName, DWORD client, DWORD server)
{
    if (client)
    {
        if (ConsoleCommandExists(client, engineName))
            return NameTarget::ClientCommand;
        if (ConsoleVarExists(client, engineName))
            return NameTarget::ClientVar;
        if (FindEngineVar(client, engineName.c_str()))
            return NameTarget::EngineVar;
    }

    if (server)
    {
        if (ConsoleCommandExists(server, engineName))
            return NameTarget::ServerCommand;
        if (ConsoleVarExists(server, engineName))
            return NameTarget::ServerVar;
    }

    return NameTarget::Unknown;
}

inline void RunRoutedCommand(const std::string& segment, const std::vector<std::string>& args)
{
    DWORD client = GetClientManager();
    DWORD server = GetServerManager();

    // "+Name value" and "-Name value" set the variable Name, the prefix only changes its save flag
    std::string name = args[0];
    if (args.size() >= 2 && name.size() > 1 && (name[0] == '+' || name[0] == '-'))
    {
        name.erase(0, 1);
    }

    std::string engineName = Utf8ToAnsi(name);

    switch (ResolveName(engineName, client, server))
    {
        case NameTarget::ClientCommand:
        case NameTarget::ClientVar:
            RunConsoleString(client, segment);
            break;

        case NameTarget::EngineVar:
        {
            // The engine only prints the variables that were set at least once
            if (args.size() == 1)
            {
                DWORD engineVar = FindEngineVar(client, engineName.c_str());
                PushOutputLine(AnsiToUtf8(*(const char**)engineVar) + " = " + StripQuotes(AnsiToUtf8(GetEngineVarValue(engineVar).c_str())));
            }
            else
            {
                RunConsoleString(client, segment);
            }
            break;
        }

        case NameTarget::ServerCommand:
            RunConsoleString(server, segment);
            break;

        case NameTarget::ServerVar:
        {
            RunConsoleString(server, segment);

            std::string actualName, value;
            if (args.size() >= 2 && ConsoleVarExists(server, engineName, &actualName, &value))
            {
                AddLine(AnsiToUtf8(actualName.c_str()) + " = " + AnsiToUtf8(value.c_str()) + "  (server)", ConsoleLineType::Info);
            }
            break;
        }

        case NameTarget::Unknown:
        {
            if (args.size() == 1)
            {
                AddLine("Unknown command or variable: " + name, ConsoleLineType::Error);
                PrintSimilarNames(name);
                break;
            }

            // Typing "Name value" creates the variable, the game reads some of those without creating them
            RunConsoleString(client, segment);

            std::string actualName, value;
            if (client && ConsoleVarExists(client, engineName, &actualName, &value))
            {
                AddLine(AnsiToUtf8(actualName.c_str()) + " = " + AnsiToUtf8(value.c_str()) + "  (new variable)", ConsoleLineType::Info);
            }
            break;
        }
    }
}

inline void HandleConsoleCommand(const PendingCommand& command)
{
    if (command.server)
    {
        DWORD state = GetServerManager();
        if (state == 0)
        {
            PushOutputLine("[ERROR] No server is running");
            return;
        }

        RunConsoleString(state, command.text);
        return;
    }

    for (const std::string& segment : SplitCommandLine(command.text.c_str()))
    {
        std::vector<std::string> args = TokenizeCommand(segment);
        if (args.empty())
            continue;

        const char* name = args[0].c_str();

        if (_stricmp(name, "help") == 0)
        {
            HandleHelpCommand(args);
        }
        else if (_stricmp(name, "clear") == 0)
        {
            ClearOutputLines();
        }
        else if (_stricmp(name, "seta") == 0)
        {
            HandleSetaCommand(segment);
        }
        else if (_stricmp(name, "ConsoleHistory") == 0)
        {
            PrintHistory();
        }
        else if (IsHiddenEngineCommand(name))
        {
            AddLine("Unknown command or variable: " + args[0], ConsoleLineType::Error);
        }
        else
        {
            RunRoutedCommand(segment, args);
        }
    }
}

// Commands typed in the console are executed from the main loop, not while rendering
inline void ExecuteCommandInternal(const char* command, bool server)
{
    // Server commands are shown as "serv" so they run on the server again from the history
    std::string displayed = server ? std::string("serv ") + command : std::string(command);
    PushOutputLine("> " + displayed, ConsoleLineType::Command);
    AddToHistory(displayed);
    EndNav();

    g_pendingCommands.push_back({ command, server });
    g_scrollToBottom = true;
}

inline void InsertIntoInput(const std::string& text)
{
    strncpy_s(g_inputBuffer, sizeof(g_inputBuffer), text.c_str(), _TRUNCATE);
    EndNav();

    g_focusInput = true;
    g_moveCursorToEnd = true;
    ImGui::SetWindowFocus(g_title.c_str());
}

// Toggle Key
inline bool IsToggleKey(WPARAM wParam)
{
    return wParam == g_toggleKey;
}

// The toggle key keeps its editing action in a text field while it still has something to do, Home goes to the start of the line first
inline bool ToggleKeyEditsText(UINT vk)
{
    if (!g_textField.active)
        return false;

    bool modifier = (GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_SHIFT) & 0x8000);

    switch (vk)
    {
        case VK_HOME:
            return modifier || g_textField.selection || g_textField.cursor > 0;

        case VK_END:
            return modifier || g_textField.selection || g_textField.cursor < g_textField.length;

        case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN:
        case VK_DELETE: case VK_BACK: case VK_INSERT: case VK_PRIOR: case VK_NEXT:
            return g_textField.length > 0;
    }

    return false;
}

// A dead key (^ or ` on some layouts) would otherwise combine with the next typed character
inline void ClearDeadKeyState()
{
    BYTE keyState[256] = {};
    WCHAR buffer[8];
    ToUnicode(VK_SPACE, MapVirtualKeyA(VK_SPACE, MAPVK_VK_TO_VSC), keyState, buffer, 8, 0);
}

inline void OpenConsole()
{
    g_visible = true;
    g_justOpened = true;
    g_focusInput = true;
    g_moveCursorToEnd = true;
    g_scrollToBottom = true;
    g_candidatesDirty = true;
    g_browserNeedsRefresh = true;
    g_mouseMessagesSeen = false;
    g_closeOnEscapeRelease = false;
    g_openTime = GetTickCount64();
    g_State.isConsoleOpen = true;

    // Keys and buttons held while the console was closed were never released for ImGui
    if (g_imguiInitialized)
    {
        ImGui::GetIO().ClearInputKeys();
        ImGui::GetIO().ClearInputMouse();
    }

    OnConsoleOpened();
}

inline void CloseConsole()
{
    g_visible = false;
    g_showBrowserWindow = false;
    g_showEditModal = false;
    g_consoleHasKeyboard = false;
    g_closeOnEscapeRelease = false;
    g_selectionDragging = false;
    g_textField = TextFieldState();
    g_State.isConsoleOpen = false;
    EndNav();
    OnConsoleClosed();
}

// Theme
inline void ApplyConsoleStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 4.0f;
    style.ChildRounding = 3.0f;
    style.FrameRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;
    style.PopupRounding = 4.0f;

    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(6.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(5.0f, 4.0f);
    style.CellPadding = ImVec2(6.0f, 2.0f);
    style.ScrollbarSize = 12.0f;
    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.TabBarOverlineSize = 2.0f;

    style.ColorMarkerSize = 0.0f;
    style.ImageRounding = 0.0f;

    ImVec4* colors = style.Colors;

    ImVec4 bgDark = ImVec4(0.05f, 0.05f, 0.06f, 0.95f);
    ImVec4 bgMid = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    ImVec4 bgLight = ImVec4(0.15f, 0.15f, 0.16f, 1.00f);
    ImVec4 border = ImVec4(0.28f, 0.25f, 0.22f, 0.80f);

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

    colors[ImGuiCol_Separator] = ImVec4(0.32f, 0.28f, 0.24f, 0.60f);
    colors[ImGuiCol_SeparatorHovered] = accentHover;
    colors[ImGuiCol_SeparatorActive] = accent;

    colors[ImGuiCol_ResizeGrip] = ImVec4(accentDim.x, accentDim.y, accentDim.z, 0.50f);
    colors[ImGuiCol_ResizeGripHovered] = accent;
    colors[ImGuiCol_ResizeGripActive] = accentHover;

    colors[ImGuiCol_Tab] = bgMid;
    colors[ImGuiCol_TabHovered] = ImVec4(0.24f, 0.22f, 0.20f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.17f, 0.14f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline] = accent;
    colors[ImGuiCol_TabUnfocused] = bgMid;
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.16f, 0.14f, 0.12f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = accentDim;

    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.12f, 0.11f, 0.10f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = border;
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.25f, 0.22f, 0.19f, 0.60f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.03f);

    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = accentHover;
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.85f, 0.50f, 0.15f, 0.40f);
    colors[ImGuiCol_NavHighlight] = accent;
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.45f);
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

    // A new context has no window positions, the console rect is applied again
    g_lastWidth = 0;
    g_lastHeight = 0;
    g_appliedRect[0] = -1.0f;

    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    // Glyphs are rasterized on demand at the requested size, so the scaling can change at any time
    ImFontConfig bitmapConfig;
    bitmapConfig.SizePixels = 13.0f;
    bitmapConfig.PixelSnapH = true;
    g_fontBitmap = io.Fonts->AddFontDefaultBitmap(&bitmapConfig);

    ImFontConfig vectorConfig;
    vectorConfig.SizePixels = 13.0f;
    g_fontVector = io.Fonts->AddFontDefaultVector(&vectorConfig);

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
        g_fontBitmap = nullptr;
        g_fontVector = nullptr;
    }
}

inline void CheckDevice()
{
    IDirect3DDevice9* pDevice = GetDevice();

    if (!pDevice)
    {
        ShutdownImGui();
        g_pLastKnownDevice = nullptr;
        return;
    }

    if (pDevice != g_pLastKnownDevice)
    {
        ShutdownImGui();
        InitImGui(pDevice, g_hWnd);
        g_pLastKnownDevice = pDevice;
    }
}

// Input
// Fallback when the game window hook isn't installed, the game window procedure swallows the mouse messages
inline void UpdateMouseInputFallback()
{
    if (!g_imguiInitialized)
        return;

    ImGuiIO& io = ImGui::GetIO();
    POINT pt;
    if (GetCursorPos(&pt) && ScreenToClient(g_hWnd, &pt))
    {
        io.AddMousePosEvent((float)pt.x, (float)pt.y);
    }

    bool swapped = GetSystemMetrics(SM_SWAPBUTTON) != 0;
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, (GetAsyncKeyState(swapped ? VK_RBUTTON : VK_LBUTTON) & 0x8000) != 0);
    io.AddMouseButtonEvent(ImGuiMouseButton_Right, (GetAsyncKeyState(swapped ? VK_LBUTTON : VK_RBUTTON) & 0x8000) != 0);
    io.AddMouseButtonEvent(ImGuiMouseButton_Middle, (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0);
}

inline int InputCallbackStub(ImGuiInputTextCallbackData* data)
{
    if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
    {
        // Ctrl+Up/Down scrolls the output like the engine console
        if (ImGui::GetIO().KeyCtrl)
        {
            g_scrollLines += (data->EventKey == ImGuiKey_UpArrow) ? -1 : 1;
            return 0;
        }

        CycleHistory(data, (data->EventKey == ImGuiKey_UpArrow) ? -1 : 1);
    }
    else if (data->EventFlag == ImGuiInputTextFlags_CallbackAlways)
    {
        // Typing ends the history / completion navigation
        if (g_navState != NavState::Normal && g_navAppliedText != data->Buf)
        {
            EndNav();
        }

        if (g_completionRequest != CompletionRequest::None)
        {
            CompletionRequest request = g_completionRequest;
            g_completionRequest = CompletionRequest::None;
            ApplyCompletion(data, request);
        }

        if (g_moveCursorToEnd)
        {
            data->CursorPos = data->BufTextLen;
            data->SelectionStart = data->SelectionEnd = data->CursorPos;
            g_moveCursorToEnd = false;
        }

        TrackTextField(data);
    }

    return 0;
}

// Rendering
inline float GetScaledMargin(float baseMargin, float currentSize, float referenceSize = 1080.0f)
{
    return baseMargin * (currentSize / referenceSize);
}

inline ImVec4 WithAlpha(const ImVec4& color, float alpha)
{
    return ImVec4(color.x, color.y, color.z, alpha);
}

// Small colored label for the type of an entry
inline void DrawTag(const char* label, const ImVec4& color)
{
    ImVec2 textSize = ImGui::CalcTextSize(label);
    float padding = std::floor(4.0f * g_uiScale);
    ImVec2 size(textSize.x + padding * 2.0f, ImGui::GetTextLineHeight());
    ImVec2 pos = ImGui::GetCursorScreenPos();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImGui::GetColorU32(WithAlpha(color, 0.15f)), 2.0f * g_uiScale);
    drawList->AddText(ImVec2(pos.x + padding, pos.y), ImGui::GetColorU32(color), label);
    ImGui::Dummy(size);
}

// Long values are cut in the suggestions, the browser shows them in full
inline std::string ShortenValue(const std::string& value, size_t maxLength = 40)
{
    if (value.size() <= maxLength)
        return value;

    // Cut on a UTF-8 character boundary
    size_t cut = maxLength;
    while (cut > 0 && ((unsigned char)value[cut] & 0xC0) == 0x80) cut--;
    return value.substr(0, cut) + "...";
}

inline void UpdateSuggestions()
{
    std::string prefix;
    if (g_navState == NavState::Complete)
        return;

    std::string base, segment;
    if (g_inputActive && GetCompletionSegment(g_inputBuffer, base, segment))
    {
        prefix = segment;
    }

    if (prefix.empty())
    {
        g_suggestions.clear();
        g_suggestionPrefix.clear();
        return;
    }

    RefreshCandidates();

    if (prefix != g_suggestionPrefix)
    {
        g_suggestionPrefix = prefix;
        g_suggestions = FindCompletionMatches(prefix);

        // Nothing left to suggest once the name is complete
        if (g_suggestions.size() == 1 && _stricmp(g_suggestions[0].name.c_str(), prefix.c_str()) == 0)
        {
            g_suggestions.clear();
        }
    }
}

inline void RenderSuggestions(const ImVec2& inputMin, const ImVec2& inputMax)
{
    // While cycling with Tab the list is the one being cycled through
    const std::vector<ConsoleEntry>& suggestions = (g_navState == NavState::Complete) ? g_completionMatches : g_suggestions;
    if (suggestions.empty() || !g_inputActive)
        return;

    const int maxRows = 10;
    int count = (int)suggestions.size();
    int rows = std::min(count, maxRows);
    int selected = (g_navState == NavState::Complete) ? g_completionIndex : -1;
    int first = (selected >= 0) ? std::clamp(selected - maxRows / 2, 0, std::max(count - maxRows, 0)) : 0;

    ImGuiStyle& style = ImGui::GetStyle();
    float height = (rows + (count > rows ? 1 : 0)) * ImGui::GetTextLineHeightWithSpacing() + style.WindowPadding.y * 2.0f;
    bool below = inputMax.y + style.ItemSpacing.y + height <= ImGui::GetIO().DisplaySize.y;

    ImVec2 pos(inputMin.x, below ? inputMax.y + style.ItemSpacing.y : inputMin.y - style.ItemSpacing.y);
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.0f, below ? 0.0f : 1.0f));

    if (ImGui::BeginTooltip())
    {
        if (ImGui::BeginTable("##Suggestions", 3, ImGuiTableFlags_SizingFixedFit))
        {
            for (int i = first; i < first + rows; i++)
            {
                const ConsoleEntry& entry = suggestions[i];
                ImGui::TableNextRow();

                if (i == selected)
                {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(WithAlpha(g_accentColor, 0.30f)));
                }

                ImGui::TableSetColumnIndex(0);
                ImGui::PushStyleColor(ImGuiCol_Text, GetEntryColor(entry.type));
                ImGui::TextUnformatted(entry.name.c_str());
                ImGui::PopStyleColor();

                ImGui::TableSetColumnIndex(1);
                if (!entry.value.empty())
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, g_valueColor);
                    ImGui::TextUnformatted(ShortenValue(entry.value).c_str());
                    ImGui::PopStyleColor();
                }

                ImGui::TableSetColumnIndex(2);
                DrawTag(GetEntryTag(entry.type), GetEntryColor(entry.type));
            }

            ImGui::EndTable();
        }

        if (count > rows)
        {
            ImGui::TextDisabled("%d more, Ctrl+Tab to list them", count - rows);
        }

        ImGui::EndTooltip();
    }
}

inline void RenderBrowserTable(int tab, const std::vector<int>& indices)
{
    const std::vector<ConsoleEntry>& entries = g_browserEntries[tab];
    const bool commandTab = tab == BrowserTab_Commands;
    const int columns = (tab == BrowserTab_EngineVars) ? 3 : 2;

    ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable;
    if (!ImGui::BeginTable("##Entries", columns, flags))
        return;

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.55f);
    ImGui::TableSetupColumn(commandTab ? "Type" : "Value", ImGuiTableColumnFlags_WidthStretch, 0.45f);
    if (columns == 3)
    {
        ImGui::TableSetupColumn("Default", ImGuiTableColumnFlags_WidthStretch, 0.25f);
    }
    ImGui::TableHeadersRow();

    ImGuiListClipper clipper;
    clipper.Begin((int)indices.size());
    while (clipper.Step())
    {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
        {
            const ConsoleEntry& entry = entries[indices[row]];
            bool isCommand = IsCommandEntry(entry.type);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushID((int)entry.type);
            ImGui::PushID(entry.name.c_str());

            ImGui::PushStyleColor(ImGuiCol_Text, GetEntryColor(entry.type));
            bool activated = ImGui::Selectable(entry.name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick);
            ImGui::PopStyleColor();

            if (activated && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                if (isCommand)
                {
                    InsertIntoInput((entry.type == ConsoleEntryType::ServerCommand ? "serv " : "") + entry.name + " ");
                }
                else
                {
                    OpenEditModal(entry);
                }
            }

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay))
            {
                ImGui::SetTooltip(isCommand ? "Double-click to insert in the console" : "Double-click to edit, right-click for more");
            }

            if (ImGui::BeginPopupContextItem("EntryContext"))
            {
                if (ImGui::MenuItem("Insert in Console"))
                {
                    InsertIntoInput((entry.type == ConsoleEntryType::ServerCommand ? "serv " : "") + entry.name + " ");
                }

                if (!isCommand && ImGui::MenuItem("Edit..."))
                {
                    OpenEditModal(entry);
                }

                if (!entry.defaultValue.empty() && ImGui::MenuItem("Reset to Default", nullptr, false, entry.value != entry.defaultValue))
                {
                    ApplyVariableValue(entry, entry.defaultValue);
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Copy Name"))
                {
                    ImGui::SetClipboardText(entry.name.c_str());
                }

                if (!isCommand && ImGui::MenuItem("Copy Value"))
                {
                    ImGui::SetClipboardText(StripQuotes(entry.value).c_str());
                }

                ImGui::EndPopup();
            }

            ImGui::TableSetColumnIndex(1);
            if (isCommand)
            {
                DrawTag(GetEntryTag(entry.type), GetEntryColor(entry.type));
            }
            else
            {
                // Values changed from the default stand out
                bool modified = !entry.defaultValue.empty() && entry.value != entry.defaultValue;
                ImGui::PushStyleColor(ImGuiCol_Text, modified ? g_accentColor : g_valueColor);
                ImGui::TextUnformatted(entry.value.c_str());
                ImGui::PopStyleColor();
            }

            if (columns == 3)
            {
                ImGui::TableSetColumnIndex(2);
                ImGui::TextDisabled("%s", entry.defaultValue.empty() ? "-" : entry.defaultValue.c_str());
            }

            ImGui::PopID();
            ImGui::PopID();
        }
    }

    ImGui::EndTable();
}

inline void RenderBrowserWindow()
{
    ImGuiIO& io = ImGui::GetIO();
    float width = io.DisplaySize.x;
    float height = io.DisplaySize.y;

    ImGuiCond posCond = g_browserNeedsReset ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
    ImGui::SetNextWindowPos(ImVec2(width * 0.15f, height * 0.1f), posCond);
    ImGui::SetNextWindowSize(ImVec2(620 * g_uiScale, 440 * g_uiScale), posCond);
    ImGui::SetNextWindowBgAlpha(std::max(GetBackgroundAlpha(), 0.85f));

    if (g_browserNeedsReset)
    {
        g_browserNeedsReset = false;
    }

    g_browserFocused = false;

    if (ImGui::Begin("Console Browser", &g_showBrowserWindow))
    {
        g_browserFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

        // Refreshed periodically so the values stay live
        if (g_browserNeedsRefresh || GetTickCount64() - g_browserRefreshTime > 500)
        {
            RefreshBrowser();
        }

        ImGui::PushItemWidth(ImGui::GetFontSize() * 18.0f);
        ImGui::InputTextWithHint("##BrowserFilter", "Filter by name or value", g_browserFilter, sizeof(g_browserFilter), ImGuiInputTextFlags_CallbackAlways, TrackTextFieldCallback);
        ImGui::PopItemWidth();

        static const char* tabNames[BrowserTab_Count] = { "Commands", "Engine Vars", "Config Vars", "Server Vars" };

        if (ImGui::BeginTabBar("BrowserTabs"))
        {
            for (int tab = 0; tab < BrowserTab_Count; tab++)
            {
                std::vector<int> indices;
                const auto& entries = g_browserEntries[tab];
                for (int i = 0; i < (int)entries.size(); i++)
                {
                    if (ContainsCaseInsensitive(entries[i].name, g_browserFilter) || ContainsCaseInsensitive(entries[i].value, g_browserFilter))
                    {
                        indices.push_back(i);
                    }
                }

                char label[64];
                snprintf(label, sizeof(label), "%s (%d)###%s", tabNames[tab], (int)indices.size(), tabNames[tab]);

                if (ImGui::BeginTabItem(label))
                {
                    if (tab == BrowserTab_ServerVars && entries.empty())
                    {
                        ImGui::TextDisabled("No server is running.");
                    }
                    else
                    {
                        RenderBrowserTable(tab, indices);
                    }

                    ImGui::EndTabItem();
                }
            }

            ImGui::EndTabBar();
        }

        RenderEditModal();
    }

    ImGui::End();
}

inline ImVec4 GetLineColor(const ConsoleLine& line)
{
    switch (line.type)
    {
        case ConsoleLineType::Command: return g_commandLineColor;
        case ConsoleLineType::Header:  return g_accentColor;
        case ConsoleLineType::Info:    return g_infoLineColor;
        case ConsoleLineType::Warning: return g_warningLineColor;
        case ConsoleLineType::Error:   return g_errorLineColor;
        default: break;
    }

    if (line.color != 0)
        return ImGui::ColorConvertU32ToFloat4(line.color);

    return ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

// Places the next toolbar item on a new line when it doesn't fit
inline void ToolbarSameLine(float nextWidth)
{
    ImGui::SameLine();
    if (ImGui::GetContentRegionAvail().x < nextWidth)
    {
        ImGui::NewLine();
    }
}

// Toggle button of the toolbar, with an optional colored dot and count
inline bool ToolbarChip(const char* id, const char* label, const char* countText, const ImVec4* dotColor, bool active, bool accent = false)
{
    ImGuiStyle& style = ImGui::GetStyle();
    float height = ImGui::GetFrameHeight();
    float radius = dotColor ? std::floor(ImGui::GetFontSize() * 0.25f) : 0.0f;
    ImVec2 labelSize = ImGui::CalcTextSize(label);
    ImVec2 countSize = countText ? ImGui::CalcTextSize(countText) : ImVec2(0.0f, 0.0f);

    float width = style.FramePadding.x * 2.0f + labelSize.x;
    if (dotColor) width += radius * 2.0f + style.ItemInnerSpacing.x;
    if (countText) width += style.ItemInnerSpacing.x + countSize.x;

    ToolbarSameLine(width);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    bool pressed = ImGui::InvisibleButton(id, ImVec2(width, height));
    bool hovered = ImGui::IsItemHovered();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 end(pos.x + width, pos.y + height);

    // On: filled like a button, accent tint for the toggles. Off: outline only
    ImU32 background;
    ImU32 border;
    if (active && accent)
    {
        background = ImGui::GetColorU32(WithAlpha(g_accentColor, hovered ? 0.30f : 0.20f));
        border = ImGui::GetColorU32(WithAlpha(g_accentColor, 0.60f));
    }
    else if (active)
    {
        background = ImGui::GetColorU32(hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
        border = ImGui::GetColorU32(ImGuiCol_Border);
    }
    else
    {
        background = hovered ? ImGui::GetColorU32(WithAlpha(ImGui::GetStyleColorVec4(ImGuiCol_Button), 0.60f)) : 0;
        border = ImGui::GetColorU32(WithAlpha(ImGui::GetStyleColorVec4(ImGuiCol_Border), 0.45f));
    }

    if (background)
        drawList->AddRectFilled(pos, end, background, style.FrameRounding);
    drawList->AddRect(pos, end, border, style.FrameRounding, 0, std::max(style.FrameBorderSize, 1.0f));

    float x = pos.x + style.FramePadding.x;
    float textY = pos.y + style.FramePadding.y;

    if (dotColor)
    {
        drawList->AddCircleFilled(ImVec2(x + radius, pos.y + height * 0.5f), radius, ImGui::GetColorU32(WithAlpha(*dotColor, active ? 1.0f : 0.35f)));
        x += radius * 2.0f + style.ItemInnerSpacing.x;
    }

    drawList->AddText(ImVec2(x, textY), ImGui::GetColorU32(active ? ImGuiCol_Text : ImGuiCol_TextDisabled), label);
    x += labelSize.x + style.ItemInnerSpacing.x;

    if (countText)
    {
        drawList->AddText(ImVec2(x, textY), ImGui::GetColorU32(ImGuiCol_TextDisabled), countText);
    }

    return pressed;
}

inline void RenderToolbar()
{
    if (ImGui::Button("Browse"))
    {
        g_showBrowserWindow = !g_showBrowserWindow;
        g_browserNeedsRefresh = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear"))
    {
        ClearOutputLines();
    }

    ImGui::SameLine();
    if (ImGui::Button("Copy"))
    {
        CopyOutputToClipboard();
    }
    ImGui::SetItemTooltip(HasLineSelection() ? "Copy the selected lines" : "Copy the visible lines");

    float filterWidth = ImGui::GetFontSize() * 14.0f;
    ToolbarSameLine(filterWidth);
    ImGui::PushItemWidth(filterWidth);
    ImGui::InputTextWithHint("##OutputFilter", "Filter output", g_outputFilter, sizeof(g_outputFilter), ImGuiInputTextFlags_CallbackAlways, TrackTextFieldCallback);
    ImGui::PopItemWidth();

    // Severity filters with the number of lines of each
    struct SeverityChip { const char* id; const char* label; int bit; int count; const ImVec4* color; };
    const SeverityChip chips[] =
    {
        { "##Log", "Log", OutputFilter_Log, g_countLog, &g_logDotColor },
        { "##Warnings", "Warnings", OutputFilter_Warnings, g_countWarnings, &g_warningLineColor },
        { "##Errors", "Errors", OutputFilter_Errors, g_countErrors, &g_errorLineColor },
    };

    for (const auto& chip : chips)
    {
        char count[16];
        snprintf(count, sizeof(count), "%d", chip.count);
        if (ToolbarChip(chip.id, chip.label, count, chip.color, (g_outputMask & chip.bit) != 0))
        {
            g_outputMask ^= chip.bit;
        }
        ImGui::SetItemTooltip("Show or hide the %s lines", chip.bit == OutputFilter_Log ? "log" : (chip.bit == OutputFilter_Warnings ? "warning" : "error"));
    }

    if (ToolbarChip("##Time", "Time", nullptr, nullptr, g_showTimestamps, true))
    {
        g_showTimestamps = !g_showTimestamps;
    }
    ImGui::SetItemTooltip("Show the time of each line");

    if (ToolbarChip("##Wrap", "Wrap", nullptr, nullptr, g_wordWrap, true))
    {
        g_wordWrap = !g_wordWrap;
    }
    ImGui::SetItemTooltip("Wrap the long lines");

    size_t lineCount;
    {
        OutputLock lock;
        lineCount = g_outputLines.size();
    }

    char countText[32];
    snprintf(countText, sizeof(countText), "%d / %d", (int)lineCount, GetBufferLength());
    float countWidth = ImGui::CalcTextSize(countText).x;

    ToolbarSameLine(countWidth);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(ImGui::GetContentRegionAvail().x - countWidth, 0.0f));
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("%s", countText);
}

// Server indicator in the title bar, the server commands and variables can be typed while a server runs
inline void RenderTitleStatus()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    float titleHeight = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;

    bool serverRunning = GetServerManager() != 0;
    const char* label = "Server";
    ImVec2 labelSize = ImGui::CalcTextSize(label);
    float radius = std::floor(ImGui::GetFontSize() * 0.25f);

    // Left of the close button
    float right = windowPos.x + windowSize.x - style.FramePadding.x - ImGui::GetFontSize() - style.ItemInnerSpacing.x * 3.0f;
    float left = right - labelSize.x - radius * 2.0f - style.ItemInnerSpacing.x;
    float textY = windowPos.y + (titleHeight - labelSize.y) * 0.5f;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->PushClipRect(windowPos, ImVec2(windowPos.x + windowSize.x, windowPos.y + titleHeight), false);
    drawList->AddCircleFilled(ImVec2(left + radius, windowPos.y + titleHeight * 0.5f), radius, ImGui::GetColorU32(serverRunning ? g_serverOnColor : WithAlpha(g_logDotColor, 0.45f)));
    drawList->AddText(ImVec2(left + radius * 2.0f + style.ItemInnerSpacing.x, textY), ImGui::GetColorU32(serverRunning ? ImGuiCol_Text : ImGuiCol_TextDisabled), label);
    drawList->PopClipRect();

    if (ImGui::IsMouseHoveringRect(ImVec2(left, windowPos.y), ImVec2(right, windowPos.y + titleHeight), false))
    {
        ImGui::SetTooltip(serverRunning ? "A server is running, its commands and variables can be typed directly" : "No server is running");
    }
}

// One output line, drawn over an invisible button that handles the selection
inline void RenderOutputRow(ConsoleLine& line, float height, float padding, float textX, float wrapWidth, float hitY)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO();

    float textWidth = wrapWidth > 0.0f ? wrapWidth : ImGui::CalcTextSize(line.text.c_str()).x;
    float width = std::max(ImGui::GetContentRegionAvail().x, textX + textWidth + style.ItemSpacing.x);

    ImGui::PushID((int)line.id);
    ImGui::InvisibleButton("##Line", ImVec2(width, height));
    ImGui::PopID();

    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    bool hovered = ImGui::IsItemHovered();

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        if (io.KeyShift && HasLineSelection())
        {
            g_selectionActive = line.id;
        }
        else
        {
            g_selectionAnchor = g_selectionActive = line.id;
        }

        g_selectionDragging = true;

        // Double-click on a typed command puts it back in the input
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && line.type == ConsoleLineType::Command && line.text.rfind("> ", 0) == 0)
        {
            InsertIntoInput(line.text.substr(2));
        }
    }
    else if (g_selectionDragging && hitY >= min.y && hitY < max.y)
    {
        g_selectionActive = line.id;
    }

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !IsLineSelected(line.id))
    {
        g_selectionAnchor = g_selectionActive = line.id;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 clipMin = drawList->GetClipRectMin();
    ImVec2 clipMax = drawList->GetClipRectMax();
    ImVec2 rowMin(std::max(min.x, clipMin.x), min.y);
    ImVec2 rowMax(std::min(max.x, clipMax.x), max.y);

    // Background: selection, then the severity tint, then the hover
    if (IsLineSelected(line.id))
        drawList->AddRectFilled(rowMin, rowMax, ImGui::GetColorU32(WithAlpha(g_accentColor, 0.24f)));
    else if (line.type == ConsoleLineType::Error)
        drawList->AddRectFilled(rowMin, rowMax, ImGui::GetColorU32(WithAlpha(g_errorLineColor, 0.07f)));
    else if (line.type == ConsoleLineType::Warning)
        drawList->AddRectFilled(rowMin, rowMax, ImGui::GetColorU32(WithAlpha(g_warningLineColor, 0.05f)));
    else if (hovered)
        drawList->AddRectFilled(rowMin, rowMax, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.03f)));

    // Severity marker in the gutter
    float gutter = std::max(2.0f, std::floor(2.0f * g_uiScale));
    const ImVec4* marker = nullptr;
    if (line.type == ConsoleLineType::Error) marker = &g_errorLineColor;
    else if (line.type == ConsoleLineType::Warning) marker = &g_warningLineColor;
    else if (line.type == ConsoleLineType::Command) marker = &g_commandLineColor;

    if (marker)
    {
        drawList->AddRectFilled(ImVec2(min.x, min.y), ImVec2(min.x + gutter, max.y), ImGui::GetColorU32(*marker));
    }

    if (g_showTimestamps)
    {
        char timestamp[16];
        FormatTimestamp(line.time, timestamp, sizeof(timestamp));
        drawList->AddText(ImVec2(min.x + gutter + style.ItemSpacing.x, min.y + padding), ImGui::GetColorU32(g_timestampColor), timestamp);
    }

    ImU32 color = ImGui::GetColorU32(GetLineColor(line));
    const char* text = line.text.c_str();
    const char* textEnd = text + line.text.size();
    ImVec2 textPos(min.x + textX, min.y + padding);

    if (line.split > 0)
    {
        // Help rows: the second column wraps under itself
        const char* middle = text + line.split;
        float firstWidth = ImGui::CalcTextSize(text, middle).x;
        ImU32 bright = ImGui::GetColorU32(ImGuiCol_Text);
        ImU32 dim = ImGui::GetColorU32(g_helpTextColor);

        drawList->AddText(textPos, line.dimFirst ? dim : bright, text, middle);
        drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(textPos.x + firstWidth, textPos.y), line.dimFirst ? bright : dim, middle, textEnd, wrapWidth > 0.0f ? std::max(wrapWidth - firstWidth, 20.0f) : 0.0f);
    }
    else if (wrapWidth > 0.0f)
    {
        drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), textPos, color, text, textEnd, wrapWidth);
    }
    else
    {
        drawList->AddText(textPos, color, text, textEnd);
    }
}

inline void RenderOutput()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO();

    // Separator + input line
    float inputFooterHeight = ImGui::GetFrameHeightWithSpacing() + style.ItemSpacing.y + 1.0f;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.22f));
    ImGui::BeginChild("OutputRegion", ImVec2(0, -inputFooterHeight), ImGuiChildFlags_Borders, g_wordWrap ? 0 : ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PopStyleColor();

    // Keyboard scrolling: PgUp/PgDn, Ctrl+PgUp/PgDn and Ctrl+Up/Down like the engine console
    bool atBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f;
    bool scrolled = g_scrollLines != 0 || g_scrollPages != 0 || g_scrollEdge != 0;
    float lineHeight = ImGui::GetTextLineHeightWithSpacing();

    if (g_scrollLines != 0)
    {
        ImGui::SetScrollY(ImGui::GetScrollY() + g_scrollLines * lineHeight);
        g_scrollLines = 0;
    }

    if (g_scrollPages != 0)
    {
        ImGui::SetScrollY(ImGui::GetScrollY() + g_scrollPages * (ImGui::GetWindowHeight() - lineHeight * 2.0f));
        g_scrollPages = 0;
    }

    if (g_scrollEdge != 0)
    {
        if (g_scrollEdge < 0)
            ImGui::SetScrollY(0.0f);
        else
            g_scrollToBottom = true;

        g_scrollEdge = 0;
    }

    ImVec2 regionMin = ImGui::GetWindowPos();
    ImVec2 regionMax(regionMin.x + ImGui::GetWindowWidth(), regionMin.y + ImGui::GetWindowHeight());

    // Releasing the button ends the selection, the keyboard goes back to the input
    if (g_selectionDragging && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        g_selectionDragging = false;
        g_focusInput = true;
        g_moveCursorToEnd = true;
    }

    // While dragging past the top or the bottom the nearest line keeps extending the selection
    float hitY = std::clamp(io.MousePos.y, regionMin.y + 1.0f, regionMax.y - 2.0f);

    const float padding = std::max(1.0f, std::floor(1.0f * g_uiScale));
    const float rowHeight = ImGui::GetTextLineHeight() + padding * 2.0f;
    const float gutter = std::max(2.0f, std::floor(2.0f * g_uiScale));
    const float textX = gutter + style.ItemSpacing.x + (g_showTimestamps ? ImGui::CalcTextSize("00:00:00").x + style.ItemSpacing.x * 2.0f : 0.0f);

    bool noLines = false;
    bool allFiltered = false;
    int selectedCount = 0;

    {
        OutputLock lock;
        RebuildVisibleRows();

        if (g_copySelectionRequest)
        {
            CopyRowsLocked(true);
            g_copySelectionRequest = false;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, 0.0f));

        if (!g_wordWrap)
        {
            ImGuiListClipper clipper;
            clipper.Begin((int)g_visibleRows.size(), rowHeight);
            while (clipper.Step())
            {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                {
                    RenderOutputRow(g_outputLines[g_visibleRows[row]], rowHeight, padding, textX, 0.0f, hitY);
                }
            }
        }
        else
        {
            // Wrapped lines have different heights, only the lines in view are drawn
            float wrapWidth = std::max(ImGui::GetContentRegionAvail().x - textX - style.ItemSpacing.x, 40.0f);
            float fontSize = ImGui::GetFontSize();
            int count = (int)g_visibleRows.size();

            g_wrapOffsets.resize(count + 1);
            g_wrapOffsets[0] = 0.0f;
            for (int i = 0; i < count; i++)
            {
                ConsoleLine& line = g_outputLines[g_visibleRows[i]];
                if (line.wrapWidth != wrapWidth || line.wrapFontSize != fontSize)
                {
                    const char* text = line.text.c_str();
                    const char* textEnd = text + line.text.size();
                    if (line.split > 0)
                    {
                        float firstWidth = ImGui::CalcTextSize(text, text + line.split).x;
                        line.wrapHeight = ImGui::CalcTextSize(text + line.split, textEnd, false, std::max(wrapWidth - firstWidth, 20.0f)).y;
                    }
                    else
                    {
                        line.wrapHeight = ImGui::CalcTextSize(text, textEnd, false, wrapWidth).y;
                    }
                    line.wrapHeight = std::max(line.wrapHeight, ImGui::GetTextLineHeight());
                    line.wrapWidth = wrapWidth;
                    line.wrapFontSize = fontSize;
                }

                g_wrapOffsets[i + 1] = g_wrapOffsets[i] + line.wrapHeight + padding * 2.0f;
            }

            float startY = ImGui::GetCursorPosY();
            float viewTop = ImGui::GetScrollY() - startY;
            float viewBottom = viewTop + ImGui::GetWindowHeight();

            int first = (int)(std::upper_bound(g_wrapOffsets.begin(), g_wrapOffsets.end(), viewTop) - g_wrapOffsets.begin()) - 1;
            first = std::clamp(first, 0, std::max(count - 1, 0));

            for (int i = first; i < count && g_wrapOffsets[i] < viewBottom; i++)
            {
                ImGui::SetCursorPosY(startY + g_wrapOffsets[i]);
                RenderOutputRow(g_outputLines[g_visibleRows[i]], g_wrapOffsets[i + 1] - g_wrapOffsets[i], padding, textX, wrapWidth, hitY);
            }

            // The content covers every line so the scrollbar matches
            ImGui::SetCursorPosY(startY + g_wrapOffsets[count]);
            ImGui::Dummy(ImVec2(0.0f, 0.0f));
        }

        ImGui::PopStyleVar();

        noLines = g_outputLines.empty();
        allFiltered = !noLines && g_visibleRows.empty();

        if (HasLineSelection())
        {
            for (int row : g_visibleRows)
            {
                if (IsLineSelected(g_outputLines[row].id))
                    selectedCount++;
            }
        }
    }

    if (allFiltered)
    {
        ImGui::TextDisabled("No line matches the filters.");
    }

    // Scrolls while a selection is dragged past the edges
    if (g_selectionDragging)
    {
        if (io.MousePos.y < regionMin.y)
            ImGui::SetScrollY(ImGui::GetScrollY() - rowHeight);
        else if (io.MousePos.y > regionMax.y)
            ImGui::SetScrollY(ImGui::GetScrollY() + rowHeight);
    }

    // Clicking the empty part clears the selection and gives the focus back to the input
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
    {
        ClearLineSelection();
        g_focusInput = true;
        g_moveCursorToEnd = true;
    }

    if (ImGui::BeginPopupContextWindow("OutputContext"))
    {
        char copyLabel[48];
        snprintf(copyLabel, sizeof(copyLabel), selectedCount > 1 ? "Copy %d Lines" : "Copy Line", selectedCount);

        if (ImGui::MenuItem(copyLabel, "Ctrl+C", false, selectedCount > 0))
        {
            OutputLock lock;
            CopyRowsLocked(true);
        }

        if (ImGui::MenuItem("Copy All", nullptr, false, !noLines))
        {
            OutputLock lock;
            CopyRowsLocked(false);
        }

        if (ImGui::MenuItem("Select All", nullptr, false, !noLines && !allFiltered))
        {
            OutputLock lock;
            RebuildVisibleRows();
            if (!g_visibleRows.empty())
            {
                g_selectionAnchor = g_outputLines[g_visibleRows.front()].id;
                g_selectionActive = g_outputLines[g_visibleRows.back()].id;
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Clear", nullptr, false, !noLines))
        {
            ClearOutputLines();
        }

        ImGui::EndPopup();
    }

    // Stays at the bottom for new lines unless scrolled up
    if (g_scrollToBottom || (atBottom && !scrolled && !g_selectionDragging))
    {
        ImGui::SetScrollHereY(1.0f);
        g_scrollToBottom = false;
    }

    ImGui::EndChild();
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

    float rect[4];
    GetConsoleRect(rect);

    if (resolutionChanged || memcmp(rect, g_appliedRect, sizeof(rect)) != 0)
    {
        memcpy(g_appliedRect, rect, sizeof(rect));

        float margin = GetScaledMargin(10.0f, height);
        ImVec2 pos(rect[0] * width + margin, rect[1] * height + margin);
        ImVec2 size((rect[2] - rect[0]) * width - margin * 2.0f, (rect[3] - rect[1]) * height - margin * 2.0f);

        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(400 * g_uiScale, 200 * g_uiScale), ImVec2(width, height));
    ImGui::SetNextWindowBgAlpha(GetBackgroundAlpha());

    if (g_justOpened)
    {
        ImGui::SetNextWindowFocus();
    }

    g_inputActive = false;

    bool windowOpen = true;
    if (ImGui::Begin(g_title.c_str(), &windowOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        RenderTitleStatus();
        RenderToolbar();
        RenderOutput();

        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(g_accentColor, ">");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);

        if (g_focusInput)
        {
            ImGui::SetKeyboardFocusHere();
            g_focusInput = false;
        }

        ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_EscapeClearsAll;
        if (ImGui::InputTextWithHint("##Input", "Type a command or a variable, Tab completes, 'help' lists the commands", g_inputBuffer, sizeof(g_inputBuffer), inputFlags, InputCallbackStub, nullptr))
        {
            if (g_inputBuffer[0] != '\0')
            {
                ExecuteCommandInternal(g_inputBuffer);
                g_inputBuffer[0] = '\0';
            }

            EndNav();
            g_focusInput = true;
        }

        g_inputActive = ImGui::IsItemActive();
        ImVec2 inputMin = ImGui::GetItemRectMin();
        ImVec2 inputMax = ImGui::GetItemRectMax();
        ImGui::PopItemWidth();

        UpdateSuggestions();
        RenderSuggestions(inputMin, inputMax);

        if (g_justOpened)
        {
            g_focusInput = true;
            g_justOpened = false;
        }
    }

    ImGui::End();

    if (!windowOpen)
    {
        CloseConsole();
    }
}

// Tab, PgUp/PgDn and Escape go to the console unless the browser, a popup or another text field has the keyboard
inline void UpdateKeyboardOwner()
{
    g_consoleHasKeyboard = g_visible && !g_browserFocused && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopup) && (!ImGui::IsAnyItemActive() || g_inputActive);
}

// Overlays
// Panels of the debug tools drawn over the game, also while the console is closed
struct ConsoleOverlay
{
    bool (*isActive)();
    void (*render)();
};

inline std::vector<ConsoleOverlay> g_overlays;
inline std::vector<void (*)()> g_updateCallbacks;

inline bool HasActiveOverlay()
{
    for (const auto& overlay : g_overlays)
    {
        if (overlay.isActive())
            return true;
    }

    return false;
}

inline void RenderOverlays()
{
    for (const auto& overlay : g_overlays)
    {
        if (overlay.isActive())
        {
            overlay.render();
        }
    }
}

// Panel in a corner of the screen (0 top left, 1 top right, 2 bottom left, 3 bottom right), it never takes the mouse or the keyboard
inline bool BeginOverlayPanel(const char* name, int corner, const char* title)
{
    ImGuiIO& io = ImGui::GetIO();
    const float margin = 12.0f * g_uiScale;
    const bool right = (corner & 1) != 0;
    const bool bottom = (corner & 2) != 0;

    ImGui::SetNextWindowPos(ImVec2(right ? io.DisplaySize.x - margin : margin, bottom ? io.DisplaySize.y - margin : margin), ImGuiCond_Always, ImVec2(right ? 1.0f : 0.0f, bottom ? 1.0f : 0.0f));
    ImGui::SetNextWindowBgAlpha(0.80f);

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin(name, nullptr, flags))
    {
        ImGui::End();
        return false;
    }

    // Stays under the console and the browser
    ImGui::BringWindowToDisplayBack(ImGui::GetCurrentWindow());

    ImGui::PushStyleColor(ImGuiCol_Text, g_accentColor);
    ImGui::TextUnformatted(title);
    ImGui::PopStyleColor();
    ImGui::Separator();
    return true;
}

inline void EndOverlayPanel()
{
    ImGui::End();
}

inline void Render(IDirect3DDevice9* pDevice)
{
    if (g_needsScaleRebuild)
    {
        RebuildImGuiForScale();
    }

    ImGuiIO& io = ImGui::GetIO();
    if (g_visible)
    {
        io.ConfigFlags &= ~(ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange);
        ShowConsoleCursor();

        if (g_addresses.cursorLockAddr)
        {
            MemoryHelper::WriteMemory<DWORD>(g_addresses.cursorLockAddr, 0);
        }
    }
    else
    {
        // Only the overlays are drawn, the game keeps the mouse and its cursor
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange;
    }

    // The DX9 backend saves and restores the device state around its own rendering
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();

    if (g_visible && !g_mouseMessagesSeen)
    {
        UpdateMouseInputFallback();
    }

    ImGui::NewFrame();
    ImGui::PushFont(GetConsoleFont(), GetFontPixelSize());

    RenderOverlays();

    if (g_visible)
    {
        // Short fade when the console opens
        float fade = std::clamp((float)(GetTickCount64() - g_openTime) / 120.0f, 0.0f, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f - (1.0f - fade) * (1.0f - fade));

        g_textFieldFrame = TextFieldState();

        RenderConsoleWindow();
        if (g_showBrowserWindow)
        {
            RenderBrowserWindow();
        }
        else
        {
            g_browserFocused = false;
        }

        UpdateKeyboardOwner();
        g_textField = g_textFieldFrame;

        ImGui::PopStyleVar();
    }

    ImGui::PopFont();
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

// Public API
namespace Console
{
    inline void Init(DWORD devicePtrAddr, HWND hWnd, const char* title, const ConsoleSettings& settings)
    {
        g_devicePtrAddr = devicePtrAddr;
        g_hWnd = hWnd;
        g_title = std::string(title) + " Console";
        g_settings = settings;
        g_highResScalingEnabled = settings.highResScaling;
        g_loggingEnabled = settings.logging;

        // Windows virtual-key code, Home when invalid
        g_toggleKey = (settings.toggleKey > 0 && settings.toggleKey < 256) ? (UINT)settings.toggleKey : VK_HOME;

        g_historyFilename = GenerateFilename(g_title, "_history.txt");
        LoadHistory();

        if (g_loggingEnabled)
        {
            std::string filename = GenerateLogFilename(g_title);
            fopen_s(&g_logFile, filename.c_str(), "w");
        }
    }

    inline void InitAddresses(const ConsoleAddresses& addresses)
    {
        g_addresses = addresses;
        RegisterMissingConsoleVars();
        CaptureEngineVarDefaults();
    }

    // Called from the main loop when the console opens or closes
    inline void SetStateCallback(bool (*callback)(bool open))
    {
        g_stateCallback = callback;
    }

    inline void Shutdown()
    {
        if (g_cursorShownByUs)
        {
            HideConsoleCursor();
        }

        ShutdownImGui();

        {
            OutputLock lock;
            if (g_logFile)
            {
                fclose(g_logFile);
                g_logFile = nullptr;
            }

            g_outputLines.clear();
            g_outputGeneration++;
        }

        g_devicePtrAddr = 0;
        g_hWnd = nullptr;
        g_pDevice = nullptr;
        g_pLastKnownDevice = nullptr;
        g_visible = false;
        g_inReset = false;
        g_commandHistory.clear();
        g_pendingCommands.clear();
        g_candidates.clear();
        g_suggestions.clear();
        for (auto& entries : g_browserEntries) entries.clear();
        g_inputBuffer[0] = '\0';
        ClearLineSelection();
        EndNav();
    }

    inline void Toggle()
    {
        if (!g_visible)
        {
            OpenConsole();
        }
        else
        {
            CloseConsole();
        }
    }

    inline bool Update()
    {
        CheckDevice();

        // The engine console state exists once the client is initialized, the settings are applied through it
        if (!g_engineReady)
        {
            DWORD state = GetClientManager();
            if (state)
            {
                g_engineReady = true;
                ResolveEngineVars(state);

                RunConsoleString(state, "ConsoleBufferLen " + std::to_string(std::max(g_settings.bufferLen, 10)));
                RunConsoleString(state, "ConsoleHistoryLen " + std::to_string(std::max(g_settings.historyLen, 5)));
                RunConsoleString(state, "ConsoleAlpha " + FormatFloat(std::clamp(g_settings.alpha, 0.0f, 1.0f)));
                RunConsoleString(state, "MoveConsole " + g_settings.position);
            }
        }

        if (g_engineReady && !g_pendingCommands.empty())
        {
            std::vector<PendingCommand> commands;
            commands.swap(g_pendingCommands);

            for (const auto& command : commands)
            {
                HandleConsoleCommand(command);
            }

            g_candidatesDirty = true;
            g_browserNeedsRefresh = true;
        }

        // The debug tools update once per frame, from the main loop like the commands
        if (g_engineReady)
        {
            for (auto callback : g_updateCallbacks)
            {
                callback();
            }
        }

        // The game input follows the console right away, without waiting for a window message
        if (g_stateCallback && g_notifiedOpen != g_visible && g_stateCallback(g_visible))
        {
            g_notifiedOpen = g_visible;
        }

        return g_visible;
    }

    // Returns true when the message is consumed by the console
    inline bool HandleKeyMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            {
                const UINT vk = (UINT)wParam & 0xFF;
                g_consumedKeys[vk] = false;

                if (IsToggleKey(wParam) && (msg == WM_KEYDOWN || (lParam & (1 << 29)) == 0) && !(g_visible && ToggleKeyEditsText(vk)))
                {
                    MSG charMsg;
                    while (PeekMessageW(&charMsg, nullptr, WM_CHAR, WM_DEADCHAR, PM_REMOVE)) {}

                    if (MapVirtualKeyA(vk, MAPVK_VK_TO_CHAR) & 0x80000000)
                    {
                        ClearDeadKeyState();
                    }

                    // Ignore the auto-repeat
                    if ((lParam & (1 << 30)) == 0)
                    {
                        Toggle();
                    }

                    g_consumedKeys[vk] = true;
                    return true;
                }

                if (msg == WM_KEYDOWN && vk == VK_ESCAPE)
                {
                    g_closeOnEscapeRelease = false;
                }

                if (msg != WM_KEYDOWN || !g_visible || !g_consoleHasKeyboard)
                    return false;

                bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

                switch (vk)
                {
                    case 'C':
                        // Ctrl+C copies the selected lines, unless text is selected in the input
                        if (!ctrl || !HasLineSelection() || g_textField.selection)
                            return false;

                        g_copySelectionRequest = true;
                        break;

                    case VK_TAB:
                        g_completionRequest = ctrl ? CompletionRequest::List : (shift ? CompletionRequest::Previous : CompletionRequest::Next);
                        g_focusInput = !g_inputActive;
                        break;

                    case VK_PRIOR:
                        if (ctrl) g_scrollEdge = -1;
                        else g_scrollPages--;
                        break;

                    case VK_NEXT:
                        if (ctrl) g_scrollEdge = 1;
                        else g_scrollPages++;
                        break;

                    case VK_ESCAPE:
                        if (HasLineSelection())
                        {
                            ClearLineSelection();
                        }
                        else if (g_inputBuffer[0] == '\0')
                        {
                            g_closeOnEscapeRelease = true;
                        }
                        else if (!g_inputActive)
                        {
                            g_inputBuffer[0] = '\0';
                            EndNav();
                            g_focusInput = true;
                        }
                        else
                        {
                            // The input clears itself
                            return false;
                        }
                        break;

                    default:
                        return false;
                }

                g_consumedKeys[vk] = true;
                return true;
            }

            // Only the releases of the key presses consumed above
            case WM_KEYUP:
            case WM_SYSKEYUP:
            {
                const UINT vk = (UINT)wParam & 0xFF;
                if (!g_consumedKeys[vk])
                    return false;

                g_consumedKeys[vk] = false;

                if (vk == VK_ESCAPE && g_closeOnEscapeRelease)
                {
                    g_closeOnEscapeRelease = false;
                    if (g_visible)
                    {
                        CloseConsole();
                    }
                }

                return true;
            }

            case WM_CHAR:
                return g_visible && wParam == '\t';

            // The releases are lost when the focus goes away
            case WM_KILLFOCUS:
                memset(g_consumedKeys, 0, sizeof(g_consumedKeys));
                g_closeOnEscapeRelease = false;
                return false;

            case WM_ACTIVATEAPP:
                if (!wParam)
                {
                    memset(g_consumedKeys, 0, sizeof(g_consumedKeys));
                    g_closeOnEscapeRelease = false;
                }
                return false;
        }

        return false;
    }

    // Window messages forwarded to ImGui while the console is open
    inline LRESULT ForwardInput(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST)
        {
            g_mouseMessagesSeen = true;
        }

        return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
    }

    inline void OnWindowReactivated()
    {
        if (g_visible)
        {
            g_cursorShownByUs = false;
            ShowConsoleCursor();
        }
    }

    inline void OnWindowDestroyed()
    {
        HideConsoleCursor();
        ShutdownImGui();
        g_pLastKnownDevice = nullptr;
    }

    inline void OnPresent()
    {
        if (!g_imguiInitialized || g_inReset || (!g_visible && !HasActiveOverlay()))
            return;

        IDirect3DDevice9* pDevice = GetDevice();
        if (!pDevice || pDevice != g_pDevice)
            return;

        if (FAILED(pDevice->BeginScene()))
            return;

        IDirect3DSurface9* pBackBuffer = nullptr;
        IDirect3DSurface9* pTarget = nullptr;
        pDevice->GetRenderTarget(0, &pTarget);

        bool swapTarget = SUCCEEDED(pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer)) && pBackBuffer != pTarget;
        if (swapTarget)
        {
            pDevice->SetRenderTarget(0, pBackBuffer);
        }

        Render(pDevice);

        if (swapTarget && pTarget)
        {
            pDevice->SetRenderTarget(0, pTarget);
        }

        if (pBackBuffer) pBackBuffer->Release();
        if (pTarget) pTarget->Release();

        pDevice->EndScene();
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

    // Called from the engine console print, possibly from another thread
    inline void AddOutput(const char* text, unsigned int color)
    {
        ImU32 lineColor = ConvertEngineColor(color);
        const char* start = text;

        // Multi-line messages are split like the engine does, without the trailing newline
        for (;;)
        {
            const char* newline = strchr(start, '\n');
            bool last = !newline || !newline[1];
            size_t length = newline ? (size_t)(newline - start) : strlen(start);

            std::string line = AnsiToUtf8(start, length);
            AddLine(line, lineColor ? ConsoleLineType::Output : ClassifyLine(line), lineColor);

            if (last)
                break;

            start = newline + 1;
        }
    }

    inline void ClearOutput() { ClearOutputLines(); }
    inline bool IsInitialized() { return g_imguiInitialized; }

    // Debug tools
    inline void AddUpdateCallback(void (*callback)())
    {
        g_updateCallbacks.push_back(callback);
    }

    inline void AddOverlay(bool (*isActive)(), void (*render)())
    {
        g_overlays.push_back({ isActive, render });
    }

    inline void Print(const std::string& text, ConsoleLineType type = ConsoleLineType::Info)
    {
        AddLine(text, type);
    }

    inline void PrintRow(const std::string& label, const std::string& value, size_t width)
    {
        AddHelpRow(label, value, width, true);
    }

    inline void PrintHeader(const std::string& text)
    {
        AddLine(text, ConsoleLineType::Header);
    }

    // Replaces the function of a console program the game registered, returns the previous one or 0 when it doesn't exist
    inline DWORD ReplaceProgram(const char* name, DWORD function)
    {
        DWORD state = GetClientManager();
        if (!state || !function)
            return 0;

        DWORD head = state + 0x10;
        for (DWORD link = *(DWORD*)head; link != 0 && link != head; link = *(DWORD*)link)
        {
            DWORD command = *(DWORD*)(link + 0x08);
            const char* commandName = command ? *(const char**)command : nullptr;
            if (commandName && (*(DWORD*)(command + 0x14) & 1) && _stricmp(commandName, name) == 0)
            {
                DWORD previous = *(DWORD*)(command + 0x04);
                *(DWORD*)(command + 0x04) = function;
                return previous;
            }
        }

        return 0;
    }

    inline bool ProgramExists(const char* name)
    {
        DWORD state = GetClientManager();
        return state && ConsoleCommandExists(state, name);
    }

    // New commands and variables show in the completion and the browser
    inline void RefreshEntries()
    {
        g_candidatesDirty = true;
        g_browserNeedsRefresh = true;
    }

    inline bool IsVisible() { return g_visible; }
    inline float GetUIScale() { return g_uiScale; }
}
