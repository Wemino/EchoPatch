#pragma once

#define MINI_CASE_SENSITIVE
#define _USE_MATH_DEFINES

#include <Windows.h>
#include <Xinput.h>
#include <string_view>
#include <string>
#include <unordered_map>

#include "../FpsLimiter.hpp"

// Pointers to core engine functions other modules call
extern int(__stdcall* SetConsoleVariableFloat)(const char*, float);

// ======================
// Constants
// ======================
extern const DWORD FEAR_TIMESTAMP;
extern const DWORD FEARMP_TIMESTAMP;
extern const DWORD FEARXP_TIMESTAMP;
extern const DWORD FEARXP_TIMESTAMP2;
extern const DWORD FEARXP2_TIMESTAMP;

static constexpr float BASE_AREA = 1024.0f * 768.0f;
static constexpr float TARGET_FRAME_TIME = 1.0f / 60.0f;

static constexpr auto XINPUT_GAMEPAD_LEFT_TRIGGER = 0x400;
static constexpr auto XINPUT_GAMEPAD_RIGHT_TRIGGER = 0x800;

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
	FEARGAME CurrentFEARGame = FEAR;
	bool IsOriginalGame() const { return CurrentFEARGame == FEAR || CurrentFEARGame == FEARMP; }
	bool IsExpansion() const { return CurrentFEARGame == FEARXP || CurrentFEARGame == FEARXP2; }

	// ======================
	// Window/Display
	// ======================
	HWND hWnd = 0;
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
	bool crosshairSliderUpdated = false;

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
	bool isSettingOption = false;

	// ======================
	// User Profile
	// ======================
	bool isLoadingDefault = false;
	int currentKeyIndex = 0;

	// ======================
	// Input Settings
	// ======================
	float overrideSensitivity = 0.25f;

	// ======================
	// Module Handles
	// ======================
	bool isInit = false;
	bool isClientLoaded = false;
	HMODULE GameClient = NULL;
	HMODULE GameServer = NULL;
	HMODULE GameClientFX = NULL;

	// ======================
	// FPS Limiter
	// ======================
	bool isUsingFpsLimiter = false;
	FpsLimiter fpsLimiter{ 240.0f };

	// ======================
	// HUD Update State
	// ======================
	bool updateLayoutReturnValue = false;
	bool slowMoBarUpdated = false;
	int healthAdditionalIntIndex = 0;
	int int32ToUpdate = 0;
	float floatToUpdate = 0.0f;

	// ======================
	// Game State
	// ======================
	int g_pGameClientShell = 0;
	bool isPlaying = false;
	bool isMsgBoxVisible = false;
	bool isEnteringWorld = false;

	// ======================
	// Controller State
	// ======================
	bool canActivate = false;
	bool canSwap = false;
	bool isOperatingTurret = false;
	double zoomMag = 0;
	int pCurrentType = 0;
	int currentType = 0;
	int maxCurrentType = 0;
	bool hookedLoadString = false;
	int screenPerformanceCPU = 0;
	int screenPerformanceGPU = 0;

	// ======================
	// Server State
	// ======================
	bool needServerTermHooking = false;
	int CPlayerInventory = 0;
	bool appliedCustomMaxWeaponCapacity = false;
	std::list<DWORD> hookedServerFunctionAddresses;

	// ======================
	// Weapon/Animation
	// ======================
	BYTE* pAimMgr = 0;
	int kAP_ACT_Fire_Id = 0;
	int actionAnimationThreshold = 0;
	bool fireAnimationInterceptionDisabled = false;
	int pUpperAnimationContext = 0;
	bool requestNextWeapon = false;
	bool requestPreviousWeapon = false;

	// ======================
	// Physics/Velocity
	// ======================
	bool useVelocitySmoothing = false;
	LONGLONG lastVelocityTime = 0;
	double smoothedVelocity = 0.0;
	bool inFriction = false;
	int remainingJumpFrames = 0;
	bool previousJumpState = false;
	float waveUpdateAccumulator = 0.0f;

	// ======================
	// PolyGrid Timing
	// ======================
	std::unordered_map<uint64_t, double> polyGridLastSplashTime;

	// ======================
	// Save Optimization
	// ======================
	struct SaveBuffer
	{
		std::vector<uint8_t> buffer{};
		LONGLONG position = 0;
		bool flushed = false;
	};

	static inline std::unordered_map<HANDLE, SaveBuffer> saveBuffers;

	// ======================
	// Static Data
	// ======================
	struct DataEntry
	{
		int TextSize;
		int ScaleType;
	};

	struct HudScalingRule
	{
		std::vector<std::string_view> attributes;
		float* scalingFactorPtr;
	};

	static inline std::unordered_map<std::string_view, DataEntry> textDataMap;
	static inline std::unordered_map<std::string_view, HudScalingRule> hudScalingRules;
};

extern GlobalState g_State;

// =============================
// Ini Variables (extern)
// =============================

// Fixes
extern bool DisableRedundantHIDInit;
extern bool HighFPSFixes;
extern bool OptimizeSaveSpeed;
extern bool FixNvidiaShadowCorruption;
extern bool DisableXPWidescreenFiltering;
extern bool FixKeyboardInputLanguage;
extern bool WeaponFixes;
extern bool FixScriptedAnimationCrash;
extern bool CheckLAAPatch;

// Graphics
extern float MaxFPS;
extern bool DynamicVsync;
extern bool HighResolutionReflections;
extern bool NoLODBias;
extern bool ReducedMipMapBias;
extern bool EnablePersistentWorldState;

// Display
extern bool HUDScaling;
extern float HUDCustomScalingFactor;
extern float SmallTextCustomScalingFactor;
extern int AutoResolution;
extern bool DisableLetterbox;
extern bool ForceWindowed;
extern bool FixWindowStyle;

// Controller
extern float MouseAimMultiplier;
extern bool XInputControllerSupport;
extern bool HideMouseCursor;
extern float GPadAimSensitivity;
extern float GPadAimEdgeThreshold;
extern float GPadAimEdgeAccelTime;
extern float GPadAimEdgeDelayTime;
extern float GPadAimEdgeMultiplier;
extern float GPadAimAspectRatio;
extern float GPadZoomMagThreshold;
extern int GAMEPAD_A, GAMEPAD_B, GAMEPAD_X, GAMEPAD_Y;
extern int GAMEPAD_LEFT_THUMB, GAMEPAD_RIGHT_THUMB;
extern int GAMEPAD_LEFT_SHOULDER, GAMEPAD_RIGHT_SHOULDER;
extern int GAMEPAD_DPAD_UP, GAMEPAD_DPAD_DOWN, GAMEPAD_DPAD_LEFT, GAMEPAD_DPAD_RIGHT;
extern int GAMEPAD_LEFT_TRIGGER, GAMEPAD_RIGHT_TRIGGER, GAMEPAD_BACK;

// SkipIntro
extern bool SkipSplashScreen;
extern bool SkipAllIntro;
extern bool SkipSierraIntro;
extern bool SkipMonolithIntro;
extern bool SkipWBGamesIntro;
extern bool SkipNvidiaIntro;
extern bool SkipTimegateIntro;
extern bool SkipDellIntro;

// Extra
extern bool RedirectSaveFolder;
extern bool InfiniteFlashlight;
extern bool EnableCustomMaxWeaponCapacity;
extern int MaxWeaponCapacity;
extern bool DisableHipFireAccuracyPenalty;
extern bool ShowErrors;

// Accessor for signature scanning
DWORD ScanModuleSignature(HMODULE Module, std::string_view Signature, const char* PatchName, int FunctionStartCheckCount = -1, bool ShowError = true);

// Core entry points used by dllmain
bool OnProcessAttach(HMODULE hModule);
void OnProcessDetach();