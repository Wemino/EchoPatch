#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"
#include "ConsoleMgr.hpp"
#include "EngineInterfaces.cpp"
#include "ClassTicks.cpp"

int(__thiscall* PresentFrame)(void*) = nullptr;
bool(__thiscall* ResetDevice)(void*, void*, unsigned char) = nullptr;
int(__stdcall* WindowProc)(HWND, UINT, WPARAM, LPARAM) = nullptr;
char(__thiscall* ConsolePrint)(void*, unsigned int, int, const char*) = nullptr;

// =======================
// ConsoleEnabled
// =======================

// Presents the frame once all the rendering of the frame is done, the console is drawn there once per frame
static int __fastcall PresentFrame_Hook(void* thisPtr, int)
{
    Console::OnPresent();
    return PresentFrame(thisPtr);
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
    // Toggle key, Tab completion and scrolling keys
    if (Console::HandleKeyMessage(Msg, wParam, lParam))
        return 0;

    if (g_State.isConsoleOpen)
    {
        Console::ForwardInput(hWnd, Msg, wParam, lParam);

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

// Receives every console line (CPrint, engine messages, variable queries...) with its color
static char __fastcall ConsolePrint_Hook(void* thisPtr, int, unsigned int color, int filterLevel, const char* pMsg)
{
    // Lines above the console filter level are dropped by the engine
    if (pMsg && pMsg[0] && filterLevel <= *(int*)((BYTE*)thisPtr + 0x228))
    {
        Console::AddOutput(pMsg, color);
    }

    return ConsolePrint(thisPtr, color, filterLevel, pMsg);
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
    oVtableFloat = GetAddress(Addr::Console_VtableFloat),
    oVtableInt = GetAddress(Addr::Console_VtableInt),
    oPresent = GetAddress(Addr::Console_Present),
    oResetDevice = GetAddress(Addr::Console_ResetDevice),
    oWindowProc = GetAddress(Addr::Console_WindowProc),
    oConsolePrint = GetAddress(Addr::Console_Print),
    oClientManager = GetAddress(Addr::Console_ClientManager),
    oServerManager = GetAddress(Addr::Console_ServerManager),
    oExecuteCommand = GetAddress(Addr::Console_ExecuteCommand),
    oDebugLevel = GetAddress(Addr::Console_DebugLevel),
    oGetEngineVars = GetAddress(Addr::Console_GetEngineVars),
    oGetNumEngineVars = GetAddress(Addr::Console_GetNumEngineVars);

    ConsoleSettings settings;
    settings.highResScaling = HighResolutionScaling;
    settings.logging = LogOutputToFile;
    settings.toggleKey = ConsoleKey;
    settings.bufferLen = ConsoleBufferLen;
    settings.historyLen = ConsoleHistoryLen;
    settings.alpha = ConsoleAlpha;
    settings.position = MoveConsole;

    Console::Init(oInit, g_State.hWnd, title, settings);

    ConsoleAddresses addresses = {};
    addresses.cursorLockAddr = oCursorLock;
    addresses.cvarListHead = oCvarListHead;
    addresses.cvarArrayStart = oCvarArrayStart;
    addresses.cvarArrayEnd = oCvarArrayEnd;
    addresses.cvarVtableFloat = oVtableFloat;
    addresses.cvarVtableInt = oVtableInt;
    addresses.clientManager = oClientManager;
    addresses.serverManager = oServerManager;

    // The engine variable table is read now, before the config files change the values, for the defaults
    if (oGetEngineVars && oGetNumEngineVars)
    {
        addresses.engineVarTable = (DWORD)reinterpret_cast<void*(__cdecl*)()>(oGetEngineVars)();
        addresses.engineVarCount = reinterpret_cast<int(__cdecl*)()>(oGetNumEngineVars)();
    }
    Console::InitAddresses(addresses);

    HookHelper::ApplyHook((void*)oPresent, &PresentFrame_Hook, (LPVOID*)&PresentFrame);
    HookHelper::ApplyHook((void*)oResetDevice, &ResetDevice_Hook, (LPVOID*)&ResetDevice);
    HookHelper::ApplyHook((void*)oWindowProc, &WindowProc_Hook, (LPVOID*)&WindowProc);
    HookHelper::ApplyHook((void*)oConsolePrint, &ConsolePrint_Hook, (LPVOID*)&ConsolePrint);

    ExecuteConsoleCommand = reinterpret_cast<decltype(ExecuteConsoleCommand)>(oExecuteCommand);

    if (DebugLevel != 0)
    {
        MemoryHelper::WriteMemory<int>(oDebugLevel, DebugLevel, false);
    }

    ApplyClassTicksHook();
}
