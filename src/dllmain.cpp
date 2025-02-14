#define MINI_CASE_SENSITIVE
#define NOMINMAX

#include <Windows.h>
#include <d3d9.h>
#include <unordered_set>
#include <chrono>

#include "MinHook.hpp"
#include "ini.hpp"
#include "dllmain.hpp"
#include "helper.hpp"
#include "FpsLimiter.hpp"

#pragma comment(lib, "libMinHook.x86.lib")

// =============================
// Original Function Pointers
// =============================
intptr_t(__cdecl* LoadGameDLL)(char*, char, DWORD*) = nullptr;
int(__stdcall* SetConsoleVariableFloat)(const char*, float) = nullptr;
int(__thiscall* FindStringCaseInsensitive)(DWORD*, char*) = nullptr;
void(__thiscall* HUDTerminate)(int) = nullptr;
char(__thiscall* HUDInit)(int) = nullptr;
int(__thiscall* ScreenDimsChanged)(int) = nullptr;
DWORD* (__stdcall* LayoutDBGetPosition)(DWORD*, int, char*, int) = nullptr;
float* (__stdcall* GetRectF)(DWORD*, int, char*, int) = nullptr;
int(__stdcall* LayoutDBGetRecord)(int, char*) = nullptr;
int(__stdcall* LayoutDBGetInt32)(int, unsigned int, int) = nullptr;
float(__stdcall* LayoutDBGetFloat)(int, unsigned int, float) = nullptr;
const char*(__stdcall* LayoutDBGetString)(int, unsigned int, int) = nullptr;
HANDLE(WINAPI* ori_CreateMutexA)(LPSECURITY_ATTRIBUTES, BOOL, LPCSTR);
HRESULT(WINAPI* ori_Present)(IDirect3DDevice9*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);

// =============================
// Constants 
// =============================
const int FEAR_TIMESTAMP = 0x44EF6AE6;
const int FEARMP_TIMESTAMP = 0x44EF6ADB;
const int FEARXP_TIMESTAMP = 0x450B3629;
const int FEARXP_TIMESTAMP2 = 0x450DA808;
const int FEARXP2_TIMESTAMP = 0x46FC10A3;

const int FEAR_CLIENT_TIMESTAMP = 0x44EF6B27;
const int FEARXP_CLIENT_TIMESTAMP = 0x450DA85F;
const int FEARXP2_CLIENT_TIMESTAMP = 0x46FC10FE;

constexpr float BASE_WIDTH = 960.0f;
constexpr float BASE_HEIGHT = 720.0f;

enum FEARGAME
{
	FEAR,
	FEARXP,
	FEARXP2
};

// =============================
// Global State
// =============================
struct GlobalState
{
	FEARGAME CurrentFEARGame;

	int currentWidth = 0;
	int currentHeight = 0;
	float scalingFactor = 0;
	float crosshairSize = 0;
	int CHUDMgr = 0;

	bool isClientLoaded = false;
	bool skipClientPatching = false;
	HMODULE GameClient = NULL;
	HMODULE GameServer = NULL;

	FpsLimiter fpsLimiter{ 120.0f };

	bool updateLayoutReturnValue = false;
	bool slowMoBarUpdated = false;
	int healthAdditionalIntIndex = 0;
	int int32ToUpdate = 0;
	float floatToUpdate = 0;

	struct DataEntry
	{
		int TextSize;
		int ScaleType;
	};

	static std::unordered_map<std::string, DataEntry> textDataMap;
	static std::unordered_map<std::string, std::unordered_set<std::string>> hudScalingRules;
};

// Initialize static members
std::unordered_map<std::string, GlobalState::DataEntry> GlobalState::textDataMap;
std::unordered_map<std::string, std::unordered_set<std::string>> GlobalState::hudScalingRules;

// Global instance
GlobalState gState;

// =============================
// Ini Variables
// =============================

// Fixes
bool DisableRedundantHIDInit = false;
bool DisableXPWidescreenFiltering = false;

// Graphics
float MaxFPS = 0;
bool NoLODBias = false;
bool NoMipMapBias = false;

// Display
bool HUDScaling = false;
float HUDCustomScalingFactor = 0;
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

static void ReadConfig()
{
	IniHelper::Init();

	// Fixes
	DisableRedundantHIDInit = IniHelper::ReadInteger("Fixes", "DisableRedundantHIDInit", 1) == 1;
	DisableXPWidescreenFiltering = IniHelper::ReadInteger("Fixes", "DisableXPWidescreenFiltering", 1) == 1;

	// Graphics
	MaxFPS = IniHelper::ReadFloat("Graphics", "MaxFPS", 120.0f);
	NoLODBias = IniHelper::ReadInteger("Graphics", "NoLODBias", 1) == 1;
	NoMipMapBias = IniHelper::ReadInteger("Graphics", "NoMipMapBias", 1) == 1;

	// Display
	HUDScaling = IniHelper::ReadInteger("Display", "HUDScaling", 1) == 1;
	HUDCustomScalingFactor = IniHelper::ReadFloat("Display", "HUDCustomScalingFactor", 1.0f);
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
}

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
		result[0] = static_cast<DWORD>((int)result[0] * gState.scalingFactor);
		result[1] = static_cast<DWORD>((int)result[1] * gState.scalingFactor);
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

			if (scalingMode == 1) // Fit the text in the HUD
			{
				scaledSize = std::round(scaledSize * 0.95f);
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
static const char* __stdcall LayoutDBGetString_Hook(int a1, unsigned int a2, int a3)
{
	return LayoutDBGetString(a1, a2, a3);
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

		// Update the size of the crosshair
		SetConsoleVariableFloat("CrosshairSize", gState.crosshairSize * (gState.scalingFactor / HUDCustomScalingFactor));
		SetConsoleVariableFloat("PerturbScale", 0.5f * (gState.scalingFactor / HUDCustomScalingFactor));
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

#pragma endregion

#pragma region Client Patches

static void ValidateClientTimestamp(int currentGame, int expectedTimestamp)
{
	if (currentGame != expectedTimestamp)
	{
		gState.skipClientPatching = true;
		MessageBoxA(NULL, "Unsupported version of GameClient.dll\nClient patching will be skipped.", "EchoPatch", MB_OK | MB_ICONWARNING);
	}
}

// Check if the version of the client is compatible
static void VerifyClientCompatibility()
{
	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)gState.GameClient;
	PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)gState.GameClient + dosHeader->e_lfanew);
	DWORD timestamp = ntHeaders->FileHeader.TimeDateStamp;

	switch (gState.CurrentFEARGame)
	{
		case FEAR:
			ValidateClientTimestamp(timestamp, FEAR_CLIENT_TIMESTAMP);
			break;
		case FEARXP:
			ValidateClientTimestamp(timestamp, FEARXP_CLIENT_TIMESTAMP);
			break;
		case FEARXP2:
			ValidateClientTimestamp(timestamp, FEARXP2_CLIENT_TIMESTAMP);
			break;
	}
}

static void ApplyClientPatch()
{
	if (gState.skipClientPatching) return;

	DWORD ClientBaseAddress = (DWORD)gState.GameClient;

	if (DisableXPWidescreenFiltering && gState.CurrentFEARGame == FEARXP)
	{
		MemoryHelper::MakeNOP(ClientBaseAddress + 0x10DDB0, 24, true);
	}

	if (SkipSplashScreen)
	{
		switch (gState.CurrentFEARGame)
		{
			case FEAR:
				MemoryHelper::MakeNOP(ClientBaseAddress + 0x81A5D, 8, true);
				break;
			case FEARXP:
				MemoryHelper::MakeNOP(ClientBaseAddress + 0xA772D, 8, true);
				break;
			case FEARXP2:
				MemoryHelper::MakeNOP(ClientBaseAddress + 0xA97AD, 8, true);
				break;
		}
	}

	if (DisableLetterbox)
	{
		switch (gState.CurrentFEARGame)
		{
			case FEAR:
				MemoryHelper::WriteMemory<char>(ClientBaseAddress + 0x81E10, 0xC3, true);
				break;
			case FEARXP:
				MemoryHelper::WriteMemory<char>(ClientBaseAddress + 0xA7AE0, 0xC3, true);
				break;
			case FEARXP2:
				MemoryHelper::WriteMemory<char>(ClientBaseAddress + 0xA9BF0, 0xC3, true);
				break;
		}
	}

	if (HUDScaling)
	{
		if (gState.CurrentFEARGame == FEAR)
		{
			int pGameDatabase = MemoryHelper::ReadMemory<int>(ClientBaseAddress + 0x1AFA30, false);
			int pLayoutDB = MemoryHelper::ReadMemory<int>(pGameDatabase, false);

			HookHelper::ApplyHook((void*)(ClientBaseAddress + 0x6FDE0), &HUDTerminate_Hook, (LPVOID*)&HUDTerminate);
			HookHelper::ApplyHook((void*)(ClientBaseAddress + 0x705A0), &HUDInit_Hook, (LPVOID*)&HUDInit);
			HookHelper::ApplyHook((void*)(ClientBaseAddress + 0x85E50), &ScreenDimsChanged_Hook, (LPVOID*)&ScreenDimsChanged);
			HookHelper::ApplyHook((void*)(ClientBaseAddress + 0x88330), &LayoutDBGetPosition_Hook, (LPVOID*)&LayoutDBGetPosition);
			HookHelper::ApplyHook((void*)(ClientBaseAddress + 0x88FE0), &GetRectF_Hook, (LPVOID*)&GetRectF);
			HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x58), &LayoutDBGetRecord_Hook, (LPVOID*)&LayoutDBGetRecord);
			HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x7C), &LayoutDBGetInt32_Hook, (LPVOID*)&LayoutDBGetInt32);
			HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x80), &LayoutDBGetFloat_Hook, (LPVOID*)&LayoutDBGetFloat);
			HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x84), &LayoutDBGetString_Hook, (LPVOID*)&LayoutDBGetString);
		}
		else if (gState.CurrentFEARGame == FEARXP)
		{
			int pGameDatabase = MemoryHelper::ReadMemory<int>(ClientBaseAddress + 0x216018, false);
			int pLayoutDB = MemoryHelper::ReadMemory<int>(pGameDatabase, false);

			HookHelper::ApplyHook((void*)(ClientBaseAddress + 0x8FD20), &HUDTerminate_Hook, (LPVOID*)&HUDTerminate);
			HookHelper::ApplyHook((void*)(ClientBaseAddress + 0x911D0), &HUDInit_Hook, (LPVOID*)&HUDInit);
			HookHelper::ApplyHook((void*)(ClientBaseAddress + 0xAB150), &ScreenDimsChanged_Hook, (LPVOID*)&ScreenDimsChanged);
			HookHelper::ApplyHook((void*)(ClientBaseAddress + 0xADDB0), &LayoutDBGetPosition_Hook, (LPVOID*)&LayoutDBGetPosition);
			HookHelper::ApplyHook((void*)(ClientBaseAddress + 0xAEA60), &GetRectF_Hook, (LPVOID*)&GetRectF);
			HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x58), &LayoutDBGetRecord_Hook, (LPVOID*)&LayoutDBGetRecord);
			HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x7C), &LayoutDBGetInt32_Hook, (LPVOID*)&LayoutDBGetInt32);
			HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x80), &LayoutDBGetFloat_Hook, (LPVOID*)&LayoutDBGetFloat);
			HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x84), &LayoutDBGetString_Hook, (LPVOID*)&LayoutDBGetString);
		}
		else if (gState.CurrentFEARGame == FEARXP2)
		{
			int pGameDatabase = MemoryHelper::ReadMemory<int>(ClientBaseAddress + 0x237910, false);
			int pLayoutDB = MemoryHelper::ReadMemory<int>(pGameDatabase, false);

			HookHelper::ApplyHook((void*)(ClientBaseAddress + 0x92320), &HUDTerminate_Hook, (LPVOID*)&HUDTerminate);
			HookHelper::ApplyHook((void*)(ClientBaseAddress + 0x93840), &HUDInit_Hook, (LPVOID*)&HUDInit);
			HookHelper::ApplyHook((void*)(ClientBaseAddress + 0xAE5A0), &ScreenDimsChanged_Hook, (LPVOID*)&ScreenDimsChanged);
			HookHelper::ApplyHook((void*)(ClientBaseAddress + 0xB1270), &LayoutDBGetPosition_Hook, (LPVOID*)&LayoutDBGetPosition);
			HookHelper::ApplyHook((void*)(ClientBaseAddress + 0xB1F20), &GetRectF_Hook, (LPVOID*)&GetRectF);
			HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x58), &LayoutDBGetRecord_Hook, (LPVOID*)&LayoutDBGetRecord);
			HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x7C), &LayoutDBGetInt32_Hook, (LPVOID*)&LayoutDBGetInt32);
			HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x80), &LayoutDBGetFloat_Hook, (LPVOID*)&LayoutDBGetFloat);
			HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x84), &LayoutDBGetString_Hook, (LPVOID*)&LayoutDBGetString);
		}

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
		gState.textDataMap["HUDChatInput"] = { 16, 0 };
		gState.textDataMap["HUDChatMessage"] = { 14, 0 };
		gState.textDataMap["HUDControlPoint"] = { 24, 0 };
		gState.textDataMap["HUDControlPointBar"] = { 24, 0 };
		gState.textDataMap["HUDControlPointList"] = { 24, 0 };
		gState.textDataMap["HUDCrosshair"] = { 12, 0 };
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
		gState.textDataMap["HUDGameMessage"] = { 14, 0 };
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
		gState.textDataMap["HUDSubtitle"] = { 14, 0 };
		gState.textDataMap["HUDSwap"] = { 12, 0 };
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
			{"HUDHealth",      {"AdditionalPoint", "IconSize", "IconOffset", "TextOffset",}},
			{"HUDDialogue",    {"AdditionalPoint", "IconSize", "TextOffset"}},
			{"HUDGrenadeList", {"AdditionalPoint", "IconSize", "TextOffset"}},
			{"HUDWeapon",      {"AdditionalPoint", "IconSize", "TextOffset"}},
			{"HUDArmor",       {"IconSize", "IconOffset", "TextOffset"}},
			{"HUDSwap",        {"IconSize", "IconOffset", "TextOffset"}},
			{"HUDGear",        {"IconSize", "IconOffset", "TextOffset"}},
			{"HUDGrenade",     {"IconSize", "IconOffset", "TextOffset"}},
			{"HUDAmmo",        {"IconSize", "IconOffset", "TextOffset"}},
			{"HUDFlashlight",  {"IconSize", "IconOffset"}},
			{"HUDSlowMo2",     {"IconSize", "IconOffset"}},
		};
	}
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

			VerifyClientCompatibility();
			ApplyClientPatch();
		}
		else // Otherwise server
		{
			gState.GameServer = ApiDLL;
		}
	}
	return result;
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
			switch (gState.CurrentFEARGame)
			{
				case FEAR:
					SystemHelper::SimulateSpacebarPress(0x56C2CC);
					break;
				case FEARXP:
					SystemHelper::SimulateSpacebarPress(0x610304);
					break;
				case FEARXP2:
					SystemHelper::SimulateSpacebarPress(0x61230C);
					break;
			}
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

static int __stdcall SetConsoleVariableFloat_Hook(char* pszVarName, float fValue)
{
	if (NoLODBias && strcmp(pszVarName, "ModelLODDistanceScale") == 0)
	{
		fValue = 0.0f;
	}

	if (HUDScaling && strcmp(pszVarName, "CrosshairSize") == 0)
	{
		gState.crosshairSize = fValue; // Keep a backup of the value as it can be adjusted in the settings
		fValue = fValue * (gState.scalingFactor / HUDCustomScalingFactor);
	}

	if (HUDScaling && strcmp(pszVarName, "PerturbScale") == 0)
	{
		fValue = fValue * (gState.scalingFactor / HUDCustomScalingFactor);
	}

	return SetConsoleVariableFloat(pszVarName, fValue);
}

// Hook of IDirect3DDevice9::Present
static HRESULT WINAPI PresentHook(IDirect3DDevice9* device, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
	gState.fpsLimiter.Limit();
	return ori_Present(device, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
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
		case FEARXP:
			MemoryHelper::MakeNOP(0x4B895D, 22, true);
			break;
		case FEARXP2:
			MemoryHelper::MakeNOP(0x4B99AD, 22, true);
			break;
	}
}

static void ApplyNoLODBias()
{
	if (!NoLODBias) return;

	switch (gState.CurrentFEARGame)
	{
		case FEAR:
			MemoryHelper::WriteMemory<float>(0x56D864, -2.0f, false);
			break;
		case FEARXP:
			MemoryHelper::WriteMemory<float>(0x612E34, -2.0f, false);
			break;
		case FEARXP2:
			MemoryHelper::WriteMemory<float>(0x614E44, -2.0f, false);
			break;
	}
}

static void ApplyNoMipMapBias()
{
	if (!NoMipMapBias) return;

	switch (gState.CurrentFEARGame)
	{
		case FEAR:
			MemoryHelper::WriteMemory<float>(0x56D5C4, -2.0f, false);
			break;
		case FEARXP:
			MemoryHelper::WriteMemory<float>(0x612B94, -2.0f, false);
			break;
		case FEARXP2:
			MemoryHelper::WriteMemory<float>(0x614BA4, -2.0f, false);
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

// For FPS limiter
static void ApplyDirect3D9Hook()
{
	if (MaxFPS == 0) return;

	IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!pD3D)
	{
		MessageBoxA(nullptr, "Failed to create IDirect3D9 object!", "Error", MB_ICONERROR);
		return;
	}

	IDirect3DDevice9* pDummyDevice = nullptr;
	D3DPRESENT_PARAMETERS d3dpp = {};
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = GetDesktopWindow();

	if (FAILED(pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice)))
	{
		pD3D->Release();
		MessageBoxA(nullptr, "Failed to create dummy D3D9 device!", "Error", MB_ICONERROR);
		return;
	}

	void** pVTable = *reinterpret_cast<void***>(pDummyDevice);
	void* pPresent = pVTable[17]; // Index 17 = IDirect3DDevice9::Present

	HookHelper::ApplyHook(pPresent, PresentHook, (LPVOID*)&ori_Present);
	gState.fpsLimiter.SetTargetFps(MaxFPS);

	pDummyDevice->Release();
	pD3D->Release();
}

#pragma endregion

#pragma region Initialization

static void Init()
{
	ReadConfig();

	// Get the handle of the client as soon as it is loaded
	ApplyClientHook();

	// Fixes
	ApplyFixDirectInputFps();

	// Graphics
	ApplyNoMipMapBias();
	ApplyNoLODBias();

	// Misc
	ApplySkipIntroHook();
	ApplyConsoleVariableHook();
	ApplyDirect3D9Hook();
}

// Hook of the first function executed
static HANDLE WINAPI CreateMutexA_Hook(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName)
{
	// Identify calls of interest, filter out the SecuROM ones (FEAR + FEARXP2 || FEARXP)
	if (strcmp(lpName, "{CB7DD3FC-417D-4c29-9BFD-CA781BF1B207}") == 0 || strcmp(lpName, "{4227F87E-A775-48e0-831A-210F7F7EE351}") == 0)
	{
		// Once Steam DRM and SecuROM did their thing, disable this hook and do the patching
		MH_DisableHook(MH_ALL_HOOKS);
		Init();
	}

	return ori_CreateMutexA(lpMutexAttributes, bInitialOwner, lpName);
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
				case FEARXP_TIMESTAMP:
				case FEARXP_TIMESTAMP2:
					gState.CurrentFEARGame = FEARXP;
					break;
				case FEARXP2_TIMESTAMP:
					gState.CurrentFEARGame = FEARXP2;
					break;
				case FEARMP_TIMESTAMP:
					SystemHelper::LoadProxyLibrary();
					return TRUE; // Skip MP
				default:
					MessageBoxA(NULL, "This .exe is not supported.", "EchoPatch", MB_ICONERROR);
					return false;
			}

			SystemHelper::LoadProxyLibrary();
			HookHelper::ApplyHookAPI(L"kernel32", "CreateMutexA", &CreateMutexA_Hook, (LPVOID*)&ori_CreateMutexA);
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