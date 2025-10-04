#include <Windows.h>
#include <Xinput.h>
#include <unordered_map>

#include "Core.hpp"
#include "../Client/Client.hpp"
#include "../Server/Server.hpp"
#include "../ClientFX/ClientFX.hpp"
#include "../Controller/Controller.hpp"
#include "MinHook.hpp"
#include "ini.hpp"
#include "DInputProxy.hpp"
#include "../helper.hpp"

// ======================
// Constants
// ======================
const DWORD FEAR_TIMESTAMP = 0x44EF6AE6;
const DWORD FEARMP_TIMESTAMP = 0x44EF6ADB;
const DWORD FEARXP_TIMESTAMP = 0x450B3629;
const DWORD FEARXP_TIMESTAMP2 = 0x450DA808;
const DWORD FEARXP2_TIMESTAMP = 0x46FC10A3;

// Global instance
GlobalState g_State;

// WinAPI function pointers
HWND(WINAPI* ori_CreateWindowExA)(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
HRESULT(WINAPI* ori_SHGetFolderPathA)(HWND, int, HANDLE, DWORD, LPSTR);
HANDLE(WINAPI* ori_CreateFileA)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE) = nullptr;
BOOL(WINAPI* ori_SetFilePointerEx)(HANDLE, LARGE_INTEGER, PLARGE_INTEGER, DWORD) = nullptr;
BOOL(WINAPI* ori_CloseHandle)(HANDLE) = nullptr;

// ======================
// Core Function Pointers
// ======================
intptr_t(__cdecl* LoadGameDLL)(char*, char, DWORD*) = nullptr;
int(__stdcall* SetConsoleVariableFloat)(const char*, float) = nullptr;
int(__thiscall* GetDeviceObjectDesc)(int, unsigned int, wchar_t*, unsigned int*) = nullptr;
void(__cdecl* ProcessTwistLimitConstraint)(int, float*, float*) = nullptr;
void(__cdecl* ProcessConeLimitConstraint)(int, float*, float*, float*) = nullptr;
void(__cdecl* BuildJacobianRow) (int, float*, float*) = nullptr;
int(__thiscall* InitializePresentationParameters)(DWORD*, DWORD*, unsigned __int8) = nullptr;
int(__thiscall* MainGameLoop)(int) = nullptr;
int(__cdecl* SetRenderMode)(int) = nullptr;
int(__thiscall* FindStringCaseInsensitive)(DWORD*, char*) = nullptr;
int(__thiscall* TerminateServer)(int) = nullptr;
bool(__thiscall* StreamWrite)(DWORD*, LPCVOID, DWORD) = nullptr;
int(__thiscall* CreateAndInitializeDevice)(DWORD*, DWORD*, DWORD*, int, char) = nullptr;

// =============================
// Ini Variables
// =============================

// Fixes
bool DisableRedundantHIDInit = false;
bool HighFPSFixes = false;
bool OptimizeSaveSpeed = false;
bool FixNvidiaShadowCorruption = false;
bool DisableXPWidescreenFiltering = false;
bool FixKeyboardInputLanguage = false;
bool WeaponFixes = false;
bool CheckLAAPatch = false;

// Graphics
float MaxFPS = 0;
bool DynamicVsync = false;
bool HighResolutionReflections = false;
bool NoLODBias = false;
bool ReducedMipMapBias = false;
bool EnablePersistentWorldState = false;

// Display
bool HUDScaling = false;
float HUDCustomScalingFactor = 0;
float SmallTextCustomScalingFactor = 0;
int AutoResolution = 0;
bool DisableLetterbox = false;
bool ForceWindowed = false;
bool FixWindowStyle = false;

// Controller
float MouseAimMultiplier = 0.0f;
bool XInputControllerSupport = false;
bool HideMouseCursor = false;
float GPadAimSensitivity = 0.0f;
float GPadAimEdgeThreshold = 0.0f;
float GPadAimEdgeAccelTime = 0.0f;
float GPadAimEdgeDelayTime = 0.0f;
float GPadAimEdgeMultiplier = 0.0f;
float GPadAimAspectRatio = 0.0f;
float GPadZoomMagThreshold = 0.0f;
int GAMEPAD_A = 0;
int GAMEPAD_B = 0;
int GAMEPAD_X = 0;
int GAMEPAD_Y = 0;
int GAMEPAD_LEFT_THUMB = 0;
int GAMEPAD_RIGHT_THUMB = 0;
int GAMEPAD_LEFT_SHOULDER = 0;
int GAMEPAD_RIGHT_SHOULDER = 0;
int GAMEPAD_DPAD_UP = 0;
int GAMEPAD_DPAD_DOWN = 0;
int GAMEPAD_DPAD_LEFT = 0;
int GAMEPAD_DPAD_RIGHT = 0;
int GAMEPAD_LEFT_TRIGGER = 0;
int GAMEPAD_RIGHT_TRIGGER = 0;
int GAMEPAD_BACK = 0;

// SkipIntro
bool SkipSplashScreen = false;
bool SkipAllIntro = false;
bool SkipSierraIntro = false;
bool SkipMonolithIntro = false;
bool SkipWBGamesIntro = false;
bool SkipNvidiaIntro = false;
bool SkipTimegateIntro = false;
bool SkipDellIntro = false;

// Extra
bool RedirectSaveFolder = false;
bool InfiniteFlashlight = false;
bool EnableCustomMaxWeaponCapacity = false;
int MaxWeaponCapacity = 0;
bool DisableHipFireAccuracyPenalty = false;
bool ShowErrors = false;

static void ReadConfig()
{
    IniHelper::Init(g_State.IsExpansion());

    // Fixes
    DisableRedundantHIDInit = IniHelper::ReadInteger("Fixes", "DisableRedundantHIDInit", 1) == 1;
    HighFPSFixes = IniHelper::ReadInteger("Fixes", "HighFPSFixes", 1) == 1;
    OptimizeSaveSpeed = IniHelper::ReadInteger("Fixes", "OptimizeSaveSpeed", 1) == 1;
    FixNvidiaShadowCorruption = IniHelper::ReadInteger("Fixes", "FixNvidiaShadowCorruption", 1) == 1;
    DisableXPWidescreenFiltering = IniHelper::ReadInteger("Fixes", "DisableXPWidescreenFiltering", 1) == 1;
    FixKeyboardInputLanguage = IniHelper::ReadInteger("Fixes", "FixKeyboardInputLanguage", 1) == 1;
    WeaponFixes = IniHelper::ReadInteger("Fixes", "WeaponFixes", 1) == 1;
    CheckLAAPatch = IniHelper::ReadInteger("Fixes", "CheckLAAPatch", 0) == 1;

    // Graphics
    MaxFPS = IniHelper::ReadFloat("Graphics", "MaxFPS", 240.0f);
    DynamicVsync = IniHelper::ReadInteger("Graphics", "DynamicVsync", 1) == 1;
    HighResolutionReflections = IniHelper::ReadInteger("Graphics", "HighResolutionReflections", 1) == 1;
    NoLODBias = IniHelper::ReadInteger("Graphics", "NoLODBias", 1) == 1;
    ReducedMipMapBias = IniHelper::ReadInteger("Graphics", "ReducedMipMapBias", 1) == 1;
    EnablePersistentWorldState = IniHelper::ReadInteger("Graphics", "EnablePersistentWorldState", 1) == 1;

    // Display
    HUDScaling = IniHelper::ReadInteger("Display", "HUDScaling", 1) == 1;
    HUDCustomScalingFactor = IniHelper::ReadFloat("Display", "HUDCustomScalingFactor", 1.0f);
    SmallTextCustomScalingFactor = IniHelper::ReadFloat("Display", "SmallTextCustomScalingFactor", 1.0f);
    AutoResolution = IniHelper::ReadInteger("Display", "AutoResolution", 1);
    DisableLetterbox = IniHelper::ReadInteger("Display", "DisableLetterbox", 0) == 1;
    ForceWindowed = IniHelper::ReadInteger("Display", "ForceWindowed", 0) == 1;
    FixWindowStyle = IniHelper::ReadInteger("Display", "FixWindowStyle", 1) == 1;

    // Controller
    MouseAimMultiplier = IniHelper::ReadFloat("Controller", "MouseAimMultiplier", 1.0f);
    XInputControllerSupport = IniHelper::ReadInteger("Controller", "XInputControllerSupport", 1) == 1;
    HideMouseCursor = IniHelper::ReadInteger("Controller", "HideMouseCursor", 0) == 1;
    GPadAimSensitivity = IniHelper::ReadFloat("Controller", "GPadAimSensitivity", 2.0f);
    GPadAimEdgeThreshold = IniHelper::ReadFloat("Controller", "GPadAimEdgeThreshold", 0.75f);
    GPadAimEdgeAccelTime = IniHelper::ReadFloat("Controller", "GPadAimEdgeAccelTime", 1.0f);
    GPadAimEdgeDelayTime = IniHelper::ReadFloat("Controller", "GPadAimEdgeDelayTime", 0.25f);
    GPadAimEdgeMultiplier = IniHelper::ReadFloat("Controller", "GPadAimEdgeMultiplier", 1.6f);
    GPadAimAspectRatio = IniHelper::ReadFloat("Controller", "GPadAimAspectRatio", 1.0f);
    GPadZoomMagThreshold = IniHelper::ReadFloat("Controller", "GPadZoomMagThreshold", 1.3f);
    GAMEPAD_A = IniHelper::ReadInteger("Controller", "GAMEPAD_A", 15);
    GAMEPAD_B = IniHelper::ReadInteger("Controller", "GAMEPAD_B", 14);
    GAMEPAD_X = IniHelper::ReadInteger("Controller", "GAMEPAD_X", 88);
    GAMEPAD_Y = IniHelper::ReadInteger("Controller", "GAMEPAD_Y", 106);
    GAMEPAD_LEFT_THUMB = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_THUMB", 70);
    GAMEPAD_RIGHT_THUMB = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_THUMB", 114);
    GAMEPAD_LEFT_SHOULDER = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_SHOULDER", 81);
    GAMEPAD_RIGHT_SHOULDER = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_SHOULDER", 19);
    GAMEPAD_DPAD_UP = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_UP", 77);
    GAMEPAD_DPAD_DOWN = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_DOWN", 73);
    GAMEPAD_DPAD_LEFT = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_LEFT", 20);
    GAMEPAD_DPAD_RIGHT = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_RIGHT", 21);
    GAMEPAD_LEFT_TRIGGER = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_TRIGGER", 71);
    GAMEPAD_RIGHT_TRIGGER = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_TRIGGER", 17);
    GAMEPAD_BACK = IniHelper::ReadInteger("Controller", "GAMEPAD_BACK", 78);

    // SkipIntro
    SkipSplashScreen = IniHelper::ReadInteger("SkipIntro", "SkipSplashScreen", 1) == 1;
    SkipAllIntro = IniHelper::ReadInteger("SkipIntro", "SkipAllIntro", 0) == 1;
    SkipSierraIntro = IniHelper::ReadInteger("SkipIntro", "SkipSierraIntro", 1) == 1;
    SkipMonolithIntro = IniHelper::ReadInteger("SkipIntro", "SkipMonolithIntro", 0) == 1;
    SkipWBGamesIntro = IniHelper::ReadInteger("SkipIntro", "SkipWBGamesIntro", 1) == 1;
    SkipNvidiaIntro = IniHelper::ReadInteger("SkipIntro", "SkipNvidiaIntro", 1) == 1;
    SkipTimegateIntro = IniHelper::ReadInteger("SkipIntro", "SkipTimegateIntro", 1) == 1;
    SkipDellIntro = IniHelper::ReadInteger("SkipIntro", "SkipDellIntro", 1) == 1;

    // Extra
    RedirectSaveFolder = IniHelper::ReadInteger("Extra", "RedirectSaveFolder", 0) == 1;
    InfiniteFlashlight = IniHelper::ReadInteger("Extra", "InfiniteFlashlight", 0) == 1;
    EnableCustomMaxWeaponCapacity = IniHelper::ReadInteger("Extra", "EnableCustomMaxWeaponCapacity", 0) == 1;
    MaxWeaponCapacity = IniHelper::ReadInteger("Extra", "MaxWeaponCapacity", 3);
    DisableHipFireAccuracyPenalty = IniHelper::ReadInteger("Extra", "DisableHipFireAccuracyPenalty", 0) == 1;
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

    if (XInputControllerSupport)
    {
        // Update g_buttonMappings with INI values
        for (auto& [button, key] : g_buttonMappings)
        {
            switch (button)
            {
                case XINPUT_GAMEPAD_A: key = GAMEPAD_A; break;
                case XINPUT_GAMEPAD_B: key = GAMEPAD_B; break;
                case XINPUT_GAMEPAD_X: key = GAMEPAD_X; break;
                case XINPUT_GAMEPAD_Y: key = GAMEPAD_Y; break;
                case XINPUT_GAMEPAD_LEFT_THUMB: key = GAMEPAD_LEFT_THUMB; break;
                case XINPUT_GAMEPAD_RIGHT_THUMB: key = GAMEPAD_RIGHT_THUMB; break;
                case XINPUT_GAMEPAD_LEFT_SHOULDER: key = GAMEPAD_LEFT_SHOULDER; break;
                case XINPUT_GAMEPAD_RIGHT_SHOULDER: key = GAMEPAD_RIGHT_SHOULDER; break;
                case XINPUT_GAMEPAD_DPAD_UP: key = GAMEPAD_DPAD_UP; break;
                case XINPUT_GAMEPAD_DPAD_DOWN: key = GAMEPAD_DPAD_DOWN; break;
                case XINPUT_GAMEPAD_DPAD_LEFT: key = GAMEPAD_DPAD_LEFT; break;
                case XINPUT_GAMEPAD_DPAD_RIGHT: key = GAMEPAD_DPAD_RIGHT; break;
                case XINPUT_GAMEPAD_LEFT_TRIGGER: key = GAMEPAD_LEFT_TRIGGER; break;
                case XINPUT_GAMEPAD_RIGHT_TRIGGER: key = GAMEPAD_RIGHT_TRIGGER; break;
                case XINPUT_GAMEPAD_BACK: key = GAMEPAD_BACK; break;
                default: break;
            }
        }
    }

    if (HUDScaling)
    {
        // Text: Interface\Credits\LineLayout
        g_State.textDataMap["Default10"] = { 10, 0 };
        g_State.textDataMap["Default12"] = { 12, 0 };
        g_State.textDataMap["Default14"] = { 14, 0 };
        g_State.textDataMap["Default16"] = { 16, 0 };
        g_State.textDataMap["Default18"] = { 18, 0 };
        g_State.textDataMap["Default24"] = { 24, 0 };
        g_State.textDataMap["Default32"] = { 32, 0 };
        g_State.textDataMap["EndCredits16"] = { 16, 0 };
        g_State.textDataMap["EndCredits18"] = { 18, 0 };
        g_State.textDataMap["Intro16"] = { 16, 0 };
        g_State.textDataMap["Objective"] = { 22, 0 };
        g_State.textDataMap["Training"] = { 22, 0 };

        // Text: Interface\HUD
        g_State.textDataMap["HUDActivate"] = { 18, 0 };
        g_State.textDataMap["HUDActivateObject"] = { 14, 0 };
        g_State.textDataMap["HUDAmmo"] = { 16, 0 };
        g_State.textDataMap["HUDArmor"] = { 25, 0 };
        g_State.textDataMap["HUDBuildVersion"] = { 12, 0 };
        g_State.textDataMap["HUDChatInput"] = { 16, 2 };
        g_State.textDataMap["HUDChatMessage"] = { 14, 2 };
        g_State.textDataMap["HUDControlPoint"] = { 24, 0 };
        g_State.textDataMap["HUDControlPointBar"] = { 24, 0 };
        g_State.textDataMap["HUDControlPointList"] = { 24, 0 };
        g_State.textDataMap["HUDCrosshair"] = { 12, 2 };
        g_State.textDataMap["HUDCTFBaseEnemy"] = { 14, 0 };
        g_State.textDataMap["HUDCTFBaseFriendly"] = { 14, 0 };
        g_State.textDataMap["HUDCTFFlag"] = { 14, 0 };
        g_State.textDataMap["HUDDamageDir"] = { 16, 0 };
        g_State.textDataMap["HUDDebug"] = { 12, 0 };
        g_State.textDataMap["HUDDecision"] = { 16, 0 };
        g_State.textDataMap["HUDDialogue"] = { 13, 1 };
        g_State.textDataMap["HUDDistance"] = { 16, 0 };
        g_State.textDataMap["HUDEndRoundMessage"] = { 32, 0 };
        g_State.textDataMap["HUDFocus"] = { 16, 0 };
        g_State.textDataMap["HUDGameMessage"] = { 14, 2 };
        g_State.textDataMap["HUDGear"] = { 14, 0 };
        g_State.textDataMap["HUDGrenade"] = { 14, 0 };
        g_State.textDataMap["HUDGrenadeList"] = { 14, 0 };
        g_State.textDataMap["HUDHealth"] = { 25, 0 };
        g_State.textDataMap["HUDObjective"] = { 22, 0 };
        g_State.textDataMap["HUDPaused"] = { 20, 0 };
        g_State.textDataMap["HUDRadio"] = { 16, 0 };
        g_State.textDataMap["HUDRespawn"] = { 18, 0 };
        g_State.textDataMap["HUDSpectator"] = { 12, 0 };
        g_State.textDataMap["HUDSubtitle"] = { 14, 2 };
        g_State.textDataMap["HUDSwap"] = { 12, 2 };
        g_State.textDataMap["HUDTimerMain"] = { 14, 0 };
        g_State.textDataMap["HUDTimerSuddenDeath"] = { 14, 0 };
        g_State.textDataMap["HUDTimerTeam0"] = { 18, 0 };
        g_State.textDataMap["HUDTimerTeam1"] = { 18, 0 };
        g_State.textDataMap["HUDTransmission"] = { 16, 0 };
        g_State.textDataMap["HUDVote"] = { 16, 0 };
        g_State.textDataMap["HUDWeapon"] = { 14, 0 };

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

        // Make sure to sort the attributes of each rule after setting them up
        for (auto& [key, rule] : g_State.hudScalingRules)
        {
            std::sort(rule.attributes.begin(), rule.attributes.end());
        }
    }

    // 10 slots max
    MaxWeaponCapacity = std::clamp(MaxWeaponCapacity, 0, 10);

    // If we need to hook the function used to unload the server
    g_State.needServerTermHooking = EnableCustomMaxWeaponCapacity;
}

#pragma region Helper

DWORD ScanModuleSignature(HMODULE Module, std::string_view Signature, const char* PatchName, int FunctionStartCheckCount, bool ShowError)
{
    DWORD Address = MemoryHelper::FindSignatureAddress(Module, Signature, FunctionStartCheckCount);
    if (Address == 0 && ShowErrors && ShowError)
    {
        std::string ErrorMessage = "Error: Unable to find signature for patch: ";
        ErrorMessage += PatchName;
        MessageBoxA(NULL, ErrorMessage.c_str(), "EchoPatch", MB_ICONERROR);
    }
    return Address;
}

#pragma endregion

#pragma region Core Hooks

// When the game is loading the Client or Server
static intptr_t __cdecl LoadGameDLL_Hook(char* FileName, char a2, DWORD* a3)
{
    intptr_t result = LoadGameDLL(FileName, a2, a3);

    // Convert FileName to wide string
    wchar_t wFileName[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, FileName, -1, wFileName, MAX_PATH);

    // Use FileName to get the module handle
    HMODULE ApiDLL = GetModuleHandleW(wFileName);
    if (ApiDLL)
    {
        if (!g_State.isClientLoaded) // First time is client
        {
            g_State.GameClient = ApiDLL;
            g_State.isClientLoaded = true;
            ApplyClientPatch();
        }
        else // Otherwise server
        {
            g_State.GameServer = ApiDLL;
            ApplyServerPatch();
        }
    }

    return result;
}

static int __stdcall SetConsoleVariableFloat_Hook(const char* pszVarName, float fValue)
{
    if (ForceWindowed && strcmp(pszVarName, "StreamResources") == 0)
    {
        // Set Windowed flag during engine initialization
        SetConsoleVariableFloat("Windowed", 1.0f);
        ForceWindowed = false;
    }

    if (NoLODBias)
    {
        if (strcmp(pszVarName, "ModelLODDistanceScale") == 0)
        {
            if (fValue > 0.0f)
            {
                fValue = 0.003f;
            }
        }
        else if (strcmp(pszVarName, "CameraFirstPersonLODBias") == 0)
        {
            fValue = 0.0f;
        }
    }

    if (XInputControllerSupport)
    {
        if (strcmp(pszVarName, "GPadAimSensitivity") == 0)
        {
            fValue = GPadAimSensitivity;
        }
        else if (strcmp(pszVarName, "GPadAimEdgeThreshold") == 0)
        {
            fValue = GPadAimEdgeThreshold;
        }
        else if (strcmp(pszVarName, "GPadAimEdgeAccelTime") == 0)
        {
            fValue = GPadAimEdgeAccelTime;
        }
        else if (strcmp(pszVarName, "GPadAimEdgeDelayTime") == 0)
        {
            fValue = GPadAimEdgeDelayTime;
        }
        else if (strcmp(pszVarName, "GPadAimEdgeMultiplier") == 0)
        {
            fValue = GPadAimEdgeMultiplier;
        }
        else if (strcmp(pszVarName, "GPadAimAspectRatio") == 0)
        {
            fValue = GPadAimAspectRatio;
        }
    }

    if (g_State.isInAutoDetect && AutoResolution != 0)
    {
        if (strcmp(pszVarName, "Performance_ScreenHeight") == 0)
        {
            fValue = static_cast<float>(g_State.screenHeight);
        }
        else if (strcmp(pszVarName, "Performance_ScreenWidth") == 0)
        {
            fValue = static_cast<float>(g_State.screenWidth);
        }
    }

    if (HUDScaling)
    {
        if (strcmp(pszVarName, "CrosshairSize") == 0)
        {
            g_State.crosshairSize = fValue; // Keep a backup of the value as it can be adjusted in the settings
            fValue = fValue * g_State.scalingFactorCrosshair;
        }
        else if (strcmp(pszVarName, "PerturbScale") == 0)
        {
            fValue = fValue * g_State.scalingFactorCrosshair;
        }
        else if (strcmp(pszVarName, "UseTextScaling") == 0)
        {
            fValue = 0.0f; // Scaling is already taken care of
        }
    }

    // Making us wait when saving a checkpoint, likely useless
    if (OptimizeSaveSpeed && strcmp(pszVarName, "CheckPointOptimizeVideoMemory") == 0)
    {
        fValue = 0.0f;
    }

    return SetConsoleVariableFloat(pszVarName, fValue);
}

// ========================
// FixKeyboardInputLanguage
// ========================

static int __fastcall GetDeviceObjectDesc_Hook(int thisPtr, int, unsigned int DeviceType, wchar_t* KeyName, unsigned int* ret)
{
    if (g_State.isLoadingDefault && DeviceType == 0) // Initialization of the keyboard layout
    {
        // Control name from 'ProfileDatabase/Defaults.Gamdb00p' with corresponding DirectInput Key Id
        static const std::unordered_map<std::wstring, unsigned int> keyMap =
        {
            {L"W", 0x11},
            {L"S", 0x1F},
            {L"A", 0x1E},
            {L"D", 0x20},
            {L"Left", 0xCB},
            {L"Right", 0xCD},
            {L"Right Ctrl", 0x9D},
            {L"Space", 0x39},
            {L"C", 0x2E},
            {L"Q", 0x10},
            {L"E", 0x12},
            {L"G", 0x22},
            {L"F", 0x21},
            {L"R", 0x13},
            {L"Shift", 0x2A},
            {L"Ctrl", 0x1D},
            {L"X", 0x2D},
            {L"Tab", 0x0F},
            {L"M", 0x32},
            {L"T", 0x14},
            {L"Y", 0x15},
            {L"V", 0x2F},
            {L"Up", 0xC8},
            {L"Down", 0xD0},
            {L"End", 0xCF},
            {L"B", 0x30},
            {L"Z", 0x2C},
            {L"H", 0x23}
        };

        auto it = keyMap.find(KeyName);
        if (it != keyMap.end())
        {
            // Get pointer to keyboard the DIK table
            unsigned int dikCode = it->second;
            int KB_DIK_Table = MemoryHelper::ReadMemory<int>(thisPtr + 0xC);
            int tableStart = MemoryHelper::ReadMemory<int>(KB_DIK_Table + 0x10);
            int tableEnd = MemoryHelper::ReadMemory<int>(KB_DIK_Table + 0x14);

            // Iterate through the table
            while (tableStart < tableEnd)
            {
                // If the corresponding DirectInput Key Id is found
                if (MemoryHelper::ReadMemory<uint8_t>(tableStart + 0x1) == dikCode)
                {
                    // Write and return the index for that DIK Id
                    *ret = MemoryHelper::ReadMemory<int>(tableStart + 0x1C);
                    return 0;
                }
                tableStart += 0x20;
            }
        }
    }
    else if (DeviceType == 0 && KeyName && wcslen(KeyName) == 1 && iswalpha(KeyName[0])) // Handle alphabet keys by converting to VK and then to scan code (DIK)
    {
        wchar_t keyChar = towupper(KeyName[0]);
        HKL layout = GetKeyboardLayout(0);
        SHORT vkScan = VkKeyScanExW(keyChar, layout);

        if (vkScan != -1)
        {
            BYTE vk = LOBYTE(vkScan);
            UINT scanCode = MapVirtualKeyEx(vk, MAPVK_VK_TO_VSC, layout);

            int KB_DIK_Table = MemoryHelper::ReadMemory<int>(thisPtr + 0xC);
            int tableStart = MemoryHelper::ReadMemory<int>(KB_DIK_Table + 0x10);
            int tableEnd = MemoryHelper::ReadMemory<int>(KB_DIK_Table + 0x14);

            while (tableStart < tableEnd)
            {
                if (MemoryHelper::ReadMemory<uint8_t>(tableStart + 0x1) == scanCode)
                {
                    *ret = MemoryHelper::ReadMemory<int>(tableStart + 0x1C);
                    return 0;
                }
                tableStart += 0x20;
            }
        }
    }

    return GetDeviceObjectDesc(thisPtr, DeviceType, KeyName, ret);
}

// ========================
// HighFPSFixes
// ========================

static void __cdecl ProcessTwistLimitConstraint_Hook(int twistParams, float* constraintInstance, float* queryIn)
{
    float timeStepBak = constraintInstance[3];
    if (constraintInstance[3] > 250.0f) constraintInstance[3] = 250.0f;
    ProcessTwistLimitConstraint(twistParams, constraintInstance, queryIn);
    constraintInstance[3] = timeStepBak;
}

static void __cdecl ProcessConeLimitConstraint_Hook(int pivotA, float* pivotB, float* constraintInstance, float* queryIn)
{
    float timeStepBak = constraintInstance[3];
    if (constraintInstance[3] > 250.0f) constraintInstance[3] = 250.0f;
    ProcessConeLimitConstraint(pivotA, pivotB, constraintInstance, queryIn);
    constraintInstance[3] = timeStepBak;
}

static void __cdecl BuildJacobianRow_Hook(int jacobianData, float* constraintInstance, float* queryIn)
{
    float timeStepBak = constraintInstance[3];
    if (constraintInstance[3] > 250.0f) constraintInstance[3] = 250.0f;
    BuildJacobianRow(jacobianData, constraintInstance, queryIn);
    constraintInstance[3] = timeStepBak;
}

// =========================
// OptimizeSaveSpeed
// =========================

static bool __fastcall StreamWrite_Hook(DWORD* thisp, int, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite)
{
    HANDLE h = reinterpret_cast<HANDLE>(thisp[1]);
    auto it = g_State.saveBuffers.find(h);

    if (it != g_State.saveBuffers.end())
    {
        GlobalState::SaveBuffer& sb = it->second;

        LONGLONG writePos = sb.position;
        LONGLONG endPos = writePos + nNumberOfBytesToWrite;

        if (endPos > (LONGLONG)sb.buffer.size())
        {
            sb.buffer.resize((size_t)endPos, 0);
        }

        const uint8_t* data = (const uint8_t*)lpBuffer;
        memcpy(sb.buffer.data() + writePos, data, nNumberOfBytesToWrite);

        sb.position = endPos;
        return true;
    }

    return StreamWrite(thisp, lpBuffer, nNumberOfBytesToWrite);
}

static HANDLE WINAPI CreateFileA_Hook(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    HANDLE h = ori_CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

    if (h != INVALID_HANDLE_VALUE && lpFileName)
    {
        std::string_view filename(lpFileName);

        // Check if it's a .sav file being opened for writing
        if (filename.ends_with(".sav") && (dwDesiredAccess & GENERIC_WRITE))
        {
            GlobalState::SaveBuffer& sb = g_State.saveBuffers[h];
            sb.position = 0;
            sb.flushed = false;
        }
    }

    return h;
}

static BOOL WINAPI SetFilePointerEx_Hook(HANDLE hFile, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod)
{
    auto it = g_State.saveBuffers.find(hFile);
    if (it != g_State.saveBuffers.end() && !it->second.flushed)
    {
        GlobalState::SaveBuffer& sb = it->second;

        switch (dwMoveMethod)
        {
            case FILE_BEGIN:
                sb.position = liDistanceToMove.QuadPart;
                break;
            case FILE_CURRENT:
                sb.position += liDistanceToMove.QuadPart;
                break;
            case FILE_END:
                sb.position = sb.buffer.size() + liDistanceToMove.QuadPart;
                break;
        }

        if (lpNewFilePointer)
        {
            lpNewFilePointer->QuadPart = sb.position;
        }

        return TRUE;
    }

    return ori_SetFilePointerEx(hFile, liDistanceToMove, lpNewFilePointer, dwMoveMethod);
}

static BOOL WINAPI CloseHandle_Hook(HANDLE hObject)
{
    auto it = g_State.saveBuffers.find(hObject);
    if (it != g_State.saveBuffers.end())
    {
        GlobalState::SaveBuffer& sb = it->second;

        if (!sb.flushed && !sb.buffer.empty())
        {
            sb.flushed = true;

            LARGE_INTEGER zero = { 0 };
            ori_SetFilePointerEx(hObject, zero, nullptr, FILE_BEGIN);

            DWORD bytesWritten = 0;
            WriteFile(hObject, sb.buffer.data(), (DWORD)sb.buffer.size(), &bytesWritten, nullptr);
            FlushFileBuffers(hObject);
        }

        g_State.saveBuffers.erase(it);
    }

    return ori_CloseHandle(hObject);
}

// ===========================
// FixNvidiaShadowCorruption
// ===========================

static char __fastcall CreateAndInitializeDevice_Hook(DWORD* thisp, int, DWORD* a2, DWORD* a3, int a4, char a5)
{
    DWORD VendorId = a2[267];

    // NVIDIA device is used for the game
    if (VendorId == 0x10DE)
    {
        switch (g_State.CurrentFEARGame)
        {
            // CreateVertexBuffer: D3DPOOL_MANAGED -> D3DPOOL_SYSTEMMEM
            case FEAR:    MemoryHelper::WriteMemory<uint8_t>(0x512234, 2); break;
            case FEARMP:  MemoryHelper::WriteMemory<uint8_t>(0x512354, 2); break;
            case FEARXP:  MemoryHelper::WriteMemory<uint8_t>(0x5B5A24, 2); break;
            case FEARXP2: MemoryHelper::WriteMemory<uint8_t>(0x5B6F94, 2); break;
        }
    }

    return CreateAndInitializeDevice(thisp, a2, a3, a4, a5);
}

// ========================
// DynamicVsync
// ========================

static int __fastcall InitializePresentationParameters_Hook(DWORD* thisPtr, int, DWORD* a2, unsigned __int8 a3)
{
    int res = InitializePresentationParameters(thisPtr, a2, a3);
    thisPtr[101] = g_State.useVsyncOverride != 0 ? 0 : 0x80000000;
    return res;
}

// ================================
// MaxFPS & XInputControllerSupport
// ================================

static int __fastcall MainGameLoop_Hook(int thisPtr, int)
{
    if (g_State.isUsingFpsLimiter)
    {
        g_State.fpsLimiter.Limit();
    }

    if (XInputControllerSupport)
    {
        PollController();
    }

    return MainGameLoop(thisPtr);
}

// =======================
// AutoResolution
// =======================

static int __cdecl SetRenderMode_Hook(int rMode)
{
    *(DWORD*)(rMode + 0x84) = g_State.screenWidth;
    *(DWORD*)(rMode + 0x88) = g_State.screenHeight;
    return SetRenderMode(rMode);
}

// =======================
// SkipIntro
// =======================

// Function utilized in the process of loading video files
static int __fastcall FindStringCaseInsensitive_Hook(DWORD* thisPtr, int, char* video_path)
{
    // Disable this hook once we get to the menu
    if (SkipAllIntro || strstr(video_path, "Menu"))
    {
        switch (g_State.CurrentFEARGame)
        {
            case FEAR:    MH_DisableHook((void*)0x510CB0); break;
            case FEARMP:  MH_DisableHook((void*)0x510DD0); break;
            case FEARXP:  MH_DisableHook((void*)0x5B3440); break;
            case FEARXP2: MH_DisableHook((void*)0x5B49C0); break;
        }

        if (SkipAllIntro)
        {
            // Skip all movies while keeping the sound of the menu
            SystemHelper::SimulateSpacebarPress(g_State.hWnd);
        }

        return FindStringCaseInsensitive(thisPtr, video_path);
    }

    static const struct { bool flag; const char* names[2]; } skips[] =
    {
        { SkipSierraIntro,   { "sierralogo.bik",       nullptr } },
        { SkipMonolithIntro, { "MonolithLogo.bik",     nullptr } },
        { SkipWBGamesIntro,  { "WBGames.bik",          nullptr } },
        { SkipNvidiaIntro,   { "TWIMTBP_640x480.bik",  "Nvidia_LogoXP2.bik" } },
        { SkipTimegateIntro, { "timegate.bik",         "TimeGate.bik" } },
        { SkipDellIntro,     { "dell_xps.bik",         "Dell_LogoXP2.bik" } },
    };

    for (const auto& s : skips)
    {
        if (s.flag)
        {
            for (const char* name : s.names)
            {
                if (name && strstr(video_path, name))
                {
                    // Clear the video file path to prevent the video from playing
                    video_path[0] = '\0';
                    return FindStringCaseInsensitive(thisPtr, video_path);
                }
            }
        }
    }

    return FindStringCaseInsensitive(thisPtr, video_path);
}

static int __fastcall TerminateServer_Hook(int thisPtr, int)
{
    // Server is unloading, remove all previously installed hooks
    for (DWORD address : g_State.hookedServerFunctionAddresses)
    {
        MH_RemoveHook((void*)address);
    }

    g_State.hookedServerFunctionAddresses.clear();
    return TerminateServer(thisPtr);
}

static HRESULT WINAPI SHGetFolderPathA_Hook(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath)
{
    int original_csidl = csidl;
    if (original_csidl == 0x802E)
    {
        csidl = 0x8005; // Change to CSIDL_MYDOCUMENTS
    }

    HRESULT hr = ori_SHGetFolderPathA(hwnd, csidl, hToken, dwFlags, pszPath);

    // Only modify the path if the original csidl was 0x802E and the call succeeded
    if (SUCCEEDED(hr) && original_csidl == 0x802E)
    {
        size_t currentLen = strlen(pszPath);
        if (currentLen > 0)
        {
            if (pszPath[currentLen - 1] != '\\')
            {
                strcat_s(pszPath, MAX_PATH, "\\");
            }
            strcat_s(pszPath, MAX_PATH, "My Games");
        }
    }

    return hr;
}

#pragma endregion

#pragma region Core Patches

static void ApplyFixDirectInputFps()
{
    // Root cause documented by Methanhydrat: https://community.pcgamingwiki.com/files/file/789-directinput-fps-fix/
    // Fix SetWindowsHookExA input lag from: https://github.com/Vityacv/fearservmod
    if (!DisableRedundantHIDInit) return;

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:
            MemoryHelper::MakeNOP(0x4840DD, 22);
            MemoryHelper::MakeNOP(0x484057, 29);
            break;
        case FEARMP:
            MemoryHelper::MakeNOP(0x4841FD, 22);
            MemoryHelper::MakeNOP(0x484177, 29);
            break;
        case FEARXP:
            MemoryHelper::MakeNOP(0x4B895D, 22);
            MemoryHelper::MakeNOP(0x4B88D7, 29);
            break;
        case FEARXP2:
            MemoryHelper::MakeNOP(0x4B99AD, 22);
            MemoryHelper::MakeNOP(0x4B9927, 29);
            break;
    }
}

static void ApplyFixHighFPSPhysics()
{
    if (!HighFPSFixes) return;

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:
            HookHelper::ApplyHook((void*)0x4A8D30, &BuildJacobianRow_Hook, (LPVOID*)&BuildJacobianRow, true);
            HookHelper::ApplyHook((void*)0x4A9940, &ProcessTwistLimitConstraint_Hook, (LPVOID*)&ProcessTwistLimitConstraint, true);
            HookHelper::ApplyHook((void*)0x4A9F00, &ProcessConeLimitConstraint_Hook, (LPVOID*)&ProcessConeLimitConstraint, true);
            break;
        case FEARMP:
            HookHelper::ApplyHook((void*)0x4A8E50, &BuildJacobianRow_Hook, (LPVOID*)&BuildJacobianRow);
            HookHelper::ApplyHook((void*)0x4A9A60, &ProcessTwistLimitConstraint_Hook, (LPVOID*)&ProcessTwistLimitConstraint);
            HookHelper::ApplyHook((void*)0x4AA020, &ProcessConeLimitConstraint_Hook, (LPVOID*)&ProcessConeLimitConstraint);
            break;
        case FEARXP:
            HookHelper::ApplyHook((void*)0x4FF710, &BuildJacobianRow_Hook, (LPVOID*)&BuildJacobianRow);
            HookHelper::ApplyHook((void*)0x500320, &ProcessTwistLimitConstraint_Hook, (LPVOID*)&ProcessTwistLimitConstraint);
            HookHelper::ApplyHook((void*)0x5008E0, &ProcessConeLimitConstraint_Hook, (LPVOID*)&ProcessConeLimitConstraint);
            break;
        case FEARXP2:
            HookHelper::ApplyHook((void*)0x500780, &BuildJacobianRow_Hook, (LPVOID*)&BuildJacobianRow);
            HookHelper::ApplyHook((void*)0x501390, &ProcessTwistLimitConstraint_Hook, (LPVOID*)&ProcessTwistLimitConstraint);
            HookHelper::ApplyHook((void*)0x501950, &ProcessConeLimitConstraint_Hook, (LPVOID*)&ProcessConeLimitConstraint);
            break;
    }
}

static void ApplyOptimizeSaveSpeed()
{
    if (!OptimizeSaveSpeed) return;

    HookHelper::ApplyHookAPI(L"kernel32.dll", "CreateFileA", &CreateFileA_Hook, (LPVOID*)&ori_CreateFileA);
    HookHelper::ApplyHookAPI(L"kernel32.dll", "SetFilePointerEx", &SetFilePointerEx_Hook, (LPVOID*)&ori_SetFilePointerEx);
    HookHelper::ApplyHookAPI(L"kernel32.dll", "CloseHandle", &CloseHandle_Hook, (LPVOID*)&ori_CloseHandle);

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:    HookHelper::ApplyHook((void*)0x41C670, &StreamWrite_Hook, (LPVOID*)&StreamWrite, true); break;
        case FEARMP:  HookHelper::ApplyHook((void*)0x41C790, &StreamWrite_Hook, (LPVOID*)&StreamWrite); break;
        case FEARXP:  HookHelper::ApplyHook((void*)0x429720, &StreamWrite_Hook, (LPVOID*)&StreamWrite); break;
        case FEARXP2: HookHelper::ApplyHook((void*)0x429900, &StreamWrite_Hook, (LPVOID*)&StreamWrite); break;
    }
}

static void ApplyFixNvidiaShadowCorruption()
{
    if (!FixNvidiaShadowCorruption) return;

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:    HookHelper::ApplyHook((void*)0x4F91E0, &CreateAndInitializeDevice_Hook, (LPVOID*)&CreateAndInitializeDevice, true); break;
        case FEARMP:  HookHelper::ApplyHook((void*)0x4F9300, &CreateAndInitializeDevice_Hook, (LPVOID*)&CreateAndInitializeDevice); break;
        case FEARXP:  HookHelper::ApplyHook((void*)0x58F930, &CreateAndInitializeDevice_Hook, (LPVOID*)&CreateAndInitializeDevice); break;
        case FEARXP2: HookHelper::ApplyHook((void*)0x590F60, &CreateAndInitializeDevice_Hook, (LPVOID*)&CreateAndInitializeDevice); break;
    }
}

static void ApplyFixKeyboardInputLanguage()
{
    if (!FixKeyboardInputLanguage) return;

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:    HookHelper::ApplyHook((void*)0x481E10, &GetDeviceObjectDesc_Hook, (LPVOID*)&GetDeviceObjectDesc, true); break;
        case FEARMP:  HookHelper::ApplyHook((void*)0x481F30, &GetDeviceObjectDesc_Hook, (LPVOID*)&GetDeviceObjectDesc); break;
        case FEARXP:  HookHelper::ApplyHook((void*)0x4B5DE0, &GetDeviceObjectDesc_Hook, (LPVOID*)&GetDeviceObjectDesc); break;
        case FEARXP2: HookHelper::ApplyHook((void*)0x4B6E10, &GetDeviceObjectDesc_Hook, (LPVOID*)&GetDeviceObjectDesc); break;
    }
}

static void ApplyReducedMipMapBias()
{
    if (!ReducedMipMapBias) return;

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:
        case FEARMP:  MemoryHelper::WriteMemory<float>(0x56D5C4, -0.5f, false); break;
        case FEARXP:  MemoryHelper::WriteMemory<float>(0x612B94, -0.5f, false); break;
        case FEARXP2: MemoryHelper::WriteMemory<float>(0x614BA4, -0.5f, false); break;
    }
}

static void ApplyClientHook()
{
    switch (g_State.CurrentFEARGame)
    {
        case FEAR:    HookHelper::ApplyHook((void*)0x47D730, &LoadGameDLL_Hook, (LPVOID*)&LoadGameDLL, true); break;
        case FEARMP:  HookHelper::ApplyHook((void*)0x47D850, &LoadGameDLL_Hook, (LPVOID*)&LoadGameDLL); break;
        case FEARXP:  HookHelper::ApplyHook((void*)0x4AF260, &LoadGameDLL_Hook, (LPVOID*)&LoadGameDLL); break;
        case FEARXP2: HookHelper::ApplyHook((void*)0x4B02C0, &LoadGameDLL_Hook, (LPVOID*)&LoadGameDLL); break;
    }
}

static void ApplySkipIntroHook()
{
    switch (g_State.CurrentFEARGame)
    {
        case FEAR:    HookHelper::ApplyHook((void*)0x510CB0, &FindStringCaseInsensitive_Hook, (LPVOID*)&FindStringCaseInsensitive, true); break;
        case FEARMP:  HookHelper::ApplyHook((void*)0x510DD0, &FindStringCaseInsensitive_Hook, (LPVOID*)&FindStringCaseInsensitive); break;
        case FEARXP:  HookHelper::ApplyHook((void*)0x5B3440, &FindStringCaseInsensitive_Hook, (LPVOID*)&FindStringCaseInsensitive); break;
        case FEARXP2: HookHelper::ApplyHook((void*)0x5B49C0, &FindStringCaseInsensitive_Hook, (LPVOID*)&FindStringCaseInsensitive); break;
    }
}

static void ApplyConsoleVariableHook()
{
    switch (g_State.CurrentFEARGame)
    {
        case FEAR:
        case FEARMP:  HookHelper::ApplyHook((void*)0x409360, &SetConsoleVariableFloat_Hook, (LPVOID*)&SetConsoleVariableFloat, true); break;
        case FEARXP:  HookHelper::ApplyHook((void*)0x410120, &SetConsoleVariableFloat_Hook, (LPVOID*)&SetConsoleVariableFloat); break;
        case FEARXP2: HookHelper::ApplyHook((void*)0x410360, &SetConsoleVariableFloat_Hook, (LPVOID*)&SetConsoleVariableFloat); break;
    }
}

static void ApplyAutoResolution()
{
    if (AutoResolution == 0) return;

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:
        case FEARMP:
            MemoryHelper::WriteMemory<int>(0x56ABF4, g_State.screenWidth, true);
            MemoryHelper::WriteMemory<int>(0x56ABF8, g_State.screenHeight, true);
            break;
        case FEARXP:
            MemoryHelper::WriteMemory<int>(0x60EC2C, g_State.screenWidth, true);
            MemoryHelper::WriteMemory<int>(0x60EC30, g_State.screenHeight, true);
            break;
        case FEARXP2:
            MemoryHelper::WriteMemory<int>(0x610C2C, g_State.screenWidth, true);
            MemoryHelper::WriteMemory<int>(0x610C30, g_State.screenHeight, true);
            break;
    }
}

static void HookIsFrameComplete()
{
    g_State.isUsingFpsLimiter = MaxFPS != 0 && !g_State.useVsyncOverride;
    if (!g_State.isUsingFpsLimiter && !XInputControllerSupport) return;

    g_State.fpsLimiter.SetTargetFps(MaxFPS);

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:    HookHelper::ApplyHook((void*)0x40FB20, &MainGameLoop_Hook, (LPVOID*)&MainGameLoop, true); break;
        case FEARMP:  HookHelper::ApplyHook((void*)0x40FC30, &MainGameLoop_Hook, (LPVOID*)&MainGameLoop); break;
        case FEARXP:  HookHelper::ApplyHook((void*)0x419100, &MainGameLoop_Hook, (LPVOID*)&MainGameLoop); break;
        case FEARXP2: HookHelper::ApplyHook((void*)0x4192B0, &MainGameLoop_Hook, (LPVOID*)&MainGameLoop); break;
    }
}

static void HookVSyncOverride()
{
    if (!DynamicVsync) return;

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:    HookHelper::ApplyHook((void*)0x4F8B80, &InitializePresentationParameters_Hook, (LPVOID*)&InitializePresentationParameters, true); break;
        case FEARMP:  HookHelper::ApplyHook((void*)0x4F8CA0, &InitializePresentationParameters_Hook, (LPVOID*)&InitializePresentationParameters); break;
        case FEARXP:  HookHelper::ApplyHook((void*)0x58F2B0, &InitializePresentationParameters_Hook, (LPVOID*)&InitializePresentationParameters); break;
        case FEARXP2: HookHelper::ApplyHook((void*)0x5908D0, &InitializePresentationParameters_Hook, (LPVOID*)&InitializePresentationParameters); break;
    }
}

static void HookTerminateServer()
{
    if (!g_State.needServerTermHooking) return;

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:    HookHelper::ApplyHook((void*)0x4634C0, &TerminateServer_Hook, (LPVOID*)&TerminateServer, true); break;
        case FEARMP:  HookHelper::ApplyHook((void*)0x4635E0, &TerminateServer_Hook, (LPVOID*)&TerminateServer); break;
        case FEARXP:  HookHelper::ApplyHook((void*)0x488B00, &TerminateServer_Hook, (LPVOID*)&TerminateServer); break;
        case FEARXP2: HookHelper::ApplyHook((void*)0x489860, &TerminateServer_Hook, (LPVOID*)&TerminateServer); break;
    }
}

static void ApplySaveFolderRedirect()
{
    if (!RedirectSaveFolder) return;

    HookHelper::ApplyHookAPI(L"shell32.dll", "SHGetFolderPathA", &SHGetFolderPathA_Hook, (LPVOID*)&ori_SHGetFolderPathA);
}

static void ApplyForceRenderMode()
{
    if (AutoResolution != 2) return;

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:
        case FEARMP:  HookHelper::ApplyHook((void*)0x40A800, &SetRenderMode_Hook, (LPVOID*)&SetRenderMode, true); break;
        case FEARXP:  HookHelper::ApplyHook((void*)0x411710, &SetRenderMode_Hook, (LPVOID*)&SetRenderMode); break;
        case FEARXP2: HookHelper::ApplyHook((void*)0x4119B0, &SetRenderMode_Hook, (LPVOID*)&SetRenderMode); break;
    }
}

static void ApplyDisableJoystick()
{
    if (!XInputControllerSupport) return;

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:    MemoryHelper::MakeNOP(0x484166, 25); break;
        case FEARMP:  MemoryHelper::MakeNOP(0x484286, 25); break;
        case FEARXP:  MemoryHelper::MakeNOP(0x4B89E6, 25); break;
        case FEARXP2: MemoryHelper::MakeNOP(0x4B9A36, 25); break;
    }
}

#pragma endregion

#pragma region Initialization

static void Init()
{
    ReadConfig();

    // LAA Patching if needed
    if (CheckLAAPatch)
    {
        SystemHelper::PerformLAAPatch(GetModuleHandleA(NULL));
    }

    // Get the handle of the client as soon as it is loaded
    ApplyClientHook();

    // Fixes
    ApplyFixDirectInputFps();
    ApplyFixHighFPSPhysics();
    ApplyOptimizeSaveSpeed();
    ApplyFixNvidiaShadowCorruption();
    ApplyFixKeyboardInputLanguage();

    // Display
    ApplyAutoResolution();

    // Graphics
    ApplyReducedMipMapBias();

    // Misc
    HookIsFrameComplete();
    HookVSyncOverride();
    HookTerminateServer();
    ApplySkipIntroHook();
    ApplyConsoleVariableHook();
    ApplySaveFolderRedirect();
    ApplyForceRenderMode();
    ApplyDisableJoystick();
}

static HWND WINAPI CreateWindowExA_Hook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    // Detect creation of F.E.A.R. window to initialize patches
    if (lpWindowName && strstr(lpWindowName, "F.E.A.R.") && nWidth == 320 && nHeight == 200)
    {
        // Disable this hook and initialize patches once the game's code has been decrypted in memory (by SecuROM or SteamDRM)
        MH_DisableHook(MH_ALL_HOOKS);
        Init();

        if (FixWindowStyle)
        {
            dwStyle = WS_POPUP | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        }

        g_State.hWnd = ori_CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
        return g_State.hWnd;
    }

    return ori_CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}

bool OnProcessAttach(HMODULE hModule)
{
    // Prevents DLL from receiving thread notifications
    DisableThreadLibraryCalls(hModule);

    uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
    IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)(base);
    IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
    DWORD timestamp = nt->FileHeader.TimeDateStamp;

    switch (timestamp)
    {
        case FEAR_TIMESTAMP:
            g_State.CurrentFEARGame = FEAR;
            break;
        case FEARMP_TIMESTAMP:
            g_State.CurrentFEARGame = FEARMP;
            break;
        case FEARXP_TIMESTAMP:
        case FEARXP_TIMESTAMP2:
            g_State.CurrentFEARGame = FEARXP;
            break;
        case FEARXP2_TIMESTAMP:
            g_State.CurrentFEARGame = FEARXP2;
            break;
        default:
            MessageBoxA(NULL, "This .exe is not supported.", "EchoPatch", MB_ICONERROR);
            return false;
    }

    SystemHelper::LoadProxyLibrary();
    HookHelper::ApplyHookAPI(L"user32.dll", "CreateWindowExA", &CreateWindowExA_Hook, (LPVOID*)&ori_CreateWindowExA);
    return true;
}

void OnProcessDetach()
{
    MH_Uninitialize();
}

#pragma endregion