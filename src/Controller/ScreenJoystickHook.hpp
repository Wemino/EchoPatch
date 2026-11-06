#pragma once

#include "../Globals.cpp"
#include "../Client/ScreenHelper.hpp"
#include "Controller.hpp"
#include "ControllerBindings.hpp"

namespace ScreenJoystickHook
{
    using namespace ScreenHelper;

    // Local settings
    static uint8_t g_RumbleEnabled = 0;
    static uint8_t g_GyroEnabled = 0;
    static uint32_t g_GyroType = 0;
    static int g_GyroSensitivityInt = 50;
    static int g_GyroSmoothingInt = 16;
    static uint8_t g_TouchpadEnabled = 1;
    static uint8_t g_GyroCalibrationPersistence = 1;
    static int g_ControllerSensitivityInt = 50;
    static int g_EdgeAccelerationInt = 43;

    // Button mapping menu settings
    static uint8_t g_TapSelection[ControllerBindings::SLOT_COUNT] = {};
    static uint8_t g_HoldSelection[ControllerBindings::SLOT_COUNT] = {};
    static int g_HoldDurationMs = 500;

    // Address storage
    static uintptr_t JoystickConstructor_Addr = 0;
    static uintptr_t MsgBoxShow_Addr = 0;
    static uintptr_t InterfaceMgr_Addr = 0;
    static uintptr_t MessageBoxOffset = 0;

    static bool(__thiscall* CScreenJoystick_Build)(int*) = nullptr;
    static uint32_t(__thiscall* CScreenJoystick_OnCommand)(int*, uint32_t, uint32_t, uint32_t) = nullptr;
    static int* (__thiscall* GetScreenFromID)(int, int) = nullptr;

    using JoystickConstructor_t = int* (__thiscall*)(void*);
    using MsgBoxCallback_t = void(__cdecl*)(bool, void*, void*);
    using MsgBoxShow_t = void(__thiscall*)(int, const wchar_t*, void*, uint32_t, bool);

    static constexpr size_t JOYSTICK_SCREEN_SIZE = 0x1D8;

    // =========================================================================
    // Button Mapping Menu
    // =========================================================================

    // The sub-menus are extra CScreenJoystick instances using screen IDs outside of the game's range.
    // GetScreenFromID_Hook resolves them and CScreenJoystick_Build_Hook builds them.
    static constexpr int CUSTOM_SCREEN_ID_BASE = 0x4A00;

    // Above CMD_CUSTOM, handled by CScreenJoystick_OnCommand_Hook
    static constexpr uint32_t CMD_OPEN_CUSTOM_SCREEN = 0x4A00;
    static constexpr uint32_t CMD_BINDING_CHANGED = 0x4A01;
    static constexpr uint32_t CMD_RESTORE_BINDINGS = 0x4A02;

    static constexpr int HOLD_DURATION_STEP = 50;
    static constexpr int LTMB_YESNO = 1;

    enum CustomScreen
    {
        CUSTOM_SCREEN_BUTTON_MAPPING,
        CUSTOM_SCREEN_FACE_BUTTONS,
        CUSTOM_SCREEN_SHOULDERS,
        CUSTOM_SCREEN_DPAD,
        CUSTOM_SCREEN_STICKS,
        CUSTOM_SCREEN_EXTRA,
        CUSTOM_SCREEN_COUNT
    };

    struct BindingGroup
    {
        const wchar_t* title;
        const wchar_t* label;
        const char* helpId;
        int firstSlot;
        int slotCount;
    };

    // One per CustomScreen after CUSTOM_SCREEN_BUTTON_MAPPING, slots index ControllerBindings::s_slots
    static const BindingGroup s_bindingGroups[] =
    {
        { L"FACE BUTTONS",          L"Face Buttons",          "IDS_HELP_BINDINGS_FACE",      0,  4 },
        { L"SHOULDERS & TRIGGERS",  L"Shoulders & Triggers",  "IDS_HELP_BINDINGS_SHOULDERS", 4,  4 },
        { L"D-PAD",                 L"D-Pad",                 "IDS_HELP_BINDINGS_DPAD",      8,  4 },
        { L"STICKS & MENU BUTTONS", L"Sticks & Menu Buttons", "IDS_HELP_BINDINGS_STICKS",    12, 4 },
        { L"EXTRA BUTTONS",         L"Extra Buttons",         "IDS_HELP_BINDINGS_EXTRA",     16, 5 },
    };

    // CMessageBox creation struct (MBCreate)
    struct MessageBoxCreate
    {
        int eType;
        MsgBoxCallback_t pFn;
        int nHotKey;
        void* pUserData;
        void* pData;
        const wchar_t* pString;
        uint32_t nMaxChars;
        int eInput;
        void* pFilterFn;
        uint32_t nFlags;
        bool bPreventEmptyString;
    };

    static bool s_buttonMappingAvailable = false;
    static int s_joystickScreenId = -1;
    static int* s_customScreens[CUSTOM_SCREEN_COUNT] = {};
    static GamepadStyle s_customScreenStyle[CUSTOM_SCREEN_COUNT] = {};

    static int s_tapExtraCommand[ControllerBindings::SLOT_COUNT] = {};
    static int s_holdExtraCommand[ControllerBindings::SLOT_COUNT] = {};

    static int GetCustomScreenIndex(int screenId)
    {
        int index = screenId - CUSTOM_SCREEN_ID_BASE;
        return (index >= 0 && index < CUSTOM_SCREEN_COUNT) ? index : -1;
    }

    static GamepadStyle GetButtonNameStyle()
    {
        GamepadStyle style = GetGamepadStyle();
        return (style == GamepadStyle::Unknown) ? GamepadStyle::Xbox : style;
    }

    // -------------------------------------------------------------------------
    // Cycle selection <-> command
    // -------------------------------------------------------------------------

    static uint8_t CommandToSelection(int commandId, int extraCommand)
    {
        const auto& actions = ControllerBindings::GetActionList();
        commandId = ControllerBindings::NormalizeCommand(commandId);

        for (size_t i = 0; i < actions.size(); i++)
        {
            if (actions[i] == commandId)
                return static_cast<uint8_t>(i);
        }

        if (extraCommand > 0 && commandId == extraCommand)
            return static_cast<uint8_t>(actions.size());

        return 0;
    }

    static int SelectionToCommand(uint8_t selection, int extraCommand)
    {
        const auto& actions = ControllerBindings::GetActionList();

        if (selection < actions.size())
            return actions[selection];

        if (selection == actions.size() && extraCommand > 0)
            return extraCommand;

        return ControllerBindings::COMMAND_NONE;
    }

    static void SyncBindingSelections()
    {
        for (int i = 0; i < ControllerBindings::SLOT_COUNT; i++)
        {
            const auto& slot = ControllerBindings::s_slots[i];
            g_TapSelection[i] = CommandToSelection(*slot.tapCommand, s_tapExtraCommand[i]);
            g_HoldSelection[i] = CommandToSelection(*slot.holdCommand, s_holdExtraCommand[i]);
        }

        g_HoldDurationMs = ControllerBindings::GetCommonHoldTime();
    }

    static const wchar_t* __cdecl HoldDurationText(int position, void*)
    {
        static wchar_t s_text[32];
        swprintf_s(s_text, L"%d ms", position);
        return s_text;
    }

    // -------------------------------------------------------------------------
    // Screen building
    // -------------------------------------------------------------------------

    static void AddBindingCycle(int* screen, int slotIndex, bool isHold, int cycleStruct[15], int fontSize)
    {
        const auto& slot = ControllerBindings::s_slots[slotIndex];
        const auto& actions = ControllerBindings::GetActionList();

        int commandId = ControllerBindings::NormalizeCommand(isHold ? *slot.holdCommand : *slot.tapCommand);
        bool isListed = std::find(actions.begin(), actions.end(), commandId) != actions.end();

        int& extraCommand = isHold ? s_holdExtraCommand[slotIndex] : s_tapExtraCommand[slotIndex];
        extraCommand = isListed ? 0 : commandId;

        uint8_t& selection = isHold ? g_HoldSelection[slotIndex] : g_TapSelection[slotIndex];
        selection = CommandToSelection(commandId, extraCommand);

        wchar_t label[64];
        const wchar_t* buttonName = GetGamepadInputName(slot.isTrigger, slot.index, false);
        if (isHold)
        {
            swprintf_s(label, L"%s (Hold)", buttonName);
        }
        else
        {
            wcscpy_s(label, buttonName);
        }

        // Every change is sent to CScreenJoystick_OnCommand_Hook
        cycleStruct[0] = CMD_BINDING_CHANGED;
        cycleStruct[1] = (int)(isHold ? "IDS_HELP_BINDING_HOLD" : "IDS_HELP_BINDING_PRESS");
        cycleStruct[2] = (int)screen;
        cycleStruct[3] = slotIndex * 2 + (isHold ? 1 : 0);
        cycleStruct[14] = (int)&selection;

        int* cycle = CallAddCycleByName(screen, label, cycleStruct, fontSize);
        if (!cycle)
            return;

        CallAddControl(screen, cycle);

        auto vtable = *(uintptr_t*)cycle;
        AddString_t addStringFn = (AddString_t)(*(uintptr_t*)(vtable + 220));

        for (int actionCommand : actions)
        {
            addStringFn(cycle, ControllerBindings::GetActionName(actionCommand));
        }

        if (extraCommand > 0)
        {
            addStringFn(cycle, ControllerBindings::GetActionName(extraCommand));
        }
    }

    static bool BuildBindingGroupScreen(int* thisPtr, int screenIndex)
    {
        const BindingGroup& group = s_bindingGroups[screenIndex - CUSTOM_SCREEN_FACE_BUTTONS];

        int screenLeft = thisPtr[SCREEN_RECT_LEFT_INDEX];
        int screenTop = thisPtr[SCREEN_RECT_TOP_INDEX];

        CallCreateTitleByName(thisPtr, group.title);

        // Wider than the controller screen for labels like "Microphone Button (Hold)"
        int kGap = 230;
        int kWidth = 200;
        int fontSize = 16;
        int numControls = group.slotCount * 2;
        int rowHeight = fontSize + thisPtr[SCREEN_ITEM_SPACING_INDEX];

        int frameLeft = screenLeft + kGap - 5;
        int frameTop = screenTop - 5;
        int frameRight = screenLeft + kGap + kWidth + 10;
        int frameBottom = screenTop + rowHeight * numControls + 13;

        int frameTextureRef = CreateFrame(thisPtr, frameLeft, frameTop, frameRight, frameBottom);

        thisPtr[SCREEN_DEFAULT_POS_X_INDEX] = screenLeft;
        thisPtr[SCREEN_DEFAULT_POS_Y_INDEX] = screenTop;

        int cycleStruct[15];
        InitControlStruct(cycleStruct, 15, kGap + kWidth, fontSize);
        cycleStruct[13] = kGap;

        for (int i = 0; i < group.slotCount; i++)
        {
            int slotIndex = group.firstSlot + i;
            AddBindingCycle(thisPtr, slotIndex, false, cycleStruct, fontSize);
            AddBindingCycle(thisPtr, slotIndex, true, cycleStruct, fontSize);
        }

        s_customScreenStyle[screenIndex] = GetButtonNameStyle();

        bool buildResult = CallBaseScreenBuild(thisPtr);
        ReleaseTextureRef(frameTextureRef);

        if (!buildResult)
            return false;

        CallUseBack(thisPtr, 1, 1, 0);
        return true;
    }

    static bool BuildButtonMappingScreen(int* thisPtr)
    {
        int screenLeft = thisPtr[SCREEN_RECT_LEFT_INDEX];
        int screenTop = thisPtr[SCREEN_RECT_TOP_INDEX];

        CallCreateTitleByName(thisPtr, L"BUTTON MAPPING");

        int kGap = 200;
        int kWidth = 200;
        int fontSize = 16;
        int itemSpacing = thisPtr[SCREEN_ITEM_SPACING_INDEX];
        int rowHeight = fontSize + itemSpacing;

        thisPtr[SCREEN_DEFAULT_POS_X_INDEX] = screenLeft;
        thisPtr[SCREEN_DEFAULT_POS_Y_INDEX] = screenTop;

        // 1-5. Button groups
        for (int i = 0; i < CUSTOM_SCREEN_COUNT - 1; i++)
        {
            const BindingGroup& group = s_bindingGroups[i];
            AddTextItem(thisPtr, group.label, group.helpId, CMD_OPEN_CUSTOM_SCREEN, CUSTOM_SCREEN_FACE_BUTTONS + i, kGap, fontSize);
        }

        // 6. Hold Duration, framed like the settings of the controller screen
        int sliderTop = screenTop + rowHeight * (CUSTOM_SCREEN_COUNT - 1) + rowHeight / 2;
        int frameBottom = sliderTop + rowHeight + 13;
        int frameTextureRef = CreateFrame(thisPtr, screenLeft + kGap - 5, sliderTop - 5, screenLeft + kGap + kWidth + 10, frameBottom);

        thisPtr[SCREEN_DEFAULT_POS_X_INDEX] = screenLeft;
        thisPtr[SCREEN_DEFAULT_POS_Y_INDEX] = sliderTop;

        int sliderStruct[25];
        InitControlStruct(sliderStruct, 25, kGap + kWidth, fontSize);
        sliderStruct[1] = (int)"IDS_HELP_HOLD_DURATION";
        sliderStruct[15] = kGap;
        sliderStruct[17] = ControllerBindings::MIN_HOLD_TIME;
        sliderStruct[18] = ControllerBindings::MAX_HOLD_TIME;
        sliderStruct[19] = HOLD_DURATION_STEP;
        sliderStruct[20] = (int)&g_HoldDurationMs;
        sliderStruct[21] = 64;
        sliderStruct[23] = (int)&HoldDurationText;
        CallAddSlider(thisPtr, "IDS_HOLD_DURATION", sliderStruct, fontSize);

        // 7. Restore Defaults
        thisPtr[SCREEN_DEFAULT_POS_Y_INDEX] = frameBottom + itemSpacing;
        AddTextItem(thisPtr, L"Restore Defaults", "IDS_HELP_RESTORE_BINDINGS", CMD_RESTORE_BINDINGS, 0, kGap, fontSize);

        bool buildResult = CallBaseScreenBuild(thisPtr);
        ReleaseTextureRef(frameTextureRef);

        if (!buildResult)
            return false;

        CallUseBack(thisPtr, 1, 1, 0);
        return true;
    }

    static bool BuildCustomScreen(int* thisPtr, int screenIndex)
    {
        g_State.isBuildingCScreenJoystick = true;

        bool result = (screenIndex == CUSTOM_SCREEN_BUTTON_MAPPING) ? BuildButtonMappingScreen(thisPtr) : BuildBindingGroupScreen(thisPtr, screenIndex);

        g_State.isBuildingCScreenJoystick = false;
        return result;
    }

    // -------------------------------------------------------------------------
    // Screen management
    // -------------------------------------------------------------------------

    static int* __fastcall GetScreenFromID_Hook(int thisPtr, int, int screenId)
    {
        int index = GetCustomScreenIndex(screenId);
        if (index >= 0)
            return s_customScreens[index];

        return GetScreenFromID(thisPtr, screenId);
    }

    static int* GetOrCreateCustomScreen(int* parentScreen, int index)
    {
        if (s_customScreens[index])
            return s_customScreens[index];

        if (s_joystickScreenId < 0)
            return nullptr;

        // Allocated by the game so CScreenMgr could free it like its own screens
        void* memory = reinterpret_cast<OperatorNew_t>(OperatorNew_Addr)(JOYSTICK_SCREEN_SIZE);
        if (!memory)
            return nullptr;

        int* screen = reinterpret_cast<JoystickConstructor_t>(JoystickConstructor_Addr)(memory);

        // Must share the vtable of the controller screen for the hooks to apply
        if (!screen || *(uintptr_t*)screen != *(uintptr_t*)parentScreen)
        {
            s_buttonMappingAvailable = false;
            return nullptr;
        }

        // The layout record is looked up from the screen ID, so use the controller screen's one before switching to the custom ID
        reinterpret_cast<ScreenInit_t>(GetVirtualFunction(screen, VTBL_SCREEN_INIT))(screen, s_joystickScreenId);
        screen[SCREEN_ID_INDEX] = CUSTOM_SCREEN_ID_BASE + index;

        s_customScreens[index] = screen;
        return screen;
    }

    // Clears the controls so the next switch to the screen builds it again
    static void ResetCustomScreen(int* screen, int index)
    {
        reinterpret_cast<ScreenTerm_t>(GetVirtualFunction(screen, VTBL_SCREEN_TERM))(screen);
        reinterpret_cast<ScreenInit_t>(GetVirtualFunction(screen, VTBL_SCREEN_INIT))(screen, s_joystickScreenId);
        screen[SCREEN_ID_INDEX] = CUSTOM_SCREEN_ID_BASE + index;
        *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(screen) + SCREEN_BUILT_OFFSET) = false;
    }

    static void OpenCustomScreen(int* currentScreen, int index)
    {
        if (index < 0 || index >= CUSTOM_SCREEN_COUNT)
            return;

        int* screen = GetOrCreateCustomScreen(currentScreen, index);
        if (!screen)
            return;

        // Rebuild the button names if another type of controller is now connected
        if (index != CUSTOM_SCREEN_BUTTON_MAPPING && IsScreenBuilt(screen) && s_customScreenStyle[index] != GetButtonNameStyle())
        {
            ResetCustomScreen(screen, index);
        }

        SyncBindingSelections();

        int screenMgr = currentScreen[SCREEN_MGR_INDEX];
        reinterpret_cast<SetCurrentScreen_t>(GetVirtualFunction((void*)screenMgr, VTBL_SCREENMGR_SET_CURRENT_SCREEN))(screenMgr, CUSTOM_SCREEN_ID_BASE + index);
    }

    // -------------------------------------------------------------------------
    // Settings changes
    // -------------------------------------------------------------------------

    static void OnBindingChanged(int* screen, uint32_t param)
    {
        int slotIndex = static_cast<int>(param >> 1);
        bool isHold = (param & 1) != 0;

        if (slotIndex >= ControllerBindings::SLOT_COUNT)
            return;

        // Copy the selections of the cycles to g_TapSelection / g_HoldSelection
        CallScreenUpdateData(screen, true);

        uint8_t selection = isHold ? g_HoldSelection[slotIndex] : g_TapSelection[slotIndex];
        int extraCommand = isHold ? s_holdExtraCommand[slotIndex] : s_tapExtraCommand[slotIndex];

        ControllerBindings::SetCommand(slotIndex, isHold, SelectionToCommand(selection, extraCommand));
        RefreshGamepadPrompts();
    }

    // Called by SliderSetSliderPos_Hook
    static void OnHoldDurationChanged(int holdTimeMs)
    {
        holdTimeMs = std::clamp(holdTimeMs, ControllerBindings::MIN_HOLD_TIME, ControllerBindings::MAX_HOLD_TIME);

        // Only an actual change applies the duration to every button, keeping custom per button values from the ini otherwise
        if (holdTimeMs == g_HoldDurationMs)
            return;

        g_HoldDurationMs = holdTimeMs;
        ControllerBindings::SetAllHoldTimes(holdTimeMs);
    }

    static void ApplyDefaultBindings(int* screen)
    {
        ControllerBindings::RestoreDefaults();
        RefreshGamepadPrompts();
        SyncBindingSelections();

        if (screen)
        {
            CallScreenUpdateData(screen, false);
        }
    }

    static void __cdecl RestoreBindingsConfirm_Callback(bool bReturn, void*, void* pUserData)
    {
        if (bReturn)
        {
            ApplyDefaultBindings(static_cast<int*>(pUserData));
        }
    }

    static void ShowRestoreBindingsConfirm(int* screen)
    {
        int interfaceMgr = InterfaceMgr_Addr ? MemoryHelper::ReadMemory<int>(InterfaceMgr_Addr) : 0;

        if (!interfaceMgr || !MsgBoxShow_Addr)
        {
            ApplyDefaultBindings(screen);
            return;
        }

        MessageBoxCreate mb = {};
        mb.eType = LTMB_YESNO;
        mb.pFn = &RestoreBindingsConfirm_Callback;
        mb.nHotKey = -1;
        mb.pUserData = screen;

        // Calls MsgBoxShow_Hook when installed, keeping g_State.isMsgBoxVisible up to date
        reinterpret_cast<MsgBoxShow_t>(MsgBoxShow_Addr)(interfaceMgr + (int)MessageBoxOffset, L"Restore the default controller button mapping?", &mb, 0, true);
    }

    static uint32_t __fastcall CScreenJoystick_OnCommand_Hook(int* thisPtr, int, uint32_t command, uint32_t param1, uint32_t param2)
    {
        switch (command)
        {
            case CMD_OPEN_CUSTOM_SCREEN:
                OpenCustomScreen(thisPtr, static_cast<int>(param1));
                return 1;

            case CMD_BINDING_CHANGED:
                OnBindingChanged(thisPtr, param1);
                return 1;

            case CMD_RESTORE_BINDINGS:
                ShowRestoreBindingsConfirm(thisPtr);
                return 1;
        }

        return CScreenJoystick_OnCommand(thisPtr, command, param1, param2);
    }

    // =========================================================================
    // Logic & Hooks
    // =========================================================================

    static void InitFromGlobals()
    {
        g_RumbleEnabled = RumbleEnabled ? 1 : 0;
        g_GyroEnabled = GyroEnabled ? 1 : 0;
        g_GyroType = (uint32_t)GyroAimingMode;
        g_TouchpadEnabled = TouchpadEnabled ? 1 : 0;

        // GPadAimSensitivity: 0.0-4.0 -> 0-100
        g_ControllerSensitivityInt = (int)(GPadAimSensitivity * 25.0f);
        if (g_ControllerSensitivityInt > 100) g_ControllerSensitivityInt = 100;
        if (g_ControllerSensitivityInt < 0) g_ControllerSensitivityInt = 0;

        // GPadAimEdgeMultiplier: 1.0-2.4 -> 0-100
        g_EdgeAccelerationInt = (int)((GPadAimEdgeMultiplier - 1.0f) / 1.4f * 100.0f);
        if (g_EdgeAccelerationInt > 100) g_EdgeAccelerationInt = 100;
        if (g_EdgeAccelerationInt < 0) g_EdgeAccelerationInt = 0;

        // GyroSensitivity: 0.0-2.0 -> 0-100
        g_GyroSensitivityInt = (int)(GyroSensitivity * 50.0f);
        if (g_GyroSensitivityInt > 100) g_GyroSensitivityInt = 100;
        if (g_GyroSensitivityInt < 0) g_GyroSensitivityInt = 0;

        // GyroSmoothing: 0.0-0.1 -> 0-100
        g_GyroSmoothingInt = (int)(GyroSmoothing * 1000.0f);
        if (g_GyroSmoothingInt > 100) g_GyroSmoothingInt = 100;
        if (g_GyroSmoothingInt < 0) g_GyroSmoothingInt = 0;
    }

    static bool __fastcall CScreenJoystick_Build_Hook(int* thisPtr, int)
    {
        int customScreen = GetCustomScreenIndex(thisPtr[SCREEN_ID_INDEX]);
        if (customScreen >= 0)
            return BuildCustomScreen(thisPtr, customScreen);

        s_joystickScreenId = thisPtr[SCREEN_ID_INDEX];

        // CScreenJoystick::OnCommand, taken from the vtable of the screen
        if (s_buttonMappingAvailable && !CScreenJoystick_OnCommand)
        {
            s_buttonMappingAvailable = HookHelper::ApplyHook((void*)GetVirtualFunction(thisPtr, VTBL_SCREEN_ON_COMMAND), &CScreenJoystick_OnCommand_Hook, (LPVOID*)&CScreenJoystick_OnCommand);
        }

        g_State.isBuildingCScreenJoystick = true;
        InitFromGlobals();

        int screenLeft = thisPtr[60];
        int screenTop = thisPtr[61];

        CallCreateTitleByName(thisPtr, L"CONTROLLER");

        int kGap = 200;
        int kWidth = 200;
        int fontSize = 16;
        int numControls = 9;

        int frameLeft = screenLeft + kGap - 5;
        int frameTop = screenTop - 5;
        int frameRight = screenLeft + kGap + kWidth + 10;
        int frameBottom = screenTop + (fontSize + 8) * numControls + 13;

        int frameTextureRef = CreateFrame(thisPtr, frameLeft, frameTop, frameRight, frameBottom);

        thisPtr[69] = thisPtr[60];
        thisPtr[70] = thisPtr[61];

        int toggleStruct[16];
        memset(toggleStruct, 0, sizeof(toggleStruct));
        toggleStruct[7] = kGap + kWidth;
        toggleStruct[8] = fontSize;
        toggleStruct[9] = 1;
        toggleStruct[10] = 0x3E800000;
        toggleStruct[11] = 0x3F99999A;
        toggleStruct[12] = 0x3F99999A;
        toggleStruct[13] = kGap;

        int sliderStruct[25];
        memset(sliderStruct, 0, sizeof(sliderStruct));
        sliderStruct[7] = kGap + kWidth;
        sliderStruct[8] = fontSize;
        sliderStruct[9] = 1;
        sliderStruct[10] = 0x3E800000;
        sliderStruct[11] = 0x3F99999A;
        sliderStruct[12] = 0x3F99999A;
        sliderStruct[15] = kGap;
        sliderStruct[19] = 2;

        int cycleStruct[15];
        memset(cycleStruct, 0, sizeof(cycleStruct));
        cycleStruct[7] = kGap + kWidth;
        cycleStruct[8] = fontSize;
        cycleStruct[9] = 1;
        cycleStruct[10] = 0x3E800000;
        cycleStruct[11] = 0x3F99999A;
        cycleStruct[12] = 0x3F99999A;
        cycleStruct[13] = kGap;

        // 1. Controller Sensitivity
        sliderStruct[1] = (int)"IDS_HELP_CONTROLLER_SENSITIVITY";
        sliderStruct[17] = 0;
        sliderStruct[18] = 100;
        sliderStruct[20] = (int)&g_ControllerSensitivityInt;
        CallAddSlider(thisPtr, "IDS_CONTROLLER_SENSITIVITY", sliderStruct, fontSize);

        // 2. Edge Acceleration
        sliderStruct[1] = (int)"IDS_HELP_EDGE_ACCELERATION";
        sliderStruct[17] = 0;
        sliderStruct[18] = 100;
        sliderStruct[20] = (int)&g_EdgeAccelerationInt;
        CallAddSlider(thisPtr, "IDS_EDGE_ACCELERATION", sliderStruct, fontSize);

        // 3. Rumble
        toggleStruct[1] = (int)"IDS_HELP_RUMBLE";
        toggleStruct[15] = (int)&g_RumbleEnabled;
        int* rumbleToggle = CallAddToggleByName(thisPtr, L"Rumble", toggleStruct, fontSize);
        if (rumbleToggle)
        {
            CallAddControl(thisPtr, rumbleToggle);
            CallSetOnString(rumbleToggle, L"On");
            CallSetOffString(rumbleToggle, L"Off");
        }

        // 4. Gyro Enabled
        toggleStruct[1] = (int)"IDS_HELP_GYRO_ENABLED";
        toggleStruct[15] = (int)&g_GyroEnabled;
        int* gyroToggle = CallAddToggleByName(thisPtr, L"Gyro", toggleStruct, fontSize);
        if (gyroToggle)
        {
            CallAddControl(thisPtr, gyroToggle);
            CallSetOnString(gyroToggle, L"On");
            CallSetOffString(gyroToggle, L"Off");
        }

        // 5. Gyro Type
        cycleStruct[1] = (int)"IDS_HELP_GYRO_TYPE";
        cycleStruct[14] = (int)&g_GyroType;
        int* gyroTypeCycle = CallAddCycleByName(thisPtr, L"Gyro Type", cycleStruct, fontSize);
        if (gyroTypeCycle)
        {
            CallAddControl(thisPtr, gyroTypeCycle);
            auto vtable = *(uintptr_t*)gyroTypeCycle;
            AddString_t addStringFn = (AddString_t)(*(uintptr_t*)(vtable + 220));
            addStringFn(gyroTypeCycle, L"Always On");
            addStringFn(gyroTypeCycle, L"Aiming Only");
            addStringFn(gyroTypeCycle, L"Hip Fire Only");
        }

        // 6. Gyro Calibration Persistence
        toggleStruct[1] = (int)"IDS_HELP_GYRO_PERSISTENCE_ENABLED";
        toggleStruct[15] = (int)&g_GyroCalibrationPersistence;
        int* gyroCalibration = CallAddToggleByName(thisPtr, L"Gyro Cal Persistence", toggleStruct, fontSize);
        if (gyroCalibration)
        {
            CallAddControl(thisPtr, gyroCalibration);
            CallSetOnString(gyroCalibration, L"On");
            CallSetOffString(gyroCalibration, L"Off");
        }

        // 7. Gyro Sensitivity
        sliderStruct[1] = (int)"IDS_HELP_GYRO_SENSITIVITY";
        sliderStruct[17] = 0;
        sliderStruct[18] = 100;
        sliderStruct[20] = (int)&g_GyroSensitivityInt;
        CallAddSlider(thisPtr, "IDS_GYRO_SENSITIVITY", sliderStruct, fontSize);

        // 8. Gyro Smoothing
        sliderStruct[1] = (int)"IDS_HELP_GYRO_SMOOTHING";
        sliderStruct[17] = 0;
        sliderStruct[18] = 100;
        sliderStruct[20] = (int)&g_GyroSmoothingInt;
        CallAddSlider(thisPtr, "IDS_GYRO_SMOOTHING", sliderStruct, fontSize);

        // 9. Touchpad
        toggleStruct[1] = (int)"IDS_HELP_TOUCHPAD";
        toggleStruct[15] = (int)&g_TouchpadEnabled;
        int* touchpadToggle = CallAddToggleByName(thisPtr, L"Touchpad", toggleStruct, fontSize);
        if (touchpadToggle)
        {
            CallAddControl(thisPtr, touchpadToggle);
            CallSetOnString(touchpadToggle, L"On");
            CallSetOffString(touchpadToggle, L"Off");
        }

        // 10. Button Mapping sub-menu
        if (s_buttonMappingAvailable)
        {
            thisPtr[SCREEN_DEFAULT_POS_X_INDEX] = screenLeft;
            thisPtr[SCREEN_DEFAULT_POS_Y_INDEX] = frameBottom + thisPtr[SCREEN_ITEM_SPACING_INDEX];
            AddTextItem(thisPtr, L"Button Mapping", "IDS_HELP_BUTTON_MAPPING", CMD_OPEN_CUSTOM_SCREEN, CUSTOM_SCREEN_BUTTON_MAPPING, kGap, fontSize);
        }

        bool buildResult = CallBaseScreenBuild(thisPtr);

        g_State.isBuildingCScreenJoystick = false;

        if (!buildResult)
        {
            ReleaseTextureRef(frameTextureRef);
            return false;
        }

        CallUseBack(thisPtr, 1, 1, 0);
        ReleaseTextureRef(frameTextureRef);
        return true;
    }

    // =========================================================================
    // Initialization
    // =========================================================================

    static bool InitializeButtonMapping()
    {
        DWORD addr_JoystickConstructor = ScanModuleSignature(g_State.GameClient, "56 8B F1 E8 ?? ?? ?? ?? 33 C0 C7 06 ?? ?? ?? ?? 89 86 C4 01 00 00 89 86 C8 01 00 00 89 86 CC 01 00 00 C7 86 D0 01 00 00 FF FF FF FF", "JoystickConstructor");
        DWORD addr_GetScreenFromID = ScanModuleSignature(g_State.GameClient, "8B 51 68 8B 49 6C 33 C0 3B D1 74 ?? 56 8B 74 24 08 57 8B 3A 39 77 10", "GetScreenFromID");

        if (CreateTextItem_Addr == 0 ||
            addr_JoystickConstructor == 0 ||
            addr_GetScreenFromID == 0)
        {
            return false;
        }

        JoystickConstructor_Addr = addr_JoystickConstructor;

        DWORD addr_MsgBoxShow = ScanModuleSignature(g_State.GameClient, "83 EC 70 56 8B F1 8A 86 78 05 00 00 84 C0 0F 85", "MsgBoxShow", -1, false);
        DWORD addr_ConfirmRestore = ScanModuleSignature(g_State.GameClient, "8B 0D ?? ?? ?? ?? 50 68 ?? ?? ?? ?? 81 C1 ?? ?? ?? ?? C7 44 24 ?? 01 00 00 00 C7 44 24 ?? ?? ?? ?? ?? 89 74 24 ?? E8", "ConfirmRestore", -1, false);

        if (addr_MsgBoxShow != 0 && addr_ConfirmRestore != 0)
        {
            MsgBoxShow_Addr = addr_MsgBoxShow;
            InterfaceMgr_Addr = MemoryHelper::ReadMemory<uintptr_t>(addr_ConfirmRestore + 0x2);
            MessageBoxOffset = MemoryHelper::ReadMemory<uintptr_t>(addr_ConfirmRestore + 0xE);
        }

        return HookHelper::ApplyHook((void*)addr_GetScreenFromID, &GetScreenFromID_Hook, (LPVOID*)&GetScreenFromID);
    }

    static void InstallHook()
    {
        if (ScreenHelper::Initialize())
        {
            DWORD addr_CScreenJoystick_Build = ScanModuleSignature(g_State.GameClient, "81 EC 6C 04 00 00 53 55 56 57", "CScreenJoystick_Build");

            if (addr_CScreenJoystick_Build)
            {
                HookHelper::ApplyHook((void*)addr_CScreenJoystick_Build, &CScreenJoystick_Build_Hook, (LPVOID*)&CScreenJoystick_Build);
                s_buttonMappingAvailable = InitializeButtonMapping();
            }
        }
    }
}