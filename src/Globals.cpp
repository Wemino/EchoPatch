#pragma once

#define NOMINMAX
#define MINI_CASE_SENSITIVE
#define _USE_MATH_DEFINES

#include <shlwapi.h>
#include <ShlObj_core.h>
#include <dxgi.h>

#include <Windows.h>
#include <d3d9.h>
#include <string_view>
#include <array>
#include <unordered_set>
#include <unordered_map>

#include "FpsLimiter.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

// Pointers to core engine functions other modules call
extern int(__stdcall* SetConsoleVariableFloat)(const char*, float);
extern HRESULT(WINAPI* D3D9_SetTexture)(IDirect3DDevice9*, DWORD, IDirect3DBaseTexture9*);

// ======================
// Constants
// ======================
const DWORD FEAR_TIMESTAMP = 0x44EF6AE6;
const DWORD FEARMP_TIMESTAMP = 0x44EF6ADB;
const DWORD FEARXP_TIMESTAMP = 0x450B3629;
const DWORD FEARXP_TIMESTAMP2 = 0x450DA808;
const DWORD FEARXP2_TIMESTAMP = 0x46FC10A3;

static constexpr float BASE_AREA = 1024.0f * 768.0f;
static constexpr float TARGET_FRAME_TIME = 1.0f / 60.0f;

enum FEARGAME
{
    FEAR,
    FEARMP,
    FEARXP,
    FEARXP2
};

// =============================
// Global State
// =============================
struct GlobalState
{
	// ======================
	// Game Identification
	// ======================
	uintptr_t BaseAddress = 0;
	FEARGAME CurrentFEARGame = FEAR;
	bool IsOriginalGame() const { return CurrentFEARGame == FEAR || CurrentFEARGame == FEARMP; }
	bool IsExpansion() const { return CurrentFEARGame == FEARXP || CurrentFEARGame == FEARXP2; }

	// ======================
	// Window/Display
	// ======================
	HWND hWnd = 0;
	uint64_t cachedVRAM = 0;
	int currentWidth = 0;
	int currentHeight = 0;
	int screenWidth = 0;
	int screenHeight = 0;
	bool useVsyncOverride = false;

	// ======================
	// HUD Scaling
	// ======================
	float scalingFactor = 0.0f;
	float scalingFactorText = 0.0f;
	float scalingFactorCrosshair = 0.0f;
	float crosshairSize = 0.0f;
	bool updateHUD = false;

	// ======================
	// HUD Elements
	// ======================
	int CHUDMgr = 0;
	int CHUDPaused = 0;
	int CHUDWeaponList = 0;
	int CHUDGrenadeList = 0;

	// ======================
	// Resolution Settings
	// ======================
	bool isInAutoDetect = false;

	// ======================
	// User Profile
	// ======================
	int currentKeyIndex = 0;
	bool isLoadingDefault = false;

	// ======================
	// Module Handles
	// ======================
	HMODULE GameClient = NULL;
	HMODULE GameServer = NULL;
	HMODULE GameClientFX = NULL;

	// ======================
	// HUD Update State
	// ======================
	int healthAdditionalIntIndex = 0;
	bool slowMoBarUpdated = false;

	// ======================
	// Game State
	// ======================
	int pGameClientShell = 0;
	bool isPlaying = false;
	bool isMsgBoxVisible = false;
	bool isEnteringWorld = false;
	bool isInCameraUpdateRenderTarget = false;

	// ======================
	// Controller State
	// ======================
	int pUseCursor = 0;
	int pCurrentType = 0;
	int currentType = 0;
	int maxCurrentType = 0;
	int screenPerformanceCPU = 0;
	int screenPerformanceGPU = 0;
	bool canActivate = false;
	bool canSwap = false;
	bool isAllowedToUseCursor = false;
	bool shouldLockCursorToCenter = false;
	bool isUsingRemoteDetonator = false;
	bool isOperatingTurret = false;
	bool isBuildingCScreenJoystick = false;

	// ======================
	// Server State
	// ======================
	int CPlayerInventory = 0;
	bool appliedCustomMaxWeaponCapacity = false;

	// ======================
	// Weapon/Animation
	// ======================
	BYTE* pAimMgr = 0;
	int kAP_ACT_Fire_Id = 0;

	// ======================
	// Physics/Velocity
	// ======================
	double simulationFrameTime = 0.0;
	double totalGameTime = 0.0;
	bool pendingVelocityFix = false;

	// ======================
	// SlowMo Fix
	// ======================
	double clientSlowMoCharge = 0.0;

	// ======================
	// Console
	// ======================
	bool isConsoleOpen = false;
	bool wasInputDisabled = false;

	// ======================
	// MipMapBias Override
	// ======================
	std::vector<void*> sharpTextures;
	bool stageIsDirty[16] = { false };

	// ======================
	// HUD Data
	// ======================
	struct DataEntry
	{
		int TextSize;
		int ScaleType;
	};

	struct HudScalingRule
	{
		std::unordered_set<std::string_view> attributes;
		float* scalingFactorPtr;
	};

	static inline std::unordered_map<std::string_view, DataEntry> textDataMap;
	static inline std::unordered_map<std::string_view, HudScalingRule> hudScalingRules;
};

GlobalState g_State;

// =============================
// Ini Variables
// =============================

// Fixes
bool DisableRedundantHIDInit = false;
bool HighFPSFixes = false;
bool HavokPhysicsFix = false;
bool OptimizeSaveSpeed = false;
bool FixNvidiaShadowCorruption = false;
bool FixAspectRatioBlur = false;
bool FastVRAMDetection = false;
bool DisableXPWidescreenFiltering = false;
bool FixKeyboardInputLanguage = false;
bool WeaponFixes = false;
bool FixSoundWrapperLoading = false;
bool FixScriptedAnimationCrash = false;
int CheckLAAPatch = 0;

// Graphics
float MaxFPS = 0;
bool DynamicVsync = false;
bool HighResolutionReflections = false;
bool NoLODBias = false;
float SSAAScale = 0;
bool ReducedMipMapBias = false;
bool EnablePersistentWorldState = false;

// Display
float CustomFOV = 0;
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
bool RumbleEnabled = false;
bool GyroEnabled = false;
int GyroAimingMode = 0;
float GyroSensitivity = 0.0f;
float GyroSmoothing = 0.0f;
bool GyroCalibrationPersistence = false;
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
int GAMEPAD_SOUTH_HOLD = -1;
int GAMEPAD_SOUTH_HOLD_TIME = 500;
int GAMEPAD_EAST = 0;
int GAMEPAD_EAST_HOLD = -1;
int GAMEPAD_EAST_HOLD_TIME = 500;
int GAMEPAD_WEST = 0;
int GAMEPAD_WEST_HOLD = -1;
int GAMEPAD_WEST_HOLD_TIME = 500;
int GAMEPAD_NORTH = 0;
int GAMEPAD_NORTH_HOLD = -1;
int GAMEPAD_NORTH_HOLD_TIME = 500;
int GAMEPAD_LEFT_STICK = 0;
int GAMEPAD_LEFT_STICK_HOLD = -1;
int GAMEPAD_LEFT_STICK_HOLD_TIME = 500;
int GAMEPAD_RIGHT_STICK = 0;
int GAMEPAD_RIGHT_STICK_HOLD = -1;
int GAMEPAD_RIGHT_STICK_HOLD_TIME = 500;
int GAMEPAD_LEFT_SHOULDER = 0;
int GAMEPAD_LEFT_SHOULDER_HOLD = -1;
int GAMEPAD_LEFT_SHOULDER_HOLD_TIME = 500;
int GAMEPAD_RIGHT_SHOULDER = 0;
int GAMEPAD_RIGHT_SHOULDER_HOLD = -1;
int GAMEPAD_RIGHT_SHOULDER_HOLD_TIME = 500;
int GAMEPAD_DPAD_UP = 0;
int GAMEPAD_DPAD_UP_HOLD = -1;
int GAMEPAD_DPAD_UP_HOLD_TIME = 500;
int GAMEPAD_DPAD_DOWN = 0;
int GAMEPAD_DPAD_DOWN_HOLD = -1;
int GAMEPAD_DPAD_DOWN_HOLD_TIME = 500;
int GAMEPAD_DPAD_LEFT = 0;
int GAMEPAD_DPAD_LEFT_HOLD = -1;
int GAMEPAD_DPAD_LEFT_HOLD_TIME = 500;
int GAMEPAD_DPAD_RIGHT = 0;
int GAMEPAD_DPAD_RIGHT_HOLD = -1;
int GAMEPAD_DPAD_RIGHT_HOLD_TIME = 500;
int GAMEPAD_LEFT_TRIGGER = 0;
int GAMEPAD_LEFT_TRIGGER_HOLD = -1;
int GAMEPAD_LEFT_TRIGGER_HOLD_TIME = 500;
int GAMEPAD_RIGHT_TRIGGER = 0;
int GAMEPAD_RIGHT_TRIGGER_HOLD = -1;
int GAMEPAD_RIGHT_TRIGGER_HOLD_TIME = 500;
int GAMEPAD_BACK = 0;
int GAMEPAD_BACK_HOLD = -1;
int GAMEPAD_BACK_HOLD_TIME = 500;
int GAMEPAD_START = 0;
int GAMEPAD_START_HOLD = -1;
int GAMEPAD_START_HOLD_TIME = 500;
int GAMEPAD_MISC1 = 0;
int GAMEPAD_MISC1_HOLD = -1;
int GAMEPAD_MISC1_HOLD_TIME = 500;
int GAMEPAD_RIGHT_PADDLE1 = 0;
int GAMEPAD_RIGHT_PADDLE1_HOLD = -1;
int GAMEPAD_RIGHT_PADDLE1_HOLD_TIME = 500;
int GAMEPAD_LEFT_PADDLE1 = 0;
int GAMEPAD_LEFT_PADDLE1_HOLD = -1;
int GAMEPAD_LEFT_PADDLE1_HOLD_TIME = 500;
int GAMEPAD_RIGHT_PADDLE2 = 0;
int GAMEPAD_RIGHT_PADDLE2_HOLD = -1;
int GAMEPAD_RIGHT_PADDLE2_HOLD_TIME = 500;
int GAMEPAD_LEFT_PADDLE2 = 0;
int GAMEPAD_LEFT_PADDLE2_HOLD = -1;
int GAMEPAD_LEFT_PADDLE2_HOLD_TIME = 500;

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
bool EnableCrashHandler = false;
bool ShowErrors = false;

// Accessor for signature scanning
DWORD ScanModuleSignature(HMODULE Module, std::string_view Signature, const char* PatchName, int FunctionStartCheckCount = -1, bool ShowError = true);

// Core entry points used by dllmain
bool OnProcessAttach(HMODULE hModule);
void OnProcessDetach();

// imgui
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include "helper.cpp"

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
