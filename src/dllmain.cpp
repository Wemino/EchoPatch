#define MINI_CASE_SENSITIVE
#define NOMINMAX

#include <Windows.h>
#include <unordered_set>

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
char(__thiscall* LoadClientFXDLL)(int, char*, char) = nullptr;
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
int(__thiscall* UpdateSlider)(int, int) = nullptr;
float(__stdcall* GetShatterLifetime)(int) = nullptr;
int(__thiscall* IsFrameComplete)(int) = nullptr;
int(__stdcall* CreateFX)(char*, int, int) = nullptr;
int(__thiscall* GetDeviceObjectDesc)(int, unsigned int, wchar_t*, unsigned int*) = nullptr;
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

	int currentWidth = 0;
	int currentHeight = 0;
	float scalingFactor = 0;
	float scalingFactorText = 0;
	float scalingFactorCrosshair = 0;
	float crosshairSize = 0;
	int CHUDMgr = 0;

	int screenWidth = 0;
	int screenHeight = 0;

	bool isClientLoaded = false;
	HMODULE GameClient = NULL;
	HMODULE GameServer = NULL;

	FpsLimiter fpsLimiter{ 120.0f };

	bool updateLayoutReturnValue = false;
	bool slowMoBarUpdated = false;
	int healthAdditionalIntIndex = 0;
	int int32ToUpdate = 0;
	float floatToUpdate = 0;
	bool crosshairSliderUpdated = false;

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
bool FixKeyboardInputLanguage = false;

// Graphics
float MaxFPS = 0;
bool NoLODBias = false;
bool NoMipMapBias = false;
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
bool ShowErrors = false;

static void ReadConfig()
{
	IniHelper::Init(gState.CurrentFEARGame == FEARXP || gState.CurrentFEARGame == FEARXP2);

	// Fixes
	DisableRedundantHIDInit = IniHelper::ReadInteger("Fixes", "DisableRedundantHIDInit", 1) == 1;
	DisableXPWidescreenFiltering = IniHelper::ReadInteger("Fixes", "DisableXPWidescreenFiltering", 1) == 1;
	FixKeyboardInputLanguage = IniHelper::ReadInteger("Fixes", "FixKeyboardInputLanguage", 1) == 1;

	// Graphics
	MaxFPS = IniHelper::ReadFloat("Graphics", "MaxFPS", 120.0f);
	NoLODBias = IniHelper::ReadInteger("Graphics", "NoLODBias", 1) == 1;
	NoMipMapBias = IniHelper::ReadInteger("Graphics", "NoMipMapBias", 0) == 1;
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
	InfiniteFlashlight = IniHelper::ReadInteger("Extra", "InfiniteFlashlight", 1) == 1;
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

#pragma endregion

#pragma region Client Patches

static DWORD ScanClientSignature(std::string_view signature, const char* patchName)
{
	DWORD targetMemoryLocation = MemoryHelper::PatternScan(gState.GameClient, signature);

	if (targetMemoryLocation == 0 && ShowErrors)
	{
		std::string errorMessage = "Error: Unable to find signature for patch: ";
		errorMessage += patchName;
		MessageBoxA(NULL, errorMessage.c_str(), "EchoPatch", MB_ICONERROR);
	}

	return targetMemoryLocation;
}

static void ApplyClientPatch()
{
	DWORD ClientBaseAddress = (DWORD)gState.GameClient;

	if (DisableXPWidescreenFiltering && gState.CurrentFEARGame == FEARXP)
	{
		MemoryHelper::MakeNOP(ClientBaseAddress + 0x10DDB0, 24, true);
	}

	if (SkipSplashScreen)
	{
		DWORD targetMemoryLocation = ScanClientSignature("53 8B 5C 24 08 55 8B 6C 24 14 56 8D 43 FF 83 F8", "SkipSplashScreen");

		if (targetMemoryLocation != 0)
		{
			MemoryHelper::MakeNOP(targetMemoryLocation + 0x13D, 8, true);
		}
	}

	if (DisableLetterbox)
	{
		DWORD targetMemoryLocation = ScanClientSignature("83 EC 54 53 55 56 57 8B", "DisableLetterbox");

		if (targetMemoryLocation != 0)
		{
			MemoryHelper::WriteMemory<uint8_t>(targetMemoryLocation, 0xC3, true);
		}
	}

	if (EnablePersistentWorldState)
	{	
		DWORD targetMemoryLocation_ShellCasing = ScanClientSignature("D9 86 88 00 00 00 D8 64 24", "EnablePersistentWorldState_ShellCasing");
		DWORD targetMemoryLocation_DecalSaving = ScanClientSignature("FF 52 0C ?? 8D ?? ?? ?? 00 00 E8 ?? ?? ?? FF 8B", "EnablePersistentWorldState_DecalSaving");
		DWORD targetMemoryLocation_Decal = ScanClientSignature("DF E0 F6 C4 01 75 34 DD 44 24", "EnablePersistentWorldState_Decal");
		DWORD targetMemoryLocation_FX = ScanClientSignature("8B CE FF ?? 04 84 C0 75 ?? 8B ?? 8B CE FF ?? 08 56 E8", "EnablePersistentWorldState_FX");
		DWORD targetMemoryLocation_Shatter = ScanClientSignature("8B C8 E8 ?? ?? ?? 00 D9 5C 24 ?? D9", "EnablePersistentWorldState_Shatter");

		if (targetMemoryLocation_ShellCasing != 0)
		{
			MemoryHelper::MakeNOP(targetMemoryLocation_ShellCasing + 0x6, 4, true);
		}

		if (targetMemoryLocation_DecalSaving != 0)
		{
			MemoryHelper::MakeNOP(targetMemoryLocation_DecalSaving + 0xF, 13, true);
		}

		if (targetMemoryLocation_Decal != 0)
		{
			MemoryHelper::WriteMemory<uint8_t>(targetMemoryLocation_Decal + 0x5, 0x74, true);
		}

		if (targetMemoryLocation_FX != 0)
		{
			// Not pretty, for devmode compatibility 
			for (int i = 0; i < 0x1000; i++) 
			{
				if (MemoryHelper::ReadMemory<uint8_t>(targetMemoryLocation_FX - 1) == 0xCC)
				{
					break;
				}
				else
				{
					targetMemoryLocation_FX--;
				}
			}

			HookHelper::ApplyHook((void*)(targetMemoryLocation_FX), &CreateFX_Hook, (LPVOID*)&CreateFX);
		}

		if (targetMemoryLocation_Shatter != 0)
		{
			int callAddr = MemoryHelper::ReadMemory<int>(targetMemoryLocation_Shatter + 0x3);
			int shatterLiftetimeAddress = (targetMemoryLocation_Shatter + 0x3) + (callAddr + 0x4);
			HookHelper::ApplyHook((void*)(shatterLiftetimeAddress), &GetShatterLifetime_Hook, (LPVOID*)&GetShatterLifetime);
		}
	}

	if (InfiniteFlashlight)
	{
		DWORD targetMemoryLocation_HUD = ScanClientSignature("8B 51 10 8A 42 18 84 C0 8A 86 04 01 00 00", "InfiniteFlashlight_HUD");
		DWORD targetMemoryLocation_Battery = ScanClientSignature("D8 4C 24 04 DC AE 88 03 00 00 DD 96 88 03 00 00", "InfiniteFlashlight_Battery");

		if (targetMemoryLocation_HUD != 0)
		{
			MemoryHelper::WriteMemory<uint8_t>(targetMemoryLocation_HUD - 0x31, 0xC3, true);
		}

		if (targetMemoryLocation_Battery != 0)
		{
			MemoryHelper::MakeNOP(targetMemoryLocation_Battery + 0xA, 6, true);
		}
	}

	if (HUDScaling)
	{
		DWORD targetMemoryLocation_GameDatabase = ScanClientSignature("8B 5E 08 55 E8 ?? ?? ?? FF 8B 0D ?? ?? ?? ?? 8B 39 68 ?? ?? ?? ?? 6A 00 68 ?? ?? ?? ?? 53 FF 57", "HUDScaling_GameDatabase");
		if (targetMemoryLocation_GameDatabase == 0) return;

		int pDB = MemoryHelper::ReadMemory<int>(targetMemoryLocation_GameDatabase + 0xB);
		int pGameDatabase = MemoryHelper::ReadMemory<int>(pDB);
		int pLayoutDB = MemoryHelper::ReadMemory<int>(pGameDatabase);

		HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x58), &LayoutDBGetRecord_Hook, (LPVOID*)&LayoutDBGetRecord);
		HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x7C), &LayoutDBGetInt32_Hook, (LPVOID*)&LayoutDBGetInt32);
		HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x80), &LayoutDBGetFloat_Hook, (LPVOID*)&LayoutDBGetFloat);
		HookHelper::ApplyHook((void*)*(int*)(pLayoutDB + 0x84), &LayoutDBGetString_Hook, (LPVOID*)&LayoutDBGetString);

		DWORD targetMemoryLocation_HUDTerminate = ScanClientSignature("53 56 8B D9 8B B3 7C 04 00 00 8B 83 80 04 00 00 57 33 FF 3B F0", "HUDScaling_HUDTerminate");
		DWORD targetMemoryLocation_HUDInit = ScanClientSignature("8B ?? ?? 8D ?? 78 04 00 00", "HUDScaling_HUDInit");
		DWORD targetMemoryLocation_ScreenDimsChanged = ScanClientSignature("A1 ?? ?? ?? ?? 81 EC 98 00 00 00 85 C0 56 8B F1", "HUDScaling_ScreenDimsChanged");
		DWORD targetMemoryLocation_LayoutDBGetPosition = ScanClientSignature("83 EC 10 8B 54 24 20 8B 0D", "HUDScaling_LayoutDBGetPosition");
		DWORD targetMemoryLocation_GetRectF = ScanClientSignature("14 8B 44 24 28 8B 4C 24 18 D9 18", "HUDScaling_GetRectF");
		DWORD targetMemoryLocation_UpdateSlider = ScanClientSignature("56 8B F1 8B 4C 24 08 8B 86 7C 01 00 00 3B C8 89 8E 80 01 00 00", "HUDScaling_UpdateSlider");

		if (targetMemoryLocation_HUDTerminate != 0)
		{
			HookHelper::ApplyHook((void*)(targetMemoryLocation_HUDTerminate), &HUDTerminate_Hook, (LPVOID*)&HUDTerminate);
		}

		if (targetMemoryLocation_HUDInit != 0)
		{
			HookHelper::ApplyHook((void*)(targetMemoryLocation_HUDInit - 0x2), &HUDInit_Hook, (LPVOID*)&HUDInit);
		}

		if (targetMemoryLocation_ScreenDimsChanged != 0)
		{
			HookHelper::ApplyHook((void*)(targetMemoryLocation_ScreenDimsChanged), &ScreenDimsChanged_Hook, (LPVOID*)&ScreenDimsChanged);
		}

		if (targetMemoryLocation_LayoutDBGetPosition != 0)
		{
			HookHelper::ApplyHook((void*)(targetMemoryLocation_LayoutDBGetPosition), &LayoutDBGetPosition_Hook, (LPVOID*)&LayoutDBGetPosition);
		}

		if (targetMemoryLocation_GetRectF != 0)
		{
			HookHelper::ApplyHook((void*)(targetMemoryLocation_GetRectF - 0x58), &GetRectF_Hook, (LPVOID*)&GetRectF);
		}

		if (targetMemoryLocation_UpdateSlider != 0)
		{
			HookHelper::ApplyHook((void*)(targetMemoryLocation_UpdateSlider), &UpdateSlider_Hook, (LPVOID*)&UpdateSlider);
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
			switch (gState.CurrentFEARGame)
			{
				case FEAR:
				case FEARMP:
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

	if (AutoResolution)
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

static int __fastcall IsFrameComplete_Hook(int thisPtr, int* _ECX)
{
	gState.fpsLimiter.Limit();
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

static void ApplyNoLODBias()
{
	if (!NoLODBias) return;

	switch (gState.CurrentFEARGame)
	{
		case FEAR:
		case FEARMP:
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
		case FEARMP:
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

static void ApplyFPSLimiterHook()
{
	if (MaxFPS == 0) return;

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

	// Get the handle of the client as soon as it is loaded
	ApplyClientHook();

	// Fixes
	ApplyFixDirectInputFps();
	ApplyFixKeyboardInputLanguage();

	// Display
	ApplyAutoResolution();

	// Graphics
	ApplyNoMipMapBias();
	ApplyNoLODBias();

	// Misc
	ApplyFPSLimiterHook();
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

	return ori_CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
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