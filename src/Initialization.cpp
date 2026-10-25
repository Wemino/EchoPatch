#pragma once

#include "Globals.cpp"
#include "LAAPatcher.hpp"
#include "CrashHandler.hpp"
#include "Controller/Controller.hpp"
#include "Controller/ControllerBindings.hpp"

static bool isInit = false;

// WinAPI function pointers
HWND(WINAPI* ori_CreateWindowExA)(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);

static void ReadConfig()
{
    IniHelper::Init(g_State.IsExpansion());

    // Fixes
    DisableRedundantHIDInit = IniHelper::ReadInteger("Fixes", "DisableRedundantHIDInit", 1) == 1;
    HighFPSFixes = IniHelper::ReadInteger("Fixes", "HighFPSFixes", 1) == 1;
    HavokPhysicsFix = IniHelper::ReadInteger("Fixes", "HavokPhysicsFix", 1) == 1;
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
    ControllerBindings::LoadFromIni();

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
    ConsoleKey = IniHelper::ReadInteger("Console", "ConsoleKey", VK_HOME);
    DebugLevel = IniHelper::ReadInteger("Console", "DebugLevel", 0);
    ConsoleBufferLen = IniHelper::ReadInteger("Console", "ConsoleBufferLen", 500);
    ConsoleHistoryLen = IniHelper::ReadInteger("Console", "ConsoleHistoryLen", 20);
    ConsoleAlpha = IniHelper::ReadFloat("Console", "ConsoleAlpha", 1.0f);
    MoveConsole = IniHelper::ReadString("Console", "MoveConsole", "Top");
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
        ControllerBindings::ApplyAll();
    }

    if (HUDScaling)
    {
        g_State.textDataMap =
        {
            { "Default10", 0 },
            { "Default12", 0 },
            { "Default14", 0 },
            { "Default16", 0 },
            { "Default18", 0 },
            { "Default24", 0 },
            { "Default32", 0 },
            { "EndCredits16", 0 },
            { "EndCredits18", 0 },
            { "Intro16", 0 },
            { "Objective", 0 },
            { "Training", 0 },

            // HUD Entries
            { "HUDActivate", 0 },
            { "HUDActivateObject", 0 },
            { "HUDAmmo", 0 },
            { "HUDArmor", 0 },
            { "HUDBuildVersion", 0 },
            { "HUDChatInput", 2 },
            { "HUDChatMessage", 2 },
            { "HUDControlPoint", 0 },
            { "HUDControlPointBar", 0 },
            { "HUDControlPointList", 0 },
            { "HUDCrosshair", 2 },
            { "HUDCTFBaseEnemy", 0 },
            { "HUDCTFBaseFriendly", 0 },
            { "HUDCTFFlag", 0 },
            { "HUDDamageDir", 0 },
            { "HUDDebug", 0 },
            { "HUDDecision", 0 },
            { "HUDDialogue", 1 },
            { "HUDDistance", 0 },
            { "HUDEndRoundMessage", 0 },
            { "HUDFocus", 0 },
            { "HUDGameMessage", 2 },
            { "HUDGear", 0 },
            { "HUDGrenade", 0 },
            { "HUDGrenadeList", 0 },
            { "HUDHealth", 0 },
            { "HUDObjective", 0 },
            { "HUDPaused", 0 },
            { "HUDRadio", 0 },
            { "HUDRespawn", 0 },
            { "HUDSpectator", 0 },
            { "HUDSubtitle", 2 },
            { "HUDSwap", 2 },
            { "HUDTimerMain", 0 },
            { "HUDTimerSuddenDeath", 0 },
            { "HUDTimerTeam0", 0 },
            { "HUDTimerTeam1", 0 },
            { "HUDTransmission", 0 },
            { "HUDVote", 0 },
            { "HUDWeapon", 0 }
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
            {"HUDGear",              {{"AdditionalPoint", "IconSize", "IconOffset", "TextOffset"}, &g_State.scalingFactor}},
            {"HUDGrenade",           {{"IconSize", "IconOffset", "TextOffset"}, &g_State.scalingFactor}},
            {"HUDAmmo",              {{"IconSize", "IconOffset", "TextOffset"}, &g_State.scalingFactor}},
            {"HUDFlashlight",        {{"IconSize", "IconOffset"}, &g_State.scalingFactor}},
            {"HUDSlowMo2",           {{"IconSize", "IconOffset"}, &g_State.scalingFactor}},
            {"HUDActivateObject",    {{"IconSize", "IconOffset", "TextOffset"}, &g_State.scalingFactor}},
            {"HUDDecision",          {{"IconSize", "TextOffset"}, &g_State.scalingFactor}},
            {"HUDRadio",             {{"IconSize", "TextOffset"}, &g_State.scalingFactor}},
            {"HUDCrosshair",         {{"TextOffset"}, &g_State.scalingFactorText}},
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
        // The expansions are patched together with FEAR.exe
        if (LAAPatcher::PerformLAAPatch(GetModuleHandleA(NULL), CheckLAAPatch != 2, g_State.CurrentFEARGame == FEAR) == LAAPatcher::PatchResult::Declined)
        {
            // Don't ask again
            IniHelper::iniReader["Fixes"]["CheckLAAPatch"] = "0";
            IniHelper::Save();
        }
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
    ApplyHavokPhysicsFix();
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
    if (!isInit && lpWindowName && strstr(lpWindowName, "F.E.A.R.") && nWidth == 320 && nHeight == 200)
    {
        // Disable this hook and initialize patches once the game's code has been decrypted in memory (by SecuROM or SteamDRM)
        MH_DisableHook(MH_ALL_HOOKS);
        Init();
        isInit = true;

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
