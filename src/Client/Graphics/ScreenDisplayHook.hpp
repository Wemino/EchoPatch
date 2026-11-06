#pragma once

#include "../../Globals.cpp"
#include "../../Engine/Graphics/MaxFPS.cpp"
#include "../ScreenHelper.hpp"

// "FPS Limit" option of the display options, added between Resolution and V Sync
namespace ScreenDisplayHook
{
    using namespace ScreenHelper;

    // Local settings
    static uint8_t g_FPSLimitSelection = 0;

    // Address storage
    static uintptr_t LTDatabase_Addr = 0;
    static uintptr_t LTClient_Addr = 0;
    static uintptr_t InterfaceMgr_Addr = 0;
    static uintptr_t SwitchingRenderModesOffset = 0;
    static uintptr_t RunConsoleCommandOffset = 0;

    struct ToggleCreate
    {
        int data[16];
    };

    static bool(__thiscall* CScreenDisplay_Build)(int*) = nullptr;
    static void(__thiscall* CScreenDisplay_OnFocus)(int*, bool) = nullptr;
    static int* (__thiscall* AddToggleByName)(int*, const wchar_t*, ToggleCreate, bool, const char*, uint32_t) = nullptr;
    static DWORD* (__stdcall* LayoutDBGetRect)(DWORD*, int, const char*, uint32_t) = nullptr;

    using DBGetAttribute_t = int(__thiscall*)(int, int, const char*);
    using DBGetInt32_t = int(__thiscall*)(int, int, unsigned int, int);
    using RunConsoleCommand_t = int(__thiscall*)(int, const char*);

    // ILTDatabase virtual functions
    static constexpr int VTBL_DATABASE_GET_ATTRIBUTE = 22;
    static constexpr int VTBL_DATABASE_GET_INT32 = 31;

    // CScreenDisplay::m_bEscape
    static constexpr int SCREEN_DISPLAY_ESCAPE_OFFSET = 0x1C4;

    // 'FrameRect' of the display options
    static constexpr uint32_t FRAME_RECT_FRAME = 0;
    static constexpr uint32_t FRAME_RECT_GAMMA_IMAGE = 1;

    static bool s_isBuildingScreenDisplay = false;
    static int s_rowHeight = 0;
    static int s_frameBottom = 0;
    static int* s_fpsLimitCycle = nullptr;

    // Refresh rates listed by the cycle
    static std::vector<UINT> s_refreshRates;

    // =========================================================================
    // FPS Limit
    // =========================================================================

    static UINT SelectionToMaxFPS(uint8_t selection)
    {
        return selection < s_refreshRates.size() ? s_refreshRates[selection] : GetMaxFPS();
    }

    static uint8_t MaxFPSToSelection(UINT maxFPS)
    {
        auto it = std::find(s_refreshRates.begin(), s_refreshRates.end(), maxFPS);
        return static_cast<uint8_t>(it - s_refreshRates.begin());
    }

    static void ApplyFPSLimit(UINT maxFPS)
    {
        // Keeps MaxFPS at -1 if the limit used until now is left selected
        if (maxFPS == GetMaxFPS())
            return;

        MaxFPS = static_cast<float>(maxFPS);
        UpdateFpsLimiter();

        IniHelper::iniReader["Graphics"]["MaxFPS"] = std::to_string(maxFPS);
        IniHelper::Save();
    }

    static void RestartRender()
    {
        int interfaceMgr = MemoryHelper::ReadMemory<int>(InterfaceMgr_Addr);
        int ltClient = MemoryHelper::ReadMemory<int>(LTClient_Addr);

        if (!interfaceMgr || !ltClient)
            return;

        bool* pSwitchingRenderModes = reinterpret_cast<bool*>(interfaceMgr + SwitchingRenderModesOffset);
        RunConsoleCommand_t runConsoleCommand = reinterpret_cast<RunConsoleCommand_t>(MemoryHelper::ReadMemory<uintptr_t>(MemoryHelper::ReadMemory<uintptr_t>(ltClient) + RunConsoleCommandOffset));

        // Same as the display options when VSync is changed
        *pSwitchingRenderModes = true;
        runConsoleCommand(ltClient, "RestartRender");
        *pSwitchingRenderModes = false;
    }

    static void AddFPSLimitCycle(int* screen, const ToggleCreate& vsyncCreate)
    {
        s_refreshRates = GetDisplayRefreshRates();
        g_FPSLimitSelection = MaxFPSToSelection(GetMaxFPS());

        // Same size as the V Sync toggle that follows
        int cycleStruct[15];
        InitControlStruct(cycleStruct, 15, vsyncCreate.data[7], vsyncCreate.data[8]);
        cycleStruct[1] = (int)"IDS_HELP_FPS_LIMIT";
        cycleStruct[13] = vsyncCreate.data[13];
        cycleStruct[14] = (int)&g_FPSLimitSelection;

        // Font size of the screen
        int* cycle = CallAddCycleByName(screen, L"FPS Limit", cycleStruct, 0);
        if (!cycle)
            return;

        CallAddControl(screen, cycle);

        auto vtable = *(uintptr_t*)cycle;
        AddString_t addStringFn = (AddString_t)(*(uintptr_t*)(vtable + 220));

        wchar_t text[32];
        for (UINT refreshRate : s_refreshRates)
        {
            swprintf_s(text, L"%u FPS", refreshRate);
            addStringFn(cycle, text);
        }

        s_fpsLimitCycle = cycle;
    }

    // =========================================================================
    // Logic & Hooks
    // =========================================================================

    static int GetLayoutInt32(int hRecord, const char* pszAttribute)
    {
        int database = MemoryHelper::ReadMemory<int>(LTDatabase_Addr);
        if (!database)
            return 0;

        int hAttribute = reinterpret_cast<DBGetAttribute_t>(GetVirtualFunction((void*)database, VTBL_DATABASE_GET_ATTRIBUTE))(database, hRecord, pszAttribute);
        return reinterpret_cast<DBGetInt32_t>(GetVirtualFunction((void*)database, VTBL_DATABASE_GET_INT32))(database, hAttribute, 0, 0);
    }

    static DWORD* __stdcall LayoutDBGetRect_Hook(DWORD* pRect, int hRecord, const char* pszAttribute, uint32_t nIndex)
    {
        DWORD* result = LayoutDBGetRect(pRect, hRecord, pszAttribute, nIndex);

        if (!s_isBuildingScreenDisplay || HashHelper::FNV1aRuntime(pszAttribute) != HashHelper::HUDHashes::FrameRect)
            return result;

        // Left, top, right, bottom
        int* rect = reinterpret_cast<int*>(result);

        if (nIndex == FRAME_RECT_FRAME)
        {
            s_frameBottom = rect[3];
            rect[3] += s_rowHeight;
        }
        else if (rect[1] >= s_frameBottom)
        {
            int width = rect[2] - rect[0];
            int height = rect[3] - rect[1];

            if (nIndex == FRAME_RECT_GAMMA_IMAGE && height > s_rowHeight * 2)
            {
                int newWidth = width * (height - s_rowHeight) / height;

                rect[0] += (width - newWidth) / 2;
                rect[1] += s_rowHeight;
                rect[2] = rect[0] + newWidth;
            }
            else
            {
                rect[1] += s_rowHeight;
                rect[3] += s_rowHeight;
            }
        }

        return result;
    }

    static int* __fastcall AddToggleByName_Hook(int* thisPtr, int, const wchar_t* pString, ToggleCreate cs, bool bFixed, const char* szFont, uint32_t nFontHeight)
    {
        const char* helpId = reinterpret_cast<const char*>(cs.data[1]);
        if (s_isBuildingScreenDisplay && helpId && HashHelper::FNV1aRuntime(helpId) == HashHelper::StringHashes::IDS_HELP_VSYNC)
        {
            AddFPSLimitCycle(thisPtr, cs);
        }

        return AddToggleByName(thisPtr, pString, cs, bFixed, szFont, nFontHeight);
    }

    static void __fastcall CScreenDisplay_OnFocus_Hook(int* thisPtr, int, bool bFocus)
    {
        if (!s_fpsLimitCycle)
        {
            CScreenDisplay_OnFocus(thisPtr, bFocus);
            return;
        }

        if (bFocus)
        {
            g_FPSLimitSelection = MaxFPSToSelection(GetMaxFPS());
            CScreenDisplay_OnFocus(thisPtr, bFocus);
            return;
        }

        bool isApplying = *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(thisPtr) + SCREEN_DISPLAY_ESCAPE_OFFSET);

        if (isApplying)
        {
            CallScreenUpdateData(thisPtr, true);
            ApplyFPSLimit(SelectionToMaxFPS(g_FPSLimitSelection));
        }

        CScreenDisplay_OnFocus(thisPtr, bFocus);

        if (isApplying && IsRefreshRateOutdated())
        {
            RestartRender();
        }
    }

    static bool __fastcall CScreenDisplay_Build_Hook(int* thisPtr, int)
    {
        // CScreenDisplay::OnFocus, taken from the vtable of the screen
        if (!CScreenDisplay_OnFocus && !HookHelper::ApplyHook((void*)GetVirtualFunction(thisPtr, VTBL_SCREEN_ON_FOCUS), &CScreenDisplay_OnFocus_Hook, (LPVOID*)&CScreenDisplay_OnFocus))
            return CScreenDisplay_Build(thisPtr);

        int fontSize = GetLayoutInt32(thisPtr[SCREEN_LAYOUT_INDEX], "FontSize");
        if (fontSize <= 0)
            return CScreenDisplay_Build(thisPtr);

        s_rowHeight = fontSize + thisPtr[SCREEN_ITEM_SPACING_INDEX];
        s_fpsLimitCycle = nullptr;

        s_isBuildingScreenDisplay = true;
        bool result = CScreenDisplay_Build(thisPtr);
        s_isBuildingScreenDisplay = false;

        return result;
    }

    // =========================================================================
    // Initialization
    // =========================================================================

    static void InstallHook()
    {
        if (!ScreenHelper::Initialize())
            return;

        DWORD addr_CScreenDisplay_Build = ScanModuleSignature(g_State.GameClient, "81 EC 24 01 00 00 53 55 56 57 68 ?? ?? ?? ?? 8B E9 E8 ?? ?? ?? ?? 8B 45 08 8B 0D ?? ?? ?? ?? 8B 31 33 DB 53 53 68", "CScreenDisplay_Build");
        DWORD addr_LayoutDBGetRect = ScanModuleSignature(g_State.GameClient, "8B 31 52 FF 56 58 8B 0D ?? ?? ?? ?? 50 8D 44 24 20 50 FF 96 94 00 00 00 D9 44 24 14 E8", "LayoutDBGetRect");
        DWORD addr_RestartRender = ScanModuleSignature(g_State.GameClient, "A1 ?? ?? ?? ?? C6 80 ?? ?? 00 00 01 8B 0D ?? ?? ?? ?? 8B 11 68 ?? ?? ?? ?? FF 92", "RestartRender");

        if (addr_CScreenDisplay_Build == 0 ||
            addr_LayoutDBGetRect == 0 ||
            addr_RestartRender == 0)
        {
            return;
        }

        LTDatabase_Addr = MemoryHelper::ReadMemory<uintptr_t>(addr_CScreenDisplay_Build + 0x1B);
        InterfaceMgr_Addr = MemoryHelper::ReadMemory<uintptr_t>(addr_RestartRender + 0x1);
        SwitchingRenderModesOffset = MemoryHelper::ReadMemory<uintptr_t>(addr_RestartRender + 0x7);
        LTClient_Addr = MemoryHelper::ReadMemory<uintptr_t>(addr_RestartRender + 0xE);
        RunConsoleCommandOffset = MemoryHelper::ReadMemory<uintptr_t>(addr_RestartRender + 0x1B);

        HookHelper::ApplyHook((void*)(addr_LayoutDBGetRect - 0x3D), &LayoutDBGetRect_Hook, (LPVOID*)&LayoutDBGetRect);
        HookHelper::ApplyHook((void*)AddToggleByName_Addr, &AddToggleByName_Hook, (LPVOID*)&AddToggleByName);
        HookHelper::ApplyHook((void*)addr_CScreenDisplay_Build, &CScreenDisplay_Build_Hook, (LPVOID*)&CScreenDisplay_Build);
    }
}