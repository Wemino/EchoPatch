#pragma once

#include "Globals.cpp"
#include "LAAPatcher.hpp"
#include "CrashHandler.hpp"
#include "Controller/Controller.hpp"

// WinAPI function pointers
HWND(WINAPI* ori_CreateWindowExA)(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);

static void ReadConfig()
{
    IniHelper::Init(g_State.IsExpansion());

    // Fixes
    DisableRedundantHIDInit = IniHelper::ReadInteger("Fixes", "DisableRedundantHIDInit", 1) == 1;
    HighFPSFixes = IniHelper::ReadInteger("Fixes", "HighFPSFixes", 1) == 1;
    OptimizeSaveSpeed = IniHelper::ReadInteger("Fixes", "OptimizeSaveSpeed", 1) == 1;
    FixNvidiaShadowCorruption = IniHelper::ReadInteger("Fixes", "FixNvidiaShadowCorruption", 1) == 1;
    FixAspectRatioBlur = IniHelper::ReadInteger("Fixes", "FixAspectRatioBlur", 1) == 1;
    FastVRAMDetection = IniHelper::ReadInteger("Fixes", "FastVRAMDetection", 1) == 1;
    DisableXPWidescreenFiltering = IniHelper::ReadInteger("Fixes", "DisableXPWidescreenFiltering", 1) == 1;
    FixKeyboardInputLanguage = IniHelper::ReadInteger("Fixes", "FixKeyboardInputLanguage", 1) == 1;
    WeaponFixes = IniHelper::ReadInteger("Fixes", "WeaponFixes", 1) == 1;
    FixSoundWrapperLoading = IniHelper::ReadInteger("Fixes", "FixSoundWrapperLoading", 1) == 1;
    FixScriptedAnimationCrash = IniHelper::ReadInteger("Fixes", "FixScriptedAnimationCrash", 1) == 1;
    CheckLAAPatch = IniHelper::ReadInteger("Fixes", "CheckLAAPatch", 1);

    // Graphics
    MaxFPS = IniHelper::ReadFloat("Graphics", "MaxFPS", 300.0f);
    DynamicVsync = IniHelper::ReadInteger("Graphics", "DynamicVsync", 1) == 1;
    HighResolutionReflections = IniHelper::ReadInteger("Graphics", "HighResolutionReflections", 1) == 1;
    NoLODBias = IniHelper::ReadInteger("Graphics", "NoLODBias", 1) == 1;
    SSAAScale = IniHelper::ReadFloat("Graphics", "SSAAScale", 1.0f);
    ReducedMipMapBias = IniHelper::ReadInteger("Graphics", "ReducedMipMapBias", 1) == 1;
    EnablePersistentWorldState = IniHelper::ReadInteger("Graphics", "EnablePersistentWorldState", 1) == 1;

    // Display
    CustomFOV = IniHelper::ReadFloat("Display", "CustomFOV", 0.0f);
    HUDScaling = IniHelper::ReadInteger("Display", "HUDScaling", 1) == 1;
    HUDCustomScalingFactor = IniHelper::ReadFloat("Display", "HUDCustomScalingFactor", 1.0f);
    SmallTextCustomScalingFactor = IniHelper::ReadFloat("Display", "SmallTextCustomScalingFactor", 1.0f);
    AutoResolution = IniHelper::ReadInteger("Display", "AutoResolution", 1);
    DisableLetterbox = IniHelper::ReadInteger("Display", "DisableLetterbox", 0) == 1;
    ForceWindowed = IniHelper::ReadInteger("Display", "ForceWindowed", 0) == 1;
    FixWindowStyle = IniHelper::ReadInteger("Display", "FixWindowStyle", 1) == 1;

    // Controller
    MouseAimMultiplier = IniHelper::ReadFloat("Controller", "MouseAimMultiplier", 1.0f);
    SDLGamepadSupport = IniHelper::ReadInteger("Controller", "SDLGamepadSupport", 1) == 1;
    RumbleEnabled = IniHelper::ReadInteger("Controller", "RumbleEnabled", 0) == 1;
    GyroEnabled = IniHelper::ReadInteger("Controller", "GyroEnabled", 0) == 1;
    GyroAimingMode = IniHelper::ReadInteger("Controller", "GyroAimingMode", 0);
    GyroSensitivity = IniHelper::ReadFloat("Controller", "GyroSensitivity", 1.0f);
    GyroSmoothing = IniHelper::ReadFloat("Controller", "GyroSmoothing", 0.016f);
    GyroCalibrationPersistence = IniHelper::ReadInteger("Controller", "GyroCalibrationPersistence", 1) == 1;
    TouchpadEnabled = IniHelper::ReadInteger("Controller", "TouchpadEnabled", 1) == 1;
    HideMouseCursor = IniHelper::ReadInteger("Controller", "HideMouseCursor", 1) == 1;
    GPadAimSensitivity = IniHelper::ReadFloat("Controller", "GPadAimSensitivity", 2.0f);
    GPadAimEdgeThreshold = IniHelper::ReadFloat("Controller", "GPadAimEdgeThreshold", 0.75f);
    GPadAimEdgeAccelTime = IniHelper::ReadFloat("Controller", "GPadAimEdgeAccelTime", 1.0f);
    GPadAimEdgeDelayTime = IniHelper::ReadFloat("Controller", "GPadAimEdgeDelayTime", 0.25f);
    GPadAimEdgeMultiplier = IniHelper::ReadFloat("Controller", "GPadAimEdgeMultiplier", 1.6f);
    GPadAimAspectRatio = IniHelper::ReadFloat("Controller", "GPadAimAspectRatio", 1.0f);
    GPadZoomMagThreshold = IniHelper::ReadFloat("Controller", "GPadZoomMagThreshold", 1.3f);
    GAMEPAD_SOUTH = IniHelper::ReadInteger("Controller", "GAMEPAD_SOUTH", 15);
    GAMEPAD_SOUTH_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_SOUTH_HOLD", -1);
    GAMEPAD_SOUTH_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_SOUTH_HOLD_TIME", 500);
    GAMEPAD_EAST = IniHelper::ReadInteger("Controller", "GAMEPAD_EAST", 14);
    GAMEPAD_EAST_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_EAST_HOLD", -1);
    GAMEPAD_EAST_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_EAST_HOLD_TIME", 500);
    GAMEPAD_WEST = IniHelper::ReadInteger("Controller", "GAMEPAD_WEST", 88);
    GAMEPAD_WEST_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_WEST_HOLD", -1);
    GAMEPAD_WEST_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_WEST_HOLD_TIME", 500);
    GAMEPAD_NORTH = IniHelper::ReadInteger("Controller", "GAMEPAD_NORTH", 106);
    GAMEPAD_NORTH_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_NORTH_HOLD", -1);
    GAMEPAD_NORTH_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_NORTH_HOLD_TIME", 500);
    GAMEPAD_LEFT_STICK = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_STICK", 70);
    GAMEPAD_LEFT_STICK_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_STICK_HOLD", -1);
    GAMEPAD_LEFT_STICK_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_STICK_HOLD_TIME", 500);
    GAMEPAD_RIGHT_STICK = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_STICK", 114);
    GAMEPAD_RIGHT_STICK_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_STICK_HOLD", -1);
    GAMEPAD_RIGHT_STICK_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_STICK_HOLD_TIME", 500);
    GAMEPAD_LEFT_SHOULDER = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_SHOULDER", 81);
    GAMEPAD_LEFT_SHOULDER_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_SHOULDER_HOLD", -1);
    GAMEPAD_LEFT_SHOULDER_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_SHOULDER_HOLD_TIME", 500);
    GAMEPAD_RIGHT_SHOULDER = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_SHOULDER", 19);
    GAMEPAD_RIGHT_SHOULDER_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_SHOULDER_HOLD", -1);
    GAMEPAD_RIGHT_SHOULDER_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_SHOULDER_HOLD_TIME", 500);
    GAMEPAD_DPAD_UP = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_UP", 77);
    GAMEPAD_DPAD_UP_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_UP_HOLD", 102);
    GAMEPAD_DPAD_UP_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_UP_HOLD_TIME", 500);
    GAMEPAD_DPAD_DOWN = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_DOWN", 73);
    GAMEPAD_DPAD_DOWN_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_DOWN_HOLD", 50);
    GAMEPAD_DPAD_DOWN_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_DOWN_HOLD_TIME", 500);
    GAMEPAD_DPAD_LEFT = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_LEFT", 20);
    GAMEPAD_DPAD_LEFT_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_LEFT_HOLD", -1);
    GAMEPAD_DPAD_LEFT_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_LEFT_HOLD_TIME", 500);
    GAMEPAD_DPAD_RIGHT = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_RIGHT", 21);
    GAMEPAD_DPAD_RIGHT_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_RIGHT_HOLD", -1);
    GAMEPAD_DPAD_RIGHT_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_RIGHT_HOLD_TIME", 500);
    GAMEPAD_LEFT_TRIGGER = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_TRIGGER", 71);
    GAMEPAD_LEFT_TRIGGER_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_TRIGGER_HOLD", -1);
    GAMEPAD_LEFT_TRIGGER_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_TRIGGER_HOLD_TIME", 500);
    GAMEPAD_RIGHT_TRIGGER = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_TRIGGER", 17);
    GAMEPAD_RIGHT_TRIGGER_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_TRIGGER_HOLD", -1);
    GAMEPAD_RIGHT_TRIGGER_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_TRIGGER_HOLD_TIME", 500);
    GAMEPAD_BACK = IniHelper::ReadInteger("Controller", "GAMEPAD_BACK", 78);
    GAMEPAD_BACK_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_BACK_HOLD", 74);
    GAMEPAD_BACK_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_BACK_HOLD_TIME", 500);
    GAMEPAD_START = IniHelper::ReadInteger("Controller", "GAMEPAD_START", 13);
    GAMEPAD_START_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_START_HOLD", 75);
    GAMEPAD_START_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_START_HOLD_TIME", 500);
    GAMEPAD_MISC1 = IniHelper::ReadInteger("Controller", "GAMEPAD_MISC1", 50);
    GAMEPAD_MISC1_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_MISC1_HOLD", -1);
    GAMEPAD_MISC1_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_MISC1_HOLD_TIME", 500);
    GAMEPAD_RIGHT_PADDLE1 = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_PADDLE1", 82);
    GAMEPAD_RIGHT_PADDLE1_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_PADDLE1_HOLD", -1);
    GAMEPAD_RIGHT_PADDLE1_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_PADDLE1_HOLD_TIME", 500);
    GAMEPAD_LEFT_PADDLE1 = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_PADDLE1", 75);
    GAMEPAD_LEFT_PADDLE1_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_PADDLE1_HOLD", -1);
    GAMEPAD_LEFT_PADDLE1_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_PADDLE1_HOLD_TIME", 500);
    GAMEPAD_RIGHT_PADDLE2 = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_PADDLE2", -1);
    GAMEPAD_RIGHT_PADDLE2_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_PADDLE2_HOLD", -1);
    GAMEPAD_RIGHT_PADDLE2_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_PADDLE2_HOLD_TIME", 500);
    GAMEPAD_LEFT_PADDLE2 = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_PADDLE2", -1);
    GAMEPAD_LEFT_PADDLE2_HOLD = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_PADDLE2_HOLD", -1);
    GAMEPAD_LEFT_PADDLE2_HOLD_TIME = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_PADDLE2_HOLD_TIME", 500);

    // SkipIntro
    SkipSplashScreen = IniHelper::ReadInteger("SkipIntro", "SkipSplashScreen", 1) == 1;
    SkipAllIntro = IniHelper::ReadInteger("SkipIntro", "SkipAllIntro", 0) == 1;
    SkipSierraIntro = IniHelper::ReadInteger("SkipIntro", "SkipSierraIntro", 1) == 1;
    SkipMonolithIntro = IniHelper::ReadInteger("SkipIntro", "SkipMonolithIntro", 0) == 1;
    SkipWBGamesIntro = IniHelper::ReadInteger("SkipIntro", "SkipWBGamesIntro", 1) == 1;
    SkipNvidiaIntro = IniHelper::ReadInteger("SkipIntro", "SkipNvidiaIntro", 1) == 1;
    SkipTimegateIntro = IniHelper::ReadInteger("SkipIntro", "SkipTimegateIntro", 1) == 1;
    SkipDellIntro = IniHelper::ReadInteger("SkipIntro", "SkipDellIntro", 1) == 1;

    // Console
    ConsoleEnabled = IniHelper::ReadInteger("Console", "ConsoleEnabled", 0) == 1;
    DebugLevel = IniHelper::ReadInteger("Console", "DebugLevel", 0);
    HighResolutionScaling = IniHelper::ReadInteger("Console", "HighResolutionScaling", 1) == 1;
    LogOutputToFile = IniHelper::ReadInteger("Console", "LogOutputToFile", 0) == 1;

    // Extra
    RedirectSaveFolder = IniHelper::ReadInteger("Extra", "RedirectSaveFolder", 0) == 1;
    InfiniteFlashlight = IniHelper::ReadInteger("Extra", "InfiniteFlashlight", 0) == 1;
    DisablePunkBuster = IniHelper::ReadInteger("Extra", "DisablePunkBuster", 1) == 1;
    EnableCustomMaxWeaponCapacity = IniHelper::ReadInteger("Extra", "EnableCustomMaxWeaponCapacity", 0) == 1;
    MaxWeaponCapacity = IniHelper::ReadInteger("Extra", "MaxWeaponCapacity", 3);
    DisableHipFireAccuracyPenalty = IniHelper::ReadInteger("Extra", "DisableHipFireAccuracyPenalty", 0) == 1;
    EnableCrashHandler = IniHelper::ReadInteger("Extra", "EnableCrashHandler", 1) == 1;
    ShowErrors = IniHelper::ReadInteger("Extra", "ShowErrors", 1) == 1;

    // Get screen resolution
    if (AutoResolution != 0)
    {
        auto [screenWidth, screenHeight] = SystemHelper::GetScreenResolution();
        g_State.screenWidth = screenWidth;
        g_State.screenHeight = screenHeight;
    }

    // Use VSync?
    if (DynamicVsync)
    {
        DWORD displayFrequency = SystemHelper::GetCurrentDisplayFrequency();
        g_State.useVsyncOverride = (MaxFPS >= displayFrequency || MaxFPS == 0.0f);
    }

    // Check if we should skip everything directly
    if (!SkipAllIntro && g_State.IsOriginalGame())
    {
        SkipAllIntro = SkipSierraIntro && SkipMonolithIntro && SkipWBGamesIntro && SkipNvidiaIntro;
    }
    else if (!SkipAllIntro && g_State.IsExpansion())
    {
        SkipAllIntro = SkipSierraIntro && SkipMonolithIntro && SkipNvidiaIntro && SkipTimegateIntro && SkipDellIntro;
    }

    if (SDLGamepadSupport)
    {
        ConfigureButton(SDL_GAMEPAD_BUTTON_SOUTH, GAMEPAD_SOUTH, GAMEPAD_SOUTH_HOLD, GAMEPAD_SOUTH_HOLD_TIME);
        ConfigureButton(SDL_GAMEPAD_BUTTON_EAST, GAMEPAD_EAST, GAMEPAD_EAST_HOLD, GAMEPAD_EAST_HOLD_TIME);
        ConfigureButton(SDL_GAMEPAD_BUTTON_WEST, GAMEPAD_WEST, GAMEPAD_WEST_HOLD, GAMEPAD_WEST_HOLD_TIME);
        ConfigureButton(SDL_GAMEPAD_BUTTON_NORTH, GAMEPAD_NORTH, GAMEPAD_NORTH_HOLD, GAMEPAD_NORTH_HOLD_TIME);
        ConfigureButton(SDL_GAMEPAD_BUTTON_LEFT_STICK, GAMEPAD_LEFT_STICK, GAMEPAD_LEFT_STICK_HOLD, GAMEPAD_LEFT_STICK_HOLD_TIME);
        ConfigureButton(SDL_GAMEPAD_BUTTON_RIGHT_STICK, GAMEPAD_RIGHT_STICK, GAMEPAD_RIGHT_STICK_HOLD, GAMEPAD_RIGHT_STICK_HOLD_TIME);
        ConfigureButton(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, GAMEPAD_LEFT_SHOULDER, GAMEPAD_LEFT_SHOULDER_HOLD, GAMEPAD_LEFT_SHOULDER_HOLD_TIME);
        ConfigureButton(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, GAMEPAD_RIGHT_SHOULDER, GAMEPAD_RIGHT_SHOULDER_HOLD, GAMEPAD_RIGHT_SHOULDER_HOLD_TIME);
        ConfigureButton(SDL_GAMEPAD_BUTTON_DPAD_UP, GAMEPAD_DPAD_UP, GAMEPAD_DPAD_UP_HOLD, GAMEPAD_DPAD_UP_HOLD_TIME);
        ConfigureButton(SDL_GAMEPAD_BUTTON_DPAD_DOWN, GAMEPAD_DPAD_DOWN, GAMEPAD_DPAD_DOWN_HOLD, GAMEPAD_DPAD_DOWN_HOLD_TIME);
        ConfigureButton(SDL_GAMEPAD_BUTTON_DPAD_LEFT, GAMEPAD_DPAD_LEFT, GAMEPAD_DPAD_LEFT_HOLD, GAMEPAD_DPAD_LEFT_HOLD_TIME);
        ConfigureButton(SDL_GAMEPAD_BUTTON_DPAD_RIGHT, GAMEPAD_DPAD_RIGHT, GAMEPAD_DPAD_RIGHT_HOLD, GAMEPAD_DPAD_RIGHT_HOLD_TIME);
        ConfigureButton(SDL_GAMEPAD_BUTTON_BACK, GAMEPAD_BACK, GAMEPAD_BACK_HOLD, GAMEPAD_BACK_HOLD_TIME);
        ConfigureButton(SDL_GAMEPAD_BUTTON_START, GAMEPAD_START, GAMEPAD_START_HOLD, GAMEPAD_START_HOLD_TIME);
        ConfigureButton(SDL_GAMEPAD_BUTTON_MISC1, GAMEPAD_MISC1, GAMEPAD_MISC1_HOLD, GAMEPAD_MISC1_HOLD_TIME);
        ConfigureButton(SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1, GAMEPAD_RIGHT_PADDLE1, GAMEPAD_RIGHT_PADDLE1_HOLD, GAMEPAD_RIGHT_PADDLE1_HOLD_TIME);
        ConfigureButton(SDL_GAMEPAD_BUTTON_LEFT_PADDLE1, GAMEPAD_LEFT_PADDLE1, GAMEPAD_LEFT_PADDLE1_HOLD, GAMEPAD_LEFT_PADDLE1_HOLD_TIME);
        ConfigureButton(SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2, GAMEPAD_RIGHT_PADDLE2, GAMEPAD_RIGHT_PADDLE2_HOLD, GAMEPAD_RIGHT_PADDLE2_HOLD_TIME);
        ConfigureButton(SDL_GAMEPAD_BUTTON_LEFT_PADDLE2, GAMEPAD_LEFT_PADDLE2, GAMEPAD_LEFT_PADDLE2_HOLD, GAMEPAD_LEFT_PADDLE2_HOLD_TIME);
        ConfigureTrigger(0, GAMEPAD_LEFT_TRIGGER, GAMEPAD_LEFT_TRIGGER_HOLD, GAMEPAD_LEFT_TRIGGER_HOLD_TIME);
        ConfigureTrigger(1, GAMEPAD_RIGHT_TRIGGER, GAMEPAD_RIGHT_TRIGGER_HOLD, GAMEPAD_RIGHT_TRIGGER_HOLD_TIME);
    }

    if (HUDScaling)
    {
        g_State.textDataMap =
        {
            { "Default10", { 10, 0 } },
            { "Default12", { 12, 0 } },
            { "Default14", { 14, 0 } },
            { "Default16", { 16, 0 } },
            { "Default18", { 18, 0 } },
            { "Default24", { 24, 0 } },
            { "Default32", { 32, 0 } },
            { "EndCredits16", { 16, 0 } },
            { "EndCredits18", { 18, 0 } },
            { "Intro16", { 16, 0 } },
            { "Objective", { 22, 0 } },
            { "Training", { 22, 0 } },

            // HUD Entries
            { "HUDActivate", { 18, 0 } },
            { "HUDActivateObject", { 14, 0 } },
            { "HUDAmmo", { 16, 0 } },
            { "HUDArmor", { 25, 0 } },
            { "HUDBuildVersion", { 12, 0 } },
            { "HUDChatInput", { 16, 2 } },
            { "HUDChatMessage", { 14, 2 } },
            { "HUDControlPoint", { 24, 0 } },
            { "HUDControlPointBar", { 24, 0 } },
            { "HUDControlPointList", { 24, 0 } },
            { "HUDCrosshair", { 12, 2 } },
            { "HUDCTFBaseEnemy", { 14, 0 } },
            { "HUDCTFBaseFriendly", { 14, 0 } },
            { "HUDCTFFlag", { 14, 0 } },
            { "HUDDamageDir", { 16, 0 } },
            { "HUDDebug", { 12, 0 } },
            { "HUDDecision", { 16, 0 } },
            { "HUDDialogue", { 13, 1 } },
            { "HUDDistance", { 16, 0 } },
            { "HUDEndRoundMessage", { 32, 0 } },
            { "HUDFocus", { 16, 0 } },
            { "HUDGameMessage", { 14, 2 } },
            { "HUDGear", { 14, 0 } },
            { "HUDGrenade", { 14, 0 } },
            { "HUDGrenadeList", { 14, 0 } },
            { "HUDHealth", { 25, 0 } },
            { "HUDObjective", { 22, 0 } },
            { "HUDPaused", { 20, 0 } },
            { "HUDRadio", { 16, 0 } },
            { "HUDRespawn", { 18, 0 } },
            { "HUDSpectator", { 12, 0 } },
            { "HUDSubtitle", { 14, 2 } },
            { "HUDSwap", { 12, 2 } },
            { "HUDTimerMain", { 14, 0 } },
            { "HUDTimerSuddenDeath", { 14, 0 } },
            { "HUDTimerTeam0", { 18, 0 } },
            { "HUDTimerTeam1", { 18, 0 } },
            { "HUDTransmission", { 16, 0 } },
            { "HUDVote", { 16, 0 } },
            { "HUDWeapon", { 14, 0 } }
        };

        // HUD
        g_State.hudScalingRules =
        {
            {"HUDHealth",            {{"AdditionalPoint", "IconSize", "IconOffset", "TextOffset"}, &g_State.scalingFactor}},
            {"HUDDialogue",          {{"AdditionalPoint", "IconSize", "TextOffset"}, &g_State.scalingFactor}},
            {"HUDGrenadeList",       {{"AdditionalPoint", "IconSize", "TextOffset"}, &g_State.scalingFactor}},
            {"HUDWeapon",            {{"AdditionalPoint", "IconSize", "TextOffset"}, &g_State.scalingFactor}},
            {"HUDArmor",             {{"IconSize", "IconOffset", "TextOffset"}, &g_State.scalingFactor}},
            {"HUDSwap",              {{"IconSize", "IconOffset", "TextOffset"}, &g_State.scalingFactorText}},
            {"HUDGear",              {{"IconSize", "IconOffset", "TextOffset"}, &g_State.scalingFactor}},
            {"HUDGrenade",           {{"IconSize", "IconOffset", "TextOffset"}, &g_State.scalingFactor}},
            {"HUDAmmo",              {{"IconSize", "IconOffset", "TextOffset"}, &g_State.scalingFactor}},
            {"HUDFlashlight",        {{"IconSize", "IconOffset"}, &g_State.scalingFactor}},
            {"HUDSlowMo2",           {{"IconSize", "IconOffset"}, &g_State.scalingFactor}},
            {"HUDActivateObject",    {{"TextOffset"}, &g_State.scalingFactor}},
        };
    }

    // 10 slots max
    MaxWeaponCapacity = std::clamp(MaxWeaponCapacity, 0, 10);

    // Controller Config
    SetRumbleEnabled(RumbleEnabled);
    SetGyroEnabled(GyroEnabled);
    SetGyroSensitivity(GyroSensitivity);
    SetGyroSmoothing(GyroSmoothing);
    SetGyroCalibrationPersistence(GyroCalibrationPersistence);
}

static void Init()
{
    ReadConfig();

    // LAA Patching if needed
    if (CheckLAAPatch)
    {
        LAAPatcher::PerformLAAPatch(GetModuleHandleA(NULL), CheckLAAPatch != 2);
    }

    // Load SDL
    if (SDLGamepadSupport)
    {
        SDLGamepadSupport = InitializeSDLGamepad();
    }

    // DSOAL fix
    if (FixSoundWrapperLoading)
    {
        DirectSoundHelper::Init();
    }

    // Install the crash handler
    if (EnableCrashHandler)
    {
        CrashHandler::Install();
    }

    // Get the handle of the client as soon as it is loaded
    ApplyClientHook();

    // Fixes
    ApplyFixDirectInputFps();
    ApplyFixHighFPSPhysics();
    ApplyOptimizeSaveSpeed();
    ApplyFixNvidiaShadowCorruption();
    ApplyFixAspectRatioBlur();
    ApplyFastVRAMDetection();
    ApplyFixKeyboardInputLanguage();
    ApplyFixScriptedAnimationCrash();
    ApplyFixWindow();

    // Display
    ApplyAutoResolution();

    // Graphics
    ApplySSAAScale();
    ApplyReducedMipMapBias();

    // Misc
    HookMainLoop();
    HookVSyncOverride();
    ApplySkipIntroHook();
    ApplyConsoleVariableHook();
    ApplySaveFolderRedirect();
    ApplyDisablePunkBuster();
    ApplyForceRenderMode();
    ApplyDisableJoystick();
    ApplyDeviceCreationHook();
}

static HWND WINAPI CreateWindowExA_Hook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    // Detect creation of F.E.A.R. window to initialize patches
    if (!g_State.isInit && lpWindowName && strstr(lpWindowName, "F.E.A.R.") && nWidth == 320 && nHeight == 200)
    {
        // Disable this hook and initialize patches once the game's code has been decrypted in memory (by SecuROM or SteamDRM)
        MH_DisableHook(MH_ALL_HOOKS);
        Init();
        g_State.isInit = true;

        if (FixWindowStyle)
        {
            dwStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        }

        g_State.hWnd = ori_CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
        InitConsole(lpClassName);
        return g_State.hWnd;
    }

    return ori_CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}
