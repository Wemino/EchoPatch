#define MINI_CASE_SENSITIVE
#define NOMINMAX
#define _USE_MATH_DEFINES

#include <Windows.h>
#include <Xinput.h>
#include <unordered_set>
#include <map>
#include <array>

#include "MinHook.hpp"
#include "ini.hpp"
#include "dllmain.hpp"
#include "helper.hpp"
#include "FpsLimiter.hpp"

#pragma comment(lib, "libMinHook.x86.lib")
#pragma comment(lib, "Xinput.lib")

// ======================
// Core Functions
// ======================

// General
intptr_t(__cdecl* LoadGameDLL)(char*, char, DWORD*) = nullptr;
int(__stdcall* SetConsoleVariableFloat)(const char*, float) = nullptr;

// FixKeyboardInputLanguage
int(__thiscall* GetDeviceObjectDesc)(int, unsigned int, wchar_t*, unsigned int*) = nullptr;

// HighFPSFixes
void(__cdecl* ProcessTwistLimitConstraint)(int, float*, float*) = nullptr;
void(__cdecl* ProcessConeLimitConstraint)(int, float*, float*, float*) = nullptr;
void(__cdecl* BuildJacobianRow) (int, float*, float*) = nullptr;

// DynamicVsync
int(__thiscall* InitializePresentationParameters)(DWORD*, DWORD*, unsigned __int8) = nullptr;

// MaxFPS & XInputControllerSupport
int(__thiscall* MainGameLoop)(int) = nullptr;

// AutoResolution
int(__cdecl* SetRenderMode)(int) = nullptr;

// SkipIntro
int(__thiscall* FindStringCaseInsensitive)(DWORD*, char*) = nullptr;

// EnableCustomMaxWeaponCapacity
int(__thiscall* TerminateServer)(int) = nullptr;


// ======================
// Client Functions
// ======================

// HighFPSFixes
bool(__thiscall* LoadFxDll)(int, char*, char) = nullptr;
void(__thiscall* UpdateOnGround)(int) = nullptr;
void(__thiscall* UpdateWaveProp)(int, float) = nullptr;
double(__thiscall* GetMaxRecentVelocityMag)(int) = nullptr;
void(__cdecl* PolyGridFXCollisionHandlerCB)(int, int, int*, int*, float, BYTE*, int) = nullptr;
void(__thiscall* UpdateNormalControlFlags)(int) = nullptr;
void(__thiscall* UpdateNormalFriction)(int) = nullptr;
double(__thiscall* GetTimerElapsedS)(int) = nullptr;

// FixKeyboardInputLanguage
void(__thiscall* LoadUserProfile)(int, bool, bool) = nullptr;
bool(__thiscall* RestoreDefaults)(int, uint8_t) = nullptr;

// WeaponFixes
BYTE* (__thiscall* AimMgrCtor)(BYTE*) = nullptr;
void(__thiscall* AnimationClearLock)(DWORD) = nullptr;
void(__thiscall* UpdateWeaponModel)(DWORD*) = nullptr;
void(__thiscall* SetAnimProp)(DWORD*, int, int) = nullptr;
bool(__thiscall* InitAnimations)(DWORD*) = nullptr;
void(__thiscall* NextWeapon)(DWORD*) = nullptr;
void(__thiscall* PreviousWeapon)(DWORD*) = nullptr;
unsigned __int8(__thiscall* GetWeaponSlot)(int, int) = nullptr;

// HighResolutionReflections
bool(__thiscall* RenderTargetGroupFXInit)(int, DWORD*) = nullptr;

// EnablePersistentWorldState
float(__stdcall* GetShatterLifetime)(int) = nullptr;
int(__stdcall* CreateFX)(char*, int, int) = nullptr;

// HUDScaling
void(__thiscall* HUDTerminate)(int) = nullptr;
bool(__thiscall* HUDInit)(int) = nullptr;
void(__thiscall* HUDRender)(int, int) = nullptr;
int(__thiscall* HUDWeaponListReset)(int) = nullptr;
bool(__thiscall* HUDWeaponListInit)(int) = nullptr;
bool(__thiscall* HUDGrenadeListInit)(int) = nullptr;
int(__thiscall* ScreenDimsChanged)(int) = nullptr;
DWORD* (__stdcall* LayoutDBGetPosition)(DWORD*, int, char*, int) = nullptr;
float* (__stdcall* GetRectF)(DWORD*, int, char*, int) = nullptr;
int(__stdcall* DBGetRecord)(int, char*) = nullptr;
int(__stdcall* DBGetInt32)(int, unsigned int, int) = nullptr;
float(__stdcall* DBGetFloat)(int, unsigned int, float) = nullptr;
const char* (__stdcall* DBGetString)(int, unsigned int, int) = nullptr;
int(__thiscall* UpdateSlider)(int, int) = nullptr;
void(__stdcall* InitAdditionalTextureData)(int, int, int*, DWORD*, DWORD*, float) = nullptr;
void(__thiscall* HUDPausedInit)(int) = nullptr;

// AutoResolution
void(__cdecl* AutoDetectPerformanceSettings)() = nullptr;
void(__thiscall* SetOption)(int, int, int, int, int) = nullptr;
bool(__thiscall* SetQueuedConsoleVariable)(int, const char*, float, int) = nullptr;

// XInputControllerSupport
bool(__thiscall* IsCommandOn)(int, int) = nullptr;
bool(__thiscall* OnCommandOn)(int, int) = nullptr;
bool(__thiscall* OnCommandOff)(int, int) = nullptr;
double(__thiscall* GetExtremalCommandValue)(int, int) = nullptr;
double(__thiscall* GetZoomMag)(int) = nullptr;
int(__thiscall* HUDActivateObjectSetObject)(int, void**, int, int, int, int) = nullptr;
int(__thiscall* SetOperatingTurret)(int, int) = nullptr;
const wchar_t* (__thiscall* GetTriggerNameFromCommandID)(int, int) = nullptr;
bool(__thiscall* ChangeState)(int, int, int) = nullptr;
void(__thiscall* UseCursor)(int, bool) = nullptr;
bool(__thiscall* OnMouseMove)(int, int, int) = nullptr;
void(__thiscall* HUDSwapUpdate)(int) = nullptr;
void(__thiscall* SwitchToScreen)(int, int) = nullptr;
void(__thiscall* SetCurrentType)(int, int) = nullptr;
void(__cdecl* HUDSwapUpdateTriggerName)() = nullptr;
const wchar_t* (__stdcall* LoadGameString)(int, char*) = nullptr;

// // EnableCustomMaxWeaponCapacity
uint8_t(__thiscall* GetWeaponCapacity)(int) = nullptr;

// DisableHipFireAccuracyPenalty
void(__thiscall* AccuracyMgrUpdate)(float*) = nullptr;

// XInputControllerSupport & HUDScaling
int(__thiscall* HUDWeaponListUpdateTriggerNames)(int) = nullptr;
int(__thiscall* HUDGrenadeListUpdateTriggerNames)(int) = nullptr;

// EnableCustomMaxWeaponCapacity & WeaponFixes
void(__thiscall* OnEnterWorld)(int) = nullptr;


// ======================
// Server Functions
// ======================

// EnableCustomMaxWeaponCapacity
void(__thiscall* SetWeaponCapacityServer)(int, uint8_t) = nullptr;
void(__thiscall* PlayerInventoryInit)(int, int) = nullptr;


// ======================
// ClientFX Functions
// ======================

// HighFPSFixes
float* (__thiscall* AddParticleBatchMarker)(int, int, bool) = nullptr;
float* (__thiscall* EmitParticleBatch)(int, float, int, int*) = nullptr;

// ======================
// WinAPI Functions
// ======================
HWND(WINAPI* ori_CreateWindowExA)(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
HRESULT(WINAPI* ori_SHGetFolderPathA)(HWND, int, HANDLE, DWORD, LPSTR);

// ======================
// Constants 
// ======================
constexpr DWORD FEAR_TIMESTAMP = 0x44EF6AE6;
constexpr DWORD FEARMP_TIMESTAMP = 0x44EF6ADB;
constexpr DWORD FEARXP_TIMESTAMP = 0x450B3629;
constexpr DWORD FEARXP_TIMESTAMP2 = 0x450DA808;
constexpr DWORD FEARXP2_TIMESTAMP = 0x46FC10A3;

constexpr float BASE_AREA = 1024.0f * 768.0f;
constexpr float TARGET_FRAME_TIME = 1.0f / 60.0f;

constexpr auto XINPUT_GAMEPAD_LEFT_TRIGGER = 0x400;
constexpr auto XINPUT_GAMEPAD_RIGHT_TRIGGER = 0x800;

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
	FEARGAME CurrentFEARGame;
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

	// ======================
	// Input Settings
	// ======================
	float overrideSensitivity = 0.25f;

	// ======================
	// Module Handles
	// ======================
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
	bool isPlaying = true;
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
	std::chrono::high_resolution_clock::time_point lastVelocityTime;
	bool velocityTimeInitialized = false;
	double smoothedVelocity = 0.0;
	bool velocitySmoothingInitialized = false;
	bool inFriction = false;
	int remainingJumpFrames = 0;
	bool previousJumpState = false;
	float waveUpdateAccumulator = 0.0f;

	// ======================
	// PolyGrid Timing
	// ======================
	LARGE_INTEGER polyGridSplashFreq = {};
	std::unordered_map<uint64_t, double> polyGridLastSplashTime;

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

// Global instance
GlobalState g_State;

// =============================
// Controller State
// =============================
struct ControllerState 
{
	XINPUT_STATE state;
	bool isConnected = false;

	// Command activation states
	bool commandActive[117] = { false };

	// Button state tracking
	struct ButtonState 
	{
		bool isPressed = false;
		bool wasHandled = false;
		ULONGLONG pressStartTime = 0;
		ULONGLONG lastRepeatTime = 0;
	};

	// Menu navigation states
	ButtonState menuButtons[6];
	ULONGLONG menuToGameTransitionTime = 0;

	// Game button states
	std::map<WORD, ButtonState> gameButtons;

	// ScreenPerformanceAdvanced
	ButtonState leftShoulderState;
	ButtonState rightShoulderState;
};

ControllerState g_Controller;

std::pair<WORD, int> g_buttonMappings[] =
{
	{ XINPUT_GAMEPAD_A, 15 }, // Jump
	{ XINPUT_GAMEPAD_B, 14 }, // Crouch
	{ XINPUT_GAMEPAD_X, 88 }, // Reload/Activate
	{ XINPUT_GAMEPAD_Y, 106 }, // SlowMo
	{ XINPUT_GAMEPAD_LEFT_THUMB, 70 }, // Medkit
	{ XINPUT_GAMEPAD_RIGHT_THUMB, 114 }, // Flashlight
	{ XINPUT_GAMEPAD_LEFT_SHOULDER, 81 }, // Throw Grenade
	{ XINPUT_GAMEPAD_RIGHT_SHOULDER, 19 }, // Melee
	{ XINPUT_GAMEPAD_DPAD_UP, 77 }, // Next Weapon
	{ XINPUT_GAMEPAD_DPAD_DOWN, 73 }, // Next Grenade
	{ XINPUT_GAMEPAD_DPAD_LEFT, 20 }, // Lean Left
	{ XINPUT_GAMEPAD_DPAD_RIGHT, 21 }, // Lean Right
	{ XINPUT_GAMEPAD_LEFT_TRIGGER, 71 }, // Aim
	{ XINPUT_GAMEPAD_RIGHT_TRIGGER, 17 }, // Fire
	{ XINPUT_GAMEPAD_BACK, 78 }, // Mission Status
	{ XINPUT_GAMEPAD_START, -1 }, // Menu
};

constexpr int MENU_NAVIGATION_MAP[][2] =
{
	{XINPUT_GAMEPAD_DPAD_UP,    VK_UP},
	{XINPUT_GAMEPAD_DPAD_DOWN,  VK_DOWN},
	{XINPUT_GAMEPAD_DPAD_LEFT,  VK_LEFT},
	{XINPUT_GAMEPAD_DPAD_RIGHT, VK_RIGHT},
	{XINPUT_GAMEPAD_A,          VK_RETURN},
	{XINPUT_GAMEPAD_B,          VK_ESCAPE}
};

// =============================
// Ini Variables
// =============================

// Fixes
bool DisableRedundantHIDInit = false;
bool HighFPSFixes = false;
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
		g_State.textDataMap["HUDEditorPosition"] = { 12, 0 };
		g_State.textDataMap["HudEndRoundMessage"] = { 32, 0 };
		g_State.textDataMap["HUDFocus"] = { 16, 0 };
		g_State.textDataMap["HUDGameMessage"] = { 14, 2 };
		g_State.textDataMap["HUDGear"] = { 14, 0 };
		g_State.textDataMap["HUDGrenade"] = { 14, 0 };
		g_State.textDataMap["HUDGrenadeList"] = { 14, 0 };
		g_State.textDataMap["HUDHealth"] = { 25, 0 };
		g_State.textDataMap["HUDLadder"] = { 18, 0 };
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
		g_State.textDataMap["HUDTurret"] = { 18, 0 };
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

static DWORD ScanModuleSignature(HMODULE Module, std::string_view Signature, const char* PatchName, int FunctionStartCheckCount = -1, bool ShowError = true)
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

#pragma region ClientFX Patches

static void ApplyHighFPSFixesClientFXPatch()
{
	if (!HighFPSFixes) return;

	DWORD targetMemoryLocation_ParticleUpdateThreshold = ScanModuleSignature(g_State.GameClientFX, "D9 44 24 04 ?? D8 1D", "FixParticleUpdateThreshold");
	DWORD targetMemoryLocation_AddParticleBatchMarker = ScanModuleSignature(g_State.GameClientFX, "C7 40 18 00 00 80 BF C2 08 00", "FixParticleLifetime_AddParticleBatchMarker");
	DWORD targetMemoryLocation_EmitParticleBatch = ScanModuleSignature(g_State.GameClientFX, "C7 40 18 00 00 80 BF ?? ?? ?? 81 C4 C0", "FixParticleLifetime_EmitParticleBatch");

	if (targetMemoryLocation_ParticleUpdateThreshold == 0 ||
		targetMemoryLocation_AddParticleBatchMarker == 0 ||
		targetMemoryLocation_EmitParticleBatch == 0) {
		return;
	}

	// Disable frametime check for FX updates
	MemoryHelper::MakeNOP(targetMemoryLocation_ParticleUpdateThreshold, 4);
	MemoryHelper::MakeNOP(targetMemoryLocation_ParticleUpdateThreshold + 0x5, 6);
	MemoryHelper::MakeNOP(targetMemoryLocation_ParticleUpdateThreshold + 0xD, 7);

	// m_fLifetime: -1 -> 0 
	// Prevent an issue inside GetParticleSizeAndColor
	MemoryHelper::WriteMemory<float>(targetMemoryLocation_AddParticleBatchMarker + 0x3, 0.0f);
	MemoryHelper::WriteMemory<float>(targetMemoryLocation_EmitParticleBatch + 0x3, 0.0f);
}

static void ApplyClientFXPatch()
{
	ApplyHighFPSFixesClientFXPatch();
}

#pragma endregion

#pragma region Client Hooks

// ======================
// HighFPSFixes
// ======================

static bool __fastcall LoadFxDll_Hook(int thisPtr, int, char* Source, char a3)
{
	// Load the DLL
	char result = LoadFxDll(thisPtr, Source, a3);

	// Get the path
	char* clientFXPath = ((char*)thisPtr + 0x24);

	// Get the handle
	wchar_t wFileName[MAX_PATH];
	MultiByteToWideChar(CP_UTF8, 0, clientFXPath, -1, wFileName, MAX_PATH);
	HMODULE clientFxDll = GetModuleHandleW(wFileName);

	if (clientFxDll)
	{
		g_State.GameClientFX = clientFxDll;
		ApplyClientFXPatch();
	}

	return result;
}

static void __fastcall UpdateOnGround_Hook(int thisPtr, int)
{
	bool* pJumped = reinterpret_cast<bool*>(thisPtr + 0x78);

	// Detect jump start
	if (!g_State.previousJumpState && *pJumped)
	{
		g_State.remainingJumpFrames = 3;
	}

	UpdateOnGround(thisPtr);

	// Maintain jump state for 3 frames
	if (g_State.remainingJumpFrames > 0)
	{
		*pJumped = true;
		g_State.remainingJumpFrames--;
	}

	// Update tracking state
	g_State.previousJumpState = *pJumped;

	// Reset counter if not jumping
	if (!g_State.previousJumpState)
	{
		g_State.remainingJumpFrames = 0;
	}
}

static void __fastcall UpdateWaveProp_Hook(int thisPtr, int, float frameDelta)
{
	// Updates water wave propagation at fixed time intervals for consistent simulation
	g_State.waveUpdateAccumulator += frameDelta;

	while (g_State.waveUpdateAccumulator >= TARGET_FRAME_TIME)
	{
		UpdateWaveProp(thisPtr, TARGET_FRAME_TIME);
		g_State.waveUpdateAccumulator -= TARGET_FRAME_TIME;
	}
}

static double __fastcall GetMaxRecentVelocityMag_Hook(int thisPtr, int)
{
	if (!g_State.useVelocitySmoothing)
	{
		return GetMaxRecentVelocityMag(thisPtr);
	}

	// Track frame time
	auto currentTime = std::chrono::high_resolution_clock::now();
	double frameTime = 0.0;
	if (g_State.velocityTimeInitialized)
	{
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - g_State.lastVelocityTime);
		frameTime = duration.count() / 1000000.0;
	}
	g_State.lastVelocityTime = currentTime;

	double raw = GetMaxRecentVelocityMag(thisPtr);

	if (!g_State.velocityTimeInitialized)
	{
		g_State.velocityTimeInitialized = true;
		return raw;
	}

	// Calculate time scale
	double timeScale = TARGET_FRAME_TIME / frameTime;
	timeScale = std::clamp(timeScale, 0.5, 2.0);
	double scaled = raw * timeScale;

	// Exponential moving average
	if (!g_State.velocitySmoothingInitialized)
	{
		g_State.smoothedVelocity = scaled;
		g_State.velocitySmoothingInitialized = true;
		return scaled;
	}

	// Adaptive smoothing factor
	double frameDelta = std::abs(frameTime - TARGET_FRAME_TIME);
	double stability = std::clamp(1.0 - (frameDelta / TARGET_FRAME_TIME), 0.1, 1.0);
	double alpha = 0.2 * stability;

	g_State.smoothedVelocity = alpha * scaled + (1.0 - alpha) * g_State.smoothedVelocity;
	return g_State.smoothedVelocity;
}

static void __cdecl PolyGridFXCollisionHandlerCB_Hook(int hBody1, int hBody2, int* a3, int* a4, float a5, BYTE* a6, int a7)
{
	// One-time init of high-res timer
	if (g_State.polyGridSplashFreq.QuadPart == 0)
	{
		QueryPerformanceFrequency(&g_State.polyGridSplashFreq);
	}

	// Get current time in seconds
	LARGE_INTEGER curr; QueryPerformanceCounter(&curr);
	double now = double(curr.QuadPart) / double(g_State.polyGridSplashFreq.QuadPart);

	// Build an order-independent 64-bit key for this body pair
	uint64_t key = (uint64_t(std::max(hBody1, hBody2)) << 32) | std::min(hBody1, hBody2);

	// Lookup last splash timestamp
	auto& splashMap = g_State.polyGridLastSplashTime;
	auto it = splashMap.find(key);

	// If first splash or interval elapsed, forward & record
	if (it == splashMap.end() || (now - it->second) >= TARGET_FRAME_TIME)
	{
		PolyGridFXCollisionHandlerCB(hBody1, hBody2, a3, a4, a5, a6, a7);
		splashMap[key] = now;

		// Evict oldest entry if cache is full
		if (splashMap.size() > 50)
		{
			auto oldest = std::min_element(splashMap.begin(), splashMap.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
			splashMap.erase(oldest);
		}
	}
}

static void __fastcall UpdateNormalControlFlags_Hook(int thisPtr, int)
{
	g_State.useVelocitySmoothing = true;
	UpdateNormalControlFlags(thisPtr);
	g_State.useVelocitySmoothing = false;
}

static void __fastcall UpdateNormalFriction_Hook(int thisPtr, int)
{
	g_State.inFriction = true;
	UpdateNormalFriction(thisPtr);
	g_State.inFriction = false;
}

static double __fastcall GetTimerElapsedS_Hook(int thisPtr, int)
{
	// When sliding on friction, clamp the reported frame time
	if (g_State.inFriction)
	{
		double elapsedTime = GetTimerElapsedS(thisPtr);
		if (elapsedTime < TARGET_FRAME_TIME)
		{
			return TARGET_FRAME_TIME;
		}
		return elapsedTime;
	}

	return GetTimerElapsedS(thisPtr);
}

// ========================
// FixKeyboardInputLanguage
// ========================

static void __fastcall LoadUserProfile_Hook(int thisPtr, int, bool bLoadDefaults, bool bLoadDisplaySettings)
{
	g_State.isLoadingDefault = bLoadDefaults;
	LoadUserProfile(thisPtr, bLoadDefaults, bLoadDisplaySettings);
	g_State.isLoadingDefault = false;
}

static bool __fastcall RestoreDefaults_Hook(int thisPtr, int, uint8_t nFlags)
{
	g_State.isLoadingDefault = true;
	bool res = RestoreDefaults(thisPtr, nFlags);
	g_State.isLoadingDefault = false;
	return res;
}

// ======================
// WeaponFixes
// ======================

static BYTE* __fastcall AimMgrCtor_Hook(BYTE* thisPtr, int)
{
	g_State.pAimMgr = thisPtr;
	return AimMgrCtor(thisPtr);
}

static void __fastcall AnimationClearLock_Hook(DWORD thisPtr, int)
{
	AnimationClearLock(thisPtr);
}

static void __fastcall UpdateWeaponModel_Hook(DWORD* thisPtr, int)
{
	if (g_State.isEnteringWorld)
	{
		// Set 'm_dwLastWeaponContextAnim' to -1 to force an update of the weapon model
		thisPtr[11] = -1;
		g_State.isEnteringWorld = false;
	}

	UpdateWeaponModel(thisPtr);
}

static void __fastcall SetAnimProp_Hook(DWORD* thisPtr, int, int eAnimPropGroup, int eAnimProp)
{
	// Skip if not an action
	if (g_State.fireAnimationInterceptionDisabled || eAnimPropGroup != 0)
	{
		SetAnimProp(thisPtr, eAnimPropGroup, eAnimProp);
		return;
	}

	g_State.actionAnimationThreshold++;

	// For the first 10 action animations after loading a map
	if (g_State.actionAnimationThreshold <= 10)
	{
		// Check if the fire animation is playing
		if (eAnimProp == g_State.kAP_ACT_Fire_Id && g_State.pUpperAnimationContext != 0)
		{
			// Unblock the player
			AnimationClearLock(g_State.pUpperAnimationContext);
			g_State.fireAnimationInterceptionDisabled = true;
		}
	}
	else
	{
		g_State.fireAnimationInterceptionDisabled = true;
	}

	SetAnimProp(thisPtr, eAnimPropGroup, eAnimProp);
}

static bool __fastcall InitAnimations_Hook(DWORD* thisPtr, int)
{
	g_State.actionAnimationThreshold = 0;
	g_State.fireAnimationInterceptionDisabled = false;
	bool res = InitAnimations(thisPtr);
	g_State.pUpperAnimationContext = thisPtr[2];
	return res;
}

static void __fastcall NextWeapon_Hook(DWORD* thisPtr, int)
{
	g_State.requestNextWeapon = true;
	NextWeapon(thisPtr);
	g_State.requestNextWeapon = false;
}

static void __fastcall PreviousWeapon_Hook(DWORD* thisPtr, int)
{
	g_State.requestPreviousWeapon = true;
	PreviousWeapon(thisPtr);
	g_State.requestPreviousWeapon = false;
}

static uint8_t __fastcall GetWeaponSlot_Hook(int thisPtr, int, int weaponHandle)
{
	// If we're not switching weapons, just call the original
	if (!g_State.requestNextWeapon && !g_State.requestPreviousWeapon)
	{
		return GetWeaponSlot(thisPtr, weaponHandle);
	}

	// Read the total number of slots and the pointer to the slot array
	uint8_t slotCount = *reinterpret_cast<uint8_t*>(thisPtr + 0x40);
	uint32_t* slotArray = *reinterpret_cast<uint32_t**>(thisPtr + 0xB4);

	// Find the index of the currently held weapon in the slot array
	int currentSlot = -1;
	for (int i = 0; i < slotCount; ++i)
	{
		if (slotArray[i] == static_cast<uint32_t>(weaponHandle))
		{
			currentSlot = i;
			break;
		}
	}

	// Not holding a weapon?
	if (currentSlot < 0)
	{
		if (g_State.requestNextWeapon)
		{
			// Position just before the first non-empty slot
			for (int i = 0; i < slotCount; i++)
			{
				if (slotArray[i] != 0)
					return static_cast<uint8_t>(i - 1);
			}
		}

		if (g_State.requestPreviousWeapon)
		{
			// Position just after the last non-empty slot
			for (int i = slotCount - 1; i >= 0; i--)
			{
				if (slotArray[i] != 0)
					return static_cast<uint8_t>(i + 1);
			}
		}

		// No weapons at all
		return 0xFF;
	}

	// NextWeapon()
	if (g_State.requestNextWeapon)
	{
		int nextIndex = currentSlot + 1;

		// Skip over empty slots
		while (nextIndex < slotCount && slotArray[nextIndex] == 0)
		{
			nextIndex++;
		}

		// If we've run past the end, handle wrapping for original game
		if (nextIndex >= slotCount)
		{
			if (g_State.IsOriginalGame())
			{
				// find first real slot
				int firstReal = -1;
				for (int j = 0; j < slotCount; j++)
				{
					if (slotArray[j] != 0)
					{
						firstReal = j;
						break;
					}
				}
				if (firstReal >= 0)
					return static_cast<uint8_t>(firstReal - 1);
			}
			// For XP/XP2 or if no real slots found, let the engine handle wrapping
			return static_cast<uint8_t>(currentSlot);
		}

		// If we've landed on a real slot, return the previous index for selection
		if (slotArray[nextIndex] != 0)
		{
			return static_cast<uint8_t>(nextIndex - 1);
		}
	}

	// PreviousWeapon()
	if (g_State.requestPreviousWeapon)
	{
		int prevIndex = currentSlot - 1;

		// Skip over empty slots
		while (prevIndex >= 0 && slotArray[prevIndex] == 0)
		{
			prevIndex--;
		}

		// If we've run before the beginning, handle wrapping for original game
		if (prevIndex < 0)
		{
			if (g_State.IsOriginalGame())
			{
				// find last real slot
				int lastReal = -1;
				for (int j = slotCount - 1; j >= 0; j--)
				{
					if (slotArray[j] != 0)
					{
						lastReal = j;
						break;
					}
				}
				if (lastReal >= 0)
					return static_cast<uint8_t>(lastReal + 1);
			}
			// For XP/XP2 or if no real slots found, let the engine handle wrapping
			return static_cast<uint8_t>(slotCount);
		}

		// If we've landed on a real slot, return the next index for selection
		if (slotArray[prevIndex] != 0)
		{
			return static_cast<uint8_t>(prevIndex + 1);
		}
	}

	// Fallback: return the actual current slot index
	return static_cast<uint8_t>(currentSlot);
}

// =========================
// HighResolutionReflections
// =========================

static bool __fastcall RenderTargetGroupFXInit_Hook(int thisPtr, int, DWORD* psfxCreateStruct)
{
	// Low
	psfxCreateStruct[3] *= 4;
	psfxCreateStruct[4] *= 4;
	// Medium
	psfxCreateStruct[5] *= 4;
	psfxCreateStruct[6] *= 4;
	// High
	psfxCreateStruct[7] *= 4;
	psfxCreateStruct[8] *= 4;
	return RenderTargetGroupFXInit(thisPtr, psfxCreateStruct);
}

// ==========================
// EnablePersistentWorldState
// ==========================

static float __stdcall GetShatterLifetime_Hook(int shatterType)
{
	return FLT_MAX;
}

static int __stdcall CreateFX_Hook(char* effectType, int fxData, int prop)
{
	if (prop)
	{
		// Decal
		if (*reinterpret_cast<uint32_t*>(effectType) == 0x61636544)
		{
			MemoryHelper::WriteMemory<float>(prop + 0x8, FLT_MAX, false);
			MemoryHelper::WriteMemory<float>(prop + 0xC, FLT_MAX, false);
		}
		// LTBModel
		else if (*reinterpret_cast<uint32_t*>(effectType) == 0x4D42544C)
		{
			int fxName = *(DWORD*)(fxData + 0x74);

			// Skip HRocket_Debris
			if (*reinterpret_cast<uint64_t*>(fxName + 0x2) != 0x65445F74656B636F)
			{
				MemoryHelper::WriteMemory<float>(prop + 0x8, FLT_MAX, false);
				MemoryHelper::WriteMemory<float>(prop + 0xC, FLT_MAX, false);
			}
		}
		// Sprite
		else if (*reinterpret_cast<uint32_t*>(effectType) == 0x69727053)
		{
			int fxName = *(DWORD*)(fxData + 0x74);

			if (*reinterpret_cast<uint64_t*>(fxName) == 0x75625F656E6F7453 // Stone_bullethole
				|| *reinterpret_cast<uint64_t*>(fxName) == 0x455F736972626544 // Debris_Electronic_Chunk
				|| *reinterpret_cast<uint64_t*>(fxName) == 0x575F736972626544 // Debris_Wood_Chunk
				|| *reinterpret_cast<uint64_t*>(fxName) == 0x4D5F736972626544 // Debris_Mug_Chunk
				|| *reinterpret_cast<uint64_t*>(fxName) == 0x565F736972626544 // Debris_Vase1_Chunk
				|| *reinterpret_cast<uint64_t*>(fxName + 0x4) == 0x3674616C70735F68) // Flesh_splat6
			{
				MemoryHelper::WriteMemory<float>(prop + 0x8, FLT_MAX, false); // m_tmEnd
				MemoryHelper::WriteMemory<float>(prop + 0xC, FLT_MAX, false); // m_tmLifetime
			}
		}
	}

	return CreateFX(effectType, fxData, prop);
}

// ======================
// HUDScaling
// ======================

// Terminate the HUD
static void __fastcall HUDTerminate_Hook(int thisPtr, int)
{
	HUDTerminate(thisPtr);
}

// Initialize the HUD
static bool __fastcall HUDInit_Hook(int thisPtr, int)
{
	g_State.CHUDMgr = thisPtr;
	return HUDInit(thisPtr);
}

static void __fastcall HUDRender_Hook(int thisPtr, int, int eHUDRenderLayer)
{
	HUDRender(thisPtr, eHUDRenderLayer);
	if (g_State.updateHUD)
	{
		*(DWORD*)(thisPtr + 0x14) = -1;  // Force full HUD refresh (update health bar)
		g_State.updateHUD = false;
	}
}

static void __fastcall HUDWeaponListReset_Hook(int thisPtr, int)
{
	HUDWeaponListReset(thisPtr);
}

static bool __fastcall HUDWeaponListInit_Hook(int thisPtr, int)
{
	g_State.CHUDWeaponList = thisPtr;
	return HUDWeaponListInit(thisPtr);
}

static bool __fastcall HUDGrenadeListInit_Hook(int thisPtr, int)
{
	g_State.CHUDGrenadeList = thisPtr;
	return HUDGrenadeListInit(thisPtr);
}

static void __fastcall ScreenDimsChanged_Hook(int thisPtr, int)
{
	ScreenDimsChanged(thisPtr);

	// Get the current resolution
	g_State.currentWidth = *(DWORD*)(thisPtr + 0x18);
	g_State.currentHeight = *(DWORD*)(thisPtr + 0x1C);

	// Calculate the new scaling factor
	g_State.scalingFactor = std::sqrt((g_State.currentWidth * g_State.currentHeight) / BASE_AREA);

	// Don't downscale the HUD
	if (g_State.scalingFactor < 1.0f) g_State.scalingFactor = 1.0f;

	// Do not change the scaling of the crosshair
	g_State.scalingFactorCrosshair = g_State.scalingFactor;

	// Apply custom scaling for the text
	g_State.scalingFactorText = g_State.scalingFactor * SmallTextCustomScalingFactor;

	// Apply custom scaling to calculated scaling
	g_State.scalingFactor *= HUDCustomScalingFactor;

	// Reset slow-mo bar update flag and HUDHealth.AdditionalInt index
	g_State.slowMoBarUpdated = false;
	g_State.healthAdditionalIntIndex = 0;

	// If the resolution is updated
	if (g_State.CHUDMgr != 0)
	{
		// Reinitialize the HUD
		HUDTerminate(g_State.CHUDMgr);
		HUDInit(g_State.CHUDMgr);
		HUDWeaponListReset(g_State.CHUDWeaponList);
		HUDWeaponListUpdateTriggerNames(g_State.CHUDWeaponList);
		HUDGrenadeListUpdateTriggerNames(g_State.CHUDGrenadeList);
		HUDPausedInit(g_State.CHUDPaused);
		g_State.updateHUD = true;

		// Update the size of the crosshair
		SetConsoleVariableFloat("CrosshairSize", g_State.crosshairSize * g_State.scalingFactorCrosshair);
		SetConsoleVariableFloat("PerturbScale", 0.5f * g_State.scalingFactorCrosshair);
	}
}

// Scale HUD position or dimension
static DWORD* __stdcall LayoutDBGetPosition_Hook(DWORD* a1, int Record, char* Attribute, int a4) 
{
	if (!Record)
		return LayoutDBGetPosition(a1, Record, Attribute, a4);

	DWORD* result = LayoutDBGetPosition(a1, Record, Attribute, a4);
	const char* hudRecordString = *(const char**)Record;

	std::string_view hudElement(hudRecordString);
	std::string_view attribute(Attribute);

	auto hudEntry = g_State.hudScalingRules.find(hudElement);
	if (hudEntry != g_State.hudScalingRules.end()) 
	{
		const auto& rule = hudEntry->second;
		if (std::binary_search(rule.attributes.begin(), rule.attributes.end(), attribute)) 
		{
			float scalingFactor = *rule.scalingFactorPtr;
			result[0] = static_cast<DWORD>(static_cast<int>(result[0]) * scalingFactor);
			result[1] = static_cast<DWORD>(static_cast<int>(result[1]) * scalingFactor);
		}
	}

	return result;
}

// Increase the length of the bar used for the flashlight and slowmo
static float* __stdcall GetRectF_Hook(DWORD* a1, int Record, char* Attribute, int a4)
{
	if (!Record)
		return GetRectF(a1, Record, Attribute, a4);

	float* result = GetRectF(a1, Record, Attribute, a4);
	char* hudRecordString = *(char**)Record;

	if (hudRecordString[4] == 'l' && strcmp(Attribute, "AdditionalRect") == 0 && (strcmp(hudRecordString, "HUDSlowMo2") == 0 || strcmp(hudRecordString, "HUDFlashlight") == 0))
	{
		result[0] *= g_State.scalingFactor;
		result[1] *= g_State.scalingFactor;
		result[2] *= g_State.scalingFactor;
		result[3] *= g_State.scalingFactor;
	}

	return result;
}

// Override 'DBGetInt32' and other related functions to return specific values
static int __stdcall DBGetRecord_Hook(int Record, char* Attribute)
{
	if (!Record)
		return DBGetRecord(Record, Attribute);

	char* hudRecordString = *(char**)Record;

	if (Attribute[4] == 'S' && strcmp(Attribute, "TextSize") == 0)
	{
		auto it = g_State.textDataMap.find(hudRecordString);
		if (it != g_State.textDataMap.end())
		{
			auto dt = it->second;
			float scaledSize = dt.TextSize * g_State.scalingFactor;

			switch (dt.ScaleType)
			{
				case 1: scaledSize = std::round(scaledSize * 0.95f); break;
				case 2: scaledSize = dt.TextSize * g_State.scalingFactorText; break;
			}

			g_State.int32ToUpdate = static_cast<int32_t>(scaledSize);
			g_State.updateLayoutReturnValue = true;
		}
	}

	// Additional?
	if (Attribute[9] == 'l')
	{
		// Update the rectangle's length
		if (strcmp(Attribute, "AdditionalFloat") == 0)
		{
			float baseValue = 0.0f;

			if (!g_State.slowMoBarUpdated && strcmp(hudRecordString, "HUDSlowMo2") == 0)
			{
				baseValue = 10.0f;
				g_State.slowMoBarUpdated = true;
			}
			else if (strcmp(hudRecordString, "HUDFlashlight") == 0)
			{
				baseValue = 6.0f;
			}

			if (baseValue != 0.0f)
			{
				g_State.updateLayoutReturnValue = true;
				g_State.floatToUpdate = baseValue * g_State.scalingFactor;
			}
		}
		// Medkit prompt when health drops below 50
		else if (strcmp(hudRecordString, "HUDHealth") == 0 && strcmp(Attribute, "AdditionalInt") == 0)
		{
			if (g_State.healthAdditionalIntIndex == 2)
			{
				g_State.updateLayoutReturnValue = true;
				g_State.int32ToUpdate = 14 * g_State.scalingFactor;
			}
			else
			{
				g_State.healthAdditionalIntIndex++;
			}
		}
	}

	return DBGetRecord(Record, Attribute);
}

// Executed right after 'DBGetRecord'
static int __stdcall DBGetInt32_Hook(int a1, unsigned int a2, int a3)
{
	if (g_State.updateLayoutReturnValue)
	{
		g_State.updateLayoutReturnValue = false;
		return g_State.int32ToUpdate;
	}

	return DBGetInt32(a1, a2, a3);
}

// Executed right after 'DBGetRecord'
static float __stdcall DBGetFloat_Hook(int a1, unsigned int a2, float a3)
{
	if (g_State.updateLayoutReturnValue)
	{
		g_State.updateLayoutReturnValue = false;
		return g_State.floatToUpdate;
	}

	return DBGetFloat(a1, a2, a3);
}

// Executed right after 'DBGetRecord'
static const char* __stdcall DBGetString_Hook(int a1, unsigned int a2, int a3)
{
	return DBGetString(a1, a2, a3);
}

static int __fastcall UpdateSlider_Hook(int thisPtr, int, int index)
{
	if (thisPtr)
	{
		const char* sliderName = *(const char**)(thisPtr + 8);

		// If 'ScreenCrosshair_Size_Help' is next
		if (strcmp(sliderName, "IDS_HELP_PICKUP_MSG_DUR") == 0)
		{
			g_State.crosshairSliderUpdated = false;
		}

		// Update the index of 'ScreenCrosshair_Size_Help' as it will be wrong on the first time
		if (strcmp(sliderName, "ScreenCrosshair_Size_Help") == 0 && !g_State.crosshairSliderUpdated && g_State.scalingFactorCrosshair > 1.0f)
		{
			float unscaledIndex = index / g_State.scalingFactorCrosshair;

			// scs.nIncrement = 2
			int newIndex = static_cast<int>((unscaledIndex / 2.0f) + 0.5f) * 2;

			// Clamp to the range [4, 16].
			newIndex = std::clamp(newIndex, 4, 16);

			index = newIndex;

			// Only needed on the first time
			g_State.crosshairSliderUpdated = true;
		}
	}

	return UpdateSlider(thisPtr, index);
}

static void __stdcall InitAdditionalTextureData_Hook(int a1, int a2, int* a3, DWORD* vPos, DWORD* vSize, float a6)
{
	vPos[0] = static_cast<DWORD>((int)vPos[0] * g_State.scalingFactor);
	vPos[1] = static_cast<DWORD>((int)vPos[1] * g_State.scalingFactor);

	vSize[0] = static_cast<DWORD>((int)vSize[0] * g_State.scalingFactor);
	vSize[1] = static_cast<DWORD>((int)vSize[1] * g_State.scalingFactor);

	InitAdditionalTextureData(a1, a2, a3, vPos, vSize, a6);
}

static void __fastcall HUDPausedInit_Hook(int thisPtr, int)
{
	g_State.CHUDPaused = thisPtr;
	HUDPausedInit(thisPtr);
}

// ======================
// AutoResolution
// ======================

static void __cdecl AutoDetectPerformanceSettings_Hook()
{
	g_State.isInAutoDetect = true;
	AutoDetectPerformanceSettings();
	g_State.isInAutoDetect = false;
}

static void __fastcall SetOption_Hook(int thisPtr, int, int a2, int a3, int a4, int a5)
{
	g_State.isSettingOption = true;
	SetOption(thisPtr, a2, a3, a4, a5);
	g_State.isSettingOption = false;
}

static bool __fastcall SetQueuedConsoleVariable_Hook(int thisPtr, int, const char* pszVar, float a3, int a4)
{
	// Don't update the resolution if we change the presets
	if (g_State.isSettingOption && (strcmp(pszVar, "Performance_ScreenHeight") == 0 || strcmp(pszVar, "Performance_ScreenWidth") == 0))
	{
		return false;
	}

	return SetQueuedConsoleVariable(thisPtr, pszVar, a3, a4);
}

// =======================
// XInputControllerSupport
// =======================

static bool __fastcall IsCommandOn_Hook(int thisPtr, int, int commandId)
{
	return (commandId < 117 && g_Controller.commandActive[commandId]) || IsCommandOn(thisPtr, commandId);
}

static bool __fastcall OnCommandOn_Hook(int thisPtr, int, int commandId)
{
	return OnCommandOn(thisPtr, commandId);
}

static bool __fastcall OnCommandOff_Hook(int thisPtr, int, int commandId)
{
	return OnCommandOff(thisPtr, commandId);
}

static double __fastcall GetExtremalCommandValue_Hook(int thisPtr, int, int commandId)
{
	if (g_Controller.isConnected)
	{
		const int DEAD_ZONE = 7849;
		const auto& gamepad = g_Controller.state.Gamepad;

		switch (commandId)
		{
			case 2: // Forward
			{
				if (abs(gamepad.sThumbLY) < DEAD_ZONE) return 0.0;
				return gamepad.sThumbLY / 32768.0;
			}
			case 5: // Strafe
			{
				if (abs(gamepad.sThumbLX) < DEAD_ZONE) return 0.0;
				return gamepad.sThumbLX / 32768.0;
			}
			case 22: // Pitch
			{
				if (abs(gamepad.sThumbRY) < DEAD_ZONE) return 0.0;
				double pitchValue = -gamepad.sThumbRY / 32768.0;
				if (g_State.zoomMag > GPadZoomMagThreshold) pitchValue *= (g_State.zoomMag / GPadZoomMagThreshold);
				return pitchValue;
			}
			case 23: // Yaw
			{
				if (abs(gamepad.sThumbRX) < DEAD_ZONE) return 0.0;
				double yawValue = gamepad.sThumbRX / 32768.0;
				if (g_State.zoomMag > GPadZoomMagThreshold) yawValue *= (g_State.zoomMag / GPadZoomMagThreshold);
				return yawValue;
			}
		}
	}

	return GetExtremalCommandValue(thisPtr, commandId);
}

static double __fastcall GetZoomMag_Hook(int thisPtr)
{
	g_State.zoomMag = GetZoomMag(thisPtr);
	return g_State.zoomMag;
}

static int __fastcall HUDActivateObjectSetObject_Hook(int thisPtr, int, void** a2, int a3, int a4, int a5, int nNewType)
{
	// If we can open a door or pick up an item
	g_State.canActivate = nNewType != -1;
	return HUDActivateObjectSetObject(thisPtr, a2, a3, a4, a5, nNewType);
}

static int __fastcall SetOperatingTurret_Hook(int thisPtr, int, int pTurret)
{
	// Operating a turret?
	g_State.isOperatingTurret = pTurret != 0;
	return SetOperatingTurret(thisPtr, pTurret);
}

static const wchar_t* __fastcall GetTriggerNameFromCommandID_Hook(int thisPtr, int, int commandId)
{
	if (!g_Controller.isConnected)
		return GetTriggerNameFromCommandID(thisPtr, commandId);

	// Left Thumbstick movement
	switch (commandId)
	{
		case 0: return L"Left Thumbstick Up";
		case 1: return L"Left Thumbstick Down";
	}

	// Reload key alias
	if (commandId == 87)
		commandId = 88;

	bool shortName = false;
	if (commandId >= 30 && commandId <= 39) { commandId = 77; shortName = true; } // HUDWeapon
	else if (commandId >= 40 && commandId <= 45) { commandId = 73; shortName = true; } // HUDGrenadeList

	// Find the matching button for this command Id
	for (size_t i = 0; i < sizeof(g_buttonMappings) / sizeof(g_buttonMappings[0]); ++i)
	{
		if (g_buttonMappings[i].second == commandId)
		{
			switch (g_buttonMappings[i].first)
			{
				case XINPUT_GAMEPAD_A:              return shortName ? L"A" : L"A Button";
				case XINPUT_GAMEPAD_B:              return shortName ? L"B" : L"B Button";
				case XINPUT_GAMEPAD_X:              return shortName ? L"X" : L"X Button";
				case XINPUT_GAMEPAD_Y:              return shortName ? L"Y" : L"Y Button";
				case XINPUT_GAMEPAD_LEFT_SHOULDER:  return shortName ? L"LB" : L"Left Bumper";
				case XINPUT_GAMEPAD_RIGHT_SHOULDER: return shortName ? L"RB" : L"Right Bumper";
				case XINPUT_GAMEPAD_LEFT_TRIGGER:   return shortName ? L"LT" : L"Left Trigger";
				case XINPUT_GAMEPAD_RIGHT_TRIGGER:  return shortName ? L"RT" : L"Right Trigger";
				case XINPUT_GAMEPAD_LEFT_THUMB:     return shortName ? L"L3" : L"Left Thumbstick";
				case XINPUT_GAMEPAD_RIGHT_THUMB:    return shortName ? L"R3" : L"Right Thumbstick";
				case XINPUT_GAMEPAD_DPAD_UP:        return shortName ? L"D-Up" : L"D-Pad Up";
				case XINPUT_GAMEPAD_DPAD_DOWN:      return shortName ? L"D-Down" : L"D-Pad Down";
				case XINPUT_GAMEPAD_DPAD_LEFT:      return shortName ? L"D-Left" : L"D-Pad Left";
				case XINPUT_GAMEPAD_DPAD_RIGHT:     return shortName ? L"D-Right" : L"D-Pad Right";
				case XINPUT_GAMEPAD_BACK:           return shortName ? L"Back" : L"Back Button";
				default: break;
			}
		}
	}

	return GetTriggerNameFromCommandID(thisPtr, commandId);
}


static bool __fastcall ChangeState_Hook(int thisPtr, int, int state, int screenId)
{
	g_State.isPlaying = state == 1;
	return ChangeState(thisPtr, state, screenId);
}

static void __fastcall UseCursor_Hook(int thisPtr, int, bool bUseCursor)
{
	if (g_Controller.isConnected)
	{
		bUseCursor = false;
	}

	UseCursor(thisPtr, bUseCursor);
}

static bool __fastcall OnMouseMove_Hook(int thisPtr, int, int x, int y)
{
	if (g_Controller.isConnected)
	{
		x = 0;
		y = 0;
	}

	return OnMouseMove(thisPtr, x, y);
}

static void __fastcall HUDSwapUpdate_Hook(int thisPtr, int)
{
	HUDSwapUpdate(thisPtr);
	g_State.canSwap = *(BYTE*)(thisPtr + 0x1C0) != 0;
}

static void __fastcall SwitchToScreen_Hook(int thisPtr, int, int pNewScreen)
{
	int currentScreenID = *(DWORD*)(pNewScreen + 0x10);

	if (currentScreenID == g_State.screenPerformanceCPU)
	{
		g_State.maxCurrentType = 3;
		g_State.currentType = 0;
	}
	else if (currentScreenID == g_State.screenPerformanceGPU)
	{
		g_State.maxCurrentType = 2;
		g_State.currentType = 0;
	}
	else
	{
		g_State.maxCurrentType = -1;
	}

	SwitchToScreen(thisPtr, pNewScreen);
}

static void __fastcall SetCurrentType_Hook(int thisPtr, int, int type)
{
	g_State.pCurrentType = thisPtr;
	SetCurrentType(thisPtr, type);
}

static void __cdecl HUDSwapUpdateTriggerName_Hook()
{
	HUDSwapUpdateTriggerName();
}

static const wchar_t* __stdcall LoadGameString_Hook(int ptr, char* String)
{
	if (g_Controller.isConnected)
	{
		if (strcmp(String, "IDS_QUICKSAVE") == 0)
		{
			return L"Quick save";
		}
		else if (strcmp(String, "ScreenFailure_PressAnyKey") == 0)
		{
			return L"Press B to return to the main menu.\nPress any other button to continue.";
		}
	}

	return LoadGameString(ptr, String);
}

// =============================
// EnableCustomMaxWeaponCapacity
// =============================

static uint8_t __fastcall GetWeaponCapacity_Hook(int thisPtr, int)
{
	return MaxWeaponCapacity;
}

// =============================
// DisableHipFireAccuracyPenalty
// =============================

static void __fastcall AccuracyMgrUpdate_Hook(float* thisPtr, int)
{
	AccuracyMgrUpdate(thisPtr);
	*thisPtr = 0.0f;
}

// =====================================
//  XInputControllerSupport & HUDScaling
// =====================================

static void __fastcall HUDWeaponListUpdateTriggerNames_Hook(int thisPtr, int)
{
	HUDWeaponListUpdateTriggerNames(thisPtr);
}

static void __fastcall HUDGrenadeListUpdateTriggerNames_Hook(int thisPtr, int)
{
	HUDGrenadeListUpdateTriggerNames(thisPtr);
}

// ============================================
//  EnableCustomMaxWeaponCapacity & WeaponFixes
// ============================================

static void __fastcall OnEnterWorld_Hook(int thisPtr, int)
{
	OnEnterWorld(thisPtr);

	// Set the flag to allow aiming (can be 0 if loading a save during a cutscene)
	if (WeaponFixes)
	{
		g_State.isEnteringWorld = true;
		g_State.pAimMgr[1] = 1;
	}

	// Update the weapon capacity
	if (EnableCustomMaxWeaponCapacity)
	{
		SetWeaponCapacityServer(g_State.CPlayerInventory, MaxWeaponCapacity);
	}
}

#pragma endregion

#pragma region Client Patches

static void ApplyHighFPSFixesClientPatch()
{
	if (!HighFPSFixes) return;
	
	DWORD targetMemoryLocation_SurfaceJumpImpulse = ScanModuleSignature(g_State.GameClient, "C7 44 24 1C 00 00 00 00 C7 44 24 10 00 00 00 00 EB", "SurfaceJumpImpulse");
	DWORD targetMemoryLocation_HeightOffset = ScanModuleSignature(g_State.GameClient, "D9 E1 D9 54 24 1C D8 1D ?? ?? ?? ?? DF E0 F6 C4 41 0F 85", "HeightOffset");
	DWORD targetMemoryLocation_UpdateOnGround = ScanModuleSignature(g_State.GameClient, "83 EC 3C 53 55 56 57 8B F1", "UpdateOnGround");
	DWORD targetMemoryLocation_UpdateWaveProp = ScanModuleSignature(g_State.GameClient, "D9 44 24 04 83 EC ?? D8 1D", "UpdateWaveProp");
	DWORD targetMemoryLocation_UpdateNormalFriction = ScanModuleSignature(g_State.GameClient, "83 EC 3C 56 8B F1 8B 46 28 F6 C4 08 C7 44 24 34", "UpdateNormalFriction");
	DWORD targetMemoryLocation_GetTimerElapsedS = ScanModuleSignature(g_State.GameClient, "04 51 8B C8 FF 52 3C 85 C0 5E", "GetTimerElapsedS");
	DWORD targetMemoryLocation_GetMaxRecentVelocityMag = ScanModuleSignature(g_State.GameClient, "F6 C4 41 75 2F 8D 8E 34 04 00 00 E8", "GetMaxRecentVelocityMag");
	DWORD targetMemoryLocation_UpdateNormalControlFlags = ScanModuleSignature(g_State.GameClient, "55 8B EC 83 E4 F8 83 EC 18 53 55 56 57 8B F1 E8", "UpdateNormalControlFlags");
	DWORD targetMemoryLocation_PolyGridFXCollisionHandlerCB = ScanModuleSignature(g_State.GameClient, "83 EC 54 53 33 DB 3B CB ?? 74 05", "PolyGridFXCollisionHandlerCB");
	targetMemoryLocation_GetMaxRecentVelocityMag = MemoryHelper::ResolveRelativeAddress(targetMemoryLocation_GetMaxRecentVelocityMag, 0xC);

	if (targetMemoryLocation_SurfaceJumpImpulse == 0 ||
		targetMemoryLocation_HeightOffset == 0 ||
		targetMemoryLocation_UpdateOnGround == 0 ||
		targetMemoryLocation_GetMaxRecentVelocityMag == 0 ||
		targetMemoryLocation_UpdateNormalControlFlags == 0 ||
		targetMemoryLocation_UpdateNormalFriction == 0 ||
		targetMemoryLocation_GetTimerElapsedS == 0 ||
		targetMemoryLocation_UpdateWaveProp == 0 ||
		targetMemoryLocation_PolyGridFXCollisionHandlerCB == 0) {
		return;
	}

	MemoryHelper::MakeNOP(targetMemoryLocation_SurfaceJumpImpulse, 0x10);
	MemoryHelper::WriteMemory<uint8_t>(targetMemoryLocation_HeightOffset + 0x12, 0x84, true);
	HookHelper::ApplyHook((void*)targetMemoryLocation_GetMaxRecentVelocityMag, &GetMaxRecentVelocityMag_Hook, (LPVOID*)&GetMaxRecentVelocityMag);
	HookHelper::ApplyHook((void*)targetMemoryLocation_UpdateNormalControlFlags, &UpdateNormalControlFlags_Hook, (LPVOID*)&UpdateNormalControlFlags);
	HookHelper::ApplyHook((void*)targetMemoryLocation_UpdateOnGround, &UpdateOnGround_Hook, (LPVOID*)&UpdateOnGround);
	HookHelper::ApplyHook((void*)targetMemoryLocation_UpdateWaveProp, &UpdateWaveProp_Hook, (LPVOID*)&UpdateWaveProp);
	HookHelper::ApplyHook((void*)targetMemoryLocation_UpdateNormalFriction, &UpdateNormalFriction_Hook, (LPVOID*)&UpdateNormalFriction);
	HookHelper::ApplyHook((void*)(targetMemoryLocation_GetTimerElapsedS - 0x20), &GetTimerElapsedS_Hook, (LPVOID*)&GetTimerElapsedS);
	HookHelper::ApplyHook((void*)(targetMemoryLocation_PolyGridFXCollisionHandlerCB - 0x6), &PolyGridFXCollisionHandlerCB_Hook, (LPVOID*)&PolyGridFXCollisionHandlerCB);
}

static void ApplyMouseAimMultiplierClientPatch()
{
	if (MouseAimMultiplier == 1.0f) return;

	DWORD targetMemoryLocation_MouseAimMultiplier = ScanModuleSignature(g_State.GameClient, "89 4C 24 14 DB 44 24 14 8D 44 24 20 6A 01 50 D8 0D", "MouseAimMultiplier");

	if (targetMemoryLocation_MouseAimMultiplier != 0)
	{
		// Write the updated multiplier
		g_State.overrideSensitivity = g_State.overrideSensitivity * MouseAimMultiplier;
		MemoryHelper::WriteMemory<uint32_t>(targetMemoryLocation_MouseAimMultiplier + 0x11, reinterpret_cast<uintptr_t>(&g_State.overrideSensitivity), true);
	}
}

static void ApplyXPWidescreenClientPatch()
{
	if (DisableXPWidescreenFiltering && g_State.CurrentFEARGame == FEARXP)
	{
		// Disable non-4:3 filtering
		MemoryHelper::MakeNOP((DWORD)g_State.GameClient + 0x10DDB0, 24);
	}
}

static void ApplySkipSplashScreenClientPatch()
{
	if (!SkipSplashScreen) return;

	DWORD targetMemoryLocation = ScanModuleSignature(g_State.GameClient, "53 8B 5C 24 08 55 8B 6C 24 14 56 8D 43 FF 83 F8", "SkipSplashScreen");

	if (targetMemoryLocation != 0)
	{
		MemoryHelper::MakeNOP(targetMemoryLocation + 0x13D, 8);
	}
}

static void ApplyDisableLetterboxClientPatch()
{
	if (!DisableLetterbox) return;

	DWORD targetMemoryLocation = ScanModuleSignature(g_State.GameClient, "83 EC 54 53 55 56 57 8B", "DisableLetterbox");

	if (targetMemoryLocation != 0)
	{
		MemoryHelper::WriteMemory<uint8_t>(targetMemoryLocation, 0xC3, true);
	}
}

static void ApplyPersistentWorldClientPatch()
{
	if (!EnablePersistentWorldState) return;

	DWORD targetMemoryLocation_ShellCasing = ScanModuleSignature(g_State.GameClient, "D9 86 88 00 00 00 D8 64 24", "ShellCasing");
	DWORD targetMemoryLocation_DecalSaving = ScanModuleSignature(g_State.GameClient, "FF 52 0C ?? 8D ?? ?? ?? 00 00 E8 ?? ?? ?? FF 8B", "DecalSaving");
	DWORD targetMemoryLocation_Decal = ScanModuleSignature(g_State.GameClient, "DF E0 F6 C4 01 75 34 DD 44 24", "Decal");
	DWORD targetMemoryLocation_FX = ScanModuleSignature(g_State.GameClient, "8B CE FF ?? 04 84 C0 75 ?? 8B ?? 8B CE FF ?? 08 56 E8", "CreateFX", 1);
	DWORD targetMemoryLocation_Shatter = ScanModuleSignature(g_State.GameClient, "8B C8 E8 ?? ?? ?? 00 D9 5C 24 ?? D9", "Shatter");
	targetMemoryLocation_Shatter = MemoryHelper::ResolveRelativeAddress(targetMemoryLocation_Shatter, 0x3);

	if (targetMemoryLocation_ShellCasing == 0 ||
		targetMemoryLocation_DecalSaving == 0 ||
		targetMemoryLocation_Decal == 0 ||
		targetMemoryLocation_FX == 0 ||
		targetMemoryLocation_Shatter == 0) {
		return;
	}

	MemoryHelper::MakeNOP(targetMemoryLocation_ShellCasing + 0x6, 4);
	MemoryHelper::MakeNOP(targetMemoryLocation_DecalSaving + 0xF, 13);
	MemoryHelper::WriteMemory<uint8_t>(targetMemoryLocation_Decal + 0x5, 0x74, true);
	HookHelper::ApplyHook((void*)targetMemoryLocation_Shatter, &GetShatterLifetime_Hook, (LPVOID*)&GetShatterLifetime);
	HookHelper::ApplyHook((void*)targetMemoryLocation_FX, &CreateFX_Hook, (LPVOID*)&CreateFX);
}

static void ApplyInfiniteFlashlightClientPatch()
{
	if (!InfiniteFlashlight) return;

	DWORD targetMemoryLocation_Update = ScanModuleSignature(g_State.GameClient, "8B 51 10 8A 42 18 84 C0 8A 86 04 01 00 00", "InfiniteFlashlight_Update");
	DWORD targetMemoryLocation_UpdateBar = ScanModuleSignature(g_State.GameClient, "A1 ?? ?? ?? ?? 85 C0 56 8B F1 74 71 D9 86 1C 04 00 00", "InfiniteFlashlight_UpdateBar");
	DWORD targetMemoryLocation_UpdateLayout = ScanModuleSignature(g_State.GameClient, "68 ?? ?? ?? ?? 6A 00 68 ?? ?? ?? ?? 50 FF 57 58 8B 0D ?? ?? ?? ?? 50 FF 97 84 00 00 00 8B 0D ?? ?? ?? ?? 8B 11 50 8D 44 24 10 50 FF 52 04 8B 4C 24 0C 8D BE C4 01 00 00", "InfiniteFlashlight_UpdateLayout");
	DWORD targetMemoryLocation_Battery = ScanModuleSignature(g_State.GameClient, "D8 4C 24 04 DC AE 88 03 00 00 DD 96 88 03 00 00", "InfiniteFlashlight_Battery");

	if (targetMemoryLocation_Update == 0 ||
		targetMemoryLocation_UpdateBar == 0 ||
		targetMemoryLocation_UpdateLayout == 0 ||
		targetMemoryLocation_Battery == 0) {
		return;
	}

	MemoryHelper::WriteMemory<uint8_t>(targetMemoryLocation_Update - 0x31, 0xC3, true);
	MemoryHelper::WriteMemory<uint8_t>(targetMemoryLocation_UpdateLayout - 0x36, 0xC3, true);
	MemoryHelper::WriteMemory<uint8_t>(targetMemoryLocation_UpdateBar, 0xC3, true);
	MemoryHelper::MakeNOP(targetMemoryLocation_Battery + 0xA, 6);
}

static void ApplyXInputControllerClientPatch()
{
	if (!XInputControllerSupport) return;

	DWORD targetMemoryLocation_pGameClientShell = ScanModuleSignature(g_State.GameClient, "C1 F8 02 C1 E0 05 2B C2 8B CB BA 01 00 00 00 D3 E2 8B CD 03 C3 50 85 11", "pGameClientShell");
	if (targetMemoryLocation_pGameClientShell == 0) return;

	DWORD targetMemoryLocation_OnCommandOn = targetMemoryLocation_pGameClientShell + MemoryHelper::ReadMemory<int>(targetMemoryLocation_pGameClientShell + 0x21) + 0x25;
	DWORD targetMemoryLocation_OnCommandOff = targetMemoryLocation_pGameClientShell + MemoryHelper::ReadMemory<int>(targetMemoryLocation_pGameClientShell + 0x28) + 0x2C;
	DWORD targetMemoryLocation_GetExtremalCommandValue = ScanModuleSignature(g_State.GameClient, "83 EC 08 56 57 8B F9 8B 77 04 3B 77 08 C7 44 24 08 00 00 00 00", "GetExtremalCommandValue");	
	DWORD targetMemoryLocation_IsCommandOn = ScanModuleSignature(g_State.GameClient, "8B D1 8A 42 4C 84 C0 56 74 58", "IsCommandOn");
	DWORD targetMemoryLocation_ChangeState = ScanModuleSignature(g_State.GameClient, "8B 44 24 0C 53 8B 5C 24 0C 57 8B 7E 08", "ChangeState");
	DWORD targetMemoryLocation_HUDActivateObjectSetObject = ScanModuleSignature(g_State.GameClient, "8B 86 D4 02 00 00 3B C3 8D BE C8 02 00 00 74 0F", "HUDActivateObjectSetObject", 1);
	DWORD targetMemoryLocation_HUDSwapUpdate = ScanModuleSignature(g_State.GameClient, "55 8B EC 83 E4 F8 81 EC 84 01", "HUDSwapUpdate");
	DWORD targetMemoryLocation_SetOperatingTurret = ScanModuleSignature(g_State.GameClient, "8B 44 24 04 89 81 F4 05 00 00 8B 0D ?? ?? ?? ?? 8B 11 FF 52 3C C2 04 00", "SetOperatingTurret");
	DWORD targetMemoryLocation_GetTriggerNameFromCommandID = ScanModuleSignature(g_State.GameClient, "81 EC 44 08 00 00", "GetTriggerNameFromCommandID");
	DWORD targetMemoryLocation_SwitchToScreen = ScanModuleSignature(g_State.GameClient, "53 55 56 8B F1 8B 6E 60 33 DB 3B EB 57 8B 7C 24 14", "SwitchToScreen");
	DWORD targetMemoryLocation_SetCurrentType = ScanModuleSignature(g_State.GameClient, "53 8B 5C 24 08 85 DB 56 57 8B F1 7C 1C 8B BE E4", "SetCurrentType");
	DWORD targetMemoryLocation_HUDSwapUpdateTriggerName = ScanModuleSignature(g_State.GameClient, "8B 0D ?? ?? ?? ?? 6A 57 E8 ?? ?? ?? ?? 50 B9", "HUDSwapUpdateTriggerName");
	DWORD targetMemoryLocation_GetZoomMag = ScanModuleSignature(g_State.GameClient, "C7 44 24 30 00 00 00 00 8B 4D 28 57 E8", "GetZoomMag");
	DWORD targetMemoryLocation_PerformanceScreenId = ScanModuleSignature(g_State.GameClient, "8B C8 E8 ?? ?? ?? ?? 8B 4E 0C 8B 01 6A ?? FF 50 6C 85 C0 74 0A 8B 10 8B C8 FF 92 88 00 00 00 8B 4E 0C 8B 01 6A", "PerformanceScreenId");
	targetMemoryLocation_GetZoomMag = MemoryHelper::ResolveRelativeAddress(targetMemoryLocation_GetZoomMag, 0xD);

	if (targetMemoryLocation_OnCommandOn == 0 ||
		targetMemoryLocation_OnCommandOff == 0 ||
		targetMemoryLocation_GetExtremalCommandValue == 0 ||
		targetMemoryLocation_IsCommandOn == 0 ||
		targetMemoryLocation_ChangeState == 0 ||
		targetMemoryLocation_HUDActivateObjectSetObject == 0 ||
		targetMemoryLocation_HUDSwapUpdate == 0 ||
		targetMemoryLocation_SetOperatingTurret == 0 ||
		targetMemoryLocation_GetTriggerNameFromCommandID == 0 ||
		targetMemoryLocation_SwitchToScreen == 0 ||
		targetMemoryLocation_SetCurrentType == 0 ||
		targetMemoryLocation_HUDSwapUpdateTriggerName == 0 ||
		targetMemoryLocation_GetZoomMag == 0 ||
		targetMemoryLocation_PerformanceScreenId == 0) {
		return;
	}

	g_State.g_pGameClientShell = MemoryHelper::ReadMemory<int>(MemoryHelper::ReadMemory<int>(targetMemoryLocation_pGameClientShell + 0x1A));

	HookHelper::ApplyHook((void*)(targetMemoryLocation_GetExtremalCommandValue), &GetExtremalCommandValue_Hook, (LPVOID*)&GetExtremalCommandValue);
	HookHelper::ApplyHook((void*)targetMemoryLocation_IsCommandOn, &IsCommandOn_Hook, (LPVOID*)&IsCommandOn);
	HookHelper::ApplyHook((void*)targetMemoryLocation_OnCommandOn, &OnCommandOn_Hook, (LPVOID*)&OnCommandOn);
	HookHelper::ApplyHook((void*)targetMemoryLocation_OnCommandOff, &OnCommandOff_Hook, (LPVOID*)&OnCommandOff);
	HookHelper::ApplyHook((void*)targetMemoryLocation_SetOperatingTurret, &SetOperatingTurret_Hook, (LPVOID*)&SetOperatingTurret);
	HookHelper::ApplyHook((void*)targetMemoryLocation_GetTriggerNameFromCommandID, &GetTriggerNameFromCommandID_Hook, (LPVOID*)&GetTriggerNameFromCommandID);
	HookHelper::ApplyHook((void*)(targetMemoryLocation_ChangeState - 0x13), &ChangeState_Hook, (LPVOID*)&ChangeState);
	HookHelper::ApplyHook((void*)targetMemoryLocation_HUDActivateObjectSetObject, &HUDActivateObjectSetObject_Hook, (LPVOID*)&HUDActivateObjectSetObject);
	HookHelper::ApplyHook((void*)targetMemoryLocation_HUDSwapUpdate, &HUDSwapUpdate_Hook, (LPVOID*)&HUDSwapUpdate);
	HookHelper::ApplyHook((void*)targetMemoryLocation_SwitchToScreen, &SwitchToScreen_Hook, (LPVOID*)&SwitchToScreen);
	HookHelper::ApplyHook((void*)targetMemoryLocation_SetCurrentType, &SetCurrentType_Hook, (LPVOID*)&SetCurrentType);
	HookHelper::ApplyHook((void*)targetMemoryLocation_HUDSwapUpdateTriggerName, &HUDSwapUpdateTriggerName_Hook, (LPVOID*)&HUDSwapUpdateTriggerName);
	HookHelper::ApplyHook((void*)targetMemoryLocation_GetZoomMag, &GetZoomMag_Hook, (LPVOID*)&GetZoomMag);

	g_State.screenPerformanceCPU = MemoryHelper::ReadMemory<uint8_t>(targetMemoryLocation_PerformanceScreenId + 0xD);
	g_State.screenPerformanceGPU = MemoryHelper::ReadMemory<uint8_t>(targetMemoryLocation_PerformanceScreenId + 0x25);

	if (!HideMouseCursor) return;

	DWORD targetMemoryLocation_UseCursor = ScanModuleSignature(g_State.GameClient, "8A 44 24 04 84 C0 56 8B F1 88 46 01 74", "UseCursor");
	DWORD targetMemoryLocation_OnMouseMove = ScanModuleSignature(g_State.GameClient, "56 8B F1 8A 86 ?? ?? 00 00 84 C0 0F 84 B3", "OnMouseMove");

	HookHelper::ApplyHook((void*)targetMemoryLocation_OnMouseMove, &OnMouseMove_Hook, (LPVOID*)&OnMouseMove);
	HookHelper::ApplyHook((void*)targetMemoryLocation_UseCursor, &UseCursor_Hook, (LPVOID*)&UseCursor);
}

static void ApplyHUDScalingClientPatch()
{
	if (!HUDScaling) return;

	DWORD targetMemoryLocation_HUDTerminate = ScanModuleSignature(g_State.GameClient, "53 56 8B D9 8B B3 7C 04 00 00 8B 83 80 04 00 00 57 33 FF 3B F0", "HUDTerminate");
	DWORD targetMemoryLocation_HUDInit = ScanModuleSignature(g_State.GameClient, "8B ?? ?? 8D ?? 78 04 00 00", "HUDInit", 1);
	DWORD targetMemoryLocation_HUDRender = ScanModuleSignature(g_State.GameClient, "53 8B D9 8A 43 08 84 C0 74", "HUDRender");
	DWORD targetMemoryLocation_ScreenDimsChanged = ScanModuleSignature(g_State.GameClient, "A1 ?? ?? ?? ?? 81 EC 98 00 00 00 85 C0 56 8B F1", "ScreenDimsChanged");
	DWORD targetMemoryLocation_LayoutDBGetPosition = ScanModuleSignature(g_State.GameClient, "83 EC 10 8B 54 24 20 8B 0D", "LayoutDBGetPosition");
	DWORD targetMemoryLocation_GetRectF = ScanModuleSignature(g_State.GameClient, "14 8B 44 24 28 8B 4C 24 18 D9 18", "GetRectF");
	DWORD targetMemoryLocation_UpdateSlider = ScanModuleSignature(g_State.GameClient, "56 8B F1 8B 4C 24 08 8B 86 7C 01 00 00 3B C8 89 8E 80 01 00 00", "UpdateSlider");
	DWORD targetMemoryLocation_HUDWeaponListReset = ScanModuleSignature(g_State.GameClient, "51 53 55 8B E9 8B 0D", "HUDWeaponListReset");
	DWORD targetMemoryLocation_HUDWeaponListInit = ScanModuleSignature(g_State.GameClient, "51 53 55 57 8B F9 8B 07 FF 50 20 8B 0D", "HUDWeaponListInit");
	DWORD targetMemoryLocation_HUDGrenadeListInit = ScanModuleSignature(g_State.GameClient, "83 EC 08 53 55 57 8B F9 8B 07 FF 50 20 8B 0D", "HUDGrenadeListInit");
	DWORD targetMemoryLocation_InitAdditionalTextureData = ScanModuleSignature(g_State.GameClient, "8B 54 24 04 8B 01 83 EC 20 57", "InitAdditionalTextureData");
	DWORD targetMemoryLocation_HUDPausedInit = ScanModuleSignature(g_State.GameClient, "56 8B F1 8B 06 57 FF 50 20", "HUDPausedInit");

	if (targetMemoryLocation_HUDTerminate == 0 ||
		targetMemoryLocation_HUDInit == 0 ||
		targetMemoryLocation_HUDRender == 0 ||
		targetMemoryLocation_ScreenDimsChanged == 0 ||
		targetMemoryLocation_LayoutDBGetPosition == 0 ||
		targetMemoryLocation_GetRectF == 0 ||
		targetMemoryLocation_UpdateSlider == 0 ||
		targetMemoryLocation_HUDWeaponListReset == 0 ||
		targetMemoryLocation_HUDWeaponListInit == 0 ||
		targetMemoryLocation_HUDGrenadeListInit == 0 ||
		targetMemoryLocation_InitAdditionalTextureData == 0 ||
		targetMemoryLocation_HUDPausedInit == 0) {
		return;
	}

	HookHelper::ApplyHook((void*)targetMemoryLocation_HUDTerminate, &HUDTerminate_Hook, (LPVOID*)&HUDTerminate);
	HookHelper::ApplyHook((void*)targetMemoryLocation_HUDInit, &HUDInit_Hook, (LPVOID*)&HUDInit);
	HookHelper::ApplyHook((void*)targetMemoryLocation_HUDRender, &HUDRender_Hook, (LPVOID*)&HUDRender);
	HookHelper::ApplyHook((void*)targetMemoryLocation_ScreenDimsChanged, &ScreenDimsChanged_Hook, (LPVOID*)&ScreenDimsChanged);
	HookHelper::ApplyHook((void*)targetMemoryLocation_LayoutDBGetPosition, &LayoutDBGetPosition_Hook, (LPVOID*)&LayoutDBGetPosition);
	HookHelper::ApplyHook((void*)(targetMemoryLocation_GetRectF - 0x58), &GetRectF_Hook, (LPVOID*)&GetRectF);
	HookHelper::ApplyHook((void*)targetMemoryLocation_UpdateSlider, &UpdateSlider_Hook, (LPVOID*)&UpdateSlider);
	HookHelper::ApplyHook((void*)targetMemoryLocation_HUDWeaponListReset, &HUDWeaponListReset_Hook, (LPVOID*)&HUDWeaponListReset);
	HookHelper::ApplyHook((void*)targetMemoryLocation_HUDWeaponListInit, &HUDWeaponListInit_Hook, (LPVOID*)&HUDWeaponListInit);
	HookHelper::ApplyHook((void*)targetMemoryLocation_HUDGrenadeListInit, &HUDGrenadeListInit_Hook, (LPVOID*)&HUDGrenadeListInit);
	HookHelper::ApplyHook((void*)(targetMemoryLocation_InitAdditionalTextureData - 6), &InitAdditionalTextureData_Hook, (LPVOID*)&InitAdditionalTextureData);
	HookHelper::ApplyHook((void*)targetMemoryLocation_HUDPausedInit, &HUDPausedInit_Hook, (LPVOID*)&HUDPausedInit);
}

static void ApplySetWeaponCapacityClientPatch()
{
	if (!EnableCustomMaxWeaponCapacity) return;

	DWORD targetMemoryLocation_GetWeaponCapacity = ScanModuleSignature(g_State.GameClient, "CC 8B 41 48 8B 0D", "GetWeaponCapacity");

	if (targetMemoryLocation_GetWeaponCapacity != 0)
	{
		HookHelper::ApplyHook((void*)(targetMemoryLocation_GetWeaponCapacity + 0x1), &GetWeaponCapacity_Hook, (LPVOID*)&GetWeaponCapacity);
		g_State.appliedCustomMaxWeaponCapacity = true;
	}
}

static void ApplyHighResolutionReflectionsClientPatch()
{
	if (!HighResolutionReflections) return;

	DWORD targetMemoryLocation = ScanModuleSignature(g_State.GameClient, "8B 47 08 89 46 4C 8A 4F 24 88 4E 68 8A 57 25", "RenderTargetGroupFXInit");

	if (targetMemoryLocation != 0)
	{
		HookHelper::ApplyHook((void*)(targetMemoryLocation - 0x31), &RenderTargetGroupFXInit_Hook, (LPVOID*)&RenderTargetGroupFXInit);
	}
}

static void ApplyAutoResolutionClientCheck()
{
	if (!AutoResolution) return; 

	DWORD targetMemoryLocation_AutoDetectPerformanceSettings = ScanModuleSignature(g_State.GameClient, "83 C4 10 83 F8 01 75 37", "AutoDetectPerformanceSettings", 2);
	DWORD targetMemoryLocation_SetQueuedConsoleVariable = ScanModuleSignature(g_State.GameClient, "83 EC 10 56 8B F1 8B 46 ?? 8B 4E ?? 8D 54 24 18", "SetQueuedConsoleVariable");
	DWORD targetMemoryLocation_SetOption = ScanModuleSignature(g_State.GameClient, "51 8B 44 24 14 85 C0 89 0C 24", "SetOption");

	if (targetMemoryLocation_AutoDetectPerformanceSettings == 0 ||
		targetMemoryLocation_SetQueuedConsoleVariable == 0 ||
		targetMemoryLocation_SetOption == 0) {
		return;
	}

	HookHelper::ApplyHook((void*)targetMemoryLocation_AutoDetectPerformanceSettings, &AutoDetectPerformanceSettings_Hook, (LPVOID*)&AutoDetectPerformanceSettings);
	HookHelper::ApplyHook((void*)targetMemoryLocation_SetQueuedConsoleVariable, &SetQueuedConsoleVariable_Hook, (LPVOID*)&SetQueuedConsoleVariable);
	HookHelper::ApplyHook((void*)targetMemoryLocation_SetOption, &SetOption_Hook, (LPVOID*)&SetOption);
}

static void ApplyKeyboardInputLanguageClientCheck()
{
	if (!FixKeyboardInputLanguage) return;

	DWORD targetMemoryLocation_LoadUserProfile = ScanModuleSignature(g_State.GameClient, "53 8B 5C 24 08 84 DB 55 56 57 8B F9", "LoadUserProfile");
	DWORD targetMemoryLocation_RestoreDefaults = ScanModuleSignature(g_State.GameClient, "57 8B F9 8B 0D ?? ?? ?? ?? 8B 01 FF 50 4C 8B 10", "RestoreDefaults");

	if (targetMemoryLocation_LoadUserProfile == 0 ||
		targetMemoryLocation_RestoreDefaults == 0) {
		return;
	}

	HookHelper::ApplyHook((void*)targetMemoryLocation_LoadUserProfile, &LoadUserProfile_Hook, (LPVOID*)&LoadUserProfile);
	HookHelper::ApplyHook((void*)targetMemoryLocation_RestoreDefaults, &RestoreDefaults_Hook, (LPVOID*)&RestoreDefaults);
}

static void ApplyWeaponFixesClientPatch()
{
	if (!WeaponFixes) return;

	DWORD targetMemoryLocation_AimMgrCtor = ScanModuleSignature(g_State.GameClient, "8B C1 C6 00 00 C6 40 01 01 C3", "AimMgrCtor");
	DWORD targetMemoryLocation_UpdateWeaponModel = ScanModuleSignature(g_State.GameClient, "83 EC 44 56 8B F1 57 8B 7E 08 85 FF", "UpdateWeaponModel");
	DWORD targetMemoryLocation_AnimationClearLock = ScanModuleSignature(g_State.GameClient, "E8 BB FF FF FF C7 41 34 FF FF FF FF C7 81 58 01", "AnimationClearLock");
	DWORD targetMemoryLocation_SetAnimProp = ScanModuleSignature(g_State.GameClient, "8B 44 24 04 83 F8 FF 74 ?? 83 F8 12 7D ?? 8B 54 24 08 89 04", "SetAnimProp");
	DWORD targetMemoryLocation_InitAnimations = ScanModuleSignature(g_State.GameClient, "6A 08 6A 7A 8B CF FF ?? 24 8B ?? 6A 08", "InitAnimations", 3);
	DWORD targetMemoryLocation_GetWeaponSlot = ScanModuleSignature(g_State.GameClient, "8A 51 40 32 C0 84 D2 76 23 56 8B B1 B4 00 00 00 57", "GetWeaponSlot");
	DWORD targetMemoryLocation_NextWeapon = ScanModuleSignature(g_State.GameClient, "84 C0 0F 84 ?? 00 00 00 8B CE E8", "NextWeapon");
	DWORD targetMemoryLocation_PreviousWeapon = ScanModuleSignature(g_State.GameClient, "8D BE ?? 57 00 00 8B CF E8 ?? ?? ?? ?? 84 C0 74 1F 8B CE E8", "PreviousWeapon");
	DWORD targetMemoryLocation_kAP_ACT_Fire_Id = ScanModuleSignature(g_State.GameClient, "84 C0 75 1E 6A 00 68 ?? 00 00 00 6A 00 8B CF", "kAP_ACT_Fire_Id");
	targetMemoryLocation_NextWeapon = MemoryHelper::ResolveRelativeAddress(targetMemoryLocation_NextWeapon, 0xB);
	targetMemoryLocation_PreviousWeapon = MemoryHelper::ResolveRelativeAddress(targetMemoryLocation_PreviousWeapon, 0x14);

	if (targetMemoryLocation_AimMgrCtor == 0 ||
		targetMemoryLocation_UpdateWeaponModel == 0 ||
		targetMemoryLocation_AnimationClearLock == 0 ||
		targetMemoryLocation_SetAnimProp == 0 ||
		targetMemoryLocation_InitAnimations == 0 ||
		targetMemoryLocation_GetWeaponSlot == 0 ||
		targetMemoryLocation_NextWeapon == 0 ||
		targetMemoryLocation_PreviousWeapon == 0 ||
		targetMemoryLocation_kAP_ACT_Fire_Id == 0) {
		return;
	}

	HookHelper::ApplyHook((void*)targetMemoryLocation_AimMgrCtor, &AimMgrCtor_Hook, (LPVOID*)&AimMgrCtor);
	HookHelper::ApplyHook((void*)targetMemoryLocation_UpdateWeaponModel, &UpdateWeaponModel_Hook, (LPVOID*)&UpdateWeaponModel);
	HookHelper::ApplyHook((void*)targetMemoryLocation_AnimationClearLock, &AnimationClearLock_Hook, (LPVOID*)&AnimationClearLock);
	HookHelper::ApplyHook((void*)targetMemoryLocation_SetAnimProp, &SetAnimProp_Hook, (LPVOID*)&SetAnimProp);
	HookHelper::ApplyHook((void*)targetMemoryLocation_InitAnimations, &InitAnimations_Hook, (LPVOID*)&InitAnimations);
	HookHelper::ApplyHook((void*)targetMemoryLocation_GetWeaponSlot, &GetWeaponSlot_Hook, (LPVOID*)&GetWeaponSlot);
	HookHelper::ApplyHook((void*)targetMemoryLocation_NextWeapon, &NextWeapon_Hook, (LPVOID*)&NextWeapon);
	HookHelper::ApplyHook((void*)targetMemoryLocation_PreviousWeapon, &PreviousWeapon_Hook, (LPVOID*)&PreviousWeapon);
	g_State.kAP_ACT_Fire_Id = MemoryHelper::ReadMemory<int>(targetMemoryLocation_kAP_ACT_Fire_Id + 0x7);
}

static void ApplyClientFXHook()
{
	if (!HighFPSFixes) return;

	DWORD targetMemoryLocation = ScanModuleSignature(g_State.GameClient, "83 EC 20 56 57 8B F1 E8 ?? ?? ?? ?? 8A 44 24 30", "LoadFxDll");

	if (targetMemoryLocation != 0)
	{
		HookHelper::ApplyHook((void*)targetMemoryLocation, &LoadFxDll_Hook, (LPVOID*)&LoadFxDll);
	}
}

static void ApplyDisableHipFireAccuracyPenalty()
{
	if (!DisableHipFireAccuracyPenalty) return;

	DWORD targetMemoryLocation = ScanModuleSignature(g_State.GameClient, "83 EC ?? A1 ?? ?? ?? ?? 8B 40 28 56 57 6A 00 8B F1", "DisableHipFireAccuracyPenalty");

	if (targetMemoryLocation != 0)
	{
		HookHelper::ApplyHook((void*)targetMemoryLocation, &AccuracyMgrUpdate_Hook, (LPVOID*)&AccuracyMgrUpdate);
	}
}

static void ApplyGameDatabaseHook()
{
	if (!HUDScaling) return;

	DWORD targetMemoryLocation_GameDatabase = ScanModuleSignature(g_State.GameClient, "8B 5E 08 55 E8 ?? ?? ?? FF 8B 0D ?? ?? ?? ?? 8B 39 68 ?? ?? ?? ?? 6A 00 68 ?? ?? ?? ?? 53 FF 57", "HUDScaling_GameDatabase");

	if (targetMemoryLocation_GameDatabase != 0)
	{
		int pDB = MemoryHelper::ReadMemory<int>(targetMemoryLocation_GameDatabase + 0xB);
		int pGameDatabase = MemoryHelper::ReadMemory<int>(pDB);
		int pLayoutDB = MemoryHelper::ReadMemory<int>(pGameDatabase);

		HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x58), &DBGetRecord_Hook, (LPVOID*)&DBGetRecord);
		HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x7C), &DBGetInt32_Hook, (LPVOID*)&DBGetInt32);
		HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x80), &DBGetFloat_Hook, (LPVOID*)&DBGetFloat);
		//HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x84), &DBGetString_Hook, (LPVOID*)&DBGetString);
	}
}

static void ApplyClientPatchSet1()
{
	if (!HUDScaling && !XInputControllerSupport) return;

	DWORD targetMemoryLocation_HUDWeaponListUpdateTriggerNames = ScanModuleSignature(g_State.GameClient, "56 32 DB 89 44 24 0C BE 1E 00 00 00", "HUDWeaponListUpdateTriggerNames");
	DWORD targetMemoryLocation_HUDGrenadeListUpdateTriggerNames = ScanModuleSignature(g_State.GameClient, "56 32 DB 89 44 24 0C BE 28 00 00 00", "HUDGrenadeListUpdateTriggerNames");

	if (targetMemoryLocation_HUDWeaponListUpdateTriggerNames == 0 ||
		targetMemoryLocation_HUDGrenadeListUpdateTriggerNames == 0) {
		return;
	}

	HookHelper::ApplyHook((void*)(targetMemoryLocation_HUDWeaponListUpdateTriggerNames - 0x10), &HUDWeaponListUpdateTriggerNames_Hook, (LPVOID*)&HUDWeaponListUpdateTriggerNames);
	HookHelper::ApplyHook((void*)(targetMemoryLocation_HUDGrenadeListUpdateTriggerNames - 0x10), &HUDGrenadeListUpdateTriggerNames_Hook, (LPVOID*)&HUDGrenadeListUpdateTriggerNames);
}

static void ApplyClientPatchSet2()
{
	if (!WeaponFixes && !EnableCustomMaxWeaponCapacity) return;

	DWORD targetMemoryLocation_OnEnterWorld = ScanModuleSignature(g_State.GameClient, "8B F1 E8 ?? ?? ?? ?? DD 05 ?? ?? ?? ?? 8B 96", "OnEnterWorld", 1);

	if (targetMemoryLocation_OnEnterWorld != 0)
	{
		HookHelper::ApplyHook((void*)targetMemoryLocation_OnEnterWorld, &OnEnterWorld_Hook, (LPVOID*)&OnEnterWorld);
	}
}

static void ApplyClientPatch()
{
	ApplyHighFPSFixesClientPatch();
	ApplyMouseAimMultiplierClientPatch();
	ApplyXPWidescreenClientPatch();
	ApplySkipSplashScreenClientPatch();
	ApplyDisableLetterboxClientPatch();
	ApplyPersistentWorldClientPatch();
	ApplyInfiniteFlashlightClientPatch();
	ApplyXInputControllerClientPatch();
	ApplyHUDScalingClientPatch();
	ApplySetWeaponCapacityClientPatch();
	ApplyHighResolutionReflectionsClientPatch();
	ApplyAutoResolutionClientCheck();
	ApplyKeyboardInputLanguageClientCheck();
	ApplyWeaponFixesClientPatch();
	ApplyClientFXHook();
	ApplyDisableHipFireAccuracyPenalty();
	ApplyGameDatabaseHook();
	ApplyClientPatchSet1();
	ApplyClientPatchSet2();
}

#pragma endregion

#pragma region Server Hooks

// ==============================
//  EnableCustomMaxWeaponCapacity
// ==============================

static void __fastcall SetWeaponCapacityServer_Hook(int thisPtr, int, uint8_t nCap)
{
	SetWeaponCapacityServer(thisPtr, MaxWeaponCapacity);
}

static void __fastcall PlayerInventoryInit_Hook(int thisPtr, int, int nCap)
{
	g_State.CPlayerInventory = thisPtr;
	PlayerInventoryInit(thisPtr, nCap);
}

#pragma endregion

#pragma region Server Patches

static void ApplyTrackedHook(DWORD address, LPVOID hookFunc, LPVOID *originalPtr)
{
	HookHelper::ApplyHook((void*)address, hookFunc, originalPtr);
	g_State.hookedServerFunctionAddresses.push_back(address);
}

static void ApplyPersistentWorldServerPatch()
{
	if (!EnablePersistentWorldState) return;

	DWORD targetMemoryLocation_BodyFading = ScanModuleSignature(g_State.GameServer, "8A 86 ?? ?? 00 00 84 C0 74 A1 8D 8E", "BodyFading");

	if (targetMemoryLocation_BodyFading != 0)
	{
		MemoryHelper::WriteMemory<uint8_t>(targetMemoryLocation_BodyFading + 0x22, 0x75, true);
	}
}

static void ApplySetWeaponCapacityServerPatch()
{
	if (!EnableCustomMaxWeaponCapacity || !g_State.appliedCustomMaxWeaponCapacity) return;

	DWORD targetMemoryLocation_SetWeaponCapacityServer = ScanModuleSignature(g_State.GameServer, "56 8B F1 8B 56 18 85 D2 8D 4E 14 57 75", "SetWeaponCapacityServer");
	DWORD targetMemoryLocation_PlayerInventoryInit = ScanModuleSignature(g_State.GameServer, "33 DB 3B CB 89 ?? 0C 74", "PlayerInventoryInit", 2);

	if (targetMemoryLocation_SetWeaponCapacityServer == 0 ||
		targetMemoryLocation_PlayerInventoryInit == 0) {
		return;
	}

	ApplyTrackedHook(targetMemoryLocation_SetWeaponCapacityServer, &SetWeaponCapacityServer_Hook, (LPVOID*)&SetWeaponCapacityServer);
	ApplyTrackedHook(targetMemoryLocation_PlayerInventoryInit, &PlayerInventoryInit_Hook, (LPVOID*)&PlayerInventoryInit);
}

static void ApplyServerPatch()
{
	if (g_State.hookedServerFunctionAddresses.size() != 0) return;

	ApplyPersistentWorldServerPatch();
	ApplySetWeaponCapacityServerPatch();
}

#pragma endregion

#pragma region Controller Handling

static void HandleControllerButton(WORD button, int commandId)
{
	auto& btnState = g_Controller.gameButtons[button];
	bool isPressed;

	// Activate instead of Reload
	if (commandId == 88 && (g_State.canActivate || g_State.canSwap || g_State.isOperatingTurret))
	{
		commandId = 87;
	}

	if (button == XINPUT_GAMEPAD_LEFT_TRIGGER)
	{
		// Left trigger
		isPressed = g_Controller.state.Gamepad.bLeftTrigger > 30;
	}
	else if (button == XINPUT_GAMEPAD_RIGHT_TRIGGER)
	{
		// Right trigger
		isPressed = g_Controller.state.Gamepad.bRightTrigger > 30;
	}
	else
	{
		// Regular button
		isPressed = (g_Controller.state.Gamepad.wButtons & button) != 0;
	}

	// Check if we recently transitioned from menu to prevent immediate actions
	ULONGLONG currentTime = GetTickCount64();
	if (g_Controller.menuToGameTransitionTime > 0 && currentTime - g_Controller.menuToGameTransitionTime < 200) // 200ms grace period
	{
		// During transition period, only register button state but don't execute commands
		btnState.isPressed = isPressed;
		return;
	}

	if (isPressed && !btnState.isPressed)
	{
		// Press Escape to show the menu
		if (commandId == -1)
		{
			PostMessage(g_State.hWnd, WM_KEYDOWN, VK_ESCAPE, 0);
		}
		else
		{
			g_Controller.commandActive[commandId] = true;
			OnCommandOn(g_State.g_pGameClientShell, commandId);
			btnState.wasHandled = true;
			btnState.pressStartTime = GetTickCount64();
		}
	}
	else if (isPressed)
	{
		if (commandId != -1)
		{
			g_Controller.commandActive[commandId] = true;
		}
	}
	else if (!isPressed && btnState.isPressed)
	{
		if (commandId == -1)
		{
			PostMessage(g_State.hWnd, WM_KEYUP, VK_ESCAPE, 0);
		}
		else
		{
			g_Controller.commandActive[commandId] = false;
			OnCommandOff(g_State.g_pGameClientShell, commandId);
			btnState.wasHandled = false;
		}
	}

	btnState.isPressed = isPressed;
}

// Main function to poll controller and process all button mappings
static void PollController()
{
	// Save previous connection state
	bool wasConnected = g_Controller.isConnected;

	// Update controller state once per frame
	g_Controller.isConnected = XInputGetState(0, &g_Controller.state) == ERROR_SUCCESS;

	// Check for connection state change
	if (wasConnected != g_Controller.isConnected)
	{
		HUDWeaponListUpdateTriggerNames(g_State.CHUDWeaponList);
		HUDGrenadeListUpdateTriggerNames(g_State.CHUDGrenadeList);
		HUDSwapUpdateTriggerName();
	}

	// Handle transition from gameplay to menu
	static bool wasPlaying = g_State.isPlaying;
	if (wasPlaying && !g_State.isPlaying && g_Controller.isConnected)
	{
		for (const auto& mapping : g_buttonMappings)
		{
			WORD button = mapping.first;
			int commandId = mapping.second;
			auto& btnState = g_Controller.gameButtons[button];

			if (btnState.isPressed)
			{
				if (commandId == -1) // Start button
				{
					PostMessage(g_State.hWnd, WM_KEYUP, VK_ESCAPE, 0);
				}
				else
				{
					g_Controller.commandActive[commandId] = false;
					OnCommandOff(g_State.g_pGameClientShell, commandId);
				}
				btnState.isPressed = false;
				btnState.wasHandled = false;
			}
		}
	}

	if (!wasPlaying && g_State.isPlaying && g_Controller.isConnected)
	{
		// Set transition timestamp to prevent immediate actions
		g_Controller.menuToGameTransitionTime = GetTickCount64();

		// Clear any lingering menu button states
		for (int i = 0; i < 6; i++)
		{
			g_Controller.menuButtons[i].isPressed = false;
			g_Controller.menuButtons[i].pressStartTime = 0;
			g_Controller.menuButtons[i].lastRepeatTime = 0;
		}
	}

	wasPlaying = g_State.isPlaying;

	if (!g_Controller.isConnected)
	{
		memset(g_Controller.commandActive, 0, sizeof(g_Controller.commandActive));
		return;
	}

	// Menu navigation
	if (!g_State.isPlaying)
	{
		const ULONGLONG currentTime = GetTickCount64();

		for (int i = 0; i < 6; i++)
		{
			auto& btnState = g_Controller.menuButtons[i];
			bool pressed = false;

			if (i < 4) // D-Pad
			{
				const bool buttonPressed = (g_Controller.state.Gamepad.wButtons & MENU_NAVIGATION_MAP[i][0]);
				bool joystickPressed = false;

				switch (i)
				{
					case 0: // Up
						joystickPressed = (g_Controller.state.Gamepad.sThumbLY > 16384) || (g_Controller.state.Gamepad.sThumbRY > 16384);
						break;
					case 1: // Down
						joystickPressed = (g_Controller.state.Gamepad.sThumbLY < -16384) || (g_Controller.state.Gamepad.sThumbRY < -16384);
						break;
					case 2: // Left
						joystickPressed = (g_Controller.state.Gamepad.sThumbLX < -16384) || (g_Controller.state.Gamepad.sThumbRX < -16384);
						break;
					case 3: // Right
						joystickPressed = (g_Controller.state.Gamepad.sThumbLX > 16384) || (g_Controller.state.Gamepad.sThumbRX > 16384);
						break;
				}

				pressed = buttonPressed || joystickPressed;

				// Handle auto-repeat
				if (pressed)
				{
					if (!btnState.isPressed)
					{
						// Initial press
						btnState.pressStartTime = currentTime;
						btnState.lastRepeatTime = currentTime;
					}
					else
					{
						// Calculate time since last valid input
						DWORD elapsedSinceStart = currentTime - btnState.pressStartTime;
						DWORD elapsedSinceLastRepeat = currentTime - btnState.lastRepeatTime;

						if (elapsedSinceStart > 500 && elapsedSinceLastRepeat > 100)
						{
							// Trigger repeat
							PostMessage(g_State.hWnd, WM_KEYDOWN, MENU_NAVIGATION_MAP[i][1], 0);
							btnState.lastRepeatTime = currentTime;
						}
					}
				}
			}
			else // A/B
			{
				pressed = (g_Controller.state.Gamepad.wButtons & MENU_NAVIGATION_MAP[i][0]);
			}

			// Handle state changes
			if (pressed != btnState.isPressed)
			{
				PostMessage(g_State.hWnd, pressed ? WM_KEYDOWN : WM_KEYUP, MENU_NAVIGATION_MAP[i][1], 0);
				btnState.isPressed = pressed;

				// Reset timing on release
				if (!pressed)
				{
					btnState.pressStartTime = 0;
					btnState.lastRepeatTime = 0;
				}
			}
		}

		// Handle shoulder buttons
		auto UpdateScreenPerformanceSetting = [&](DWORD button, auto& btnState, int direction)
		{
			bool pressed = (g_Controller.state.Gamepad.wButtons & button);
			if (pressed != btnState.isPressed)
			{
				if (pressed)
				{
					if (g_State.pCurrentType != 0 && g_State.maxCurrentType != -1)
					{
						g_State.currentType = (g_State.currentType + direction + g_State.maxCurrentType) % g_State.maxCurrentType;
						SetCurrentType(g_State.pCurrentType, g_State.currentType);
					}
				}
				btnState.isPressed = pressed;
			}
		};

		UpdateScreenPerformanceSetting(XINPUT_GAMEPAD_LEFT_SHOULDER, g_Controller.leftShoulderState, -1);
		UpdateScreenPerformanceSetting(XINPUT_GAMEPAD_RIGHT_SHOULDER, g_Controller.rightShoulderState, 1);
	}
	// Handle in-game controls
	else
	{
		// Reset command states
		memset(g_Controller.commandActive, 0, sizeof(g_Controller.commandActive));

		// Process buttons
		for (const auto& mapping : g_buttonMappings)
		{
			HandleControllerButton(mapping.first, mapping.second);
		}
	}

	// Hook 'LoadString' to override menu strings
	if (!g_State.hookedLoadString)
	{
		g_State.hookedLoadString = true;

		DWORD targetMemoryLocation = ScanModuleSignature(g_State.GameClient, "8B 4C 24 18 03 C1 8B 0D ?? ?? ?? ?? 03 F7 85 C9", "LoadString", -1, false);

		if (targetMemoryLocation != 0)
		{
			int StringEditRuntimePtr = MemoryHelper::ReadMemory<int>(targetMemoryLocation + 0x8);
			int StringEditRuntime = MemoryHelper::ReadMemory<int>(StringEditRuntimePtr);
			int vTable = MemoryHelper::ReadMemory<int>(StringEditRuntime);
			int pLoadString = MemoryHelper::ReadMemory<int>(vTable + 0x1C);

			HookHelper::ApplyHook((void*)pLoadString, &LoadGameString_Hook, (LPVOID*)&LoadGameString);
		}
	}
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

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
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
					return FALSE;
			}

			SystemHelper::LoadProxyLibrary();
			HookHelper::ApplyHookAPI(L"user32.dll", "CreateWindowExA", &CreateWindowExA_Hook, (LPVOID*)&ori_CreateWindowExA);
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			MH_Uninitialize();
			break;
		}
	}
	return TRUE;
}

#pragma endregion