#include "Core.hpp"
#include "../Client/Client.hpp"
#include "../Server/Server.hpp"
#include "../ClientFX/ClientFX.hpp"
#include "../Controller/Controller.hpp"
#include "MinHook.hpp"
#include "ini.hpp"
#include "DInputProxy.hpp"
#include "ConsoleMgr.hpp"
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
BOOL(WINAPI* ori_AdjustWindowRect)(LPRECT, DWORD, BOOL);
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
void(__thiscall* ProcessBallSocketConstraint)(int, float*, float*) = nullptr;
void(__thiscall* ProcessLimitedHingeConstraint)(int, float*, float*) = nullptr;
int(__thiscall* InitializePresentationParameters)(DWORD*, DWORD*, unsigned __int8) = nullptr;
int(__thiscall* MainGameLoop)(int) = nullptr;
int(__cdecl* SetRenderMode)(int) = nullptr;
int(__thiscall* FindStringCaseInsensitive)(DWORD*, char*) = nullptr;
bool(__thiscall* FileWrite)(DWORD*, LPCVOID, DWORD) = nullptr;
bool(__thiscall* FileOpen)(DWORD*, LPCSTR, char);
bool(__thiscall* FileSeek)(HANDLE*, LARGE_INTEGER) = nullptr;
bool(__thiscall* FileSeekEnd)(HANDLE*) = nullptr;
bool(__thiscall* FileTell)(DWORD*, DWORD*) = nullptr;
bool(__thiscall* FileClose)(DWORD*) = nullptr;
char(__thiscall* CreateAndInitializeDevice)(DWORD*, DWORD*, DWORD*, int, char) = nullptr;
char(__thiscall* GetSocketTransform)(DWORD*, unsigned int, DWORD*, char) = nullptr;
bool(__cdecl* EndScene)() = nullptr;
bool(__thiscall* ResetDevice)(void*, void*, unsigned char) = nullptr;
int(__stdcall* WindowProc)(HWND, UINT, WPARAM, LPARAM) = nullptr;
int(__cdecl* ConsoleOutput)(int, int, char*) = nullptr;
int(__stdcall* RegisterConsoleProgramClient)(char*, int) = nullptr;
int(__stdcall* RegisterConsoleProgramServer)(char*, int) = nullptr;
int(__stdcall* UnregisterConsoleProgramClient)(char*) = nullptr;
int(__stdcall* UnregisterConsoleProgramServer)(char*) = nullptr;

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
bool FixScriptedAnimationCrash = false;
int CheckLAAPatch = 0;

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
bool SDLGamepadSupport = false;
bool GyroEnabled = false;
int GyroAimingMode = 0;
float GyroSensitivity = 0.0f;
float GyroSmoothing = 0.0f;
bool TouchpadEnabled = false;
bool HideMouseCursor = false;
float GPadAimSensitivity = 0.0f;
float GPadAimEdgeThreshold = 0.0f;
float GPadAimEdgeAccelTime = 0.0f;
float GPadAimEdgeDelayTime = 0.0f;
float GPadAimEdgeMultiplier = 0.0f;
float GPadAimAspectRatio = 0.0f;
float GPadZoomMagThreshold = 0.0f;
int GAMEPAD_SOUTH = 0;
int GAMEPAD_EAST = 0;
int GAMEPAD_WEST = 0;
int GAMEPAD_NORTH = 0;
int GAMEPAD_LEFT_STICK = 0;
int GAMEPAD_RIGHT_STICK = 0;
int GAMEPAD_LEFT_SHOULDER = 0;
int GAMEPAD_RIGHT_SHOULDER = 0;
int GAMEPAD_DPAD_UP = 0;
int GAMEPAD_DPAD_DOWN = 0;
int GAMEPAD_DPAD_LEFT = 0;
int GAMEPAD_DPAD_RIGHT = 0;
int GAMEPAD_LEFT_TRIGGER = 0;
int GAMEPAD_RIGHT_TRIGGER = 0;
int GAMEPAD_BACK = 0;
int GAMEPAD_START = 0;
int GAMEPAD_MISC1 = 0;
int GAMEPAD_RIGHT_PADDLE1 = 0;
int GAMEPAD_LEFT_PADDLE1 = 0;
int GAMEPAD_RIGHT_PADDLE2 = 0;
int GAMEPAD_LEFT_PADDLE2 = 0;

// SkipIntro
bool SkipSplashScreen = false;
bool SkipAllIntro = false;
bool SkipSierraIntro = false;
bool SkipMonolithIntro = false;
bool SkipWBGamesIntro = false;
bool SkipNvidiaIntro = false;
bool SkipTimegateIntro = false;
bool SkipDellIntro = false;

// Console
bool ConsoleEnabled = false;
int DebugLevel = 0;
bool HighResolutionScaling = false;
bool LogOutputToFile = false;

// Extra
bool RedirectSaveFolder = false;
bool InfiniteFlashlight = false;
bool DisablePunkBuster = false;
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
    FixScriptedAnimationCrash = IniHelper::ReadInteger("Fixes", "FixScriptedAnimationCrash", 1) == 1;
    CheckLAAPatch = IniHelper::ReadInteger("Fixes", "CheckLAAPatch", 1);

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
    SDLGamepadSupport = IniHelper::ReadInteger("Controller", "SDLGamepadSupport", 1) == 1;
    GyroEnabled = IniHelper::ReadInteger("Controller", "GyroEnabled", 0) == 1;
    GyroAimingMode = IniHelper::ReadInteger("Controller", "GyroAimingMode", 0);
    GyroSensitivity = IniHelper::ReadFloat("Controller", "GyroSensitivity", 1.0f);
    GyroSmoothing = IniHelper::ReadFloat("Controller", "GyroSmoothing", 0.016f);
    TouchpadEnabled = IniHelper::ReadInteger("Controller", "TouchpadEnabled", 1) == 1;
    HideMouseCursor = IniHelper::ReadInteger("Controller", "HideMouseCursor", 0) == 1;
    GPadAimSensitivity = IniHelper::ReadFloat("Controller", "GPadAimSensitivity", 2.0f);
    GPadAimEdgeThreshold = IniHelper::ReadFloat("Controller", "GPadAimEdgeThreshold", 0.75f);
    GPadAimEdgeAccelTime = IniHelper::ReadFloat("Controller", "GPadAimEdgeAccelTime", 1.0f);
    GPadAimEdgeDelayTime = IniHelper::ReadFloat("Controller", "GPadAimEdgeDelayTime", 0.25f);
    GPadAimEdgeMultiplier = IniHelper::ReadFloat("Controller", "GPadAimEdgeMultiplier", 1.6f);
    GPadAimAspectRatio = IniHelper::ReadFloat("Controller", "GPadAimAspectRatio", 1.0f);
    GPadZoomMagThreshold = IniHelper::ReadFloat("Controller", "GPadZoomMagThreshold", 1.3f);
    GAMEPAD_SOUTH = IniHelper::ReadInteger("Controller", "GAMEPAD_SOUTH", 15);
    GAMEPAD_EAST = IniHelper::ReadInteger("Controller", "GAMEPAD_EAST", 14);
    GAMEPAD_WEST = IniHelper::ReadInteger("Controller", "GAMEPAD_WEST", 88);
    GAMEPAD_NORTH = IniHelper::ReadInteger("Controller", "GAMEPAD_NORTH", 106);
    GAMEPAD_LEFT_STICK = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_STICK", 70);
    GAMEPAD_RIGHT_STICK = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_STICK", 114);
    GAMEPAD_LEFT_SHOULDER = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_SHOULDER", 81);
    GAMEPAD_RIGHT_SHOULDER = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_SHOULDER", 19);
    GAMEPAD_DPAD_UP = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_UP", 77);
    GAMEPAD_DPAD_DOWN = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_DOWN", 73);
    GAMEPAD_DPAD_LEFT = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_LEFT", 20);
    GAMEPAD_DPAD_RIGHT = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_RIGHT", 21);
    GAMEPAD_LEFT_TRIGGER = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_TRIGGER", 71);
    GAMEPAD_RIGHT_TRIGGER = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_TRIGGER", 17);
    GAMEPAD_BACK = IniHelper::ReadInteger("Controller", "GAMEPAD_BACK", 78);
    GAMEPAD_START = IniHelper::ReadInteger("Controller", "GAMEPAD_START", 13);
    GAMEPAD_MISC1 = IniHelper::ReadInteger("Controller", "GAMEPAD_MISC1", 50);
    GAMEPAD_RIGHT_PADDLE1 = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_PADDLE1", 82);
    GAMEPAD_LEFT_PADDLE1 = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_PADDLE1", 102);
    GAMEPAD_RIGHT_PADDLE2 = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_PADDLE2", 75);
    GAMEPAD_LEFT_PADDLE2 = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_PADDLE2", 74);

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
        ConfigureGamepadMappings(GAMEPAD_SOUTH, GAMEPAD_EAST, GAMEPAD_WEST, GAMEPAD_NORTH, GAMEPAD_LEFT_STICK, GAMEPAD_RIGHT_STICK, GAMEPAD_LEFT_SHOULDER, GAMEPAD_RIGHT_SHOULDER, GAMEPAD_DPAD_UP, GAMEPAD_DPAD_DOWN, GAMEPAD_DPAD_LEFT, GAMEPAD_DPAD_RIGHT, GAMEPAD_BACK, GAMEPAD_START, GAMEPAD_LEFT_TRIGGER, GAMEPAD_RIGHT_TRIGGER, GAMEPAD_MISC1, GAMEPAD_RIGHT_PADDLE1, GAMEPAD_LEFT_PADDLE1, GAMEPAD_RIGHT_PADDLE2, GAMEPAD_LEFT_PADDLE2);
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

    // Gyro Config
    SetGyroEnabled(GyroEnabled);
    SetGyroSensitivity(GyroSensitivity);
    SetGyroSmoothing(GyroSmoothing);
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

static bool ShouldClampRagdoll(int thisPtr)
{
    int owner = *reinterpret_cast<int*>(thisPtr + 0x14);  // Physics aggregate owner
    if (!owner)
        return false;

    // Ragdolls have 8+ bodies (bones), simple props have 2-4
    int bodyCount = *reinterpret_cast<int*>(owner + 0x40);
    if (bodyCount < 8)
        return false;

    double currentTime = g_State.totalGameTime;
    double expireThreshold = g_State.RAGDOLL_STABILIZE_TIME * 2.0;

    auto* cache = g_State.ragdollCache.data();

    GlobalState::RagdollEntry* freeSlot = nullptr;
    GlobalState::RagdollEntry* oldestSlot = cache;

    for (size_t i = 0; i < g_State.ragdollCache.size(); i++)
    {
        if (cache[i].owner == owner)
            return (currentTime - cache[i].firstSeenTime) < g_State.RAGDOLL_STABILIZE_TIME;

        // Reuse empty or expired slots
        if (!freeSlot && (cache[i].owner == 0 || (currentTime - cache[i].firstSeenTime) > expireThreshold))
            freeSlot = &cache[i];

        if (cache[i].firstSeenTime < oldestSlot->firstSeenTime)
            oldestSlot = &cache[i];
    }

    // New ragdoll: use free slot if available, otherwise evict oldest
    GlobalState::RagdollEntry* slot = freeSlot ? freeSlot : oldestSlot;
    slot->owner = owner;
    slot->firstSeenTime = currentTime;
    return true;
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
    switch (pszVarName[0])
    {
        case 'G':  // GPad*
        {
            if (!SDLGamepadSupport) break;

            const uint32_t varHash = HashHelper::FNV1aRuntime(pszVarName);
            switch (varHash)
            {
                case HashHelper::CVarHashes::GPadAimSensitivity:
                    fValue = GPadAimSensitivity; break;
                case HashHelper::CVarHashes::GPadAimEdgeThreshold:
                    fValue = GPadAimEdgeThreshold; break;
                case HashHelper::CVarHashes::GPadAimEdgeAccelTime:
                    fValue = GPadAimEdgeAccelTime; break;
                case HashHelper::CVarHashes::GPadAimEdgeDelayTime:
                    fValue = GPadAimEdgeDelayTime; break;
                case HashHelper::CVarHashes::GPadAimEdgeMultiplier:
                    fValue = GPadAimEdgeMultiplier; break;
                case HashHelper::CVarHashes::GPadAimAspectRatio:
                    fValue = GPadAimAspectRatio; break;
            }
            break;
        }

        case 'M':  // ModelLODDistanceScale, MouseInvertY
        {
            if (!NoLODBias && !(SDLGamepadSupport && GyroEnabled)) break;

            const uint32_t varHash = HashHelper::FNV1aRuntime(pszVarName);
            if (NoLODBias && varHash == HashHelper::CVarHashes::ModelLODDistanceScale)
            {
                if (fValue > 0.0f)
                    fValue = 0.003f;
            }
            else if (SDLGamepadSupport && GyroEnabled && varHash == HashHelper::CVarHashes::MouseInvertY)
            {
                SetGyroInvertY(fValue == 1.0f);
            }
            break;
        }

        case 'C':  // CameraFirstPersonLODBias, CrosshairSize, CheckPointOptimizeVideoMemory
        {
            if (!NoLODBias && !HUDScaling && !OptimizeSaveSpeed) break;

            const uint32_t varHash = HashHelper::FNV1aRuntime(pszVarName);
            if (NoLODBias && varHash == HashHelper::CVarHashes::CameraFirstPersonLODBias)
            {
                fValue = 0.0f;
            }
            else if (HUDScaling && varHash == HashHelper::CVarHashes::CrosshairSize)
            {
                g_State.crosshairSize = fValue;
                fValue *= g_State.scalingFactorCrosshair;
            }
            else if (OptimizeSaveSpeed && varHash == HashHelper::CVarHashes::CheckPointOptimizeVideoMemory)
            {
                fValue = 0.0f;
            }
            break;
        }

        case 'S':  // StreamResources
        {
            if (!ForceWindowed) break;

            const uint32_t varHash = HashHelper::FNV1aRuntime(pszVarName);
            if (varHash == HashHelper::CVarHashes::StreamResources)
            {
                SetConsoleVariableFloat("Windowed", 1.0f);
                ForceWindowed = false;
            }
            break;
        }

        case 'P':  // Performance_Screen*, PerturbScale
        {
            if (!(g_State.isInAutoDetect && AutoResolution != 0) && !HUDScaling) break;

            const uint32_t varHash = HashHelper::FNV1aRuntime(pszVarName);
            if (g_State.isInAutoDetect && AutoResolution != 0)
            {
                if (varHash == HashHelper::CVarHashes::Performance_ScreenHeight)
                {
                    fValue = static_cast<float>(g_State.screenHeight);
                }
                else if (varHash == HashHelper::CVarHashes::Performance_ScreenWidth)
                {
                    fValue = static_cast<float>(g_State.screenWidth);
                }
            }
            if (HUDScaling && varHash == HashHelper::CVarHashes::PerturbScale)
            {
                fValue *= g_State.scalingFactorCrosshair;
            }
            break;
        }

        case 'U':  // UseTextScaling
        {
            if (!HUDScaling) break;

            const uint32_t varHash = HashHelper::FNV1aRuntime(pszVarName);
            if (varHash == HashHelper::CVarHashes::UseTextScaling)
            {
                fValue = 0.0f;
            }
            break;
        }
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
        // Index to DirectInput Key Id mapping
        static const std::unordered_map<int, unsigned int> keyMap =
        {
            {0, 0x11},   // W
            {1, 0x1F},   // S
            {2, 0x1E},   // A
            {3, 0x20},   // D
            {4, 0xCB},   // Left
            {5, 0xCD},   // Right
            {6, 0x9D},   // Right Ctrl
            {8, 0x39},   // Space
            {9, 0x2E},   // C
            {10, 0x10},  // Q
            {11, 0x12},  // E
            {13, 0x22},  // G
            {14, 0x21},  // F
            {15, 0x13},  // R
            {16, 0x2A},  // Shift
            {17, 0x1D},  // Ctrl
            {18, 0x2D},  // X
            {19, 0x0F},  // Tab
            {20, 0x32},  // M
            {21, 0x14},  // T
            {22, 0x15},  // Y
            {23, 0x2F},  // V
            {24, 0xC8},  // Up
            {25, 0xD0},  // Down
            {26, 0xCF},  // End
            {31, 0x30},  // B
            {36, 0x2C},  // Z
            {37, 0x23}   // H
        };

        auto it = keyMap.find(g_State.currentKeyIndex);
        g_State.currentKeyIndex++;

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

static void __fastcall ProcessBallSocketConstraint_Hook(int thisPtr, int, float* in, float* out)
{
    g_State.isProcessingRagdoll = ShouldClampRagdoll(thisPtr);
    ProcessBallSocketConstraint(thisPtr, in, out);
    g_State.isProcessingRagdoll = false;
}

static void __fastcall ProcessLimitedHingeConstraint_Hook(int thisPtr, int, float* in, float* out)
{
    g_State.isProcessingRagdoll = ShouldClampRagdoll(thisPtr);
    ProcessLimitedHingeConstraint(thisPtr, in, out);
    g_State.isProcessingRagdoll = false;
}

static void __cdecl BuildJacobianRow_Hook(int jacobianData, float* constraintInstance, float* queryIn)
{
    float timeStepBak = constraintInstance[3];
    if (g_State.isProcessingRagdoll && constraintInstance[3] > 250.0f)
        constraintInstance[3] = 250.0f;

    BuildJacobianRow(jacobianData, constraintInstance, queryIn);
    constraintInstance[3] = timeStepBak;
}

static void __cdecl ProcessTwistLimitConstraint_Hook(int twistParams, float* constraintInstance, float* queryIn)
{
    float timeStepBak = constraintInstance[3];
    if (g_State.isProcessingRagdoll && constraintInstance[3] > 250.0f)
        constraintInstance[3] = 250.0f;

    ProcessTwistLimitConstraint(twistParams, constraintInstance, queryIn);
    constraintInstance[3] = timeStepBak;
}

static void __cdecl ProcessConeLimitConstraint_Hook(int pivotA, float* pivotB, float* constraintInstance, float* queryIn)
{
    float timeStepBak = constraintInstance[3];
    if (g_State.isProcessingRagdoll && constraintInstance[3] > 250.0f)
        constraintInstance[3] = 250.0f;

    ProcessConeLimitConstraint(pivotA, pivotB, constraintInstance, queryIn);
    constraintInstance[3] = timeStepBak;
}

// =========================
// OptimizeSaveSpeed
// =========================

static bool __fastcall FileWrite_Hook(DWORD* thisp, int, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite)
{
    HANDLE h = reinterpret_cast<HANDLE>(thisp[1]);

    if (g_State.saveBuffer.handle == h)
    {
        LONGLONG endPos = g_State.saveBuffer.position + nNumberOfBytesToWrite;

        if (endPos > (LONGLONG)g_State.saveBuffer.buffer.size())
        {
            g_State.saveBuffer.buffer.resize((size_t)endPos, 0);
        }

        memcpy(g_State.saveBuffer.buffer.data() + g_State.saveBuffer.position, lpBuffer, nNumberOfBytesToWrite);

        g_State.saveBuffer.position = endPos;
        if (endPos > g_State.saveBuffer.size)
        {
            g_State.saveBuffer.size = endPos;
        }

        return true;
    }

    return FileWrite(thisp, lpBuffer, nNumberOfBytesToWrite);
}

static bool __fastcall FileOpen_Hook(DWORD* thisp, int, LPCSTR lpFileName, char a3)
{
    bool result = FileOpen(thisp, lpFileName, a3);

    if (result && lpFileName)
    {
        HANDLE h = reinterpret_cast<HANDLE>(thisp[1]);

        if (h != INVALID_HANDLE_VALUE)
        {
            std::string_view filename(lpFileName);

            if (filename.ends_with(".sav"))
            {
                g_State.saveBuffer.handle = h;
                g_State.saveBuffer.position = 0;
                g_State.saveBuffer.size = 0;
                g_State.saveBuffer.flushed = false;
                g_State.saveBuffer.buffer.resize(4 * 1024 * 1024, 0);
            }
        }
    }

    return result;
}

static bool __fastcall FileSeek_Hook(HANDLE* thisp, int, LARGE_INTEGER liDistanceToMove)
{
    HANDLE h = thisp[1];

    if (g_State.saveBuffer.handle == h && !g_State.saveBuffer.flushed)
    {
        g_State.saveBuffer.position = liDistanceToMove.QuadPart;
        return true;
    }

    return FileSeek(thisp, liDistanceToMove);
}

static bool __fastcall FileSeekEnd_Hook(HANDLE* thisp, int)
{
    HANDLE h = thisp[1];
    if (g_State.saveBuffer.handle == h && !g_State.saveBuffer.flushed)
    {
        g_State.saveBuffer.position = g_State.saveBuffer.size;
        return true;
    }

    return FileSeekEnd(thisp);
}

static bool __fastcall FileTell_Hook(DWORD* thisp, int, DWORD* a2)
{
    HANDLE h = reinterpret_cast<HANDLE>(thisp[1]);

    if (g_State.saveBuffer.handle == h && !g_State.saveBuffer.flushed)
    {
        a2[0] = (DWORD)g_State.saveBuffer.position;
        a2[1] = (DWORD)(g_State.saveBuffer.position >> 32);
        return 1;
    }

    return FileTell(thisp, a2);
}

static bool __fastcall FileClose_Hook(DWORD* thisp, int)
{
    HANDLE h = reinterpret_cast<HANDLE>(thisp[1]);

    if (g_State.saveBuffer.handle == h)
    {
        if (!g_State.saveBuffer.flushed && g_State.saveBuffer.size > 0)
        {
            g_State.saveBuffer.flushed = true;

            LARGE_INTEGER zero = { 0 };
            FileSeek((HANDLE*)thisp, zero);

            FileWrite(thisp, g_State.saveBuffer.buffer.data(), (DWORD)g_State.saveBuffer.size);
        }

        g_State.saveBuffer.Reset();
    }

    return FileClose(thisp);
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
            case FEAR:    MemoryHelper::WriteMemory<uint8_t>(g_State.BaseAddress + 0x112234, 2); break;
            case FEARMP:  MemoryHelper::WriteMemory<uint8_t>(g_State.BaseAddress + 0x112354, 2); break;
            case FEARXP:  MemoryHelper::WriteMemory<uint8_t>(g_State.BaseAddress + 0x1B5A24, 2); break;
            case FEARXP2: MemoryHelper::WriteMemory<uint8_t>(g_State.BaseAddress + 0x1B6F94, 2); break;
        }
    }
    else
    {
        switch (g_State.CurrentFEARGame)
        {
            // Should never happen but do it anyway
            case FEAR:    MemoryHelper::WriteMemory<uint8_t>(g_State.BaseAddress + 0x112234, 1); break;
            case FEARMP:  MemoryHelper::WriteMemory<uint8_t>(g_State.BaseAddress + 0x112354, 1); break;
            case FEARXP:  MemoryHelper::WriteMemory<uint8_t>(g_State.BaseAddress + 0x1B5A24, 1); break;
            case FEARXP2: MemoryHelper::WriteMemory<uint8_t>(g_State.BaseAddress + 0x1B6F94, 1); break;
        }
    }

    return CreateAndInitializeDevice(thisp, a2, a3, a4, a5);
}

// ===========================
// FixScriptedAnimationCrash
// ===========================

static char __fastcall GetSocketTransform_Hook(DWORD* thisp, int, unsigned int nodeIndex, DWORD* outTransform, char a4)
{
    int modelData = *(thisp + 68);

    // Attempts to retrieve bone transforms for a character whose model data has been freed
    if (modelData == 0)
    {
        // Return identity transform and return success
        outTransform[0] = 0;
        outTransform[1] = 0;
        outTransform[2] = 0;
        outTransform[3] = 0;
        outTransform[4] = 0;
        outTransform[5] = 0;
        outTransform[6] = 0x3F800000;
        outTransform[7] = 0x3F800000;

        return 1;
    }

    return GetSocketTransform(thisp, nodeIndex, outTransform, a4);
}

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
        size_t len = strlen(Source);
        if (len > 0)
        {
            if (Source[len - 1] == '\n')
            {
                char buffer[512];
                size_t copyLen = (len - 1 < sizeof(buffer) - 1) ? len - 1 : sizeof(buffer) - 1;
                memcpy(buffer, Source, copyLen);
                buffer[copyLen] = '\0';
                Console::AddOutput("%s", buffer);
            }
            else
            {
                Console::AddOutput("%s", Source);
            }
        }
    }

    return ConsoleOutput(a1, a2, Source);
}

static char __cdecl SetCvarString_Hook(int a1, const char* name, char* value)
{
    if (name && name[0] && value)
    {
        auto it = FindCvarCaseInsensitive(std::string(name));
        if (it != g_dynamicCvars.end())
        {
            it->second = { value, a1, CvarType::String };
        }
        else
        {
            g_dynamicCvars[name] = { value, a1, CvarType::String };
        }
    }

    return SetCvarString(a1, name, value);
}

static char __cdecl SetCvarFloat_Hook(int a1, const char* name, int valueInt)
{
    if (name && name[0])
    {
        float value = *(float*)&valueInt;
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f", value);

        auto it = FindCvarCaseInsensitive(std::string(name));
        if (it != g_dynamicCvars.end())
        {
            it->second = { buf, a1, CvarType::Float };
        }
        else
        {
            g_dynamicCvars[name] = { buf, a1, CvarType::Float };
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

// ========================
// DynamicVsync
// ========================

static int __fastcall InitializePresentationParameters_Hook(DWORD* thisPtr, int, DWORD* a2, unsigned __int8 a3)
{
    int res = InitializePresentationParameters(thisPtr, a2, a3);
    thisPtr[101] = g_State.useVsyncOverride != 0 ? 0 : 0x80000000;
    return res;
}

// ===============================================
// MaxFPS & SDLGamepadSupport & ConsoleEnabled
// ===============================================

static int __fastcall MainGameLoop_Hook(int thisPtr, int)
{
    if (g_State.isUsingFpsLimiter)
    {
        g_State.fpsLimiter.Limit();
    }

    if (SDLGamepadSupport)
    {
        PollController();
    }

    if (ConsoleEnabled)
    {
        g_State.isConsoleOpen = Console::Update();
    }

    if (HighFPSFixes)
    {
        if (auto* pTimingStruct = *reinterpret_cast<void**>(thisPtr + 0x5D0))
        {
            g_State.currentFrameTime = *reinterpret_cast<float*>(static_cast<char*>(pTimingStruct) + 0x2C);
            g_State.totalGameTime += g_State.currentFrameTime;
        }
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
            case FEAR:    MH_DisableHook((void*)(g_State.BaseAddress + 0x110CB0)); break;
            case FEARMP:  MH_DisableHook((void*)(g_State.BaseAddress + 0x110DD0)); break;
            case FEARXP:  MH_DisableHook((void*)(g_State.BaseAddress + 0x1B3440)); break;
            case FEARXP2: MH_DisableHook((void*)(g_State.BaseAddress + 0x1B49C0)); break;
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

static BOOL WINAPI AdjustWindowRect_Hook(LPRECT lpRect, DWORD dwStyle, BOOL bMenu)
{
    if (dwStyle == 0xCF0000)
    {
        dwStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    }

    return ori_AdjustWindowRect(lpRect, dwStyle, bMenu);
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
            MemoryHelper::MakeNOP(g_State.BaseAddress + 0x840DD, 22);
            MemoryHelper::MakeNOP(g_State.BaseAddress + 0x84057, 29);
            break;
        case FEARMP:
            MemoryHelper::MakeNOP(g_State.BaseAddress + 0x841FD, 22);
            MemoryHelper::MakeNOP(g_State.BaseAddress + 0x84177, 29);
            break;
        case FEARXP:
            MemoryHelper::MakeNOP(g_State.BaseAddress + 0xB895D, 22);
            MemoryHelper::MakeNOP(g_State.BaseAddress + 0xB88D7, 29);
            break;
        case FEARXP2:
            MemoryHelper::MakeNOP(g_State.BaseAddress + 0xB99AD, 22);
            MemoryHelper::MakeNOP(g_State.BaseAddress + 0xB9927, 29);
            break;
    }
}

static void ApplyFixHighFPSPhysics()
{
    if (!HighFPSFixes) return;

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x98390), &ProcessBallSocketConstraint_Hook, (LPVOID*)&ProcessBallSocketConstraint, true);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x99050), &ProcessLimitedHingeConstraint_Hook, (LPVOID*)&ProcessLimitedHingeConstraint, true);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xA8D30), &BuildJacobianRow_Hook, (LPVOID*)&BuildJacobianRow, true);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xA9940), &ProcessTwistLimitConstraint_Hook, (LPVOID*)&ProcessTwistLimitConstraint, true);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xA9F00), &ProcessConeLimitConstraint_Hook, (LPVOID*)&ProcessConeLimitConstraint, true);
            break;
        case FEARMP:
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x984B0), &ProcessBallSocketConstraint_Hook, (LPVOID*)&ProcessBallSocketConstraint);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x99170), &ProcessLimitedHingeConstraint_Hook, (LPVOID*)&ProcessLimitedHingeConstraint);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xA8E50), &BuildJacobianRow_Hook, (LPVOID*)&BuildJacobianRow);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xA9A60), &ProcessTwistLimitConstraint_Hook, (LPVOID*)&ProcessTwistLimitConstraint);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xAA020), &ProcessConeLimitConstraint_Hook, (LPVOID*)&ProcessConeLimitConstraint);
            break;
        case FEARXP:
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xDC4B0), &ProcessBallSocketConstraint_Hook, (LPVOID*)&ProcessBallSocketConstraint);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xDD4C0), &ProcessLimitedHingeConstraint_Hook, (LPVOID*)&ProcessLimitedHingeConstraint);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xFF710), &BuildJacobianRow_Hook, (LPVOID*)&BuildJacobianRow);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x100320), &ProcessTwistLimitConstraint_Hook, (LPVOID*)&ProcessTwistLimitConstraint);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x1008E0), &ProcessConeLimitConstraint_Hook, (LPVOID*)&ProcessConeLimitConstraint);
            break;
        case FEARXP2:
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xDD520), &ProcessBallSocketConstraint_Hook, (LPVOID*)&ProcessBallSocketConstraint);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xDE530), &ProcessLimitedHingeConstraint_Hook, (LPVOID*)&ProcessLimitedHingeConstraint);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x100780), &BuildJacobianRow_Hook, (LPVOID*)&BuildJacobianRow);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x101390), &ProcessTwistLimitConstraint_Hook, (LPVOID*)&ProcessTwistLimitConstraint);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x101950), &ProcessConeLimitConstraint_Hook, (LPVOID*)&ProcessConeLimitConstraint);
            break;
    }
}

static void ApplyOptimizeSaveSpeed()
{
    if (!OptimizeSaveSpeed) return;

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x1C670), &FileWrite_Hook, (LPVOID*)&FileWrite, true);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x1C810), &FileOpen_Hook, (LPVOID*)&FileOpen, true);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x1C6B0), &FileSeek_Hook, (LPVOID*)&FileSeek, true);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x1C6E0), &FileSeekEnd_Hook, (LPVOID*)&FileSeekEnd, true);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x1C700), &FileTell_Hook, (LPVOID*)&FileTell, true);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x1C7A0), &FileClose_Hook, (LPVOID*)&FileClose, true);
            break;
        case FEARMP:
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x1C790), &FileWrite_Hook, (LPVOID*)&FileWrite);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x1C930), &FileOpen_Hook, (LPVOID*)&FileOpen);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x1C7D0), &FileSeek_Hook, (LPVOID*)&FileSeek);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x1C800), &FileSeekEnd_Hook, (LPVOID*)&FileSeekEnd);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x1C820), &FileTell_Hook, (LPVOID*)&FileTell);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x1C8C0), &FileClose_Hook, (LPVOID*)&FileClose);
            break;
        case FEARXP:
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x29720), &FileWrite_Hook, (LPVOID*)&FileWrite);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x298F0), &FileOpen_Hook, (LPVOID*)&FileOpen);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x29760), &FileSeek_Hook, (LPVOID*)&FileSeek);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x29790), &FileSeekEnd_Hook, (LPVOID*)&FileSeekEnd);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x297B0), &FileTell_Hook, (LPVOID*)&FileTell);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x29850), &FileClose_Hook, (LPVOID*)&FileClose);
            break;
        case FEARXP2:
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x29900), &FileWrite_Hook, (LPVOID*)&FileWrite);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x29AD0), &FileOpen_Hook, (LPVOID*)&FileOpen);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x29940), &FileSeek_Hook, (LPVOID*)&FileSeek);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x29970), &FileSeekEnd_Hook, (LPVOID*)&FileSeekEnd);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x29990), &FileTell_Hook, (LPVOID*)&FileTell);
            HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x29A30), &FileClose_Hook, (LPVOID*)&FileClose);
            break;
    }
}

static void ApplyFixNvidiaShadowCorruption()
{
    if (!FixNvidiaShadowCorruption) return;

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:    HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xF91E0), &CreateAndInitializeDevice_Hook, (LPVOID*)&CreateAndInitializeDevice, true); break;
        case FEARMP:  HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xF9300), &CreateAndInitializeDevice_Hook, (LPVOID*)&CreateAndInitializeDevice); break;
        case FEARXP:  HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x18F930), &CreateAndInitializeDevice_Hook, (LPVOID*)&CreateAndInitializeDevice); break;
        case FEARXP2: HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x190F60), &CreateAndInitializeDevice_Hook, (LPVOID*)&CreateAndInitializeDevice); break;
    }
}

static void ApplyFixScriptedAnimationCrash()
{
    if (!FixScriptedAnimationCrash) return;

    // Only happen on the first game
    if (g_State.CurrentFEARGame == FEAR)
    {
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x36F10), &GetSocketTransform_Hook, (LPVOID*)&GetSocketTransform, true);
    }
}

static void ApplyFixKeyboardInputLanguage()
{
    if (!FixKeyboardInputLanguage) return;

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:    HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x81E10), &GetDeviceObjectDesc_Hook, (LPVOID*)&GetDeviceObjectDesc, true); break;
        case FEARMP:  HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x81F30), &GetDeviceObjectDesc_Hook, (LPVOID*)&GetDeviceObjectDesc); break;
        case FEARXP:  HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xB5DE0), &GetDeviceObjectDesc_Hook, (LPVOID*)&GetDeviceObjectDesc); break;
        case FEARXP2: HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xB6E10), &GetDeviceObjectDesc_Hook, (LPVOID*)&GetDeviceObjectDesc); break;
    }
}

static void ApplyFixWindow()
{
    if (!FixWindowStyle) return;

    HookHelper::ApplyHookAPI(L"user32.dll", "AdjustWindowRect", &AdjustWindowRect_Hook, (LPVOID*)&ori_AdjustWindowRect);

    // Remove 'DISCL_NOWINKEY'
    switch (g_State.CurrentFEARGame)
    {
        case FEAR:    MemoryHelper::WriteMemory<uint8_t>(g_State.BaseAddress + 0x81B4B, 6); break;
        case FEARMP:  MemoryHelper::WriteMemory<uint8_t>(g_State.BaseAddress + 0x81C6B, 6); break;
        case FEARXP:  MemoryHelper::WriteMemory<uint8_t>(g_State.BaseAddress + 0xB58CB, 6); break;
        case FEARXP2: MemoryHelper::WriteMemory<uint8_t>(g_State.BaseAddress + 0xB68FB, 6); break;
    }
}

static void ApplyReducedMipMapBias()
{
    if (!ReducedMipMapBias) return;

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:
        case FEARMP:  MemoryHelper::WriteMemory<float>(g_State.BaseAddress + 0x16D5C4, -0.5f, false); break;
        case FEARXP:  MemoryHelper::WriteMemory<float>(g_State.BaseAddress + 0x212B94, -0.5f, false); break;
        case FEARXP2: MemoryHelper::WriteMemory<float>(g_State.BaseAddress + 0x214BA4, -0.5f, false); break;
    }
}

static void ApplyClientHook()
{
    switch (g_State.CurrentFEARGame)
    {
        case FEAR:    HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x7D730), &LoadGameDLL_Hook, (LPVOID*)&LoadGameDLL, true); break;
        case FEARMP:  HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x7D850), &LoadGameDLL_Hook, (LPVOID*)&LoadGameDLL); break;
        case FEARXP:  HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xAF260), &LoadGameDLL_Hook, (LPVOID*)&LoadGameDLL); break;
        case FEARXP2: HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xB02C0), &LoadGameDLL_Hook, (LPVOID*)&LoadGameDLL); break;
    }
}

static void ApplySkipIntroHook()
{
    switch (g_State.CurrentFEARGame)
    {
        case FEAR:    HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x110CB0), &FindStringCaseInsensitive_Hook, (LPVOID*)&FindStringCaseInsensitive, true); break;
        case FEARMP:  HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x110DD0), &FindStringCaseInsensitive_Hook, (LPVOID*)&FindStringCaseInsensitive); break;
        case FEARXP:  HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x1B3440), &FindStringCaseInsensitive_Hook, (LPVOID*)&FindStringCaseInsensitive); break;
        case FEARXP2: HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x1B49C0), &FindStringCaseInsensitive_Hook, (LPVOID*)&FindStringCaseInsensitive); break;
    }
}

static void ApplyConsoleVariableHook()
{
    switch (g_State.CurrentFEARGame)
    {
        case FEAR:
        case FEARMP:  HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x9360), &SetConsoleVariableFloat_Hook, (LPVOID*)&SetConsoleVariableFloat, true); break;
        case FEARXP:  HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x10120), &SetConsoleVariableFloat_Hook, (LPVOID*)&SetConsoleVariableFloat); break;
        case FEARXP2: HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x10360), &SetConsoleVariableFloat_Hook, (LPVOID*)&SetConsoleVariableFloat); break;
    }
}

static void ApplyAutoResolution()
{
    if (AutoResolution == 0) return;

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:
        case FEARMP:
            MemoryHelper::WriteMemory<int>(g_State.BaseAddress + 0x16ABF4, g_State.screenWidth, false);
            MemoryHelper::WriteMemory<int>(g_State.BaseAddress + 0x16ABF8, g_State.screenHeight, false);
            break;
        case FEARXP:
            MemoryHelper::WriteMemory<int>(g_State.BaseAddress + 0x20EC2C, g_State.screenWidth, false);
            MemoryHelper::WriteMemory<int>(g_State.BaseAddress + 0x20EC30, g_State.screenHeight, false);
            break;
        case FEARXP2:
            MemoryHelper::WriteMemory<int>(g_State.BaseAddress + 0x210C2C, g_State.screenWidth, false);
            MemoryHelper::WriteMemory<int>(g_State.BaseAddress + 0x210C30, g_State.screenHeight, false);
            break;
    }
}

static void HookMainLoop()
{
    g_State.isUsingFpsLimiter = MaxFPS != 0 && !g_State.useVsyncOverride;
    if (!g_State.isUsingFpsLimiter && !SDLGamepadSupport && !HighFPSFixes && !ConsoleEnabled) return;

    g_State.fpsLimiter.SetTargetFps(MaxFPS);

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:    HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xFB20), &MainGameLoop_Hook, (LPVOID*)&MainGameLoop, true); break;
        case FEARMP:  HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xFC30), &MainGameLoop_Hook, (LPVOID*)&MainGameLoop); break;
        case FEARXP:  HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x19100), &MainGameLoop_Hook, (LPVOID*)&MainGameLoop); break;
        case FEARXP2: HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x192B0), &MainGameLoop_Hook, (LPVOID*)&MainGameLoop); break;
    }
}

static void HookVSyncOverride()
{
    if (!DynamicVsync) return;

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:    HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xF8B80), &InitializePresentationParameters_Hook, (LPVOID*)&InitializePresentationParameters, true); break;
        case FEARMP:  HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xF8CA0), &InitializePresentationParameters_Hook, (LPVOID*)&InitializePresentationParameters); break;
        case FEARXP:  HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x18F2B0), &InitializePresentationParameters_Hook, (LPVOID*)&InitializePresentationParameters); break;
        case FEARXP2: HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x1908D0), &InitializePresentationParameters_Hook, (LPVOID*)&InitializePresentationParameters); break;
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
        case FEARMP:  HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xA800), &SetRenderMode_Hook, (LPVOID*)&SetRenderMode, true); break;
        case FEARXP:  HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x11710), &SetRenderMode_Hook, (LPVOID*)&SetRenderMode); break;
        case FEARXP2: HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x119B0), &SetRenderMode_Hook, (LPVOID*)&SetRenderMode); break;
    }
}

static void ApplyDisableJoystick()
{
    if (!SDLGamepadSupport && !DisableRedundantHIDInit) return;

    switch (g_State.CurrentFEARGame)
    {
        case FEAR:    MemoryHelper::MakeNOP(g_State.BaseAddress + 0x84166, 25); break;
        case FEARMP:  MemoryHelper::MakeNOP(g_State.BaseAddress + 0x84286, 25); break;
        case FEARXP:  MemoryHelper::MakeNOP(g_State.BaseAddress + 0xB89E6, 25); break;
        case FEARXP2: MemoryHelper::MakeNOP(g_State.BaseAddress + 0xB9A36, 25); break;
    }
}

static void InitConsole(LPCSTR title)
{
    if (!ConsoleEnabled || !title) return;

    if (g_State.CurrentFEARGame == FEAR)
    {
        Console::Init(g_State.BaseAddress + 0x176FF0, g_State.hWnd, title, HighResolutionScaling, LogOutputToFile);

        ConsoleAddresses addresses = {};
        addresses.cursorLockAddr = g_State.BaseAddress + 0x16ABB4;
        addresses.cvarListHead = g_State.BaseAddress + 0x1773D4;
        addresses.cvarArrayStart = g_State.BaseAddress + 0x16D0F8;
        addresses.cvarArrayEnd = g_State.BaseAddress + 0x16DAB8;
        addresses.cmdArrayStart = g_State.BaseAddress + 0x16AC50;
        addresses.cmdArrayEnd = g_State.BaseAddress + 0x16B4D8;
        addresses.cvarVtableFloat = g_State.BaseAddress + 0x15E698;
        addresses.cvarVtableInt = g_State.BaseAddress + 0x15E6A0;
        Console::InitAddresses(addresses);

        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xF8670), &EndScene_Hook, (LPVOID*)&EndScene);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xF9350), &ResetDevice_Hook, (LPVOID*)&ResetDevice);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x7D9C0), &WindowProc_Hook, (LPVOID*)&WindowProc);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x16880), &ConsoleOutput_Hook, (LPVOID*)&ConsoleOutput);

        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x15D10), &SetCvarString_Hook, (LPVOID*)&SetCvarString);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x15D40), &SetCvarFloat_Hook, (LPVOID*)&SetCvarFloat);

        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xA220), &RegisterConsoleProgramClient_Hook, (LPVOID*)&RegisterConsoleProgramClient);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x5C780), &RegisterConsoleProgramServer_Hook, (LPVOID*)&RegisterConsoleProgramServer);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xA290), &UnregisterConsoleProgramClient_Hook, (LPVOID*)&UnregisterConsoleProgramClient);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x5C800), &UnregisterConsoleProgramServer_Hook, (LPVOID*)&UnregisterConsoleProgramServer);

        RunConsoleCommand = reinterpret_cast<decltype(RunConsoleCommand)>(g_State.BaseAddress + 0x9320);

        if (DebugLevel != 0)
        {
            MemoryHelper::WriteMemory<int>(g_State.BaseAddress + 0x16F6C4, DebugLevel, false);
        }
    }
    else if (g_State.CurrentFEARGame == FEARXP)
    {
        Console::Init(g_State.BaseAddress + 0x21BFD0, g_State.hWnd, title, HighResolutionScaling, LogOutputToFile);

        ConsoleAddresses addresses = {};
        addresses.cursorLockAddr = g_State.BaseAddress + 0x20EBEC;
        addresses.cvarListHead = g_State.BaseAddress + 0x21C3B4;
        addresses.cvarArrayStart = g_State.BaseAddress + 0x2126C8;
        addresses.cvarArrayEnd = g_State.BaseAddress + 0x213088;
        addresses.cmdArrayStart = g_State.BaseAddress + 0x20EC88;
        addresses.cmdArrayEnd = g_State.BaseAddress + 0x20F510;
        addresses.cvarVtableFloat = g_State.BaseAddress + 0x1FEBF8;
        addresses.cvarVtableInt = g_State.BaseAddress + 0x1FEC00;
        Console::InitAddresses(addresses);

        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x18EC30), &EndScene_Hook, (LPVOID*)&EndScene);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x18FAA0), &ResetDevice_Hook, (LPVOID*)&ResetDevice);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xAF650), &WindowProc_Hook, (LPVOID*)&WindowProc);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x21510), &ConsoleOutput_Hook, (LPVOID*)&ConsoleOutput);

        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x20640), &SetCvarString_Hook, (LPVOID*)&SetCvarString);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x20670), &SetCvarFloat_Hook, (LPVOID*)&SetCvarFloat);

        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x11020), &RegisterConsoleProgramClient_Hook, (LPVOID*)&RegisterConsoleProgramClient);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x80390), &RegisterConsoleProgramServer_Hook, (LPVOID*)&RegisterConsoleProgramServer);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x11090), &UnregisterConsoleProgramClient_Hook, (LPVOID*)&UnregisterConsoleProgramClient);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x80410), &UnregisterConsoleProgramServer_Hook, (LPVOID*)&UnregisterConsoleProgramServer);

        RunConsoleCommand = reinterpret_cast<decltype(RunConsoleCommand)>(g_State.BaseAddress + 0x100E0);

        if (DebugLevel != 0)
        {
            MemoryHelper::WriteMemory<int>(g_State.BaseAddress + 0x21370C, DebugLevel, false);
        }
    }
    else if (g_State.CurrentFEARGame == FEARXP2)
    {
        Console::Init(g_State.BaseAddress + 0x21E010, g_State.hWnd, title, HighResolutionScaling, LogOutputToFile);

        ConsoleAddresses addresses = {};
        addresses.cursorLockAddr = g_State.BaseAddress + 0x210BEC;
        addresses.cvarListHead = g_State.BaseAddress + 0x21E414;
        addresses.cvarArrayStart = g_State.BaseAddress + 0x2146D8;
        addresses.cvarArrayEnd = g_State.BaseAddress + 0x215098;
        addresses.cmdArrayStart = g_State.BaseAddress + 0x210C88;
        addresses.cmdArrayEnd = g_State.BaseAddress + 0x211510;
        addresses.cvarVtableFloat = g_State.BaseAddress + 0x200C48;
        addresses.cvarVtableInt = g_State.BaseAddress + 0x200C50;
        Console::InitAddresses(addresses);

        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x190230), &EndScene_Hook, (LPVOID*)&EndScene);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x1910D0), &ResetDevice_Hook, (LPVOID*)&ResetDevice);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0xB06B0), &WindowProc_Hook, (LPVOID*)&WindowProc);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x216C0), &ConsoleOutput_Hook, (LPVOID*)&ConsoleOutput);

        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x207F0), &SetCvarString_Hook, (LPVOID*)&SetCvarString);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x20820), &SetCvarFloat_Hook, (LPVOID*)&SetCvarFloat);

        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x112C0), &RegisterConsoleProgramClient_Hook, (LPVOID*)&RegisterConsoleProgramClient);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x810D0), &RegisterConsoleProgramServer_Hook, (LPVOID*)&RegisterConsoleProgramServer);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x11330), &UnregisterConsoleProgramClient_Hook, (LPVOID*)&UnregisterConsoleProgramClient);
        HookHelper::ApplyHook((void*)(g_State.BaseAddress + 0x81150), &UnregisterConsoleProgramServer_Hook, (LPVOID*)&UnregisterConsoleProgramServer);

        RunConsoleCommand = reinterpret_cast<decltype(RunConsoleCommand)>(g_State.BaseAddress + 0x10320);

        if (DebugLevel != 0)
        {
            MemoryHelper::WriteMemory<int>(g_State.BaseAddress + 0x21572C, DebugLevel, false);
        }
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
        LAAPatcher::PerformLAAPatch(GetModuleHandleA(NULL), CheckLAAPatch != 2);
    }

    // Load SDL
    if (SDLGamepadSupport)
    {
        SDLGamepadSupport = InitializeSDLGamepad();
    }

    // Get the handle of the client as soon as it is loaded
    ApplyClientHook();

    // Fixes
    ApplyFixDirectInputFps();
    ApplyFixHighFPSPhysics();
    ApplyOptimizeSaveSpeed();
    ApplyFixNvidiaShadowCorruption();
    ApplyFixKeyboardInputLanguage();
    ApplyFixScriptedAnimationCrash();
    ApplyFixWindow();

    // Display
    ApplyAutoResolution();

    // Graphics
    ApplyReducedMipMapBias();

    // Misc
    HookMainLoop();
    HookVSyncOverride();
    ApplySkipIntroHook();
    ApplyConsoleVariableHook();
    ApplySaveFolderRedirect();
    ApplyForceRenderMode();
    ApplyDisableJoystick();
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

bool OnProcessAttach(HMODULE hModule)
{
    // Prevents DLL from receiving thread notifications
    DisableThreadLibraryCalls(hModule);

    uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
    g_State.BaseAddress = base;
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

    if (SDLGamepadSupport)
    {
        ShutdownSDLGamepad();
    }
}

#pragma endregion