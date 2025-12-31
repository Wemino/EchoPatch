#pragma once

#define MINI_CASE_SENSITIVE
#define _USE_MATH_DEFINES

#include <Windows.h>
#include <string_view>
#include <string>
#include <array>
#include <unordered_set>
#include <unordered_map>

#include "FpsLimiter.hpp"
#include "LAAPatcher.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

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
	uintptr_t BaseAddress = 0;
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
	int screenPerformanceCPU = 0;
	int screenPerformanceGPU = 0;
	bool updateGyroCamera = false;
	bool isAiming = false;

	// ======================
	// Server State
	// ======================
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
	bool inFriction = false;
	bool previousJumpState = false;
	double currentFrameTime = 0.0;
	double totalGameTime = 0.0;
	double jumpElapsedTime = -1.0;
	float waveUpdateAccumulator = 0.0f;
	bool useVelocitySmoothing = false;
	double velocityAccumulator = 0.0;
	double velocityTimeAccumulator = 0.0;
	double lastReportedVelocity = 0.0;
	double prevWindowSpeed = 0.0;

	// ======================
	// Ragdoll Physics Fix
	// ======================
	struct RagdollEntry
	{
		int owner = 0;
		double firstSeenTime = 0.0;
	};

	bool isProcessingRagdoll = false;
	static inline std::array<RagdollEntry, 32> ragdollCache{};
	static constexpr double RAGDOLL_STABILIZE_TIME = 5.0;

	// ======================
	// Console
	// ======================
	bool isConsoleOpen = false;
	bool wasConsoleOpened = false;
	bool wasInputDisabled = false;

	// ======================
	// PolyGrid Timing
	// ======================
	struct SplashEntry 
	{ 
		uint64_t key;
		double lastTime;
	};

	inline static std::array<SplashEntry, 64> splashCache{};
	inline static size_t splashIndex = 0;

	// ======================
	// Save Optimization
	// ======================
	struct SaveBuffer
	{
		HANDLE handle = INVALID_HANDLE_VALUE;
		std::vector<uint8_t> buffer{};
		LONGLONG position = 0;
		LONGLONG size = 0;
		bool flushed = false;

		void Reset()
		{
			handle = INVALID_HANDLE_VALUE;
			buffer.clear();
			position = 0;
			size = 0;
			flushed = false;
		}

		bool IsActive() const { return handle != INVALID_HANDLE_VALUE; }
	};

	static inline SaveBuffer saveBuffer{};

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
extern int CheckLAAPatch;

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
extern bool SDLGamepadSupport;
extern bool GyroEnabled;
extern int GyroAimingMode;
extern float GyroSensitivity;
extern float GyroSmoothing;
extern bool TouchpadEnabled;
extern bool HideMouseCursor;
extern float GPadAimSensitivity;
extern float GPadAimEdgeThreshold;
extern float GPadAimEdgeAccelTime;
extern float GPadAimEdgeDelayTime;
extern float GPadAimEdgeMultiplier;
extern float GPadAimAspectRatio;
extern float GPadZoomMagThreshold;
extern int GAMEPAD_SOUTH, GAMEPAD_EAST, GAMEPAD_WEST, GAMEPAD_NORTH;
extern int GAMEPAD_LEFT_STICK, GAMEPAD_RIGHT_STICK;
extern int GAMEPAD_LEFT_SHOULDER, GAMEPAD_RIGHT_SHOULDER;
extern int GAMEPAD_DPAD_UP, GAMEPAD_DPAD_DOWN, GAMEPAD_DPAD_LEFT, GAMEPAD_DPAD_RIGHT;
extern int GAMEPAD_LEFT_TRIGGER, GAMEPAD_RIGHT_TRIGGER;
extern int GAMEPAD_BACK, GAMEPAD_START;
extern int GAMEPAD_MISC1, GAMEPAD_RIGHT_PADDLE1, GAMEPAD_LEFT_PADDLE1, GAMEPAD_RIGHT_PADDLE2, GAMEPAD_LEFT_PADDLE2;

// SkipIntro
extern bool SkipSplashScreen;
extern bool SkipAllIntro;
extern bool SkipSierraIntro;
extern bool SkipMonolithIntro;
extern bool SkipWBGamesIntro;
extern bool SkipNvidiaIntro;
extern bool SkipTimegateIntro;
extern bool SkipDellIntro;

// Console
extern bool ConsoleEnabled;
extern int DebugLevel;
extern bool HighResolutionScaling;
extern bool LogOutputToFile;

// Extra
extern bool RedirectSaveFolder;
extern bool InfiniteFlashlight;
extern bool DisablePunkBuster;
extern bool EnableCustomMaxWeaponCapacity;
extern int MaxWeaponCapacity;
extern bool DisableHipFireAccuracyPenalty;
extern bool ShowErrors;

// Accessor for signature scanning
DWORD ScanModuleSignature(HMODULE Module, std::string_view Signature, const char* PatchName, int FunctionStartCheckCount = -1, bool ShowError = true);

// Core entry points used by dllmain
bool OnProcessAttach(HMODULE hModule);
void OnProcessDetach();

// imgui
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);