#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"
#include "ConsoleMgr.hpp"

bool(__cdecl* EndScene)() = nullptr;
bool(__thiscall* ResetDevice)(void*, void*, unsigned char) = nullptr;
int(__stdcall* WindowProc)(HWND, UINT, WPARAM, LPARAM) = nullptr;
int(__cdecl* ConsoleOutput)(int, int, char*) = nullptr;
int(__stdcall* RegisterConsoleProgramClient)(char*, int) = nullptr;
int(__stdcall* RegisterConsoleProgramServer)(char*, int) = nullptr;
int(__stdcall* UnregisterConsoleProgramClient)(char*) = nullptr;
int(__stdcall* UnregisterConsoleProgramServer)(char*) = nullptr;

// =======================
// ConsoleEnabled
// =======================

static bool __cdecl EndScene_Hook()
{
    Console::OnEndScene();
    return EndScene();
}

static bool __fastcall ResetDevice_Hook(DWORD* thisPtr, int, void* a2, unsigned char a3)
{
    Console::OnBeforeReset();
    bool result = ResetDevice(thisPtr, a2, a3);
    Console::OnAfterReset(result != 0);
    return result;
}

static int __stdcall WindowProc_Hook(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (g_State.isConsoleOpen)
    {
        ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam);

        switch (Msg)
        {
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
                if (wParam == VK_F4)
                    break;
                return 0;

            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_CHAR:
                return 0;
        }
    }

    int result = WindowProc(hWnd, Msg, wParam, lParam);

    if (Msg == WM_ACTIVATEAPP && wParam && g_State.isConsoleOpen)
    {
        Console::OnWindowReactivated();
    }

    return result;
}

static int __cdecl ConsoleOutput_Hook(int a1, int a2, char* Source)
{
    if (Source && Source[0])
    {
        Console::AddOutput(Source);
    }

    return ConsoleOutput(a1, a2, Source);
}

static char __cdecl SetCvarString_Hook(int a1, const char* name, char* value)
{
    if (name && name[0] && value)
    {
        std::string strValue = value;
        CvarType type = CvarType::String;

        float floatVal;
        if (IsNumericFloat(value, &floatVal))
        {
            strValue = FormatFloat(floatVal);
            type = CvarType::Float;
        }

        auto it = FindCvarCaseInsensitive(std::string(name));
        if (it != g_dynamicCvars.end())
        {
            it->second = { strValue, a1, type };
        }
        else
        {
            g_dynamicCvars[name] = { strValue, a1, type };
        }
    }

    return SetCvarString(a1, name, value);
}

static char __cdecl SetCvarFloat_Hook(int a1, const char* name, int valueInt)
{
    if (name && name[0])
    {
        float value = *(float*)&valueInt;
        std::string formatted = FormatFloat(value);

        auto it = FindCvarCaseInsensitive(std::string(name));
        if (it != g_dynamicCvars.end())
        {
            it->second = { formatted, a1, CvarType::Float };
        }
        else
        {
            g_dynamicCvars[name] = { formatted, a1, CvarType::Float };
        }
    }

    return SetCvarFloat(a1, name, valueInt);
}

static int __stdcall RegisterConsoleProgramClient_Hook(char* name, int funcPtr)
{
    int result = RegisterConsoleProgramClient(name, funcPtr);

    if (result == 0 && name && name[0])
    {
        g_consolePrograms.insert(name);
    }

    return result;
}

static int __stdcall RegisterConsoleProgramServer_Hook(char* name, int funcPtr)
{
    int result = RegisterConsoleProgramServer(name, funcPtr);

    if (result == 0 && name && name[0])
    {
        g_consolePrograms.insert(name);
    }

    return result;
}

static int __stdcall UnregisterConsoleProgramClient_Hook(char* name)
{
    int result = UnregisterConsoleProgramClient(name);

    if (result == 0 && name && name[0])
    {
        g_consolePrograms.erase(name);
    }

    return result;
}

static int __stdcall UnregisterConsoleProgramServer_Hook(char* name)
{
    int result = UnregisterConsoleProgramServer(name);

    if (result == 0 && name && name[0])
    {
        g_consolePrograms.erase(name);
    }

    return result;
}

static void InitConsole(LPCSTR title)
{
    if (!ConsoleEnabled || !title) return;

    const int game = g_State.CurrentFEARGame;
    if (game != FEAR && game != FEARXP && game != FEARXP2) return;

    const uintptr_t
    oInit = GetAddress(Addr::Console_Init),
    oCursorLock = GetAddress(Addr::Console_CursorLock),
    oCvarListHead = GetAddress(Addr::Console_CvarListHead),
    oCvarArrayStart = GetAddress(Addr::Console_CvarArrayStart),
    oCvarArrayEnd = GetAddress(Addr::Console_CvarArrayEnd),
    oCmdArrayStart = GetAddress(Addr::Console_CmdArrayStart),
    oCmdArrayEnd = GetAddress(Addr::Console_CmdArrayEnd),
    oVtableFloat = GetAddress(Addr::Console_VtableFloat),
    oVtableInt = GetAddress(Addr::Console_VtableInt),
    oEndScene = GetAddress(Addr::Console_EndScene),
    oResetDevice = GetAddress(Addr::Console_ResetDevice),
    oWindowProc = GetAddress(Addr::Console_WindowProc),
    oConsoleOutput = GetAddress(Addr::Console_ConsoleOutput),
    oSetCvarString = GetAddress(Addr::Console_SetCvarString),
    oSetCvarFloat = GetAddress(Addr::Console_SetCvarFloat),
    oRegClient = GetAddress(Addr::Console_RegClient),
    oRegServer = GetAddress(Addr::Console_RegServer),
    oUnregClient = GetAddress(Addr::Console_UnregClient),
    oUnregServer = GetAddress(Addr::Console_UnregServer),
    oRunCmd = GetAddress(Addr::Console_RunCmd),
    oDebugLevel = GetAddress(Addr::Console_DebugLevel);

    Console::Init(oInit, g_State.hWnd, title, HighResolutionScaling, LogOutputToFile);

    ConsoleAddresses addresses = {};
    addresses.cursorLockAddr = oCursorLock;
    addresses.cvarListHead = oCvarListHead;
    addresses.cvarArrayStart = oCvarArrayStart;
    addresses.cvarArrayEnd = oCvarArrayEnd;
    addresses.cmdArrayStart = oCmdArrayStart;
    addresses.cmdArrayEnd = oCmdArrayEnd;
    addresses.cvarVtableFloat = oVtableFloat;
    addresses.cvarVtableInt = oVtableInt;
    Console::InitAddresses(addresses);

    HookHelper::ApplyHook((void*)oEndScene, &EndScene_Hook, (LPVOID*)&EndScene);
    HookHelper::ApplyHook((void*)oResetDevice, &ResetDevice_Hook, (LPVOID*)&ResetDevice);
    HookHelper::ApplyHook((void*)oWindowProc, &WindowProc_Hook, (LPVOID*)&WindowProc);
    HookHelper::ApplyHook((void*)oConsoleOutput, &ConsoleOutput_Hook, (LPVOID*)&ConsoleOutput);

    HookHelper::ApplyHook((void*)oSetCvarString, &SetCvarString_Hook, (LPVOID*)&SetCvarString);
    HookHelper::ApplyHook((void*)oSetCvarFloat, &SetCvarFloat_Hook, (LPVOID*)&SetCvarFloat);

    HookHelper::ApplyHook((void*)oRegClient, &RegisterConsoleProgramClient_Hook, (LPVOID*)&RegisterConsoleProgramClient);
    HookHelper::ApplyHook((void*)oRegServer, &RegisterConsoleProgramServer_Hook, (LPVOID*)&RegisterConsoleProgramServer);
    HookHelper::ApplyHook((void*)oUnregClient, &UnregisterConsoleProgramClient_Hook, (LPVOID*)&UnregisterConsoleProgramClient);
    HookHelper::ApplyHook((void*)oUnregServer, &UnregisterConsoleProgramServer_Hook, (LPVOID*)&UnregisterConsoleProgramServer);

    RunConsoleCommand = reinterpret_cast<decltype(RunConsoleCommand)>(oRunCmd);

    if (DebugLevel != 0)
    {
        MemoryHelper::WriteMemory<int>(oDebugLevel, DebugLevel, false);
    }
}
