#define MINI_CASE_SENSITIVE
#define NOMINMAX

#include <Windows.h>
#include <Xinput.h>
#include <unordered_set>
#include <map>

#include "MinHook.hpp"
#include "ini.hpp"
#include "dllmain.hpp"
#include "helper.hpp"
#include "FpsLimiter.hpp"

#pragma comment(lib, "libMinHook.x86.lib")
#pragma comment(lib, "Xinput.lib")

// Disclaimer: The following code is chaotic and will be cleaned up at a later time

// =============================
// Original Function Pointers
// =============================
intptr_t(__cdecl* LoadGameDLL)(char*, char, DWORD*) = nullptr;
char(__thiscall* LoadClientFXDLL)(int, char*, char) = nullptr;
int(__stdcall* SetConsoleVariableFloat)(const char*, float) = nullptr;
int(__thiscall* FindStringCaseInsensitive)(DWORD*, char*) = nullptr;
void(__thiscall* HUDTerminate)(int) = nullptr;
char(__thiscall* HUDInit)(int) = nullptr;
void(__thiscall* HUDRender)(int, int) = nullptr;
int(__thiscall* HUDWeaponListReset)(int) = nullptr;
int(__thiscall* HUDWeaponListUpdateTriggerNames)(int) = nullptr;
int(__thiscall* HUDGrenadeListUpdateTriggerNames)(int) = nullptr;
bool(__thiscall* HUDWeaponListInit)(int) = nullptr;
bool(__thiscall* HUDGrenadeListInit)(int) = nullptr;
int(__thiscall* ScreenDimsChanged)(int) = nullptr;
DWORD* (__stdcall* LayoutDBGetPosition)(DWORD*, int, char*, int) = nullptr;
float* (__stdcall* GetRectF)(DWORD*, int, char*, int) = nullptr;
int(__stdcall* LayoutDBGetRecord)(int, char*) = nullptr;
int(__stdcall* LayoutDBGetInt32)(int, unsigned int, int) = nullptr;
float(__stdcall* LayoutDBGetFloat)(int, unsigned int, float) = nullptr;
const char*(__stdcall* LayoutDBGetString)(int, unsigned int, int) = nullptr;
int(__thiscall* UpdateSlider)(int, int) = nullptr;
float(__stdcall* GetShatterLifetime)(int) = nullptr;
int(__thiscall* IsFrameComplete)(int) = nullptr;
int(__stdcall* CreateFX)(char*, int, int) = nullptr;
int(__thiscall* GetDeviceObjectDesc)(int, unsigned int, wchar_t*, unsigned int*) = nullptr;
void(__thiscall* PauseGame)(int, bool, bool) = nullptr;
bool(__thiscall* IsCommandOn)(int, int) = nullptr;
bool(__thiscall* OnCommandOn)(int, int) = nullptr;
bool(__thiscall* OnCommandOff)(int, int) = nullptr;
double(__thiscall* GetExtremalCommandValue)(int, int) = nullptr;
int(__thiscall* HUDActivateObjectSetObject)(int, void**, int, int, int, int) = nullptr;
int(__thiscall* SetOperatingTurret)(int, int) = nullptr;
const wchar_t*(__thiscall* GetTriggerNameFromCommandID)(int, int) = nullptr;
void(__thiscall* UseCursor)(int, bool) = nullptr;
bool(__thiscall* OnMouseMove)(int, int, int) = nullptr;
uint8_t(__thiscall* GetWeaponCapacity)(int) = nullptr;
void(__thiscall* SetWeaponCapacityServer)(int, uint8_t) = nullptr;
void(__thiscall* PlayerInventoryInit)(int, int) = nullptr;
void(__thiscall* OnEnterWorld)(int) = nullptr;
void(__thiscall* DisconnectFromServer)(int) = nullptr;
bool(__thiscall* RenderTargetGroupFXInit)(int, int) = nullptr;
DWORD*(__thiscall* AddParticleBatchMarker)(int, int, bool) = nullptr;
DWORD*(__thiscall* EmitParticleBatch)(int, float, int, int*) = nullptr;
int(__thiscall* StepPhysicsSimulation)(int, float*) = nullptr;
void(__cdecl* AutoDetectPerformanceSettings)() = nullptr;
void(__stdcall* InitAdditionalTextureData)(int, int, int*, DWORD*, DWORD*, float) = nullptr;
void(__thiscall* HUDSwapUpdate)(int) = nullptr;
void(__thiscall* HUDPausedInit)(int) = nullptr;
void(__thiscall* SwitchToScreen)(int, int) = nullptr;
void(__thiscall* SetCurrentType)(int, int) = nullptr;
const wchar_t*(__stdcall* LoadGameString)(int, char*) = nullptr;
void(__cdecl* HUDSwapUpdateTriggerName)() = nullptr;
HWND(WINAPI* ori_CreateWindowExA)(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);

// =============================
// Constants 
// =============================
const int FEAR_TIMESTAMP = 0x44EF6AE6;
const int FEARMP_TIMESTAMP = 0x44EF6ADB;
const int FEARXP_TIMESTAMP = 0x450B3629;
const int FEARXP_TIMESTAMP2 = 0x450DA808;
const int FEARXP2_TIMESTAMP = 0x46FC10A3;

constexpr float BASE_WIDTH = 960.0f;
constexpr float BASE_HEIGHT = 720.0f;

#define XINPUT_GAMEPAD_LEFT_TRIGGER   0x400
#define XINPUT_GAMEPAD_RIGHT_TRIGGER  0x800

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
	FEARGAME CurrentFEARGame;
	HWND hWnd = 0;

	int currentWidth = 0;
	int currentHeight = 0;
	float scalingFactor = 0;
	float scalingFactorText = 0;
	float scalingFactorCrosshair = 0;
	float crosshairSize = 0;
	int CHUDMgr = 0;
	int CHUDPaused = 0;
	int CHUDWeaponList = 0;
	int CHUDGrenadeList = 0;
	bool updateHUD = false;

	int screenWidth = 0;
	int screenHeight = 0;
	bool isInAutoDetect = false;

	bool isClientLoaded = false;
	HMODULE GameClient = NULL;
	HMODULE GameServer = NULL;
	HMODULE GameClientFX = NULL;

	bool isUsingFpsLimiter = false;
	FpsLimiter fpsLimiter{ 120.0f };

	bool updateLayoutReturnValue = false;
	bool slowMoBarUpdated = false;
	int healthAdditionalIntIndex = 0;
	int int32ToUpdate = 0;
	float floatToUpdate = 0;
	bool crosshairSliderUpdated = false;

	int g_pGameClientShell = 0;
	bool isGamePaused = true;
	bool canActivate = false;
	bool canSwap = false;
	bool isOperatingTurret = false;
	int pCurrentType = 0;
	int currentType = 0;
	int maxCurrentType = 0;
	bool hookedLoadString = false;

	int CPlayerInventory = 0;
	bool appliedCustomMaxWeaponCapacity = false;

	struct DataEntry
	{
		int TextSize;
		int ScaleType;
	};

	static std::unordered_map<std::string, DataEntry> textDataMap;
	static std::unordered_map<std::string, std::unordered_set<std::string>> hudScalingRules;

	std::list<DWORD> hookedServerFunctionAddresses;
};

// Initialize static members
std::unordered_map<std::string, GlobalState::DataEntry> GlobalState::textDataMap;
std::unordered_map<std::string, std::unordered_set<std::string>> GlobalState::hudScalingRules;

// Global instance
GlobalState gState;

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
	{ XINPUT_GAMEPAD_B, 19 }, // Melee
	{ XINPUT_GAMEPAD_X, 88 }, // Reload
	{ XINPUT_GAMEPAD_Y, 70 }, // Medkit
	{ XINPUT_GAMEPAD_LEFT_THUMB, 14 }, // Crouch
	{ XINPUT_GAMEPAD_RIGHT_THUMB, 71 }, // Zoom
	{ XINPUT_GAMEPAD_LEFT_SHOULDER, 106 }, // SlowMo
	{ XINPUT_GAMEPAD_RIGHT_SHOULDER, 77 }, // Next Weapon
	{ XINPUT_GAMEPAD_DPAD_UP, 73 }, // Next Grenade
	{ XINPUT_GAMEPAD_DPAD_DOWN, 114 }, // Flashlight
	{ XINPUT_GAMEPAD_DPAD_LEFT, 20 }, // Lean Left
	{ XINPUT_GAMEPAD_DPAD_RIGHT, 21 }, // Lean Right
	{ XINPUT_GAMEPAD_LEFT_TRIGGER, 81 }, // Throw Grenade
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
bool FixHighFPSPhysics = false;
bool FixParticleLifetimeCalculation = false;
bool FixParticleUpdateThreshold = false;
bool DisableXPWidescreenFiltering = false;
bool FixKeyboardInputLanguage = false;
bool CheckLAAPatch = false;

// Controller
bool XInputControllerSupport = false;
bool HideMouseCursor = false;
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

// Graphics
float MaxFPS = 0;
bool HighResolutionReflections = false;
bool NoLODBias = false;
bool ReducedMipMapBias = false;
bool EnablePersistentWorldState = false;

// Display
bool HUDScaling = false;
float HUDCustomScalingFactor = 0;
float SmallTextCustomScalingFactor = 0;
bool AutoResolution = false;
bool DisableLetterbox = false;

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
bool InfiniteFlashlight = false;
bool EnableCustomMaxWeaponCapacity = false;
int MaxWeaponCapacity = 0;
bool ShowErrors = false;

static void ReadConfig()
{
	IniHelper::Init(gState.CurrentFEARGame == FEARXP || gState.CurrentFEARGame == FEARXP2);

	// Fixes
	DisableRedundantHIDInit = IniHelper::ReadInteger("Fixes", "DisableRedundantHIDInit", 1) == 1;
	FixHighFPSPhysics = IniHelper::ReadInteger("Fixes", "FixHighFPSPhysics", 1) == 1;
	FixParticleLifetimeCalculation = IniHelper::ReadInteger("Fixes", "FixParticleLifetimeCalculation", 1) == 1;
	FixParticleUpdateThreshold = IniHelper::ReadInteger("Fixes", "FixParticleUpdateThreshold", 1) == 1;
	DisableXPWidescreenFiltering = IniHelper::ReadInteger("Fixes", "DisableXPWidescreenFiltering", 1) == 1;
	FixKeyboardInputLanguage = IniHelper::ReadInteger("Fixes", "FixKeyboardInputLanguage", 1) == 1;
	CheckLAAPatch = IniHelper::ReadInteger("Fixes", "CheckLAAPatch", 0) == 1;

	// Controller
	XInputControllerSupport = IniHelper::ReadInteger("Controller", "XInputControllerSupport", 1) == 1;
	HideMouseCursor = IniHelper::ReadInteger("Controller", "HideMouseCursor", 1) == 1;
	GAMEPAD_A = IniHelper::ReadInteger("Controller", "GAMEPAD_A", 15);
	GAMEPAD_B = IniHelper::ReadInteger("Controller", "GAMEPAD_B", 19);
	GAMEPAD_X = IniHelper::ReadInteger("Controller", "GAMEPAD_X", 88);
	GAMEPAD_Y = IniHelper::ReadInteger("Controller", "GAMEPAD_Y", 70);
	GAMEPAD_LEFT_THUMB = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_THUMB", 14);
	GAMEPAD_RIGHT_THUMB = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_THUMB", 71);
	GAMEPAD_LEFT_SHOULDER = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_SHOULDER", 106);
	GAMEPAD_RIGHT_SHOULDER = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_SHOULDER", 77);
	GAMEPAD_DPAD_UP = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_UP", 73);
	GAMEPAD_DPAD_DOWN = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_DOWN", 114);
	GAMEPAD_DPAD_LEFT = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_LEFT", 20);
	GAMEPAD_DPAD_RIGHT = IniHelper::ReadInteger("Controller", "GAMEPAD_DPAD_RIGHT", 21);
	GAMEPAD_LEFT_TRIGGER = IniHelper::ReadInteger("Controller", "GAMEPAD_LEFT_TRIGGER", 81);
	GAMEPAD_RIGHT_TRIGGER = IniHelper::ReadInteger("Controller", "GAMEPAD_RIGHT_TRIGGER", 17);
	GAMEPAD_BACK = IniHelper::ReadInteger("Controller", "GAMEPAD_BACK", 78);

	// Graphics
	MaxFPS = IniHelper::ReadFloat("Graphics", "MaxFPS", 120.0f);
	HighResolutionReflections = IniHelper::ReadInteger("Graphics", "HighResolutionReflections", 1) == 1;
	NoLODBias = IniHelper::ReadInteger("Graphics", "NoLODBias", 1) == 1;
	ReducedMipMapBias = IniHelper::ReadInteger("Graphics", "ReducedMipMapBias", 1) == 1;
	EnablePersistentWorldState = IniHelper::ReadInteger("Graphics", "EnablePersistentWorldState", 1) == 1;

	// Display
	HUDScaling = IniHelper::ReadInteger("Display", "HUDScaling", 1) == 1;
	HUDCustomScalingFactor = IniHelper::ReadFloat("Display", "HUDCustomScalingFactor", 1.0f);
	SmallTextCustomScalingFactor = IniHelper::ReadFloat("Display", "SmallTextCustomScalingFactor", 1.0f);
	AutoResolution = IniHelper::ReadInteger("Display", "AutoResolution", 1) == 1;
	DisableLetterbox = IniHelper::ReadInteger("Display", "DisableLetterbox", 0) == 1;

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
	InfiniteFlashlight = IniHelper::ReadInteger("Extra", "InfiniteFlashlight", 0) == 1;
	EnableCustomMaxWeaponCapacity = IniHelper::ReadInteger("Extra", "EnableCustomMaxWeaponCapacity", 0) == 1;
	MaxWeaponCapacity = IniHelper::ReadInteger("Extra", "MaxWeaponCapacity", 3);
	ShowErrors = IniHelper::ReadInteger("Extra", "ShowErrors", 1) == 1;

	// Get screen resolution
	if (AutoResolution)
	{
		auto [screenWidth, screenHeight] = SystemHelper::GetScreenResolution();
		gState.screenWidth = screenWidth;
		gState.screenHeight = screenHeight;
	}

	// Check if we should skip everything directly
	if (!SkipAllIntro && gState.CurrentFEARGame == FEAR || gState.CurrentFEARGame == FEARMP)
	{
		SkipAllIntro = SkipSierraIntro && SkipMonolithIntro && SkipWBGamesIntro && SkipNvidiaIntro;
	}
	else if (!SkipAllIntro && gState.CurrentFEARGame == FEARXP || gState.CurrentFEARGame == FEARXP2)
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
		gState.textDataMap["Default10"] = { 12, 0 };
		gState.textDataMap["Default12"] = { 12, 0 };
		gState.textDataMap["Default14"] = { 14, 0 };
		gState.textDataMap["Default16"] = { 16, 0 };
		gState.textDataMap["Default18"] = { 18, 0 };
		gState.textDataMap["Default24"] = { 24, 0 };
		gState.textDataMap["Default32"] = { 32, 0 };
		gState.textDataMap["EndCredits16"] = { 16, 0 };
		gState.textDataMap["EndCredits18"] = { 18, 0 };
		gState.textDataMap["Intro16"] = { 16, 0 };
		gState.textDataMap["Objective"] = { 22, 0 };
		gState.textDataMap["Training"] = { 22, 0 };

		// Text: Interface\HUD
		gState.textDataMap["HUDActivate"] = { 18, 0 };
		gState.textDataMap["HUDActivateObject"] = { 14, 0 };
		gState.textDataMap["HUDAmmo"] = { 16, 0 };
		gState.textDataMap["HUDArmor"] = { 25, 0 };
		gState.textDataMap["HUDBuildVersion"] = { 12, 0 };
		gState.textDataMap["HUDChatInput"] = { 16, 2 };
		gState.textDataMap["HUDChatMessage"] = { 14, 2 };
		gState.textDataMap["HUDControlPoint"] = { 24, 0 };
		gState.textDataMap["HUDControlPointBar"] = { 24, 0 };
		gState.textDataMap["HUDControlPointList"] = { 24, 0 };
		gState.textDataMap["HUDCrosshair"] = { 12, 2 };
		gState.textDataMap["HUDCTFBaseEnemy"] = { 14, 0 };
		gState.textDataMap["HUDCTFBaseFriendly"] = { 14, 0 };
		gState.textDataMap["HUDCTFFlag"] = { 14, 0 };
		gState.textDataMap["HUDDamageDir"] = { 16, 0 };
		gState.textDataMap["HUDDebug"] = { 12, 0 };
		gState.textDataMap["HUDDecision"] = { 16, 0 };
		gState.textDataMap["HUDDialogue"] = { 13, 1 };
		gState.textDataMap["HUDDistance"] = { 16, 0 };
		gState.textDataMap["HUDEditorPosition"] = { 12, 0 };
		gState.textDataMap["HudEndRoundMessage"] = { 32, 0 };
		gState.textDataMap["HUDFocus"] = { 16, 0 };
		gState.textDataMap["HUDGameMessage"] = { 14, 2 };
		gState.textDataMap["HUDGear"] = { 14, 0 };
		gState.textDataMap["HUDGrenade"] = { 14, 0 };
		gState.textDataMap["HUDGrenadeList"] = { 14, 0 };
		gState.textDataMap["HUDHealth"] = { 25, 0 };
		gState.textDataMap["HUDLadder"] = { 18, 0 };
		gState.textDataMap["HUDObjective"] = { 22, 0 };
		gState.textDataMap["HUDPaused"] = { 20, 0 };
		gState.textDataMap["HUDPlayerList"] = { 14, 0 };
		gState.textDataMap["HUDRadio"] = { 16, 0 };
		gState.textDataMap["HUDRespawn"] = { 18, 0 };
		gState.textDataMap["HUDScoreDiff"] = { 16, 0 };
		gState.textDataMap["HUDScores"] = { 14, 0 };
		gState.textDataMap["HUDSpectator"] = { 12, 0 };
		gState.textDataMap["HUDSubtitle"] = { 14, 2 };
		gState.textDataMap["HUDSwap"] = { 12, 2 };
		gState.textDataMap["HUDTeamScoreControl"] = { 24, 0 };
		gState.textDataMap["HUDTeamScoreCTF"] = { 14, 0 };
		gState.textDataMap["HUDTeamScoreTDM"] = { 14, 0 };
		gState.textDataMap["HUDTimerMain"] = { 14, 0 };
		gState.textDataMap["HUDTimerSuddenDeath"] = { 14, 0 };
		gState.textDataMap["HUDTimerTeam0"] = { 18, 0 };
		gState.textDataMap["HUDTimerTeam1"] = { 18, 0 };
		gState.textDataMap["HUDTransmission"] = { 16, 0 };
		gState.textDataMap["HUDTurret"] = { 18, 0 };
		gState.textDataMap["HUDVote"] = { 16, 0 };
		gState.textDataMap["HUDWeapon"] = { 14, 0 };

		// HUD
		gState.hudScalingRules = 
		{
			{"HUDHealth",            {"AdditionalPoint", "IconSize", "IconOffset", "TextOffset"}},
			{"HUDDialogue",          {"AdditionalPoint", "IconSize", "TextOffset"}},
			{"HUDGrenadeList",       {"AdditionalPoint", "IconSize", "TextOffset"}},
			{"HUDWeapon",            {"AdditionalPoint", "IconSize", "TextOffset"}},
			{"HUDArmor",             {"IconSize", "IconOffset", "TextOffset"}},
			{"HUDSwap",              {"IconSize", "IconOffset", "TextOffset"}},
			{"HUDGear",              {"IconSize", "IconOffset", "TextOffset"}},
			{"HUDGrenade",           {"IconSize", "IconOffset", "TextOffset"}},
			{"HUDAmmo",              {"IconSize", "IconOffset", "TextOffset"}},
			{"HUDFlashlight",        {"IconSize", "IconOffset"}},
			{"HUDSlowMo2",           {"IconSize", "IconOffset"}},
			{"HUDActivateObject",    {"TextOffset"}}
		};
	}

	// 10 slots max
	MaxWeaponCapacity = std::clamp(MaxWeaponCapacity, 0, 10);
}

#pragma region Helper

static DWORD ScanModuleSignature(HMODULE module, std::string_view signature, const char* patchName, int functionStartCheckCount = -1)
{
	DWORD targetMemoryLocation = MemoryHelper::PatternScan(module, signature);

	if (targetMemoryLocation == 0)
	{
		if (ShowErrors)
		{
			std::string errorMessage = "Error: Unable to find signature for patch: ";
			errorMessage += patchName;
			MessageBoxA(NULL, errorMessage.c_str(), "EchoPatch", MB_ICONERROR);
		}
		return 0;
	}

	if (functionStartCheckCount >= 0)
	{
		DWORD addr = targetMemoryLocation;
		for (int i = 0; i < 0x1000; i++)
		{
			bool valid = true;
			for (int j = 1; j <= functionStartCheckCount; j++)
			{
				if (MemoryHelper::ReadMemory<uint8_t>(addr - j) != 0xCC)
				{
					valid = false;
					break;
				}
			}
			if (valid)
			{
				break;
			}

			addr--;
		}
		targetMemoryLocation = addr;
	}

	return targetMemoryLocation;
}

#pragma endregion

#pragma region ClientFX Hooks

static void ProcessParticleResult(DWORD* result)
{
	// If m_fLifetime == -1
	if (result && result[6] == 0xBF800000)
	{
		// Prevent an issue inside GetParticleSizeAndColor
		result[6] = 0;
	}
}

static DWORD* __fastcall AddParticleBatchMarker_Hook(int thisPtr, int _ECX, int a2, bool a3)
{
	DWORD* result = AddParticleBatchMarker(thisPtr, a2, a3);
	ProcessParticleResult(result);
	return result;
}

static DWORD* __fastcall EmitParticleBatch_Hook(int thisPtr, int _ECX, float a2, int a3, int* a4)
{
	DWORD* result = EmitParticleBatch(thisPtr, a2, a3, a4);
	ProcessParticleResult(result);
	return result;
}

#pragma endregion

#pragma region ClientFX Patches

static void ApplyFixParticleLifetimeClientFXPatch()
{
	if (!FixParticleLifetimeCalculation) return;

	DWORD targetMemoryLocation_AddParticleBatchMarker = ScanModuleSignature(gState.GameClientFX, "83 C1 10 E8 ?? ?? ?? ?? 85 C0 74 24 8A 4C 24 08", "FixParticleLifetime_AddParticleBatchMarker");
	DWORD targetMemoryLocation_EmitParticleBatch = ScanModuleSignature(gState.GameClientFX, "CC CC CC CC 81 EC C0 00 00 00", "FixParticleLifetime_EmitParticleBatch");

	if (targetMemoryLocation_AddParticleBatchMarker == 0 ||
		targetMemoryLocation_EmitParticleBatch == 0) {
		return;
	}

	HookHelper::ApplyHook((void*)targetMemoryLocation_AddParticleBatchMarker, &AddParticleBatchMarker_Hook, (LPVOID*)&AddParticleBatchMarker);
	HookHelper::ApplyHook((void*)(targetMemoryLocation_EmitParticleBatch + 0x4), &EmitParticleBatch_Hook, (LPVOID*)&EmitParticleBatch);
}

static void ApplyFixParticleUpdateThresholdClientFXPatch()
{
	if (!FixParticleUpdateThreshold) return;

	DWORD targetMemoryLocation = ScanModuleSignature(gState.GameClientFX, "9A 99 99 3E 6F 12 83 3A", "FixParticleUpdateThreshold");

	if (targetMemoryLocation == 0) return;

	// Threshold  
	MemoryHelper::WriteMemory<float>(targetMemoryLocation + 0x4, 0.0001f, true);
}

static void ApplyClientFXPatch()
{
	ApplyFixParticleLifetimeClientFXPatch();
	ApplyFixParticleUpdateThresholdClientFXPatch();
}

#pragma endregion

#pragma region Client Hooks

// Scale HUD position or dimension
static DWORD* __stdcall LayoutDBGetPosition_Hook(DWORD* a1, int Record, char* Attribute, int a4)
{
	if (!Record)
		return LayoutDBGetPosition(a1, Record, Attribute, a4);

	DWORD* result = LayoutDBGetPosition(a1, Record, Attribute, a4);
	char* hudRecordString = *(char**)Record;

	std::string hudElement(hudRecordString);
	std::string attribute(Attribute);

	// Check if this HUD element and attribute require scaling
	auto hudEntry = gState.hudScalingRules.find(hudElement);
	if (hudEntry != gState.hudScalingRules.end() && hudEntry->second.count(attribute))
	{
		float scalingFactor = (hudElement == "HUDSwap") ? gState.scalingFactorText : gState.scalingFactor;

		result[0] = static_cast<DWORD>((int)result[0] * scalingFactor);
		result[1] = static_cast<DWORD>((int)result[1] * scalingFactor);
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

	if (strcmp(Attribute, "AdditionalRect") == 0 && (strcmp(hudRecordString, "HUDSlowMo2") == 0 || strcmp(hudRecordString, "HUDFlashlight") == 0))
	{
		result[0] *= gState.scalingFactor;
		result[1] *= gState.scalingFactor;
		result[2] *= gState.scalingFactor;
		result[3] *= gState.scalingFactor;
	}

	return result;
}

// Override 'LayoutDBGetInt32' and other related functions to return specific values
static int __stdcall LayoutDBGetRecord_Hook(int Record, char* Attribute)
{
	if (!Record)
		return LayoutDBGetRecord(Record, Attribute);

	char* hudRecordString = *(char**)Record;

	if (strcmp(Attribute, "TextSize") == 0)
	{
		auto textDt = gState.textDataMap.find(hudRecordString);

		if (textDt != gState.textDataMap.end())
		{
			gState.updateLayoutReturnValue = true;

			int scalingMode = textDt->second.ScaleType;
			float scaledSize = textDt->second.TextSize * gState.scalingFactor;

			switch (scalingMode)
			{
				case 1: // Fit the text in the HUD
					scaledSize = std::round(scaledSize * 0.95f);
					break;
				case 2: // Small texts
					scaledSize = textDt->second.TextSize * gState.scalingFactorText;
					break;
			}

			gState.int32ToUpdate = static_cast<int32_t>(scaledSize);
		}
	}

	// Update the rectangle's length
	if (strcmp(Attribute, "AdditionalFloat") == 0)
	{
		float originalValue = 0.0f;

		if (!gState.slowMoBarUpdated && strcmp(hudRecordString, "HUDSlowMo2") == 0)
		{
			originalValue = 10.0f;
			gState.slowMoBarUpdated = true;
		}
		else if (strcmp(hudRecordString, "HUDFlashlight") == 0)
		{
			originalValue = 6.0f;
		}

		if (originalValue != 0.0f)
		{
			gState.updateLayoutReturnValue = true;
			gState.floatToUpdate = originalValue * gState.scalingFactor;
		}
	}

	// Medkit prompt when health drops below 50
	if (strcmp(hudRecordString, "HUDHealth") == 0 && strcmp(Attribute, "AdditionalInt") == 0)
	{
		if (gState.healthAdditionalIntIndex == 2)
		{
			gState.updateLayoutReturnValue = true;
			gState.int32ToUpdate = 14 * gState.scalingFactor;
		}
		else
		{
			gState.healthAdditionalIntIndex++;
		}
	}

	return LayoutDBGetRecord(Record, Attribute);
}

// Executed right after 'LayoutDBGetRecord'
static int __stdcall LayoutDBGetInt32_Hook(int a1, unsigned int a2, int a3)
{
	if (gState.updateLayoutReturnValue)
	{
		gState.updateLayoutReturnValue = false;
		return gState.int32ToUpdate;
	}
	return LayoutDBGetInt32(a1, a2, a3);
}

// Executed right after 'LayoutDBGetRecord'
static float __stdcall LayoutDBGetFloat_Hook(int a1, unsigned int a2, float a3)
{
	if (gState.updateLayoutReturnValue)
	{
		gState.updateLayoutReturnValue = false;
		return gState.floatToUpdate;
	}
	return LayoutDBGetFloat(a1, a2, a3);
}

// Executed right after 'LayoutDBGetRecord'
static const char* __stdcall LayoutDBGetString_Hook(int a1, unsigned int a2, int a3)
{
	return LayoutDBGetString(a1, a2, a3);
}

static int __fastcall UpdateSlider_Hook(int thisPtr, int _ECX, int index)
{
	if (thisPtr)
	{
		char* sliderName = MemoryHelper::ReadMemory<char*>(thisPtr + 0x8, false);

		// If 'ScreenCrosshair_Size_Help' is next
		if (strcmp(sliderName, "IDS_HELP_PICKUP_MSG_DUR") == 0)
		{
			gState.crosshairSliderUpdated = false;
		}

		// Update the index of 'ScreenCrosshair_Size_Help' as it will be wrong on the first time
		if (strcmp(sliderName, "ScreenCrosshair_Size_Help") == 0 && !gState.crosshairSliderUpdated && gState.scalingFactorCrosshair > 1.0f)
		{
			float unscaledIndex = index / gState.scalingFactorCrosshair;

			// scs.nIncrement = 2
			int newIndex = static_cast<int>((unscaledIndex / 2.0f) + 0.5f) * 2;

			// Clamp to the range [4, 16].
			newIndex = std::clamp(newIndex, 4, 16);

			index = newIndex;

			// Only needed on the first time
			gState.crosshairSliderUpdated = true;
		}
	}
	return UpdateSlider(thisPtr, index);
}

static void __fastcall ScreenDimsChanged_Hook(int thisPtr, int _ECX)
{
	ScreenDimsChanged(thisPtr);

	// Get the current resolution
	gState.currentWidth = MemoryHelper::ReadMemory<int>(thisPtr + 0x18, false);
	gState.currentHeight = MemoryHelper::ReadMemory<int>(thisPtr + 0x1C, false);

	// Calculate the new scaling factor
	float originalAspect = BASE_WIDTH / BASE_HEIGHT;
	float currentAspect = static_cast<float>(gState.currentWidth) / static_cast<float>(gState.currentHeight);

	gState.scalingFactor = std::sqrt((gState.currentWidth * gState.currentHeight) / (BASE_WIDTH * BASE_HEIGHT));

	// Don't downscale the HUD
	if (gState.scalingFactor < 1.0f) gState.scalingFactor = 1.0f;

	// Do not change the scaling of the crosshair
	gState.scalingFactorCrosshair = gState.scalingFactor;

	// Apply custom scaling for the text
	gState.scalingFactorText = gState.scalingFactor * SmallTextCustomScalingFactor;

	// Apply custom scaling to calculated scaling
	gState.scalingFactor *= HUDCustomScalingFactor;

	// Reset slow-mo bar update flag and HUDHealth.AdditionalInt index
	gState.slowMoBarUpdated = false;
	gState.healthAdditionalIntIndex = 0;

	// If the resolution is updated
	if (gState.CHUDMgr != 0)
	{
		// Reinitialize the HUD
		HUDTerminate(gState.CHUDMgr);
		HUDInit(gState.CHUDMgr);
		HUDWeaponListReset(gState.CHUDWeaponList);
		HUDWeaponListUpdateTriggerNames(gState.CHUDWeaponList);
		HUDGrenadeListUpdateTriggerNames(gState.CHUDGrenadeList);
		HUDPausedInit(gState.CHUDPaused);
		gState.updateHUD = true;

		// Update the size of the crosshair
		SetConsoleVariableFloat("CrosshairSize", gState.crosshairSize * gState.scalingFactorCrosshair);
		SetConsoleVariableFloat("PerturbScale", 0.5f * gState.scalingFactorCrosshair);
	}
}

// Initialize the HUD
static char __fastcall HUDInit_Hook(int thisPtr, int _ECX)
{
	gState.CHUDMgr = thisPtr;
	return HUDInit(thisPtr);
}

// Terminate the HUD
static void __fastcall HUDTerminate_Hook(int thisPtr, int _ECX)
{
	HUDTerminate(thisPtr);
}

static void __fastcall HUDRender_Hook(int thisPtr, int _ECX, int eHUDRenderLayer)
{
	HUDRender(thisPtr, eHUDRenderLayer);
	if (gState.updateHUD)
	{
		MemoryHelper::WriteMemory<int>(thisPtr + 0x14, -1, false);
		gState.updateHUD = false;
	}
}

static bool __fastcall HUDWeaponListInit_Hook(int thisPtr, int _ECX)
{
	gState.CHUDWeaponList = thisPtr;
	return HUDWeaponListInit(thisPtr);
}

static void __fastcall HUDWeaponListReset_Hook(int thisPtr, int _ECX)
{
	HUDWeaponListReset(thisPtr);
}

static void __fastcall HUDWeaponListUpdateTriggerNames_Hook(int thisPtr, int _ECX)
{
	HUDWeaponListUpdateTriggerNames(thisPtr);
}

static bool __fastcall HUDGrenadeListInit_Hook(int thisPtr, int _ECX)
{
	gState.CHUDGrenadeList = thisPtr;
	return HUDGrenadeListInit(thisPtr);
}

static void __fastcall HUDPausedInit_Hook(int thisPtr, int _ECX)
{
	gState.CHUDPaused = thisPtr;
	HUDPausedInit(thisPtr);
}

static void __fastcall HUDGrenadeListUpdateTriggerNames_Hook(int thisPtr, int _ECX)
{
	HUDGrenadeListUpdateTriggerNames(thisPtr);
}

static void __fastcall SetCurrentType_Hook(int thisPtr, int _ECX, int type)
{
	gState.pCurrentType = thisPtr;
	SetCurrentType(thisPtr, type);
}

static void __cdecl HUDSwapUpdateTriggerName_Hook()
{
	HUDSwapUpdateTriggerName();
}

static void __fastcall SwitchToScreen_Hook(int thisPtr, int _ECX, int pNewScreen)
{
	int currentScreenID = *(DWORD*)(pNewScreen + 0x10);
	int ScreenPerformanceCPU, ScreenPerformanceGPU;

	switch (gState.CurrentFEARGame) 
	{
		case FEARXP:
			ScreenPerformanceCPU = 18;
			ScreenPerformanceGPU = 19;
			break;
		case FEARXP2:
			ScreenPerformanceCPU = 20;
			ScreenPerformanceGPU = 21;
			break;
		case FEAR:
		default:
			ScreenPerformanceCPU = 19;
			ScreenPerformanceGPU = 20;
			break;
	}

	if (currentScreenID == ScreenPerformanceCPU) 
	{
		gState.maxCurrentType = 3;
		gState.currentType = 0;
	}
	else if (currentScreenID == ScreenPerformanceGPU) 
	{
		gState.maxCurrentType = 2;
		gState.currentType = 0;
	}
	else 
	{
		gState.maxCurrentType = -1;
	}
	SwitchToScreen(thisPtr, pNewScreen);
}


static void __stdcall InitAdditionalTextureData_Hook(int a1, int a2, int* a3, DWORD* vPos, DWORD* vSize, float a6)
{
	vPos[0] = static_cast<DWORD>((int)vPos[0] * gState.scalingFactor);
	vPos[1] = static_cast<DWORD>((int)vPos[1] * gState.scalingFactor);

	vSize[0] = static_cast<DWORD>((int)vSize[0] * gState.scalingFactor);
	vSize[1] = static_cast<DWORD>((int)vSize[1] * gState.scalingFactor);

	InitAdditionalTextureData(a1, a2, a3, vPos, vSize, a6);
}

static float __stdcall GetShatterLifetime_Hook(int shatterType)
{
	return FLT_MAX;
}

static int __stdcall CreateFX_Hook(char* effectType, int fxData, int prop)
{
	if (prop)
	{
		// Decal & LTBModel
		if (*reinterpret_cast<uint32_t*>(effectType) == 0x61636544 || *reinterpret_cast<uint32_t*>(effectType) == 0x4D42544C)
		{
			MemoryHelper::WriteMemory<float>(prop + 0x8, FLT_MAX, false); // m_tmEnd
			MemoryHelper::WriteMemory<float>(prop + 0xC, FLT_MAX, false); // m_tmLifetime
		}
		else if (*reinterpret_cast<uint32_t*>(effectType) == 0x69727053)
		{
			char* fxName = MemoryHelper::ReadMemory<char*>(fxData + 0x74, false);

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

static void __fastcall PauseGame_Hook(int thisPtr, int _ECX, bool pause, bool pauseSound)
{
	MessageBoxA(NULL, std::to_string(pause).c_str(), "Base Address Info", MB_OK | MB_ICONINFORMATION);

	gState.isGamePaused = pause;
	return PauseGame(thisPtr, pause, pauseSound);
}

static bool __fastcall IsCommandOn_Hook(int thisPtr, int _ECX, int commandId) 
{
	return (commandId < 117 && g_Controller.commandActive[commandId]) || IsCommandOn(thisPtr, commandId);
}

static bool __fastcall OnCommandOn_Hook(int thisPtr, int _ECX, int commandId)
{
	return OnCommandOn(thisPtr, commandId);
}

static bool __fastcall OnCommandOff_Hook(int thisPtr, int _ECX, int commandId)
{
	return OnCommandOff(thisPtr, commandId);
}

static double __fastcall GetExtremalCommandValue_Hook(int thisPtr, int _ECX, int commandId) 
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
				return -gamepad.sThumbRY / 32768.0;
			}
			case 23: // Yaw
			{
				if (abs(gamepad.sThumbRX) < DEAD_ZONE) return 0.0;
				return gamepad.sThumbRX / 32768.0;
			}
		}
	}

	return GetExtremalCommandValue(thisPtr, commandId);
}

static int __fastcall HUDActivateObjectSetObject_Hook(int thisPtr, int _ECX, void** a2, int a3, int a4, int a5, int a6)
{
	gState.canActivate = a6 != -1;
	return HUDActivateObjectSetObject(thisPtr, a2, a3, a4, a5, a6);
}

static void __fastcall HUDSwapUpdate_Hook(int thisPtr, int _ECX)
{
	 HUDSwapUpdate(thisPtr);
	 gState.canSwap = MemoryHelper::ReadMemory<uint8_t>(thisPtr + 0x1C0) != 0;
}

static int __fastcall SetOperatingTurret_Hook(int thisPtr, int _ECX, int pTurret)
{
	gState.isOperatingTurret = pTurret != 0;
	return SetOperatingTurret(thisPtr, pTurret);
}

static const wchar_t* __fastcall GetTriggerNameFromCommandID_Hook(int thisPtr, int* ECX, int commandId)
{
	if (g_Controller.isConnected)
	{
		bool useShortNames = false;

		// Left Thumbstick movement
		if (commandId == 0)
		{
			return L"Left Thumbstick Up";
		}
		if (commandId == 1)
		{
			return L"Left Thumbstick Down";
		}

		// Activate Key = Reload Key
		if (commandId == 87)
		{
			commandId = 88;
		}

		// HUDWeapon -> Next Weapon
		if (commandId >= 30 && commandId <= 39)
		{
			commandId = 77;
			useShortNames = true;
		}

		// HUDGrenadeList -> Next Grenade
		if (commandId >= 40 && commandId <= 45)
		{
			commandId = 73;
			useShortNames = true;
		}

		for (size_t i = 0; i < sizeof(g_buttonMappings) / sizeof(g_buttonMappings[0]); i++)
		{
			if (g_buttonMappings[i].second == commandId)
			{
				if (useShortNames) 
				{
					switch (g_buttonMappings[i].first)
					{
						case XINPUT_GAMEPAD_A: return L"A";
						case XINPUT_GAMEPAD_B: return L"B";
						case XINPUT_GAMEPAD_X: return L"X";
						case XINPUT_GAMEPAD_Y: return L"Y";
						case XINPUT_GAMEPAD_LEFT_THUMB: return L"L3";
						case XINPUT_GAMEPAD_RIGHT_THUMB: return L"R3";
						case XINPUT_GAMEPAD_LEFT_SHOULDER: return L"LB";
						case XINPUT_GAMEPAD_RIGHT_SHOULDER: return L"RB";
						case XINPUT_GAMEPAD_DPAD_UP: return L"D-Up";
						case XINPUT_GAMEPAD_DPAD_DOWN: return L"D-Down";
						case XINPUT_GAMEPAD_DPAD_LEFT: return L"D-Left";
						case XINPUT_GAMEPAD_DPAD_RIGHT: return L"D-Right";
						case XINPUT_GAMEPAD_LEFT_TRIGGER: return L"LT";
						case XINPUT_GAMEPAD_RIGHT_TRIGGER: return L"RT";
						case XINPUT_GAMEPAD_BACK: return L"Back";
						default: break;
					}
				}
				else 
				{
					switch (g_buttonMappings[i].first)
					{
						case XINPUT_GAMEPAD_A: return L"A Button";
						case XINPUT_GAMEPAD_B: return L"B Button";
						case XINPUT_GAMEPAD_X: return L"X Button";
						case XINPUT_GAMEPAD_Y: return L"Y Button";
						case XINPUT_GAMEPAD_LEFT_THUMB: return L"Left Thumbstick";
						case XINPUT_GAMEPAD_RIGHT_THUMB: return L"Right Thumbstick";
						case XINPUT_GAMEPAD_LEFT_SHOULDER: return L"Left Bumper";
						case XINPUT_GAMEPAD_RIGHT_SHOULDER: return L"Right Bumper";
						case XINPUT_GAMEPAD_DPAD_UP: return L"D-Pad Up";
						case XINPUT_GAMEPAD_DPAD_DOWN: return L"D-Pad Down";
						case XINPUT_GAMEPAD_DPAD_LEFT: return L"D-Pad Left";
						case XINPUT_GAMEPAD_DPAD_RIGHT: return L"D-Pad Right";
						case XINPUT_GAMEPAD_LEFT_TRIGGER: return L"Left Trigger";
						case XINPUT_GAMEPAD_RIGHT_TRIGGER: return L"Right Trigger";
						case XINPUT_GAMEPAD_BACK: return L"Back Button";
						default: break;
					}
				}
			}
		}
	}
	return GetTriggerNameFromCommandID(thisPtr, commandId);
}

static void __fastcall UseCursor_Hook(int thisPtr, int _ECX, bool bUseCursor)
{
	if (g_Controller.isConnected && bUseCursor) bUseCursor = false;
	UseCursor(thisPtr, bUseCursor);
}

static bool __fastcall OnMouseMove_Hook(int thisPtr, int _ECX, int x, int y)
{
	if (g_Controller.isConnected)
	{
		x = 0;
		y = 0;
	}
	return OnMouseMove(thisPtr, x, y);
}

static uint8_t __fastcall GetWeaponCapacity_Hook(int thisPtr, int _ECX)
{
	return MaxWeaponCapacity;
}

static void __fastcall OnEnterWorld_Hook(int thisPtr, int _ECX)
{
	OnEnterWorld(thisPtr);
	SetWeaponCapacityServer(gState.CPlayerInventory, MaxWeaponCapacity);
}

static void __fastcall DisconnectFromServer_Hook(int thisPtr, int _ECX)
{
	for (DWORD address : gState.hookedServerFunctionAddresses) 
	{
		MH_RemoveHook((void*)address);
	}

	gState.hookedServerFunctionAddresses.clear();

	DisconnectFromServer(thisPtr);
}

static bool __fastcall RenderTargetGroupFXInit_Hook(int thisPtr, int _ECX, int psfxCreateStruct)
{
	// Low
	MemoryHelper::WriteMemory<int>(psfxCreateStruct + 0xC, 512, false);
	MemoryHelper::WriteMemory<int>(psfxCreateStruct + 0x10, 512, false);
	// Medium
	MemoryHelper::WriteMemory<int>(psfxCreateStruct + 0x14, 1024, false);
	MemoryHelper::WriteMemory<int>(psfxCreateStruct + 0x18, 1024, false);
	// High
	MemoryHelper::WriteMemory<int>(psfxCreateStruct + 0x1C, 2048, false);
	MemoryHelper::WriteMemory<int>(psfxCreateStruct + 0x20, 2048, false);
	return RenderTargetGroupFXInit(thisPtr, psfxCreateStruct);
}

static const wchar_t* __stdcall LoadGameString_Hook(int ptr, char* String)
{
	if (g_Controller.isConnected && strcmp(String, "ScreenFailure_PressAnyKey") == 0)
	{
		return L"Press B to return to the main menu.\nPress any other button to continue.";
	}
	return LoadGameString(ptr, String);
}


static char __fastcall LoadClientFXDLL_Hook(int thisPtr, int _ECX, char* Source, char a3)
{
	// Load the DLL
	char result = LoadClientFXDLL(thisPtr, Source, a3);

	// Get the path
	char* clientFXPath = ((char*)thisPtr + 0x24);

	// Get the handle
	wchar_t wFileName[MAX_PATH];
	MultiByteToWideChar(CP_UTF8, 0, clientFXPath, -1, wFileName, MAX_PATH);
	HMODULE clientFxDll = GetModuleHandleW(wFileName);

	if (clientFxDll)
	{
		gState.GameClientFX = clientFxDll;

		ApplyClientFXPatch();
	}

	return result;
}

#pragma endregion

#pragma region Client Patches

static void ApplyXPWidescreenClientPatch()
{
	if (DisableXPWidescreenFiltering && gState.CurrentFEARGame == FEARXP)
	{
		MemoryHelper::MakeNOP((DWORD)gState.GameClient + 0x10DDB0, 24, true);
	}
}

static void ApplySkipSplashScreenClientPatch()
{
	if (!SkipSplashScreen) return;

	DWORD targetMemoryLocation = ScanModuleSignature(gState.GameClient, "53 8B 5C 24 08 55 8B 6C 24 14 56 8D 43 FF 83 F8", "SkipSplashScreen");

	if (targetMemoryLocation != 0)
	{
		MemoryHelper::MakeNOP(targetMemoryLocation + 0x13D, 8, true);
	}
}

static void ApplyDisableLetterboxClientPatch()
{
	if (!DisableLetterbox) return;

	DWORD targetMemoryLocation = ScanModuleSignature(gState.GameClient, "83 EC 54 53 55 56 57 8B", "DisableLetterbox");

	if (targetMemoryLocation != 0)
	{
		MemoryHelper::WriteMemory<uint8_t>(targetMemoryLocation, 0xC3, true);
	}
}

static void ApplyPersistentWorldClientPatch()
{
	if (!EnablePersistentWorldState) return;

	DWORD targetMemoryLocation_ShellCasing = ScanModuleSignature(gState.GameClient, "D9 86 88 00 00 00 D8 64 24", "EnablePersistentWorldState_ShellCasing");
	DWORD targetMemoryLocation_DecalSaving = ScanModuleSignature(gState.GameClient, "FF 52 0C ?? 8D ?? ?? ?? 00 00 E8 ?? ?? ?? FF 8B", "EnablePersistentWorldState_DecalSaving");
	DWORD targetMemoryLocation_Decal = ScanModuleSignature(gState.GameClient, "DF E0 F6 C4 01 75 34 DD 44 24", "EnablePersistentWorldState_Decal");
	DWORD targetMemoryLocation_FX = ScanModuleSignature(gState.GameClient, "8B CE FF ?? 04 84 C0 75 ?? 8B ?? 8B CE FF ?? 08 56 E8", "EnablePersistentWorldState_FX", 1);
	DWORD targetMemoryLocation_Shatter = ScanModuleSignature(gState.GameClient, "8B C8 E8 ?? ?? ?? 00 D9 5C 24 ?? D9", "EnablePersistentWorldState_Shatter");

	if (targetMemoryLocation_ShellCasing == 0 ||
		targetMemoryLocation_DecalSaving == 0 ||
		targetMemoryLocation_Decal == 0 ||
		targetMemoryLocation_FX == 0 ||
		targetMemoryLocation_Shatter == 0) {
		return;
	}

	MemoryHelper::MakeNOP(targetMemoryLocation_ShellCasing + 0x6, 4, true);
	MemoryHelper::MakeNOP(targetMemoryLocation_DecalSaving + 0xF, 13, true);
	MemoryHelper::WriteMemory<uint8_t>(targetMemoryLocation_Decal + 0x5, 0x74, true);

	int callAddr = MemoryHelper::ReadMemory<int>(targetMemoryLocation_Shatter + 0x3);
	int shatterLiftetimeAddress = (targetMemoryLocation_Shatter + 0x3) + (callAddr + 0x4);
	HookHelper::ApplyHook((void*)shatterLiftetimeAddress, &GetShatterLifetime_Hook, (LPVOID*)&GetShatterLifetime);
	HookHelper::ApplyHook((void*)targetMemoryLocation_FX, &CreateFX_Hook, (LPVOID*)&CreateFX);
}

static void ApplyInfiniteFlashlightClientPatch()
{
	if (!InfiniteFlashlight) return;
	DWORD targetMemoryLocation_Update = ScanModuleSignature(gState.GameClient, "8B 51 10 8A 42 18 84 C0 8A 86 04 01 00 00", "InfiniteFlashlight_Update");
	DWORD targetMemoryLocation_UpdateBar = ScanModuleSignature(gState.GameClient, "A1 ?? ?? ?? ?? 85 C0 56 8B F1 74 71 D9 86 1C 04 00 00", "InfiniteFlashlight_UpdateBar");
	DWORD targetMemoryLocation_UpdateLayout = ScanModuleSignature(gState.GameClient, "68 ?? ?? ?? ?? 6A 00 68 ?? ?? ?? ?? 50 FF 57 58 8B 0D ?? ?? ?? ?? 50 FF 97 84 00 00 00 8B 0D ?? ?? ?? ?? 8B 11 50 8D 44 24 10 50 FF 52 04 8B 4C 24 0C 8D BE C4 01 00 00", "InfiniteFlashlight_UpdateLayout");
	DWORD targetMemoryLocation_Battery = ScanModuleSignature(gState.GameClient, "D8 4C 24 04 DC AE 88 03 00 00 DD 96 88 03 00 00", "InfiniteFlashlight_Battery");

	if (targetMemoryLocation_Update == 0 ||
		targetMemoryLocation_UpdateBar == 0 ||
		targetMemoryLocation_UpdateLayout == 0 ||
		targetMemoryLocation_Battery == 0) {
		return;
	}

	MemoryHelper::WriteMemory<uint8_t>(targetMemoryLocation_Update - 0x31, 0xC3, true);
	MemoryHelper::WriteMemory<uint8_t>(targetMemoryLocation_UpdateLayout - 0x36, 0xC3, true);
	MemoryHelper::WriteMemory<uint8_t>(targetMemoryLocation_UpdateBar, 0xC3, true);
	MemoryHelper::MakeNOP(targetMemoryLocation_Battery + 0xA, 6, true);
}

static void ApplyXInputControllerClientPatch()
{
	if (!XInputControllerSupport) return;

	DWORD targetMemoryLocation_pGameClientShell = ScanModuleSignature(gState.GameClient, "C1 F8 02 C1 E0 05 2B C2 8B CB BA 01 00 00 00 D3 E2 8B CD 03 C3 50 85 11", "Controller_pGameClientShell");
	if (targetMemoryLocation_pGameClientShell == 0) return;

	DWORD targetMemoryLocation_OnCommandOn = targetMemoryLocation_pGameClientShell + MemoryHelper::ReadMemory<int>(targetMemoryLocation_pGameClientShell + 0x21) + 0x25;
	DWORD targetMemoryLocation_OnCommandOff = targetMemoryLocation_pGameClientShell + MemoryHelper::ReadMemory<int>(targetMemoryLocation_pGameClientShell + 0x28) + 0x2C;
	DWORD targetMemoryLocation_GetExtremalCommandValue = ScanModuleSignature(gState.GameClient, "83 EC 08 56 57 8B F9 8B 77 04 3B 77 08 C7 44 24 08 00 00 00 00", "Controller_GetExtremalCommandValue");	
	DWORD targetMemoryLocation_IsCommandOn = ScanModuleSignature(gState.GameClient, "8B D1 8A 42 4C 84 C0 56 74 58", "Controller_IsCommandOn");
	DWORD targetMemoryLocation_PauseGame = ScanModuleSignature(gState.GameClient, "8A C3 F6 D8 6A 01 1B C0 05 A1 00 00 00 50", "Controller_PauseGame", 2);
	DWORD targetMemoryLocation_HUDActivateObjectSetObject = ScanModuleSignature(gState.GameClient, "8B 86 D4 02 00 00 3B C3 8D BE C8 02 00 00 74 0F", "Controller_HUDActivateObjectSetObject", 1);
	DWORD targetMemoryLocation_HUDSwapUpdate = ScanModuleSignature(gState.GameClient, "55 8B EC 83 E4 F8 81 EC 84 01", "Controller_HUDSwapUpdate");
	DWORD targetMemoryLocation_SetOperatingTurret = ScanModuleSignature(gState.GameClient, "8B 44 24 04 89 81 F4 05 00 00 8B 0D ?? ?? ?? ?? 8B 11 FF 52 3C C2 04 00", "Controller_SetOperatingTurret");
	DWORD targetMemoryLocation_GetTriggerNameFromCommandID = ScanModuleSignature(gState.GameClient, "81 EC 44 08 00 00", "Controller_GetTriggerNameFromCommandID");
	DWORD targetMemoryLocation_SwitchToScreen = ScanModuleSignature(gState.GameClient, "53 55 56 8B F1 8B 6E 60 33 DB 3B EB 57 8B 7C 24 14", "Controller_SwitchToScreen");
	DWORD targetMemoryLocation_SetCurrentType = ScanModuleSignature(gState.GameClient, "53 8B 5C 24 08 85 DB 56 57 8B F1 7C 1C 8B BE E4", "Controller_SetCurrentType");
	DWORD targetMemoryLocation_HUDSwapUpdateTriggerName = ScanModuleSignature(gState.GameClient, "8B 0D ?? ?? ?? ?? 6A 57 E8 ?? ?? ?? ?? 50 B9", "Controller_HUDSwapUpdateTriggerName");

	if (targetMemoryLocation_OnCommandOn == 0 ||
		targetMemoryLocation_OnCommandOff == 0 ||
		targetMemoryLocation_GetExtremalCommandValue == 0 ||
		targetMemoryLocation_IsCommandOn == 0 ||
		targetMemoryLocation_PauseGame == 0 ||
		targetMemoryLocation_HUDActivateObjectSetObject == 0 ||
		targetMemoryLocation_HUDSwapUpdate == 0 ||
		targetMemoryLocation_SetOperatingTurret == 0 ||
		targetMemoryLocation_GetTriggerNameFromCommandID == 0 ||
		targetMemoryLocation_SwitchToScreen == 0 ||
		targetMemoryLocation_SetCurrentType == 0 ||
		targetMemoryLocation_HUDSwapUpdateTriggerName == 0) {
		return;
	}

	gState.g_pGameClientShell = MemoryHelper::ReadMemory<int>(MemoryHelper::ReadMemory<int>(targetMemoryLocation_pGameClientShell + 0x1A));

	HookHelper::ApplyHook((void*)(targetMemoryLocation_GetExtremalCommandValue), &GetExtremalCommandValue_Hook, (LPVOID*)&GetExtremalCommandValue);
	HookHelper::ApplyHook((void*)targetMemoryLocation_IsCommandOn, &IsCommandOn_Hook, (LPVOID*)&IsCommandOn);
	HookHelper::ApplyHook((void*)targetMemoryLocation_OnCommandOn, &OnCommandOn_Hook, (LPVOID*)&OnCommandOn);
	HookHelper::ApplyHook((void*)targetMemoryLocation_OnCommandOff, &OnCommandOff_Hook, (LPVOID*)&OnCommandOff);
	HookHelper::ApplyHook((void*)targetMemoryLocation_SetOperatingTurret, &SetOperatingTurret_Hook, (LPVOID*)&SetOperatingTurret);
	HookHelper::ApplyHook((void*)targetMemoryLocation_GetTriggerNameFromCommandID, &GetTriggerNameFromCommandID_Hook, (LPVOID*)&GetTriggerNameFromCommandID);
	HookHelper::ApplyHook((void*)targetMemoryLocation_PauseGame, &PauseGame_Hook, (LPVOID*)&PauseGame);
	HookHelper::ApplyHook((void*)targetMemoryLocation_HUDActivateObjectSetObject, &HUDActivateObjectSetObject_Hook, (LPVOID*)&HUDActivateObjectSetObject);
	HookHelper::ApplyHook((void*)targetMemoryLocation_HUDSwapUpdate, &HUDSwapUpdate_Hook, (LPVOID*)&HUDSwapUpdate);
	HookHelper::ApplyHook((void*)targetMemoryLocation_SwitchToScreen, &SwitchToScreen_Hook, (LPVOID*)&SwitchToScreen);
	HookHelper::ApplyHook((void*)targetMemoryLocation_SetCurrentType, &SetCurrentType_Hook, (LPVOID*)&SetCurrentType);
	HookHelper::ApplyHook((void*)targetMemoryLocation_HUDSwapUpdateTriggerName, &HUDSwapUpdateTriggerName_Hook, (LPVOID*)&HUDSwapUpdateTriggerName);

	if (!HideMouseCursor) return;

	DWORD targetMemoryLocation_UseCursor = ScanModuleSignature(gState.GameClient, "8A 44 24 04 84 C0 56 8B F1 88 46 01 74", "Controller_UseCursor");
	DWORD targetMemoryLocation_OnMouseMove = ScanModuleSignature(gState.GameClient, "56 8B F1 8A 86 ?? ?? 00 00 84 C0 0F 84 B3", "Controller_OnMouseMove");

	HookHelper::ApplyHook((void*)targetMemoryLocation_OnMouseMove, &OnMouseMove_Hook, (LPVOID*)&OnMouseMove);
	HookHelper::ApplyHook((void*)targetMemoryLocation_UseCursor, &UseCursor_Hook, (LPVOID*)&UseCursor);
}

static void ApplyHUDScalingClientPatch()
{
	if (!HUDScaling) return;

	DWORD targetMemoryLocation_GameDatabase = ScanModuleSignature(gState.GameClient, "8B 5E 08 55 E8 ?? ?? ?? FF 8B 0D ?? ?? ?? ?? 8B 39 68 ?? ?? ?? ?? 6A 00 68 ?? ?? ?? ?? 53 FF 57", "HUDScaling_GameDatabase");
	DWORD targetMemoryLocation_HUDTerminate = ScanModuleSignature(gState.GameClient, "53 56 8B D9 8B B3 7C 04 00 00 8B 83 80 04 00 00 57 33 FF 3B F0", "HUDScaling_HUDTerminate");
	DWORD targetMemoryLocation_HUDInit = ScanModuleSignature(gState.GameClient, "8B ?? ?? 8D ?? 78 04 00 00", "HUDScaling_HUDInit", 1);
	DWORD targetMemoryLocation_HUDRender = ScanModuleSignature(gState.GameClient, "53 8B D9 8A 43 08 84 C0 74", "HUDScaling_HUDRender");
	DWORD targetMemoryLocation_ScreenDimsChanged = ScanModuleSignature(gState.GameClient, "A1 ?? ?? ?? ?? 81 EC 98 00 00 00 85 C0 56 8B F1", "HUDScaling_ScreenDimsChanged");
	DWORD targetMemoryLocation_LayoutDBGetPosition = ScanModuleSignature(gState.GameClient, "83 EC 10 8B 54 24 20 8B 0D", "HUDScaling_LayoutDBGetPosition");
	DWORD targetMemoryLocation_GetRectF = ScanModuleSignature(gState.GameClient, "14 8B 44 24 28 8B 4C 24 18 D9 18", "HUDScaling_GetRectF");
	DWORD targetMemoryLocation_UpdateSlider = ScanModuleSignature(gState.GameClient, "56 8B F1 8B 4C 24 08 8B 86 7C 01 00 00 3B C8 89 8E 80 01 00 00", "HUDScaling_UpdateSlider");
	DWORD targetMemoryLocation_HUDWeaponListReset = ScanModuleSignature(gState.GameClient, "51 53 55 8B E9 8B 0D", "HUDScaling_HUDWeaponListReset");
	DWORD targetMemoryLocation_HUDWeaponListInit = ScanModuleSignature(gState.GameClient, "51 53 55 57 8B F9 8B 07 FF 50 20 8B 0D", "HUDScaling_HUDWeaponListInit");
	DWORD targetMemoryLocation_HUDGrenadeListInit = ScanModuleSignature(gState.GameClient, "83 EC 08 53 55 57 8B F9 8B 07 FF 50 20 8B 0D", "HUDScaling_HUDGrenadeListInit");
	DWORD targetMemoryLocation_InitAdditionalTextureData = ScanModuleSignature(gState.GameClient, "8B 54 24 04 8B 01 83 EC 20 57", "HUDScaling_InitAdditionalTextureData");
	DWORD targetMemoryLocation_HUDPausedInit = ScanModuleSignature(gState.GameClient, "56 8B F1 8B 06 57 FF 50 20", "HUDScaling_HUDPausedInit");

	if (targetMemoryLocation_GameDatabase == 0 ||
		targetMemoryLocation_HUDTerminate == 0 ||
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

	int pDB = MemoryHelper::ReadMemory<int>(targetMemoryLocation_GameDatabase + 0xB);
	int pGameDatabase = MemoryHelper::ReadMemory<int>(pDB);
	int pLayoutDB = MemoryHelper::ReadMemory<int>(pGameDatabase);

	HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x58), &LayoutDBGetRecord_Hook, (LPVOID*)&LayoutDBGetRecord);
	HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x7C), &LayoutDBGetInt32_Hook, (LPVOID*)&LayoutDBGetInt32);
	HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x80), &LayoutDBGetFloat_Hook, (LPVOID*)&LayoutDBGetFloat);
	HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x84), &LayoutDBGetString_Hook, (LPVOID*)&LayoutDBGetString);

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

	DWORD targetMemoryLocation_DisconnectFromServer = ScanModuleSignature(gState.GameClient, "81 EC 08 02 00 00 E8", "WeaponCapacity_DisconnectFromServer");
	DWORD targetMemoryLocation_OnEnterWorld = ScanModuleSignature(gState.GameClient, "8B F1 E8 ?? ?? ?? ?? DD 05 ?? ?? ?? ?? 8B 96", "WeaponCapacity_OnEnterWorld", 1);
	DWORD targetMemoryLocation_GetWeaponCapacity = ScanModuleSignature(gState.GameClient, "CC 8B 41 48 8B 0D", "WeaponCapacity_GetWeaponCapacity");

	if (targetMemoryLocation_DisconnectFromServer == 0 ||
		targetMemoryLocation_OnEnterWorld == 0 ||
		targetMemoryLocation_GetWeaponCapacity == 0) {
		return;
	}

	HookHelper::ApplyHook((void*)targetMemoryLocation_DisconnectFromServer, &DisconnectFromServer_Hook, (LPVOID*)&DisconnectFromServer);
	HookHelper::ApplyHook((void*)targetMemoryLocation_OnEnterWorld, &OnEnterWorld_Hook, (LPVOID*)&OnEnterWorld);
	HookHelper::ApplyHook((void*)(targetMemoryLocation_GetWeaponCapacity + 0x1), &GetWeaponCapacity_Hook, (LPVOID*)&GetWeaponCapacity);

	gState.appliedCustomMaxWeaponCapacity = true;
}

static void ApplyHighResolutionReflectionsClientPatch()
{
	if (!HighResolutionReflections) return;

	DWORD targetMemoryLocation = ScanModuleSignature(gState.GameClient, "8B 47 08 89 46 4C 8A 4F 24 88 4E 68 8A 57 25", "HighResolutionReflections_RenderTargetGroupFXInit");

	if (targetMemoryLocation == 0) return;

	HookHelper::ApplyHook((void*)(targetMemoryLocation - 0x31), &RenderTargetGroupFXInit_Hook, (LPVOID*)&RenderTargetGroupFXInit);
}

static void __cdecl AutoDetectPerformanceSettings_Hook()
{
	gState.isInAutoDetect = true;
	AutoDetectPerformanceSettings();
	gState.isInAutoDetect = false;
}

static void ApplyAutoResolutionClientCheck()
{
	if (!AutoResolution) return; 

	DWORD targetMemoryLocation = ScanModuleSignature(gState.GameClient, "83 C4 10 83 F8 01 75 37", "AutoResolution_AutoDetectPerformanceSettings", 2);

	if (targetMemoryLocation == 0) return;

	HookHelper::ApplyHook((void*)targetMemoryLocation, &AutoDetectPerformanceSettings_Hook, (LPVOID*)&AutoDetectPerformanceSettings);
}

static void ApplyClientFXHook()
{
	if (!FixParticleLifetimeCalculation && !FixParticleUpdateThreshold) return;

	DWORD targetMemoryLocation = ScanModuleSignature(gState.GameClient, "83 EC 20 56 57 8B F1 E8 ?? ?? ?? ?? 8A 44 24 30", "ClientFXHook");

	if (targetMemoryLocation == 0) return;

	HookHelper::ApplyHook((void*)targetMemoryLocation, &LoadClientFXDLL_Hook, (LPVOID*)&LoadClientFXDLL);
}

static void ApplyClientPatchSet1()
{
	if (!HUDScaling && !XInputControllerSupport) return;

	DWORD targetMemoryLocation_HUDWeaponListUpdateTriggerNames = ScanModuleSignature(gState.GameClient, "56 32 DB 89 44 24 0C BE 1E 00 00 00", "HUDWeaponListUpdateTriggerNames");
	DWORD targetMemoryLocation_HUDGrenadeListUpdateTriggerNames = ScanModuleSignature(gState.GameClient, "56 32 DB 89 44 24 0C BE 28 00 00 00", "HUDGrenadeListUpdateTriggerNames");

	if (targetMemoryLocation_HUDWeaponListUpdateTriggerNames == 0 ||
		targetMemoryLocation_HUDGrenadeListUpdateTriggerNames == 0) {
		return;
	}

	HookHelper::ApplyHook((void*)(targetMemoryLocation_HUDWeaponListUpdateTriggerNames - 0x10), &HUDWeaponListUpdateTriggerNames_Hook, (LPVOID*)&HUDWeaponListUpdateTriggerNames);
	HookHelper::ApplyHook((void*)(targetMemoryLocation_HUDGrenadeListUpdateTriggerNames - 0x10), &HUDGrenadeListUpdateTriggerNames_Hook, (LPVOID*)&HUDGrenadeListUpdateTriggerNames);
}

static void ApplyClientPatch()
{
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
	ApplyClientFXHook();
	ApplyClientPatchSet1();
}

#pragma endregion

#pragma region Server Hooks

static void __fastcall SetWeaponCapacityServer_Hook(int thisPtr, int _ECX, uint8_t nCap)
{
	nCap = MaxWeaponCapacity;
	SetWeaponCapacityServer(thisPtr, nCap);
}

static void __fastcall PlayerInventoryInit_Hook(int thisPtr, int _ECX, int nCap)
{
	gState.CPlayerInventory = thisPtr;
	PlayerInventoryInit(thisPtr, nCap);
}

#pragma endregion

#pragma region Server Patches

static void ApplyTrackedHook(DWORD address, LPVOID hookFunc, LPVOID *originalPtr)
{
	HookHelper::ApplyHook((void*)address, hookFunc, originalPtr);
	gState.hookedServerFunctionAddresses.push_back(address);
}

static void ApplyPersistentWorldServerPatch()
{
	if (!EnablePersistentWorldState) return;

	DWORD targetMemoryLocation_BodyFading = ScanModuleSignature(gState.GameServer, "8A 86 ?? ?? 00 00 84 C0 74 A1 8D 8E", "EnablePersistentWorldState_BodyFading");

	if (targetMemoryLocation_BodyFading != 0)
	{
		MemoryHelper::WriteMemory<uint8_t>(targetMemoryLocation_BodyFading + 0x22, 0x75, true);
	}
}

static void ApplySetWeaponCapacityServerPatch()
{
	if (!EnableCustomMaxWeaponCapacity || !gState.appliedCustomMaxWeaponCapacity) return;

	DWORD targetMemoryLocation_SetWeaponCapacityServer = ScanModuleSignature(gState.GameServer, "56 8B F1 8B 56 18 85 D2 8D 4E 14 57 75", "WeaponCapacity_SetWeaponCapacityServer");
	DWORD targetMemoryLocation_PlayerInventoryInit = ScanModuleSignature(gState.GameServer, "33 DB 3B CB 89 ?? 0C 74", "WeaponCapacity_PlayerInventoryInit", 2);

	if (targetMemoryLocation_SetWeaponCapacityServer == 0 ||
		targetMemoryLocation_PlayerInventoryInit == 0) {
		return;
	}

	ApplyTrackedHook(targetMemoryLocation_SetWeaponCapacityServer, &SetWeaponCapacityServer_Hook, (LPVOID*)&SetWeaponCapacityServer);
	ApplyTrackedHook(targetMemoryLocation_PlayerInventoryInit, &PlayerInventoryInit_Hook, (LPVOID*)&PlayerInventoryInit);
}

static void ApplyServerPatch()
{
	ApplyPersistentWorldServerPatch();
	ApplySetWeaponCapacityServerPatch();
}

#pragma endregion

#pragma region Hooks

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
		if (!gState.isClientLoaded) // First time is client
		{
			gState.GameClient = ApiDLL;
			gState.isClientLoaded = true;

			ApplyClientPatch();
		}
		else // Otherwise server
		{
			gState.GameServer = ApiDLL;

			ApplyServerPatch();
		}
	}
	return result;
}

static int __fastcall StepPhysicsSimulation_Hook(int thisPtr, int _ECX, float* timeStepParams)
{
	if (timeStepParams[1] > 60) timeStepParams[1] = 60;
	return StepPhysicsSimulation(thisPtr, timeStepParams);
}

// Function utilized in the process of loading video files
static int __fastcall FindStringCaseInsensitive_Hook(DWORD* thisPtr, int* _ECX, char* video_path)
{
	// Disable this hook once we get to the menu
	if (SkipAllIntro || strstr(video_path, "Menu"))
	{
		switch (gState.CurrentFEARGame)
		{
			case FEAR:
				MH_DisableHook((void*)0x510CB0);
				break;
			case FEARMP:
				MH_DisableHook((void*)0x510DD0);
				break;
			case FEARXP:
				MH_DisableHook((void*)0x5B3440);
				break;
			case FEARXP2:
				MH_DisableHook((void*)0x5B49C0);
				break;
		}

		if (SkipAllIntro)
		{
			// Skip all movies while keeping the sound of the menu
			SystemHelper::SimulateSpacebarPress(gState.hWnd);
		}
	}

	if (SkipSierraIntro && strstr(video_path, "sierralogo.bik")) { video_path[0] = '\0'; }
	if (SkipMonolithIntro && strstr(video_path, "MonolithLogo.bik")) { video_path[0] = '\0'; }
	if (SkipWBGamesIntro && strstr(video_path, "WBGames.bik")) { video_path[0] = '\0'; }

	if (SkipNvidiaIntro && (
		strstr(video_path, "TWIMTBP_640x480.bik") ||
		strstr(video_path, "Nvidia_LogoXP2.bik")
		)) {
		video_path[0] = '\0';
	}

	if (SkipTimegateIntro && (
		strstr(video_path, "timegate.bik") ||
		strstr(video_path, "TimeGate.bik")
		)) {
		video_path[0] = '\0';
	}

	if (SkipDellIntro && (
		strstr(video_path, "dell_xps.bik") ||
		strstr(video_path, "Dell_LogoXP2.bik")
		)) {
		video_path[0] = '\0';
	}
	return FindStringCaseInsensitive(thisPtr, video_path);
}

static int __fastcall GetDeviceObjectDesc_Hook(int thisPtr, int _ECX, unsigned int DeviceType, wchar_t* KeyName, unsigned int* ret)
{
	if (DeviceType == 0) // Keyboard
	{
		// Control name from 'ProfileDatabase/Defaults.Gamdb00p' with corresponding DirectInput Key Id
		static const std::unordered_map<std::wstring, unsigned int> keyMap = 
		{
			{L"Left", 0xCB},
			{L"Right", 0xCD},
			{L"Right Ctrl", 0x9D},
			{L"Space", 0x39},
			{L"Shift", 0x2A},
			{L"Ctrl", 0x1D},
			{L"Tab", 0x0F},
			{L"Up", 0xC8},
			{L"Down", 0xD0},
			{L"End", 0xCF},
		};

		auto it = keyMap.find(KeyName);
		if (it != keyMap.end())
		{
			// Get pointer to keyboard the DIK table
			int KB_DIK_Table = MemoryHelper::ReadMemory<int>(thisPtr + 0xC, false);
			int tableStart = MemoryHelper::ReadMemory<int>(KB_DIK_Table + 0x10, false);
			int tableEnd = MemoryHelper::ReadMemory<int>(KB_DIK_Table + 0x14, false);

			// Iterate through the table
			while (tableStart < tableEnd)
			{
				// If the corresponding DirectInput Key Id is found
				if (MemoryHelper::ReadMemory<uint8_t>(tableStart + 0x1, false) == it->second)
				{
					// Write and return the index for that DIK Id
					*ret = MemoryHelper::ReadMemory<int>(tableStart + 0x1C, false);
					return 0;
				}

				tableStart += 0x20;
			}
		}
	}

	return GetDeviceObjectDesc(thisPtr, DeviceType, KeyName, ret);
}

static int __stdcall SetConsoleVariableFloat_Hook(char* pszVarName, float fValue)
{
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

	if (EnablePersistentWorldState)
	{
		if (strcmp(pszVarName, "BodyLifetime") == 0)
		{
			fValue = -1.0f;
		}
		else if (strcmp(pszVarName, "BodyCapRadius") == 0)
		{
			fValue = 1000000.0f;
		}
		else if (strcmp(pszVarName, "BodyCapRadiusCount") == 0)
		{
			fValue = 1000000.0f;
		}
		else if (strcmp(pszVarName, "BodyCapTotalCount") == 0)
		{
			fValue = 1000000.0f;
		}
	}

	if (gState.isInAutoDetect && AutoResolution)
	{
		if (strcmp(pszVarName, "Performance_ScreenHeight") == 0)
		{
			fValue = static_cast<float>(gState.screenHeight);
		}
		else if (strcmp(pszVarName, "Performance_ScreenWidth") == 0)
		{
			fValue = static_cast<float>(gState.screenWidth);
		}
	}

	if (HUDScaling)
	{
		if (strcmp(pszVarName, "CrosshairSize") == 0)
		{
			gState.crosshairSize = fValue; // Keep a backup of the value as it can be adjusted in the settings
			fValue = fValue * gState.scalingFactorCrosshair;
		}
		else if (strcmp(pszVarName, "PerturbScale") == 0)
		{
			fValue = fValue * gState.scalingFactorCrosshair;
		}
		else if (strcmp(pszVarName, "UseTextScaling") == 0)
		{
			fValue = 0.0f; // Scaling is already taken care of
		}
	}

	return SetConsoleVariableFloat(pszVarName, fValue);
}

static void HandleControllerButton(WORD button, int commandId)
{
	auto& btnState = g_Controller.gameButtons[button];
	bool isPressed;

	// Activate instead of Reload
	if (commandId == 88 && (gState.canActivate || gState.canSwap || gState.isOperatingTurret))
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

	if (isPressed && !btnState.isPressed)
	{
		// Press Escape to show the menu
		if (commandId == -1)
		{
			PostMessage(gState.hWnd, WM_KEYDOWN, VK_ESCAPE, 0);
		}
		else
		{
			g_Controller.commandActive[commandId] = true;
			OnCommandOn(gState.g_pGameClientShell, commandId);
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
			PostMessage(gState.hWnd, WM_KEYUP, VK_ESCAPE, 0);
		}
		else
		{
			g_Controller.commandActive[commandId] = false;
			OnCommandOff(gState.g_pGameClientShell, commandId);
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
		HUDWeaponListUpdateTriggerNames(gState.CHUDWeaponList);
		HUDGrenadeListUpdateTriggerNames(gState.CHUDGrenadeList);
		HUDSwapUpdateTriggerName();
	}

	if (!g_Controller.isConnected)
	{
		memset(g_Controller.commandActive, 0, sizeof(g_Controller.commandActive));
		return;
	}

	if (gState.isGamePaused)
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
						const DWORD elapsedSinceStart = currentTime - btnState.pressStartTime;
						const DWORD elapsedSinceLastRepeat = currentTime - btnState.lastRepeatTime;

						if (elapsedSinceStart > 500 && elapsedSinceLastRepeat > 100)
						{
							// Trigger repeat
							PostMessage(gState.hWnd, WM_KEYDOWN, MENU_NAVIGATION_MAP[i][1], 0);
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
				PostMessage(gState.hWnd, pressed ? WM_KEYDOWN : WM_KEYUP, MENU_NAVIGATION_MAP[i][1], 0);
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
					if (gState.pCurrentType != 0 && gState.maxCurrentType != -1)
					{
						gState.currentType = (gState.currentType + direction + gState.maxCurrentType) % gState.maxCurrentType;
						SetCurrentType(gState.pCurrentType, gState.currentType);
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

	// Hook 'LoadString' to override 'ScreenFailure_PressAnyKey'
	if (!gState.hookedLoadString)
	{
		gState.hookedLoadString = true;

		DWORD targetMemoryLocation = ScanModuleSignature(gState.GameClient, "8B 4C 24 18 03 C1 8B 0D ?? ?? ?? ?? 03 F7 85 C9", "LoadString");

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

static int __fastcall IsFrameComplete_Hook(int thisPtr, int* _ECX)
{
	if (gState.isUsingFpsLimiter)
	{
		gState.fpsLimiter.Limit();
	}

	if (XInputControllerSupport)
	{
		PollController();
	}

	return IsFrameComplete(thisPtr);
}

#pragma endregion

#pragma region Patches

static void ApplyFixDirectInputFps()
{
	// root cause documented by Methanhydrat: https://community.pcgamingwiki.com/files/file/789-directinput-fps-fix/
	if (!DisableRedundantHIDInit) return;

	switch (gState.CurrentFEARGame)
	{
		case FEAR:
			MemoryHelper::MakeNOP(0x4840DD, 22, true);
			break;
		case FEARMP:
			MemoryHelper::MakeNOP(0x4841FD, 22, true);
			break;
		case FEARXP:
			MemoryHelper::MakeNOP(0x4B895D, 22, true);
			break;
		case FEARXP2:
			MemoryHelper::MakeNOP(0x4B99AD, 22, true);
			break;
	}
}

static void ApplyFixHighFPSPhysics()
{
	if (!FixHighFPSPhysics) return;

	switch (gState.CurrentFEARGame)
	{
		case FEAR:
			HookHelper::ApplyHook((void*)0x495CD0, &StepPhysicsSimulation_Hook, (LPVOID*)&StepPhysicsSimulation);
			break;
		case FEARMP:
			HookHelper::ApplyHook((void*)0x495DF0, &StepPhysicsSimulation_Hook, (LPVOID*)&StepPhysicsSimulation);
			break;
		case FEARXP:
			HookHelper::ApplyHook((void*)0x4D9480, &StepPhysicsSimulation_Hook, (LPVOID*)&StepPhysicsSimulation);
			break;
		case FEARXP2:
			HookHelper::ApplyHook((void*)0x4DA4F0, &StepPhysicsSimulation_Hook, (LPVOID*)&StepPhysicsSimulation);
			break;
	}
}

static void ApplyFixKeyboardInputLanguage()
{
	if (!FixKeyboardInputLanguage) return;

	switch (gState.CurrentFEARGame)
	{
		case FEAR:
			HookHelper::ApplyHook((void*)0x481E10, &GetDeviceObjectDesc_Hook, (LPVOID*)&GetDeviceObjectDesc);
			break;
		case FEARMP:
			HookHelper::ApplyHook((void*)0x481F30, &GetDeviceObjectDesc_Hook, (LPVOID*)&GetDeviceObjectDesc);
			break;
		case FEARXP:
			HookHelper::ApplyHook((void*)0x4B5DE0, &GetDeviceObjectDesc_Hook, (LPVOID*)&GetDeviceObjectDesc);
			break;
		case FEARXP2:
			HookHelper::ApplyHook((void*)0x4B6E10, &GetDeviceObjectDesc_Hook, (LPVOID*)&GetDeviceObjectDesc);
			break;
	}
}

static void ApplyReducedMipMapBias()
{
	if (!ReducedMipMapBias) return;

	switch (gState.CurrentFEARGame)
	{
		case FEAR:
		case FEARMP:
			MemoryHelper::WriteMemory<float>(0x56D5C4, -0.5f, false);
			break;
		case FEARXP:
			MemoryHelper::WriteMemory<float>(0x612B94, -0.5f, false);
			break;
		case FEARXP2:
			MemoryHelper::WriteMemory<float>(0x614BA4, -0.5f, false);
			break;
	}
}

static void ApplyClientHook()
{
	switch (gState.CurrentFEARGame)
	{
		case FEAR:
			HookHelper::ApplyHook((void*)0x47D730, &LoadGameDLL_Hook, (LPVOID*)&LoadGameDLL);
			break;
		case FEARMP:
			HookHelper::ApplyHook((void*)0x47D850, &LoadGameDLL_Hook, (LPVOID*)&LoadGameDLL);
			break;
		case FEARXP:
			HookHelper::ApplyHook((void*)0x4AF260, &LoadGameDLL_Hook, (LPVOID*)&LoadGameDLL);
			break;
		case FEARXP2:
			HookHelper::ApplyHook((void*)0x4B02C0, &LoadGameDLL_Hook, (LPVOID*)&LoadGameDLL);
			break;
	}
}

static void ApplySkipIntroHook()
{
	switch (gState.CurrentFEARGame)
	{
		case FEAR:
			HookHelper::ApplyHook((void*)0x510CB0, &FindStringCaseInsensitive_Hook, (LPVOID*)&FindStringCaseInsensitive);
			break;
		case FEARMP:
			HookHelper::ApplyHook((void*)0x510DD0, &FindStringCaseInsensitive_Hook, (LPVOID*)&FindStringCaseInsensitive);
			break;
		case FEARXP:
			HookHelper::ApplyHook((void*)0x5B3440, &FindStringCaseInsensitive_Hook, (LPVOID*)&FindStringCaseInsensitive);
			break;
		case FEARXP2:
			HookHelper::ApplyHook((void*)0x5B49C0, &FindStringCaseInsensitive_Hook, (LPVOID*)&FindStringCaseInsensitive);
			break;
	}
}

static void ApplyConsoleVariableHook()
{
	switch (gState.CurrentFEARGame)
	{
		case FEAR:
		case FEARMP:
			HookHelper::ApplyHook((void*)0x409360, &SetConsoleVariableFloat_Hook, (LPVOID*)&SetConsoleVariableFloat);
			break;
		case FEARXP:
			HookHelper::ApplyHook((void*)0x410120, &SetConsoleVariableFloat_Hook, (LPVOID*)&SetConsoleVariableFloat);
			break;
		case FEARXP2:
			HookHelper::ApplyHook((void*)0x410360, &SetConsoleVariableFloat_Hook, (LPVOID*)&SetConsoleVariableFloat);
			break;
	}
}

static void ApplyAutoResolution()
{
	if (!AutoResolution) return;

	switch (gState.CurrentFEARGame)
	{
		case FEAR:
		case FEARMP:
			MemoryHelper::WriteMemory<int>(0x56ABF4, gState.screenWidth, true);
			MemoryHelper::WriteMemory<int>(0x56ABF8, gState.screenHeight, true);
			break;
		case FEARXP:
			MemoryHelper::WriteMemory<int>(0x60EC2C, gState.screenWidth, true);
			MemoryHelper::WriteMemory<int>(0x60EC30, gState.screenHeight, true);
			break;
		case FEARXP2:
			MemoryHelper::WriteMemory<int>(0x610C2C, gState.screenWidth, true);
			MemoryHelper::WriteMemory<int>(0x610C30, gState.screenHeight, true);
			break;
	}
}

static void HookIsFrameComplete()
{
	gState.isUsingFpsLimiter = MaxFPS != 0;

	if (!gState.isUsingFpsLimiter && !XInputControllerSupport) return;

	gState.fpsLimiter.SetTargetFps(MaxFPS);

	switch (gState.CurrentFEARGame)
	{
		case FEAR:
			HookHelper::ApplyHook((void*)0x40FB20, &IsFrameComplete_Hook, (LPVOID*)&IsFrameComplete);
			break;
		case FEARMP:
			HookHelper::ApplyHook((void*)0x40FC30, &IsFrameComplete_Hook, (LPVOID*)&IsFrameComplete);
			break;
		case FEARXP:
			HookHelper::ApplyHook((void*)0x419100, &IsFrameComplete_Hook, (LPVOID*)&IsFrameComplete);
			break;
		case FEARXP2:
			HookHelper::ApplyHook((void*)0x4192B0, &IsFrameComplete_Hook, (LPVOID*)&IsFrameComplete);
			break;
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
	ApplySkipIntroHook();
	ApplyConsoleVariableHook();
}

static HWND WINAPI CreateWindowExA_Hook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	if (lpWindowName && strstr(lpWindowName, "F.E.A.R.") && dwStyle == 0xC10000 && nWidth == 320 && nHeight == 200)
	{
		MH_DisableHook(MH_ALL_HOOKS);
		Init();
	}

	gState.hWnd = ori_CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	return gState.hWnd;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		{
			uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
			IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)(base);
			IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
			DWORD timestamp = nt->FileHeader.TimeDateStamp;

			switch (timestamp)
			{
				case FEAR_TIMESTAMP:
					gState.CurrentFEARGame = FEAR;
					break;
				case FEARMP_TIMESTAMP:
					gState.CurrentFEARGame = FEARMP;
					break;
				case FEARXP_TIMESTAMP:
				case FEARXP_TIMESTAMP2:
					gState.CurrentFEARGame = FEARXP;
					break;
				case FEARXP2_TIMESTAMP:
					gState.CurrentFEARGame = FEARXP2;
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