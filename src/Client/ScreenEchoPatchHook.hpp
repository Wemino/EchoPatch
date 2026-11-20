#pragma once

#include "../Globals.cpp"
#include "../Engine/Console/EngineInterfaces.cpp"
#include "ScreenHelper.hpp"
#include "LAAPatcher.hpp"

namespace ScreenEchoPatchHook
{
    using namespace ScreenHelper;

    // Address storage
    static uintptr_t CScreenOptions_Build_Addr = 0;
    static uintptr_t AddScreen_Addr = 0;
    static uintptr_t MouseAimSensitivity_Addr = 0;

    static bool(__thiscall* BaseScreenBuild)(int*) = nullptr;
    static uint32_t(__thiscall* CScreenOptions_OnCommand)(int*, uint32_t, uint32_t, uint32_t) = nullptr;
    static void(__thiscall* CScreenOptions_OnFocus)(int*, bool) = nullptr;

    using AddScreen_t = void(__thiscall*)(int, int);
    using SliderText_t = const wchar_t* (__cdecl*)(int, void*);

    static constexpr int CUSTOM_SCREEN_ID_BASE = 0x4B00;

    static constexpr uint32_t CMD_OPEN_CUSTOM_SCREEN = 0x4B00;
    static constexpr uint32_t CMD_APPLY_LAA_PATCH = 0x4B01;
    static constexpr uint32_t CMD_REQUIRED_SETTING_CHANGED = 0x4B02;

    // CScreenMgr::m_screenArray
    static constexpr int SCREENMGR_SCREENS_END_OFFSET = 0x6C;

    // CBaseScreen virtual functions
    static constexpr int SCREEN_VTABLE_SIZE = 34;

    // FovY and FovYWidescreen of the game
    static constexpr float DEFAULT_FOV = 70.0f;

    // Multiplied by MouseAimMultiplier
    static constexpr float MOUSE_AIM_SENSITIVITY = 0.25f;

    enum CustomScreen
    {
        CUSTOM_SCREEN_ECHOPATCH,
        CUSTOM_SCREEN_GRAPHICS,
        CUSTOM_SCREEN_DISPLAY,
        CUSTOM_SCREEN_MOUSE,
        CUSTOM_SCREEN_MEMORY,
        CUSTOM_SCREEN_EXTRA,
        CUSTOM_SCREEN_COUNT
    };

    struct Category
    {
        const wchar_t* title;
        const wchar_t* label;
        const char* helpId;
    };

    // One per CustomScreen after CUSTOM_SCREEN_ECHOPATCH
    static const Category s_categories[] =
    {
        { L"GRAPHICS", L"Graphics", "IDS_HELP_ECHOPATCH_GRAPHICS" },
        { L"DISPLAY",  L"Display",  "IDS_HELP_ECHOPATCH_DISPLAY" },
        { L"MOUSE",    L"Mouse",    "IDS_HELP_ECHOPATCH_MOUSE" },
        { L"MEMORY",   L"Memory",   "IDS_HELP_ECHOPATCH_MEMORY" },
        { L"EXTRA",    L"Extra",    "IDS_HELP_ECHOPATCH_EXTRA" },
    };

    enum SettingIndex
    {
        HIGH_RESOLUTION_REFLECTIONS,
        SSAA_SCALE,
        NO_LOD_BIAS,
        ENABLE_PERSISTENT_WORLD_STATE,
        SAVE_PERSISTENT_WORLD_STATE,
        CUSTOM_FOV,
        HUD_SCALING,
        HUD_CUSTOM_SCALING_FACTOR,
        SMALL_TEXT_CUSTOM_SCALING_FACTOR,
        DISABLE_LETTERBOX,
        MOUSE_AIM_MULTIPLIER,
        REDUCE_TEXTURE_MEMORY,
        INFINITE_FLASHLIGHT,
        ENABLE_CUSTOM_MAX_WEAPON_CAPACITY,
        MAX_WEAPON_CAPACITY,
        DOOR_BASHING,
        UNCAP_DISMEMBERMENT,
        GIB_FREQUENCY_CAP,
        DISABLE_HIP_FIRE_ACCURACY_PENALTY,
        SETTING_COUNT
    };

    static const wchar_t* __cdecl SSAAScaleText(int position, void*)
    {
        static wchar_t s_text[16];

        if (position == 100)
            return L"Off";

        swprintf_s(s_text, L"%.2fx", position / 100.0f);
        return s_text;
    }

    static const wchar_t* __cdecl CustomFOVText(int position, void*)
    {
        static wchar_t s_text[16];

        if (position == static_cast<int>(DEFAULT_FOV))
            return L"Default";

        swprintf_s(s_text, L"%d", position);
        return s_text;
    }

    static const wchar_t* __cdecl NumberText(int position, void*)
    {
        static wchar_t s_text[16];
        swprintf_s(s_text, L"%d", position);
        return s_text;
    }

    static const wchar_t* __cdecl MultiplierText(int position, void*)
    {
        static wchar_t s_text[16];
        swprintf_s(s_text, L"%.2fx", position / 100.0f);
        return s_text;
    }

    static const wchar_t* __cdecl HUDSizeText(int position, void*)
    {
        // Limited by UpdateHUDScaling at this resolution
        if (g_State.currentWidth != 0 && position / 100.0f > GetMaxHUDCustomScalingFactor())
            return L"Max";

        return MultiplierText(position, nullptr);
    }

    struct Setting
    {
        int screen;
        const char* section;
        const char* key;
        const char* helpId;
        bool requiresRestart;

        // Toggles, 'inverted' shows the opposite of the setting
        const wchar_t* label;
        bool inverted;

        // Sliders, the position is the value multiplied by 'scale'
        const char* labelId;
        int scale;
        int min;
        int max;
        int step;
        SliderText_t text;
    };

    // Same order as SettingIndex
    static const Setting s_settings[] =
    {
        // Graphics
        { CUSTOM_SCREEN_GRAPHICS, "Graphics",   "HighResolutionReflections",     "IDS_HELP_HIGH_RESOLUTION_REFLECTIONS", false, L"High Resolution Reflections" },
        { CUSTOM_SCREEN_GRAPHICS, "Graphics",   "SSAAScale",                     "IDS_HELP_SUPERSAMPLING",               false, nullptr, false, "IDS_SUPERSAMPLING",          100, 100, 400, 25, &SSAAScaleText },
        { CUSTOM_SCREEN_GRAPHICS, "Graphics",   "NoLODBias",                     "IDS_HELP_MAXIMUM_MODEL_DETAIL",        false, L"Maximum Model Detail" },
        { CUSTOM_SCREEN_GRAPHICS, "Graphics",   "EnablePersistentWorldState",    "IDS_HELP_PERSISTENT_WORLD",            true,  L"Persistent World" },
        { CUSTOM_SCREEN_GRAPHICS, "Graphics",   "SavePersistentWorldState",      "IDS_HELP_SAVE_PERSISTENT_WORLD",       true,  L"Save Persistent World" },

        // Display
        { CUSTOM_SCREEN_DISPLAY,  "Display",    "CustomFOV",                     "IDS_HELP_FIELD_OF_VIEW",               false, nullptr, false, "IDS_FIELD_OF_VIEW",          1,   70,  130, 1,  &CustomFOVText },
        { CUSTOM_SCREEN_DISPLAY,  "Display",    "HUDScaling",                    "IDS_HELP_HUD_SCALING",                 false, L"HUD Scaling" },
        { CUSTOM_SCREEN_DISPLAY,  "Display",    "HUDCustomScalingFactor",        "IDS_HELP_HUD_SIZE",                    false, nullptr, false, "IDS_HUD_SIZE",               100, 50,  200, 5,  &HUDSizeText },
        { CUSTOM_SCREEN_DISPLAY,  "Display",    "SmallTextCustomScalingFactor",  "IDS_HELP_SMALL_TEXT_SIZE",             false, nullptr, false, "IDS_SMALL_TEXT_SIZE",        100, 50,  200, 5,  &MultiplierText },
        { CUSTOM_SCREEN_DISPLAY,  "Display",    "DisableLetterbox",              "IDS_HELP_LETTERBOX",                   false, L"Letterbox", true },

        // Mouse
        { CUSTOM_SCREEN_MOUSE,    "Controller", "MouseAimMultiplier",            "IDS_HELP_SENSITIVITY_MULTIPLIER",      false, nullptr, false, "IDS_SENSITIVITY_MULTIPLIER", 100, 5,   200, 5,  &MultiplierText },

        // Memory
        { CUSTOM_SCREEN_MEMORY,   "Fixes",      "ReduceTextureMemory",           "IDS_HELP_REDUCE_TEXTURE_MEMORY",       true,  L"Reduce Texture Memory" },

        // Extra
        { CUSTOM_SCREEN_EXTRA,    "Extra",      "InfiniteFlashlight",            "IDS_HELP_INFINITE_FLASHLIGHT",         true,  L"Infinite Flashlight" },
        { CUSTOM_SCREEN_EXTRA,    "Extra",      "EnableCustomMaxWeaponCapacity", "IDS_HELP_CUSTOM_WEAPON_CAPACITY",      true,  L"Custom Weapon Capacity" },
        { CUSTOM_SCREEN_EXTRA,    "Extra",      "MaxWeaponCapacity",             "IDS_HELP_WEAPON_CAPACITY",             true,  nullptr, false, "IDS_WEAPON_CAPACITY",        1,   0,   10,  1,  &NumberText },
        { CUSTOM_SCREEN_EXTRA,    "Extra",      "DoorBashing",                   "IDS_HELP_DOOR_BASHING",                true,  L"Door Bashing" },
        { CUSTOM_SCREEN_EXTRA,    "Extra",      "UncapDismemberment",            "IDS_HELP_UNCAP_DISMEMBERMENT",         true,  L"Uncap Dismemberment" },
        { CUSTOM_SCREEN_EXTRA,    "Extra",      "GibFrequencyCap",               "IDS_HELP_GIB_FREQUENCY_CAP",           true,  nullptr, false, "IDS_GIB_FREQUENCY_CAP",      1,   0,   10,  1,  &NumberText },
        { CUSTOM_SCREEN_EXTRA,    "Extra",      "DisableHipFireAccuracyPenalty", "IDS_HELP_ACCURACY_PENALTY",            true,  L"Accuracy Penalty", true },
    };

    // Local settings
    static bool g_Toggles[SETTING_COUNT] = {};
    static int g_SliderPositions[SETTING_COUNT] = {};

    // Controls of the built screens, to disable the ones whose required setting is off
    static int* s_controls[SETTING_COUNT] = {};

    // Values of EchoPatch.ini when the game started and when the menu was shown
    static float s_startupValues[SETTING_COUNT] = {};
    static float s_shownValues[SETTING_COUNT] = {};

    static int* s_customScreens[CUSTOM_SCREEN_COUNT] = {};
    static int s_optionsScreenId = -1;
    static uintptr_t s_optionsVtable = 0;
    static uintptr_t s_vtable[SCREEN_VTABLE_SIZE] = {};
    static float s_mouseAimSensitivity = MOUSE_AIM_SENSITIVITY;

    // =========================================================================
    // EchoPatch Menu
    // =========================================================================

    static int GetCustomScreenIndex(int screenId)
    {
        int index = screenId - CUSTOM_SCREEN_ID_BASE;
        return (index >= 0 && index < CUSTOM_SCREEN_COUNT) ? index : -1;
    }

    // -------------------------------------------------------------------------
    // Settings
    // -------------------------------------------------------------------------

    static bool IsSlider(const Setting& setting)
    {
        return setting.labelId != nullptr;
    }

    static bool IsSettingShown(int index)
    {
        switch (index)
        {
            // Extraction Point and Perseus Mandate already have it
            case DOOR_BASHING:
                return g_State.CurrentFEARGame == FEAR;
        }

        return true;
    }

    static int GetRequiredSetting(int index)
    {
        switch (index)
        {
            case SAVE_PERSISTENT_WORLD_STATE:
                return ENABLE_PERSISTENT_WORLD_STATE;

            case MAX_WEAPON_CAPACITY:
                return ENABLE_CUSTOM_MAX_WEAPON_CAPACITY;

            case GIB_FREQUENCY_CAP:
                return UNCAP_DISMEMBERMENT;
        }

        return -1;
    }

    static bool IsRequiredSetting(int index)
    {
        for (int i = 0; i < SETTING_COUNT; i++)
        {
            if (GetRequiredSetting(i) == index)
                return true;
        }

        return false;
    }

    static float GetCurrentValue(int index)
    {
        switch (index)
        {
            case HIGH_RESOLUTION_REFLECTIONS: return HighResolutionReflections;
            case SSAA_SCALE: return SSAAScale;
            case NO_LOD_BIAS: return NoLODBias;
            case ENABLE_PERSISTENT_WORLD_STATE: return EnablePersistentWorldState;
            case SAVE_PERSISTENT_WORLD_STATE: return SavePersistentWorldState;
            case CUSTOM_FOV: return CustomFOV;
            case HUD_SCALING: return HUDScaling;
            case HUD_CUSTOM_SCALING_FACTOR: return HUDCustomScalingFactor;
            case SMALL_TEXT_CUSTOM_SCALING_FACTOR: return SmallTextCustomScalingFactor;
            case DISABLE_LETTERBOX: return DisableLetterbox;
            case MOUSE_AIM_MULTIPLIER: return MouseAimMultiplier;
            case REDUCE_TEXTURE_MEMORY: return ReduceTextureMemory;
            case INFINITE_FLASHLIGHT: return InfiniteFlashlight;
            case ENABLE_CUSTOM_MAX_WEAPON_CAPACITY: return EnableCustomMaxWeaponCapacity;
            case MAX_WEAPON_CAPACITY: return static_cast<float>(MaxWeaponCapacity);
            case DOOR_BASHING: return DoorBashing;
            case UNCAP_DISMEMBERMENT: return UncapDismemberment;
            case GIB_FREQUENCY_CAP: return static_cast<float>(GibFrequencyCap);
            case DISABLE_HIP_FIRE_ACCURACY_PENALTY: return DisableHipFireAccuracyPenalty;
        }

        return 0.0f;
    }

    static float ReadSetting(int index)
    {
        const Setting& setting = s_settings[index];

        if (IsSlider(setting))
            return IniHelper::ReadFloat(setting.section, setting.key, GetCurrentValue(index));

        return IniHelper::ReadInteger(setting.section, setting.key, static_cast<int>(GetCurrentValue(index))) == 1 ? 1.0f : 0.0f;
    }

    static float GetSettingValue(int index)
    {
        const Setting& setting = s_settings[index];

        if (!IsSlider(setting))
            return g_Toggles[index] ? 1.0f : 0.0f;

        // 0 = Disabled
        if (index == CUSTOM_FOV && g_SliderPositions[index] == setting.min)
            return 0.0f;

        return static_cast<float>(g_SliderPositions[index]) / setting.scale;
    }

    static std::string GetIniValue(int index, float value)
    {
        if (!IsSlider(s_settings[index]) || s_settings[index].scale == 1)
            return std::to_string(static_cast<int>(value));

        return std::to_string(value);
    }

    static void LoadSettings()
    {
        for (int i = 0; i < SETTING_COUNT; i++)
        {
            const Setting& setting = s_settings[i];
            float value = ReadSetting(i);

            if (IsSlider(setting))
            {
                int position = static_cast<int>(std::lround(value * setting.scale / setting.step)) * setting.step;
                g_SliderPositions[i] = (i == CUSTOM_FOV && value == 0.0f) ? setting.min : std::clamp(position, setting.min, setting.max);
            }
            else
            {
                g_Toggles[i] = value != 0.0f;
            }

            // Only saved if changed, a value outside of the range of the slider is kept otherwise
            s_shownValues[i] = GetSettingValue(i);
        }
    }

    static bool IsRestartPending()
    {
        for (int i = 0; i < SETTING_COUNT; i++)
        {
            if (s_settings[i].requiresRestart && ReadSetting(i) != s_startupValues[i])
                return true;
        }

        return false;
    }

    static void UpdateRequiredSettingToggles(int screenIndex)
    {
        for (int i = 0; i < SETTING_COUNT; i++)
        {
            int requiredSetting = GetRequiredSetting(i);

            if (s_settings[i].screen == screenIndex && requiredSetting >= 0 && s_controls[i])
            {
                CallControlEnable(s_controls[i], g_Toggles[requiredSetting]);
            }
        }
    }

    // -------------------------------------------------------------------------
    // Screen building
    // -------------------------------------------------------------------------

    // Same as the options
    static bool BuildEchoPatchScreen(int* thisPtr)
    {
        CallCreateTitleByName(thisPtr, L"ECHOPATCH");

        int width = thisPtr[SCREEN_RECT_RIGHT_INDEX] - thisPtr[SCREEN_RECT_LEFT_INDEX];
        int fontSize = GetLayoutInt32(thisPtr[SCREEN_LAYOUT_INDEX], "FontSize");

        thisPtr[SCREEN_DEFAULT_POS_X_INDEX] = thisPtr[SCREEN_RECT_LEFT_INDEX];
        thisPtr[SCREEN_DEFAULT_POS_Y_INDEX] = thisPtr[SCREEN_RECT_TOP_INDEX];

        for (int i = CUSTOM_SCREEN_GRAPHICS; i < CUSTOM_SCREEN_COUNT; i++)
        {
            const Category& category = s_categories[i - CUSTOM_SCREEN_GRAPHICS];
            AddTextItem(thisPtr, category.label, category.helpId, CMD_OPEN_CUSTOM_SCREEN, i, width, fontSize);
        }

        if (!CallBaseScreenBuild(thisPtr))
            return false;

        CallUseBack(thisPtr, 1, 1, 0);
        return true;
    }

    static bool BuildCategoryScreen(int* thisPtr, int screenIndex)
    {
        const Category& category = s_categories[screenIndex - CUSTOM_SCREEN_GRAPHICS];

        LoadSettings();

        int screenLeft = thisPtr[SCREEN_RECT_LEFT_INDEX];
        int screenTop = thisPtr[SCREEN_RECT_TOP_INDEX];

        CallCreateTitleByName(thisPtr, category.title);

        int kGap = 270;
        int kWidth = 180;
        int fontSize = 16;
        int numControls = 0;
        int rowHeight = fontSize + thisPtr[SCREEN_ITEM_SPACING_INDEX];

        for (int i = 0; i < SETTING_COUNT; i++)
        {
            if (s_settings[i].screen == screenIndex && IsSettingShown(i))
                numControls++;
        }

        int frameLeft = screenLeft + kGap - 5;
        int frameTop = screenTop - 5;
        int frameRight = screenLeft + kGap + kWidth + 10;
        int frameBottom = screenTop + rowHeight * numControls + 13;

        int frameTextureRef = CreateFrame(thisPtr, frameLeft, frameTop, frameRight, frameBottom);

        thisPtr[SCREEN_DEFAULT_POS_X_INDEX] = screenLeft;
        thisPtr[SCREEN_DEFAULT_POS_Y_INDEX] = screenTop;

        int toggleStruct[16];
        InitControlStruct(toggleStruct, 16, kGap + kWidth, fontSize);
        toggleStruct[13] = kGap;

        int sliderStruct[25];
        InitControlStruct(sliderStruct, 25, kGap + kWidth, fontSize);
        sliderStruct[15] = kGap;
        sliderStruct[21] = 80;

        for (int i = 0; i < SETTING_COUNT; i++)
        {
            const Setting& setting = s_settings[i];

            if (setting.screen != screenIndex || !IsSettingShown(i))
                continue;

            if (IsSlider(setting))
            {
                sliderStruct[1] = (int)setting.helpId;
                sliderStruct[17] = setting.min;
                sliderStruct[18] = setting.max;
                sliderStruct[19] = setting.step;
                sliderStruct[20] = (int)&g_SliderPositions[i];
                sliderStruct[23] = (int)setting.text;
                s_controls[i] = CallAddSlider(thisPtr, setting.labelId, sliderStruct, fontSize);
                continue;
            }

            // A required setting sends its changes to CustomScreen_OnCommand, with its index
            bool isRequiredSetting = IsRequiredSetting(i);
            toggleStruct[0] = isRequiredSetting ? CMD_REQUIRED_SETTING_CHANGED : 0;
            toggleStruct[1] = (int)setting.helpId;
            toggleStruct[2] = isRequiredSetting ? (int)thisPtr : 0;
            toggleStruct[3] = i;
            toggleStruct[15] = (int)&g_Toggles[i];

            int* toggle = CallAddToggleByName(thisPtr, setting.label, toggleStruct, fontSize);
            s_controls[i] = toggle;

            if (toggle)
            {
                CallAddControl(thisPtr, toggle);
                CallSetOnString(toggle, setting.inverted ? L"Off" : L"On");
                CallSetOffString(toggle, setting.inverted ? L"On" : L"Off");
            }
        }

        UpdateRequiredSettingToggles(screenIndex);

        // Only if the game can't use 4GB of memory yet
        if (screenIndex == CUSTOM_SCREEN_MEMORY && !LAAPatcher::HasLargeAddressSpace())
        {
            thisPtr[SCREEN_DEFAULT_POS_X_INDEX] = screenLeft;
            thisPtr[SCREEN_DEFAULT_POS_Y_INDEX] = frameBottom + thisPtr[SCREEN_ITEM_SPACING_INDEX];
            AddTextItem(thisPtr, L"Apply 4GB Patch", "IDS_HELP_4GB_PATCH", CMD_APPLY_LAA_PATCH, 0, kGap, fontSize);
        }

        bool buildResult = CallBaseScreenBuild(thisPtr);
        ReleaseTextureRef(frameTextureRef);

        if (!buildResult)
            return false;

        CallUseBack(thisPtr, 1, 1, 0);
        return true;
    }

    static bool BuildCustomScreen(int* thisPtr, int screenIndex)
    {
        return (screenIndex == CUSTOM_SCREEN_ECHOPATCH) ? BuildEchoPatchScreen(thisPtr) : BuildCategoryScreen(thisPtr, screenIndex);
    }

    // -------------------------------------------------------------------------
    // Screen management
    // -------------------------------------------------------------------------

    static int* GetOrCreateCustomScreen(int* currentScreen, int index)
    {
        if (s_customScreens[index])
            return s_customScreens[index];

        if (s_optionsScreenId < 0)
            return nullptr;

        // Created by CScreenMgr, which also frees it like its own screens
        int screenMgr = currentScreen[SCREEN_MGR_INDEX];
        reinterpret_cast<AddScreen_t>(AddScreen_Addr)(screenMgr, s_optionsScreenId);

        int* screen = reinterpret_cast<int**>(MemoryHelper::ReadMemory<uintptr_t>(screenMgr + SCREENMGR_SCREENS_END_OFFSET))[-1];
        if (*(uintptr_t*)screen != s_optionsVtable)
            return nullptr;

        // The layout record of the options was already used by Init
        *reinterpret_cast<uintptr_t**>(screen) = s_vtable;
        screen[SCREEN_ID_INDEX] = CUSTOM_SCREEN_ID_BASE + index;

        s_customScreens[index] = screen;
        return screen;
    }

    static void OpenCustomScreen(int* currentScreen, int index)
    {
        if (index < 0 || index >= CUSTOM_SCREEN_COUNT)
            return;

        if (!GetOrCreateCustomScreen(currentScreen, index))
            return;

        int screenMgr = currentScreen[SCREEN_MGR_INDEX];
        reinterpret_cast<SetCurrentScreen_t>(GetVirtualFunction((void*)screenMgr, VTBL_SCREENMGR_SET_CURRENT_SCREEN))(screenMgr, CUSTOM_SCREEN_ID_BASE + index);
    }

    // -------------------------------------------------------------------------
    // Settings changes
    // -------------------------------------------------------------------------

    // Value asked by the game, kept by the console unless EchoPatch changed it to 0
    static float GetGameValue(const char* varName, float lastValue)
    {
        float value = EngineInterface::GetVariableFloat(EngineInterface::GetClient(), varName);
        return (value != 0.0f) ? value : lastValue;
    }

    // Returns true if the renderer must be restarted to turn FSAA on or off
    static bool ApplySSAAScale(float scale)
    {
        bool wasEnabled = SSAAScale != 1.0f;
        g_State.antiAliasFSOverSample = GetGameValue("AntiAliasFSOverSample", g_State.antiAliasFSOverSample);
        SSAAScale = scale;

        // Turned off while supersampling
        SetConsoleVariableFloat_Hook("AntiAliasFSOverSample", g_State.antiAliasFSOverSample);
        return wasEnabled != (scale != 1.0f) && g_State.antiAliasFSOverSample != 0.0f;
    }

    static void ApplyNoLODBias(bool enabled)
    {
        NoLODBias = enabled;

        // Values asked by the game, changed while the setting is on
        SetConsoleVariableFloat_Hook("ModelLODDistanceScale", g_State.modelLODDistanceScale);
        SetConsoleVariableFloat_Hook("CameraFirstPersonLODBias", g_State.cameraFirstPersonLODBias);
    }

    static void ApplyDisableLetterbox(bool enabled)
    {
        DisableLetterbox = enabled;

        if (g_State.letterboxAddr != 0)
        {
            MemoryHelper::WriteMemory<uint8_t>(g_State.letterboxAddr, enabled ? 0xC3 : 0x83);
        }
    }

    static void ApplyCustomFOV(float fov)
    {
        CustomFOV = fov;

        float fovY = (fov != 0.0f) ? fov : DEFAULT_FOV;
        SetConsoleVariableFloat("FovY", fovY);
        SetConsoleVariableFloat("FovYWidescreen", fovY);
        SetConsoleVariableFloat("CameraAttachedOffsetZ", (fov != 0.0f) ? GetCameraOffset(fov) : 0.0f);
    }

    static void ApplyHUDScaling()
    {
        // Once calculated for the resolution
        if (g_State.currentWidth != 0)
        {
            UpdateHUDScaling();
        }
    }

    static void ApplyHUDScaling(bool enabled)
    {
        g_State.useTextScaling = GetGameValue("UseTextScaling", g_State.useTextScaling);
        HUDScaling = enabled;

        // Turned off while scaling the HUD
        SetConsoleVariableFloat_Hook("UseTextScaling", g_State.useTextScaling);
        ApplyHUDScaling();
    }

    static void ApplyMouseAimMultiplier(float multiplier)
    {
        MouseAimMultiplier = multiplier;

        if (MouseAimSensitivity_Addr == 0)
            return;

        // Also applied when the multiplier is 1.0 at startup, unlike ApplyMouseAimMultiplierClientPatch
        s_mouseAimSensitivity = MOUSE_AIM_SENSITIVITY * multiplier;
        MemoryHelper::WriteMemory<uint32_t>(MouseAimSensitivity_Addr, reinterpret_cast<uintptr_t>(&s_mouseAimSensitivity));
    }

    // Returns true if the renderer must be restarted
    static bool ApplySetting(int index, float value)
    {
        switch (index)
        {
            case HIGH_RESOLUTION_REFLECTIONS:
                // The render targets are created again by RenderTargetGroupFXUpdate_Hook
                HighResolutionReflections = value != 0.0f;
                break;

            case SSAA_SCALE:
                return ApplySSAAScale(value);

            case NO_LOD_BIAS:
                ApplyNoLODBias(value != 0.0f);
                break;

            case CUSTOM_FOV:
                ApplyCustomFOV(value);
                break;

            case HUD_SCALING:
                ApplyHUDScaling(value != 0.0f);
                break;

            case HUD_CUSTOM_SCALING_FACTOR:
                HUDCustomScalingFactor = value;
                ApplyHUDScaling();
                break;

            case SMALL_TEXT_CUSTOM_SCALING_FACTOR:
                SmallTextCustomScalingFactor = value;
                ApplyHUDScaling();
                break;

            case MOUSE_AIM_MULTIPLIER:
                ApplyMouseAimMultiplier(value);
                break;

            case DISABLE_LETTERBOX:
                ApplyDisableLetterbox(value != 0.0f);
                break;
        }

        return false;
    }

    static void RestartGame(bool applyLAAPatch)
    {
        wchar_t exePath[MAX_PATH];
        DWORD length = GetModuleFileNameW(NULL, exePath, MAX_PATH);

        // Not closed normally, the game doesn't expect it in the middle of a frame
        if (length > 0 && length < MAX_PATH && LAAPatcher::StartNewInstance(exePath, applyLAAPatch))
        {
            TerminateProcess(GetCurrentProcess(), 0);
        }

        ShowMessageBox(L"The game could not be restarted, please restart it manually.");
    }

    static void __cdecl RestartConfirm_Callback(bool bReturn, void*, void*)
    {
        if (bReturn)
        {
            RestartGame(false);
        }
    }

    static void __cdecl LAAPatchConfirm_Callback(bool bReturn, void*, void*)
    {
        if (bReturn)
        {
            RestartGame(true);
        }
    }

    // Same question as at startup, the patch is applied by the next instance
    static void ShowLAAPatchConfirm()
    {
        wchar_t exePath[MAX_PATH];
        if (GetModuleFileNameW(NULL, exePath, MAX_PATH) == 0)
            return;

        std::string question = LAAPatcher::GetPatchQuestion(fs::path(exePath).filename(), g_State.CurrentFEARGame == FEAR);
        ShowMessageBox(std::wstring(question.begin(), question.end()).c_str(), &LAAPatchConfirm_Callback);
    }

    // Returns true if the renderer must be restarted
    static bool ApplySettings(int* screen)
    {
        bool isSaveNeeded = false;
        bool isRestartSettingChanged = false;
        bool isRestartRenderNeeded = false;

        // Copy the values of the controls to g_Toggles / g_SliderPositions
        CallScreenUpdateData(screen, true);

        for (int i = 0; i < SETTING_COUNT; i++)
        {
            float value = GetSettingValue(i);
            if (value == s_shownValues[i])
                continue;

            const Setting& setting = s_settings[i];
            IniHelper::iniReader[setting.section][setting.key] = GetIniValue(i, value);
            s_shownValues[i] = value;
            isSaveNeeded = true;

            if (setting.requiresRestart)
            {
                isRestartSettingChanged = true;
            }
            else if (ApplySetting(i, value))
            {
                isRestartRenderNeeded = true;
            }
        }

        if (!isSaveNeeded)
            return false;

        IniHelper::Save();

        if (isRestartSettingChanged && IsRestartPending())
        {
            ShowMessageBox(L"Some changes require a restart.\n\nRestart the game now?", &RestartConfirm_Callback, nullptr, false);
        }

        return isRestartRenderNeeded;
    }

    // =========================================================================
    // Logic & Hooks
    // =========================================================================

    static bool __fastcall CustomScreen_Build(int* thisPtr, int)
    {
        return BuildCustomScreen(thisPtr, GetCustomScreenIndex(thisPtr[SCREEN_ID_INDEX]));
    }

    static uint32_t __fastcall CustomScreen_OnCommand(int* thisPtr, int, uint32_t command, uint32_t param1, uint32_t param2)
    {
        switch (command)
        {
            case CMD_OPEN_CUSTOM_SCREEN:
                OpenCustomScreen(thisPtr, static_cast<int>(param1));
                return 1;

            case CMD_APPLY_LAA_PATCH:
                ShowLAAPatchConfirm();
                return 1;

            case CMD_REQUIRED_SETTING_CHANGED:
            {
                int index = static_cast<int>(param1);
                if (index >= 0 && index < SETTING_COUNT && s_controls[index])
                {
                    CallControlUpdateData(s_controls[index], true);
                    UpdateRequiredSettingToggles(GetCustomScreenIndex(thisPtr[SCREEN_ID_INDEX]));
                }
                return 1;
            }
        }

        return CScreenOptions_OnCommand(thisPtr, command, param1, param2);
    }

    static void __fastcall CustomScreen_OnFocus(int* thisPtr, int, bool bFocus)
    {
        bool isRestartRenderNeeded = false;
        int screenIndex = GetCustomScreenIndex(thisPtr[SCREEN_ID_INDEX]);

        if (screenIndex != CUSTOM_SCREEN_ECHOPATCH)
        {
            if (bFocus)
            {
                LoadSettings();
                UpdateRequiredSettingToggles(screenIndex);
            }
            else
            {
                isRestartRenderNeeded = ApplySettings(thisPtr);
            }
        }

        CScreenOptions_OnFocus(thisPtr, bFocus);

        // Same as the display options
        if (isRestartRenderNeeded)
        {
            RestartRender();
        }
    }

    static uint32_t __fastcall CScreenOptions_OnCommand_Hook(int* thisPtr, int, uint32_t command, uint32_t param1, uint32_t param2)
    {
        if (command == CMD_OPEN_CUSTOM_SCREEN)
        {
            OpenCustomScreen(thisPtr, CUSTOM_SCREEN_ECHOPATCH);
            return 1;
        }

        return CScreenOptions_OnCommand(thisPtr, command, param1, param2);
    }

    static void InitCustomScreens(int* optionsScreen)
    {
        s_optionsScreenId = optionsScreen[SCREEN_ID_INDEX];
        s_optionsVtable = *(uintptr_t*)optionsScreen;
        CScreenOptions_OnFocus = reinterpret_cast<decltype(CScreenOptions_OnFocus)>(GetVirtualFunction(optionsScreen, VTBL_SCREEN_ON_FOCUS));

        memcpy(s_vtable, *reinterpret_cast<uintptr_t**>(optionsScreen), sizeof(s_vtable));
        s_vtable[VTBL_SCREEN_ON_COMMAND] = reinterpret_cast<uintptr_t>(&CustomScreen_OnCommand);
        s_vtable[VTBL_SCREEN_BUILD] = reinterpret_cast<uintptr_t>(&CustomScreen_Build);
        s_vtable[VTBL_SCREEN_ON_FOCUS] = reinterpret_cast<uintptr_t>(&CustomScreen_OnFocus);
    }

    static bool __fastcall BaseScreenBuild_Hook(int* thisPtr, int)
    {
        // CScreenOptions::Build, before the back button is added
        if (GetVirtualFunction(thisPtr, VTBL_SCREEN_BUILD) != CScreenOptions_Build_Addr)
            return BaseScreenBuild(thisPtr);

        // CScreenOptions::OnCommand, taken from the vtable of the screen
        if (CScreenOptions_OnCommand || HookHelper::ApplyHook((void*)GetVirtualFunction(thisPtr, VTBL_SCREEN_ON_COMMAND), &CScreenOptions_OnCommand_Hook, (LPVOID*)&CScreenOptions_OnCommand))
        {
            InitCustomScreens(thisPtr);

            // Same size as the other items
            int width = thisPtr[SCREEN_RECT_RIGHT_INDEX] - thisPtr[SCREEN_RECT_LEFT_INDEX];
            int fontSize = GetLayoutInt32(thisPtr[SCREEN_LAYOUT_INDEX], "FontSize");
            AddTextItem(thisPtr, L"EchoPatch", "IDS_HELP_ECHOPATCH", CMD_OPEN_CUSTOM_SCREEN, CUSTOM_SCREEN_ECHOPATCH, width, fontSize);
        }

        return BaseScreenBuild(thisPtr);
    }

    // =========================================================================
    // Initialization
    // =========================================================================

    static void InstallHook()
    {
        if (!ScreenHelper::Initialize())
            return;

        DWORD addr_CScreenOptions_Build = ScanModuleSignature(g_State.GameClient, "83 EC 34 53 55 56 57 68 ?? ?? ?? ?? 8B D9 E8 ?? ?? ?? ?? 8B BB F0 00 00 00 8B B3 F8 00 00 00 8B 0D", "CScreenOptions_Build");
        DWORD addr_AddScreen = ScanModuleSignature(g_State.GameClient, "56 57 8B 7C 24 0C 8D 47 FF 83 F8 ?? 8B F1 0F 87", "AddScreen");
        DWORD addr_MouseAimMultiplier = ScanModuleSignature(g_State.GameClient, "89 4C 24 14 DB 44 24 14 8D 44 24 20 6A 01 50 D8 0D", "MouseAimMultiplier", -1, false);

        if (CreateTextItem_Addr == 0 ||
            addr_CScreenOptions_Build == 0 ||
            addr_AddScreen == 0)
        {
            return;
        }

        CScreenOptions_Build_Addr = addr_CScreenOptions_Build;
        AddScreen_Addr = addr_AddScreen;
        LTDatabase_Addr = MemoryHelper::ReadMemory<uintptr_t>(addr_CScreenOptions_Build + 0x21);

        if (addr_MouseAimMultiplier != 0)
        {
            MouseAimSensitivity_Addr = addr_MouseAimMultiplier + 0x11;
        }

        for (int i = 0; i < SETTING_COUNT; i++)
        {
            s_startupValues[i] = ReadSetting(i);
        }

        HookHelper::ApplyHook((void*)BaseScreenBuild_Addr, &BaseScreenBuild_Hook, (LPVOID*)&BaseScreenBuild);
    }
}