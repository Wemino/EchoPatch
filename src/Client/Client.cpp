#define NOMINMAX

#include "../Core/Core.hpp"
#include "../Controller/Controller.hpp"
#include "../ClientFX/ClientFX.hpp"
#include "../Server/Server.hpp"
#include "MinHook.hpp"
#include "../helper.hpp"

// ======================
// Client Function Pointers
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

// SDLGamepadSupport
bool(__thiscall* IsCommandOn)(int, int) = nullptr;
bool(__thiscall* OnCommandOn)(int, int) = nullptr;
bool(__thiscall* OnCommandOff)(int, int) = nullptr;
double(__thiscall* GetExtremalCommandValue)(int, int) = nullptr;
double(__thiscall* GetZoomMag)(int) = nullptr;
int(__thiscall* HUDActivateObjectSetObject)(int, void**, int, int, int, int) = nullptr;
int(__thiscall* SetOperatingTurret)(int, int) = nullptr;
const wchar_t* (__thiscall* GetTriggerNameFromCommandID)(int, int) = nullptr;
void(__thiscall* UseCursor)(int, bool) = nullptr;
bool(__thiscall* OnMouseMove)(int, int, int) = nullptr;
void(__thiscall* HUDSwapUpdate)(int) = nullptr;
void(__thiscall* SwitchToScreen)(int, int) = nullptr;
void(__thiscall* SetCurrentType)(int, int) = nullptr;
void(__cdecl* HUDSwapUpdateTriggerName)() = nullptr;
void(__thiscall* ApplyLocalRotationOffset)(int, float*) = nullptr;
void(__thiscall* UpdatePlayerMovement)(int) = nullptr;
void(__thiscall* BeginAim)(BYTE*) = nullptr;
void(__thiscall* EndAim)(BYTE*) = nullptr;
const wchar_t* (__stdcall* LoadGameString)(int, char*) = nullptr;
bool(__stdcall* DEditLoadModule)(const char*) = nullptr;

// ConsoleEnabled
void(__stdcall* SetInputState)(bool) = nullptr;

// EnableCustomMaxWeaponCapacity
uint8_t(__thiscall* GetWeaponCapacity)(int) = nullptr;

// DisableHipFireAccuracyPenalty
void(__thiscall* AccuracyMgrUpdate)(float*) = nullptr;

// SDLGamepadSupport & HUDScaling
int(__thiscall* HUDWeaponListUpdateTriggerNames)(int) = nullptr;
int(__thiscall* HUDGrenadeListUpdateTriggerNames)(int) = nullptr;

// EnableCustomMaxWeaponCapacity & WeaponFixes
void(__thiscall* OnEnterWorld)(int) = nullptr;

// SDLGamepadSupport & ConsoleEnabled
static int(__stdcall* HookedWindowProc)(HWND, UINT, WPARAM, LPARAM) = nullptr;
bool(__thiscall* ChangeState)(int, int, int) = nullptr;
void(__thiscall* MsgBoxShow)(int, const wchar_t*, int, int, bool) = nullptr;
void(__thiscall* MsgBoxHide)(int, int) = nullptr;

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
		g_State.jumpElapsedTime = 0.0;
	}

	UpdateOnGround(thisPtr);

	// Maintain jump state for a few frames
	if (g_State.jumpElapsedTime >= 0.0)
	{
		if (g_State.jumpElapsedTime < TARGET_FRAME_TIME)
		{
			*pJumped = true;
			g_State.jumpElapsedTime += g_State.currentFrameTime;
		}
		else
		{
			g_State.jumpElapsedTime = -1.0; // Mark as inactive
		}
	}

	// Update tracking state
	g_State.previousJumpState = *pJumped;
}

static void __fastcall UpdateWaveProp_Hook(int thisPtr, int, float frameDelta)
{
	// Updates water wave propagation at fixed time intervals for consistent simulation
	g_State.waveUpdateAccumulator += frameDelta;

	if (g_State.waveUpdateAccumulator > TARGET_FRAME_TIME * 5)
		g_State.waveUpdateAccumulator = TARGET_FRAME_TIME * 5;

	while (g_State.waveUpdateAccumulator >= TARGET_FRAME_TIME)
	{
		UpdateWaveProp(thisPtr, TARGET_FRAME_TIME);
		g_State.waveUpdateAccumulator -= TARGET_FRAME_TIME;
	}
}

static double __fastcall GetMaxRecentVelocityMag_Hook(int thisPtr, int)
{
	if (!g_State.useVelocitySmoothing)
		return GetMaxRecentVelocityMag(thisPtr);

	double dt = g_State.currentFrameTime;

	if (dt <= 0.0 || dt > 0.2)
		return g_State.lastReportedVelocity > 0.0 ? g_State.lastReportedVelocity : GetMaxRecentVelocityMag(thisPtr);

	g_State.velocityAccumulator += GetMaxRecentVelocityMag(thisPtr);
	g_State.velocityTimeAccumulator += dt;

	// Convert accumulated distance to normalized speed: (distance / time) * targetFrameTime
	double currentWindowSpeed = (g_State.velocityAccumulator / g_State.velocityTimeAccumulator) * TARGET_FRAME_TIME;

	bool timeIsUp = g_State.velocityTimeAccumulator >= 0.05;
	bool isStartingToMove = g_State.lastReportedVelocity < 0.1 && currentWindowSpeed > 0.1;

	if (timeIsUp || isStartingToMove)
	{
		if (isStartingToMove)
		{
			g_State.lastReportedVelocity = currentWindowSpeed;
			g_State.prevWindowSpeed = currentWindowSpeed;
		}
		else
		{
			// Max-of-two windows prevents aliasing dips at extreme FPS
			g_State.lastReportedVelocity = std::max(currentWindowSpeed, g_State.prevWindowSpeed);

			// Snap near-zero to true zero for clean idle state
			if (g_State.lastReportedVelocity < 0.01)
				g_State.lastReportedVelocity = 0.0;

			// Decay prevWindowSpeed slowly to prevent double-dip glitches
			if (currentWindowSpeed > g_State.prevWindowSpeed)
				g_State.prevWindowSpeed = currentWindowSpeed;
			else
				g_State.prevWindowSpeed = std::max(currentWindowSpeed, g_State.prevWindowSpeed * 0.95);
		}

		g_State.velocityAccumulator = 0.0;
		g_State.velocityTimeAccumulator = 0.0;
	}

	return g_State.lastReportedVelocity;
}

static void __cdecl PolyGridFXCollisionHandlerCB_Hook(int hBody1, int hBody2, int* a3, int* a4, float a5, BYTE* a6, int a7)
{
	uint32_t b1 = static_cast<uint32_t>(hBody1);
	uint32_t b2 = static_cast<uint32_t>(hBody2);

	// Create order-independent key so (A,B) and (B,A) collisions map to the same entry
	uint64_t key = (b1 < b2) ? (uint64_t(b2) << 32) | b1 : (uint64_t(b1) << 32) | b2;

	double currentGameTime = g_State.totalGameTime;

	// Search for existing entry in circular buffer cache
	GlobalState::SplashEntry* foundEntry = nullptr;
	for (auto& entry : g_State.splashCache)
	{
		if (entry.key == key)
		{
			foundEntry = &entry;
			break;
		}
	}

	// Only process splash if this pair hasn't splashed this frame
	if (!foundEntry || (currentGameTime - foundEntry->lastTime) >= TARGET_FRAME_TIME)
	{
		PolyGridFXCollisionHandlerCB(hBody1, hBody2, a3, a4, a5, a6, a7);

		if (foundEntry)
		{
			foundEntry->lastTime = currentGameTime;
		}
		else
		{
			// Overwrite oldest entry in circular buffer (size 64)
			g_State.splashCache[g_State.splashIndex] = { key, currentGameTime };
			g_State.splashIndex = (g_State.splashIndex + 1) % g_State.splashCache.size();
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
	g_State.currentKeyIndex = 0;
	LoadUserProfile(thisPtr, bLoadDefaults, bLoadDisplaySettings);
	g_State.isLoadingDefault = false;
}

static bool __fastcall RestoreDefaults_Hook(int thisPtr, int, uint8_t nFlags)
{
	g_State.isLoadingDefault = true;
	g_State.currentKeyIndex = 0;
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

// Scale HUD position or dimension
static DWORD* __stdcall LayoutDBGetPosition_Hook(DWORD* a1, int Record, char* Attribute, int a4)
{
	if (!Record)
		return LayoutDBGetPosition(a1, Record, Attribute, a4);

	DWORD* result = LayoutDBGetPosition(a1, Record, Attribute, a4);
	const char* hudRecordString = *reinterpret_cast<const char**>(Record);

	std::string_view hudElement(hudRecordString);
	std::string_view attribute(Attribute);

	auto hudEntry = g_State.hudScalingRules.find(hudElement);
	if (hudEntry != g_State.hudScalingRules.end())
	{
		const auto& rule = hudEntry->second;

		if (rule.attributes.contains(attribute))
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
	const char* hudRecordString = *reinterpret_cast<const char**>(Record);

	// Quick early-out: 'l' at index 4 matches "HUDSlowMo2" and "HUDFlashlight"
	if (hudRecordString[4] == 'l')
	{
		const uint32_t attrHash = HashHelper::FNV1aRuntime(Attribute);
		if (attrHash == HashHelper::HUDHashes::AdditionalRect)
		{
			const uint32_t elemHash = HashHelper::FNV1aRuntime(hudRecordString);
			if (elemHash == HashHelper::HUDHashes::HUDSlowMo2 || elemHash == HashHelper::HUDHashes::HUDFlashlight)
			{
				result[0] *= g_State.scalingFactor;
				result[1] *= g_State.scalingFactor;
				result[2] *= g_State.scalingFactor;
				result[3] *= g_State.scalingFactor;
			}
		}
	}

	return result;
}

// Override 'DBGetInt32' and other related functions to return specific values
static int __stdcall DBGetRecord_Hook(int Record, char* Attribute)
{
	if (!Record)
		return DBGetRecord(Record, Attribute);

	if (Attribute[5] == 'i')
	{
		const char* hudRecordString = *reinterpret_cast<const char**>(Record);
		const uint32_t attrHash = HashHelper::FNV1aRuntime(Attribute);
		const uint32_t elemHash = HashHelper::FNV1aRuntime(hudRecordString);

		// TextSize handling
		if (attrHash == HashHelper::HUDHashes::TextSize)
		{
			auto it = g_State.textDataMap.find(hudRecordString);
			if (it != g_State.textDataMap.end())
			{
				auto& dt = it->second;
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
		else if (Attribute[9] == 'l')
		{
			// AdditionalFloat handling
			if (attrHash == HashHelper::HUDHashes::AdditionalFloat)
			{
				float baseValue = 0.0f;
				if (!g_State.slowMoBarUpdated && elemHash == HashHelper::HUDHashes::HUDSlowMo2)
				{
					baseValue = 10.0f;
					g_State.slowMoBarUpdated = true;
				}
				else if (elemHash == HashHelper::HUDHashes::HUDFlashlight)
				{
					baseValue = 6.0f;
				}

				if (baseValue != 0.0f)
				{
					g_State.updateLayoutReturnValue = true;
					g_State.floatToUpdate = baseValue * g_State.scalingFactor;
				}
			}
			// AdditionalInt handling (HUDHealth medkit prompt)
			else if (attrHash == HashHelper::HUDHashes::AdditionalInt && elemHash == HashHelper::HUDHashes::HUDHealth)
			{
				if (g_State.healthAdditionalIntIndex == 2)
				{
					g_State.updateLayoutReturnValue = true;
					g_State.int32ToUpdate = static_cast<int32_t>(std::round(14 * g_State.scalingFactor));
				}
				else
				{
					g_State.healthAdditionalIntIndex++;
				}
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
	const char* sliderName = *reinterpret_cast<const char**>(thisPtr + 8);

	if (sliderName[0] != 'I' && sliderName[0] != 'S')  // IDS_* and Screen*
		return UpdateSlider(thisPtr, index);

	const uint32_t nameHash = HashHelper::FNV1aRuntime(sliderName);

	// If 'ScreenCrosshair_Size_Help' is next
	if (nameHash == HashHelper::StringHashes::IDS_HELP_PICKUP_MSG_DUR)
	{
		g_State.crosshairSliderUpdated = false;
	}

	// Update the index of 'ScreenCrosshair_Size_Help' as it will be wrong on the first time
	if (nameHash == HashHelper::StringHashes::ScreenCrosshair_Size_Help && !g_State.crosshairSliderUpdated && g_State.scalingFactorCrosshair > 1.0f)
	{
		float unscaledIndex = index / g_State.scalingFactorCrosshair;

		// scs.nIncrement = 2
		int newIndex = static_cast<int>((unscaledIndex / 2.0f) + 0.5f) * 2;

		// Clamp to the range [4, 16].
		index = std::clamp(newIndex, 4, 16);

		// Only needed on the first time
		g_State.crosshairSliderUpdated = true;
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
// SDLGamepadSupport
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
	if (!g_Controller.isConnected || g_State.isConsoleOpen)
		return GetExtremalCommandValue(thisPtr, commandId);

	SDL_Gamepad* pGamepad = GetGamepad();
	if (!pGamepad)
		return GetExtremalCommandValue(thisPtr, commandId);

	static constexpr int DEAD_ZONE = 7849;

	auto ReadAxis = [&](SDL_GamepadAxis axis) -> double
	{
		Sint16 value = SDL_GetGamepadAxis(pGamepad, axis);
		if (abs(value) < DEAD_ZONE)
			return 0.0;
		return value / 32768.0;
	};

	auto ApplyZoomScale = [](double value) -> double
	{
		if (g_State.zoomMag > GPadZoomMagThreshold)
			return value * (g_State.zoomMag / GPadZoomMagThreshold);
		return value;
	};

	switch (commandId)
	{
		case 2:  return -ReadAxis(SDL_GAMEPAD_AXIS_LEFTY);                      // Forward
		case 5:  return  ReadAxis(SDL_GAMEPAD_AXIS_LEFTX);                      // Strafe
		case 22: return  ApplyZoomScale(ReadAxis(SDL_GAMEPAD_AXIS_RIGHTY));     // Pitch
		case 23: return  ApplyZoomScale(ReadAxis(SDL_GAMEPAD_AXIS_RIGHTX));     // Yaw
		default: return  GetExtremalCommandValue(thisPtr, commandId);
	}
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
	if (!ShouldShowControllerPrompts() || g_State.isLoadingDefault)
		return GetTriggerNameFromCommandID(thisPtr, commandId);

	// Left Thumbstick movement
	switch (commandId)
	{
		case 0: return L"Left Thumbstick Up";
		case 1: return L"Left Thumbstick Down";
	}

	// Reload key alias
	if (commandId == 87) commandId = 88;

	// HUDWeapon
	bool shortName = false;
	if (commandId >= 30 && commandId <= 39) 
	{ 
		commandId = 77; 
		shortName = true; 
	}
	// HUDGrenadeList
	else if (commandId >= 40 && commandId <= 45) 
	{ 
		commandId = 73; 
		shortName = true; 
	}

	const wchar_t* name = GetGamepadButtonName(commandId, shortName);
	if (name)
		return name;

	return GetTriggerNameFromCommandID(thisPtr, commandId);
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

static void __fastcall UpdatePlayerMovement_Hook(int thisPtr, int)
{
	g_State.updateGyroCamera = true;
	UpdatePlayerMovement(thisPtr);
	g_State.updateGyroCamera = false;
}

static void __fastcall ApplyLocalRotationOffset_Hook(int thisPtr, int, float* vPYROffset)
{
	if (g_State.updateGyroCamera && g_Controller.isConnected && IsGyroEnabled() && !g_State.isConsoleOpen)
	{
		bool shouldApplyGyro = (GyroAimingMode == 0) 
			|| (GyroAimingMode == 1 && g_State.isAiming) 
			|| (GyroAimingMode == 2 && !g_State.isAiming);

		if (shouldApplyGyro)
		{
			float gyroYaw = 0.0f;
			float gyroPitch = 0.0f;

			GetProcessedGyroDelta(gyroYaw, gyroPitch);
			vPYROffset[0] += gyroPitch;
			vPYROffset[1] += gyroYaw;
		}
	}

	ApplyLocalRotationOffset(thisPtr, vPYROffset);
}

static void __fastcall BeginAim_Hook(BYTE* thisPtr, int)
{
	BeginAim(thisPtr);
	g_State.isAiming = *thisPtr;
}

static void __fastcall EndAim_Hook(BYTE* thisPtr, int)
{
	EndAim(thisPtr);
	g_State.isAiming = *thisPtr;
}

static const wchar_t* __stdcall LoadGameString_Hook(int ptr, char* String)
{
	if (ShouldShowControllerPrompts())
	{
		const uint32_t strHash = HashHelper::FNV1aRuntime(String);

		if (strHash == HashHelper::StringHashes::IDS_QUICKSAVE)
		{
			return L"Quick save";
		}
		else if (strHash == HashHelper::StringHashes::ScreenFailure_PressAnyKey)
		{
			switch (GetGamepadStyle())
			{
				case GamepadStyle::PlayStation:
					return L"Press Circle to return to the main menu.\nPress any other button to continue.";
				case GamepadStyle::Nintendo:
					return L"Press B to return to the main menu.\nPress any other button to continue.";
				case GamepadStyle::Xbox:
				default:
					return L"Press B to return to the main menu.\nPress any other button to continue.";
			}
		}
	}

	return LoadGameString(ptr, String);
}

static bool __stdcall DEditLoadModule_Hook(const char* pszProjectPath)
{
	// Load StringEditRuntime.dll
	bool res = DEditLoadModule(pszProjectPath);

	// Get and hook 'LoadString'
	DWORD addr = ScanModuleSignature(g_State.GameClient, "8B 4C 24 18 03 C1 8B 0D ?? ?? ?? ?? 03 F7 85 C9", "LoadString", -1, false);

	if (addr != 0)
	{
		int StringEditRuntimePtr = MemoryHelper::ReadMemory<int>(addr + 0x8);
		int StringEditRuntime = MemoryHelper::ReadMemory<int>(StringEditRuntimePtr);
		int vTable = MemoryHelper::ReadMemory<int>(StringEditRuntime);
		int pLoadString = MemoryHelper::ReadMemory<int>(vTable + 0x1C);

		HookHelper::ApplyHook((void*)pLoadString, &LoadGameString_Hook, (LPVOID*)&LoadGameString);
	}

	return res;
}

// =======================
// ConsoleEnabled
// =======================

static void __stdcall SetInputState_Hook(bool bAllowInput)
{
	g_State.wasInputDisabled = !bAllowInput;

	if (g_State.isConsoleOpen)
	{
		bAllowInput = false;
	}

	SetInputState(bAllowInput);
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
//  SDLGamepadSupport & HUDScaling
// =====================================

static void __fastcall ScreenDimsChanged_Hook(int thisPtr, int)
{
	ScreenDimsChanged(thisPtr);

	// Get the current resolution
	g_State.currentWidth = *(DWORD*)(thisPtr + 0x18);
	g_State.currentHeight = *(DWORD*)(thisPtr + 0x1C);

	g_TouchpadConfig.currentWidth = g_State.currentWidth;
	g_TouchpadConfig.currentHeight = g_State.currentHeight;

	if (!HUDScaling) return;

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

// ============================================
//  EnableCustomMaxWeaponCapacity & WeaponFixes
// ============================================

static void __fastcall OnEnterWorld_Hook(int thisPtr, int)
{
	OnEnterWorld(thisPtr);

	// Set the flag to allow aiming (can be 0 if loading a save during a cutscene)
	if (WeaponFixes && g_State.pAimMgr != 0)
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

// ============================================
//  SDLGamepadSupport & ConsoleEnabled
// ============================================

static int __stdcall HookedWindowProc_Hook(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	// Block input to game UI when console is visible
	if (g_State.isConsoleOpen)
	{
		if (!g_State.wasConsoleOpened)
		{
			SetInputState(false);
			g_State.wasConsoleOpened = true;
		}

		if (Msg == WM_MOUSEWHEEL)
		{
			ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam);
			return 0;
		}

		switch (Msg)
		{
			case WM_MOUSEMOVE:
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_LBUTTONDBLCLK:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_RBUTTONDBLCLK:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
			case WM_MBUTTONDBLCLK:
				return 0;
		}
	}
	else if (g_State.wasConsoleOpened)
	{
		SetInputState(!g_State.wasInputDisabled);
		g_State.wasConsoleOpened = false;
	}

	if (SDLGamepadSupport)
	{
		switch (Msg)
		{
			case WM_KEYDOWN:
				if (g_Controller.simulatedKeyPressCount > 0)
					g_Controller.simulatedKeyPressCount--;
				else
					OnKeyboardMouseInput();
			break;
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN:
				OnKeyboardMouseInput();
				break;
		}
	}

	return HookedWindowProc(hWnd, Msg, wParam, lParam);
}

static bool __fastcall ChangeState_Hook(int thisPtr, int, int state, int screenId)
{
	g_State.isPlaying = state == 1;
	return ChangeState(thisPtr, state, screenId);
}

static void __fastcall MsgBoxShow_Hook(int thisPtr, int, const wchar_t* pString, int pCreate, int nFontSize, bool bDefaultReturn)
{
	MsgBoxShow(thisPtr, pString, pCreate, nFontSize, bDefaultReturn);
	g_State.isMsgBoxVisible = *(BYTE*)(thisPtr + 0x578);
}

static void __fastcall MsgBoxHide_Hook(int thisPtr, int, int a2)
{
	MsgBoxHide(thisPtr, a2);
	g_State.isMsgBoxVisible = *(BYTE*)(thisPtr + 0x578);
}

#pragma endregion

#pragma region Client Patches

static void ApplyHighFPSFixesClientPatch()
{
    if (!HighFPSFixes) return;

    DWORD addr_SurfaceJumpImpulse = ScanModuleSignature(g_State.GameClient, "C7 44 24 1C 00 00 00 00 C7 44 24 10 00 00 00 00 EB", "SurfaceJumpImpulse");
    DWORD addr_HeightOffset = ScanModuleSignature(g_State.GameClient, "D9 E1 D9 54 24 1C D8 1D ?? ?? ?? ?? DF E0 F6 C4 41 0F 85", "HeightOffset");
    DWORD addr_UpdateOnGround = ScanModuleSignature(g_State.GameClient, "83 EC 3C 53 55 56 57 8B F1", "UpdateOnGround");
    DWORD addr_UpdateWaveProp = ScanModuleSignature(g_State.GameClient, "D9 44 24 04 83 EC ?? D8 1D", "UpdateWaveProp");
    DWORD addr_UpdateNormalFriction = ScanModuleSignature(g_State.GameClient, "83 EC 3C 56 8B F1 8B 46 28 F6 C4 08 C7 44 24 34", "UpdateNormalFriction");
    DWORD addr_GetTimerElapsedS = ScanModuleSignature(g_State.GameClient, "04 51 8B C8 FF 52 3C 85 C0 5E", "GetTimerElapsedS");
    DWORD addr_GetMaxRecentVelocityMag = ScanModuleSignature(g_State.GameClient, "F6 C4 41 75 2F 8D 8E 34 04 00 00 E8", "GetMaxRecentVelocityMag");
    DWORD addr_UpdateNormalControlFlags = ScanModuleSignature(g_State.GameClient, "55 8B EC 83 E4 F8 83 EC 18 53 55 56 57 8B F1 E8", "UpdateNormalControlFlags");
    DWORD addr_PolyGridFXCollisionHandlerCB = ScanModuleSignature(g_State.GameClient, "83 EC 54 53 33 DB 3B CB ?? 74 05", "PolyGridFXCollisionHandlerCB");
    addr_GetMaxRecentVelocityMag = MemoryHelper::ResolveRelativeAddress(addr_GetMaxRecentVelocityMag, 0xC);

    if (addr_SurfaceJumpImpulse == 0 ||
        addr_HeightOffset == 0 ||
        addr_UpdateOnGround == 0 ||
        addr_GetMaxRecentVelocityMag == 0 ||
        addr_UpdateNormalControlFlags == 0 ||
        addr_UpdateNormalFriction == 0 ||
        addr_GetTimerElapsedS == 0 ||
        addr_UpdateWaveProp == 0 ||
        addr_PolyGridFXCollisionHandlerCB == 0) {
        return;
    }

    MemoryHelper::MakeNOP(addr_SurfaceJumpImpulse, 0x10);
	MemoryHelper::WriteMemory<uint16_t>(addr_HeightOffset + 0x11, 0xE990);
    HookHelper::ApplyHook((void*)addr_GetMaxRecentVelocityMag, &GetMaxRecentVelocityMag_Hook, (LPVOID*)&GetMaxRecentVelocityMag);
    HookHelper::ApplyHook((void*)addr_UpdateNormalControlFlags, &UpdateNormalControlFlags_Hook, (LPVOID*)&UpdateNormalControlFlags);
    HookHelper::ApplyHook((void*)addr_UpdateOnGround, &UpdateOnGround_Hook, (LPVOID*)&UpdateOnGround);
    HookHelper::ApplyHook((void*)addr_UpdateWaveProp, &UpdateWaveProp_Hook, (LPVOID*)&UpdateWaveProp);
    HookHelper::ApplyHook((void*)addr_UpdateNormalFriction, &UpdateNormalFriction_Hook, (LPVOID*)&UpdateNormalFriction);
    HookHelper::ApplyHook((void*)(addr_GetTimerElapsedS - 0x20), &GetTimerElapsedS_Hook, (LPVOID*)&GetTimerElapsedS);
    HookHelper::ApplyHook((void*)(addr_PolyGridFXCollisionHandlerCB - 0x6), &PolyGridFXCollisionHandlerCB_Hook, (LPVOID*)&PolyGridFXCollisionHandlerCB);
}

static void ApplyMouseAimMultiplierClientPatch()
{
    if (MouseAimMultiplier == 1.0f) return;

    DWORD addr_MouseAimMultiplier = ScanModuleSignature(g_State.GameClient, "89 4C 24 14 DB 44 24 14 8D 44 24 20 6A 01 50 D8 0D", "MouseAimMultiplier");

    if (addr_MouseAimMultiplier != 0)
    {
        // Write the updated multiplier
        g_State.overrideSensitivity = g_State.overrideSensitivity * MouseAimMultiplier;
        MemoryHelper::WriteMemory<uint32_t>(addr_MouseAimMultiplier + 0x11, reinterpret_cast<uintptr_t>(&g_State.overrideSensitivity));
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

    DWORD addr = ScanModuleSignature(g_State.GameClient, "53 8B 5C 24 08 55 8B 6C 24 14 56 8D 43 FF 83 F8", "SkipSplashScreen");

    if (addr != 0)
    {
        MemoryHelper::MakeNOP(addr + 0x13D, 8);
    }
}

static void ApplyDisableLetterboxClientPatch()
{
    if (!DisableLetterbox) return;

    DWORD addr = ScanModuleSignature(g_State.GameClient, "83 EC 54 53 55 56 57 8B", "DisableLetterbox");

    if (addr != 0)
    {
        MemoryHelper::WriteMemory<uint8_t>(addr, 0xC3);
    }
}

static void ApplyPersistentWorldClientPatch()
{
    if (!EnablePersistentWorldState) return;

    DWORD addr_ShellCasing = ScanModuleSignature(g_State.GameClient, "D9 86 88 00 00 00 D8 64 24", "ShellCasing");
    DWORD addr_DecalSaving = ScanModuleSignature(g_State.GameClient, "FF 52 0C ?? 8D ?? ?? ?? 00 00 E8 ?? ?? ?? FF 8B", "DecalSaving");
    DWORD addr_Decal = ScanModuleSignature(g_State.GameClient, "DF E0 F6 C4 01 75 34 DD 44 24", "Decal");
    DWORD addr_FX = ScanModuleSignature(g_State.GameClient, "8B CE FF ?? 04 84 C0 75 ?? 8B ?? 8B CE FF ?? 08 56 E8", "CreateFX", 1);
    DWORD addr_Shatter = ScanModuleSignature(g_State.GameClient, "8B C8 E8 ?? ?? ?? 00 D9 5C 24 ?? D9", "Shatter");
    addr_Shatter = MemoryHelper::ResolveRelativeAddress(addr_Shatter, 0x3);

    if (addr_ShellCasing == 0 ||
        addr_DecalSaving == 0 ||
        addr_Decal == 0 ||
        addr_FX == 0 ||
        addr_Shatter == 0) {
        return;
    }

    MemoryHelper::MakeNOP(addr_ShellCasing + 0x6, 4);
    MemoryHelper::MakeNOP(addr_DecalSaving + 0xF, 13);
    MemoryHelper::WriteMemory<uint8_t>(addr_Decal + 0x5, 0x74);
    HookHelper::ApplyHook((void*)addr_Shatter, &GetShatterLifetime_Hook, (LPVOID*)&GetShatterLifetime);
    HookHelper::ApplyHook((void*)addr_FX, &CreateFX_Hook, (LPVOID*)&CreateFX);
}

static void ApplyInfiniteFlashlightClientPatch()
{
    if (!InfiniteFlashlight) return;

    DWORD addr_Update = ScanModuleSignature(g_State.GameClient, "8B 51 10 8A 42 18 84 C0 8A 86 04 01 00 00", "InfiniteFlashlight_Update");
    DWORD addr_UpdateBar = ScanModuleSignature(g_State.GameClient, "A1 ?? ?? ?? ?? 85 C0 56 8B F1 74 71 D9 86 1C 04 00 00", "InfiniteFlashlight_UpdateBar");
    DWORD addr_UpdateLayout = ScanModuleSignature(g_State.GameClient, "68 ?? ?? ?? ?? 6A 00 68 ?? ?? ?? ?? 50 FF 57 58 8B 0D ?? ?? ?? ?? 50 FF 97 84 00 00 00 8B 0D ?? ?? ?? ?? 8B 11 50 8D 44 24 10 50 FF 52 04 8B 4C 24 0C 8D BE C4 01 00 00", "InfiniteFlashlight_UpdateLayout");
    DWORD addr_Battery = ScanModuleSignature(g_State.GameClient, "D8 4C 24 04 DC AE 88 03 00 00 DD 96 88 03 00 00", "InfiniteFlashlight_Battery");

    if (addr_Update == 0 ||
        addr_UpdateBar == 0 ||
        addr_UpdateLayout == 0 ||
        addr_Battery == 0) {
        return;
    }

    MemoryHelper::WriteMemory<uint8_t>(addr_Update - 0x31, 0xC3);
    MemoryHelper::WriteMemory<uint8_t>(addr_UpdateLayout - 0x36, 0xC3);
    MemoryHelper::WriteMemory<uint8_t>(addr_UpdateBar, 0xC3);
    MemoryHelper::MakeNOP(addr_Battery + 0xA, 6);
}

static void ApplyControllerClientPatch()
{
    if (!SDLGamepadSupport) return;

    DWORD addr_pGameClientShell = ScanModuleSignature(g_State.GameClient, "C1 F8 02 C1 E0 05 2B C2 8B CB BA 01 00 00 00 D3 E2 8B CD 03 C3 50 85 11", "pGameClientShell");
    if (addr_pGameClientShell == 0) return;

    DWORD addr_OnCommandOn = addr_pGameClientShell + MemoryHelper::ReadMemory<int>(addr_pGameClientShell + 0x21) + 0x25;
    DWORD addr_OnCommandOff = addr_pGameClientShell + MemoryHelper::ReadMemory<int>(addr_pGameClientShell + 0x28) + 0x2C;
    DWORD addr_GetExtremalCommandValue = ScanModuleSignature(g_State.GameClient, "83 EC 08 56 57 8B F9 8B 77 04 3B 77 08 C7 44 24 08 00 00 00 00", "GetExtremalCommandValue");
    DWORD addr_IsCommandOn = ScanModuleSignature(g_State.GameClient, "8B D1 8A 42 4C 84 C0 56 74 58", "IsCommandOn");
    DWORD addr_HUDActivateObjectSetObject = ScanModuleSignature(g_State.GameClient, "8B 86 D4 02 00 00 3B C3 8D BE C8 02 00 00 74 0F", "HUDActivateObjectSetObject", 1);
    DWORD addr_HUDSwapUpdate = ScanModuleSignature(g_State.GameClient, "55 8B EC 83 E4 F8 81 EC 84 01", "HUDSwapUpdate");
    DWORD addr_SetOperatingTurret = ScanModuleSignature(g_State.GameClient, "8B 44 24 04 89 81 F4 05 00 00 8B 0D ?? ?? ?? ?? 8B 11 FF 52 3C C2 04 00", "SetOperatingTurret");
    DWORD addr_GetTriggerNameFromCommandID = ScanModuleSignature(g_State.GameClient, "81 EC 44 08 00 00", "GetTriggerNameFromCommandID");
    DWORD addr_SwitchToScreen = ScanModuleSignature(g_State.GameClient, "53 55 56 8B F1 8B 6E 60 33 DB 3B EB 57 8B 7C 24 14", "SwitchToScreen");
    DWORD addr_SetCurrentType = ScanModuleSignature(g_State.GameClient, "53 8B 5C 24 08 85 DB 56 57 8B F1 7C 1C 8B BE E4", "SetCurrentType");
    DWORD addr_HUDSwapUpdateTriggerName = ScanModuleSignature(g_State.GameClient, "8B 0D ?? ?? ?? ?? 6A 57 E8 ?? ?? ?? ?? 50 B9", "HUDSwapUpdateTriggerName");
    DWORD addr_GetZoomMag = ScanModuleSignature(g_State.GameClient, "C7 44 24 30 00 00 00 00 8B 4D 28 57 E8", "GetZoomMag");
    DWORD addr_DEditLoadModule = ScanModuleSignature(g_State.GameClient, "83 C4 04 84 C0 75 17 8B 4C 24 04", "DEditLoadModule");
    DWORD addr_PerformanceScreenId = ScanModuleSignature(g_State.GameClient, "8B C8 E8 ?? ?? ?? ?? 8B 4E 0C 8B 01 6A ?? FF 50 6C 85 C0 74 0A 8B 10 8B C8 FF 92 88 00 00 00 8B 4E 0C 8B 01 6A", "PerformanceScreenId");
    addr_GetZoomMag = MemoryHelper::ResolveRelativeAddress(addr_GetZoomMag, 0xD);

    if (addr_OnCommandOn == 0 ||
        addr_OnCommandOff == 0 ||
        addr_GetExtremalCommandValue == 0 ||
        addr_IsCommandOn == 0 ||
        addr_HUDActivateObjectSetObject == 0 ||
        addr_HUDSwapUpdate == 0 ||
        addr_SetOperatingTurret == 0 ||
        addr_GetTriggerNameFromCommandID == 0 ||
        addr_SwitchToScreen == 0 ||
        addr_SetCurrentType == 0 ||
        addr_HUDSwapUpdateTriggerName == 0 ||
        addr_GetZoomMag == 0 ||
        addr_DEditLoadModule == 0 ||
        addr_PerformanceScreenId == 0) {
        return;
    }

    g_State.g_pGameClientShell = MemoryHelper::ReadMemory<int>(MemoryHelper::ReadMemory<int>(addr_pGameClientShell + 0x1A));

    HookHelper::ApplyHook((void*)(addr_GetExtremalCommandValue), &GetExtremalCommandValue_Hook, (LPVOID*)&GetExtremalCommandValue);
    HookHelper::ApplyHook((void*)addr_IsCommandOn, &IsCommandOn_Hook, (LPVOID*)&IsCommandOn);
    HookHelper::ApplyHook((void*)addr_OnCommandOn, &OnCommandOn_Hook, (LPVOID*)&OnCommandOn);
    HookHelper::ApplyHook((void*)addr_OnCommandOff, &OnCommandOff_Hook, (LPVOID*)&OnCommandOff);
    HookHelper::ApplyHook((void*)addr_SetOperatingTurret, &SetOperatingTurret_Hook, (LPVOID*)&SetOperatingTurret);
    HookHelper::ApplyHook((void*)addr_GetTriggerNameFromCommandID, &GetTriggerNameFromCommandID_Hook, (LPVOID*)&GetTriggerNameFromCommandID);
    HookHelper::ApplyHook((void*)addr_HUDActivateObjectSetObject, &HUDActivateObjectSetObject_Hook, (LPVOID*)&HUDActivateObjectSetObject);
    HookHelper::ApplyHook((void*)addr_HUDSwapUpdate, &HUDSwapUpdate_Hook, (LPVOID*)&HUDSwapUpdate);
    HookHelper::ApplyHook((void*)addr_SwitchToScreen, &SwitchToScreen_Hook, (LPVOID*)&SwitchToScreen);
    HookHelper::ApplyHook((void*)addr_SetCurrentType, &SetCurrentType_Hook, (LPVOID*)&SetCurrentType);
    HookHelper::ApplyHook((void*)addr_GetZoomMag, &GetZoomMag_Hook, (LPVOID*)&GetZoomMag);
    HookHelper::ApplyHook((void*)(addr_DEditLoadModule - 0xA), &DEditLoadModule_Hook, (LPVOID*)&DEditLoadModule);
	HUDSwapUpdateTriggerName = reinterpret_cast<decltype(HUDSwapUpdateTriggerName)>(addr_HUDSwapUpdateTriggerName);

    g_State.screenPerformanceCPU = MemoryHelper::ReadMemory<uint8_t>(addr_PerformanceScreenId + 0xD);
    g_State.screenPerformanceGPU = MemoryHelper::ReadMemory<uint8_t>(addr_PerformanceScreenId + 0x25);

	if (GyroEnabled)
	{
		DWORD addr_ApplyLocalRotationOffset = ScanModuleSignature(g_State.GameClient, "DA E9 DF E0 F6 C4 44 7A 24 D9 ?? 04 D9", "ApplyLocalRotationOffset", 2);
		DWORD addr_UpdatePlayerMovement = ScanModuleSignature(g_State.GameClient, "83 EC 0C 56 8B F1 E8 ?? ?? ?? ?? 8A 88 9E 05 00 00", "UpdatePlayerMovement");
		DWORD addr_BeginAim = ScanModuleSignature(g_State.GameClient, "A1 ?? ?? ?? ?? 56 8B F1 8B 48 28 8B 81 5C 01 00 00", "BeginAim");
		DWORD addr_EndAim = ScanModuleSignature(g_State.GameClient, "A1 ?? ?? ?? ?? 56 8B F1 8B 88 F4 05 00 00 85 C9", "EndAim");

		if (addr_ApplyLocalRotationOffset != 0 &&
			addr_UpdatePlayerMovement != 0 &&
			addr_BeginAim != 0 &&
			addr_EndAim != 0)
		{
			HookHelper::ApplyHook((void*)addr_ApplyLocalRotationOffset, &ApplyLocalRotationOffset_Hook, (LPVOID*)&ApplyLocalRotationOffset);
			HookHelper::ApplyHook((void*)addr_UpdatePlayerMovement, &UpdatePlayerMovement_Hook, (LPVOID*)&UpdatePlayerMovement);
			HookHelper::ApplyHook((void*)addr_BeginAim, &BeginAim_Hook, (LPVOID*)&BeginAim);
			HookHelper::ApplyHook((void*)addr_EndAim, &EndAim_Hook, (LPVOID*)&EndAim);
		}
	}

    if (!HideMouseCursor) return;

    DWORD addr_UseCursor = ScanModuleSignature(g_State.GameClient, "8A 44 24 04 84 C0 56 8B F1 88 46 01 74", "UseCursor");
    DWORD addr_OnMouseMove = ScanModuleSignature(g_State.GameClient, "56 8B F1 8A 86 ?? ?? 00 00 84 C0 0F 84 B3", "OnMouseMove");

    HookHelper::ApplyHook((void*)addr_OnMouseMove, &OnMouseMove_Hook, (LPVOID*)&OnMouseMove);
    HookHelper::ApplyHook((void*)addr_UseCursor, &UseCursor_Hook, (LPVOID*)&UseCursor);
}

static void ApplyHUDScalingClientPatch()
{
    if (!HUDScaling) return;

    DWORD addr_HUDTerminate = ScanModuleSignature(g_State.GameClient, "53 56 8B D9 8B B3 7C 04 00 00 8B 83 80 04 00 00 57 33 FF 3B F0", "HUDTerminate");
    DWORD addr_HUDInit = ScanModuleSignature(g_State.GameClient, "8B ?? ?? 8D ?? 78 04 00 00", "HUDInit", 1);
    DWORD addr_HUDRender = ScanModuleSignature(g_State.GameClient, "53 8B D9 8A 43 08 84 C0 74", "HUDRender");
    DWORD addr_LayoutDBGetPosition = ScanModuleSignature(g_State.GameClient, "83 EC 10 8B 54 24 20 8B 0D", "LayoutDBGetPosition");
    DWORD addr_GetRectF = ScanModuleSignature(g_State.GameClient, "14 8B 44 24 28 8B 4C 24 18 D9 18", "GetRectF");
    DWORD addr_UpdateSlider = ScanModuleSignature(g_State.GameClient, "56 8B F1 8B 4C 24 08 8B 86 7C 01 00 00 3B C8 89 8E 80 01 00 00", "UpdateSlider");
    DWORD addr_HUDWeaponListReset = ScanModuleSignature(g_State.GameClient, "51 53 55 8B E9 8B 0D", "HUDWeaponListReset");
    DWORD addr_InitAdditionalTextureData = ScanModuleSignature(g_State.GameClient, "8B 54 24 04 8B 01 83 EC 20 57", "InitAdditionalTextureData");
    DWORD addr_HUDPausedInit = ScanModuleSignature(g_State.GameClient, "56 8B F1 8B 06 57 FF 50 20", "HUDPausedInit");

    if (addr_HUDTerminate == 0 ||
        addr_HUDInit == 0 ||
        addr_HUDRender == 0 ||
        addr_LayoutDBGetPosition == 0 ||
        addr_GetRectF == 0 ||
        addr_UpdateSlider == 0 ||
        addr_HUDWeaponListReset == 0 ||
        addr_InitAdditionalTextureData == 0 ||
        addr_HUDPausedInit == 0) {
        return;
    }

    HookHelper::ApplyHook((void*)addr_HUDInit, &HUDInit_Hook, (LPVOID*)&HUDInit);
    HookHelper::ApplyHook((void*)addr_HUDRender, &HUDRender_Hook, (LPVOID*)&HUDRender);
    HookHelper::ApplyHook((void*)addr_LayoutDBGetPosition, &LayoutDBGetPosition_Hook, (LPVOID*)&LayoutDBGetPosition);
    HookHelper::ApplyHook((void*)(addr_GetRectF - 0x58), &GetRectF_Hook, (LPVOID*)&GetRectF);
    HookHelper::ApplyHook((void*)addr_UpdateSlider, &UpdateSlider_Hook, (LPVOID*)&UpdateSlider);
    HookHelper::ApplyHook((void*)(addr_InitAdditionalTextureData - 6), &InitAdditionalTextureData_Hook, (LPVOID*)&InitAdditionalTextureData);
    HookHelper::ApplyHook((void*)addr_HUDPausedInit, &HUDPausedInit_Hook, (LPVOID*)&HUDPausedInit);
	HUDTerminate = reinterpret_cast<decltype(HUDTerminate)>(addr_HUDTerminate);
	HUDWeaponListReset = reinterpret_cast<decltype(HUDWeaponListReset)>(addr_HUDWeaponListReset);
}

static void ApplySetWeaponCapacityClientPatch()
{
    if (!EnableCustomMaxWeaponCapacity) return;

    DWORD addr_GetWeaponCapacity = ScanModuleSignature(g_State.GameClient, "CC 8B 41 48 8B 0D", "GetWeaponCapacity");

    if (addr_GetWeaponCapacity != 0)
    {
        HookHelper::ApplyHook((void*)(addr_GetWeaponCapacity + 0x1), &GetWeaponCapacity_Hook, (LPVOID*)&GetWeaponCapacity);
        g_State.appliedCustomMaxWeaponCapacity = true;
    }
}

static void ApplyHighResolutionReflectionsClientPatch()
{
    if (!HighResolutionReflections) return;

    DWORD addr = ScanModuleSignature(g_State.GameClient, "8B 47 08 89 46 4C 8A 4F 24 88 4E 68 8A 57 25", "RenderTargetGroupFXInit");

    if (addr != 0)
    {
        HookHelper::ApplyHook((void*)(addr - 0x31), &RenderTargetGroupFXInit_Hook, (LPVOID*)&RenderTargetGroupFXInit);
    }
}

static void ApplyAutoResolutionClientCheck()
{
    if (!AutoResolution) return;

    DWORD addr_AutoDetectPerformanceSettings = ScanModuleSignature(g_State.GameClient, "83 C4 10 83 F8 01 75 37", "AutoDetectPerformanceSettings", 2);
    DWORD addr_SetQueuedConsoleVariable = ScanModuleSignature(g_State.GameClient, "83 EC 10 56 8B F1 8B 46 ?? 8B 4E ?? 8D 54 24 18", "SetQueuedConsoleVariable");
    DWORD addr_SetOption = ScanModuleSignature(g_State.GameClient, "51 8B 44 24 14 85 C0 89 0C 24", "SetOption");

    if (addr_AutoDetectPerformanceSettings == 0 ||
        addr_SetQueuedConsoleVariable == 0 ||
        addr_SetOption == 0) {
        return;
    }

    HookHelper::ApplyHook((void*)addr_AutoDetectPerformanceSettings, &AutoDetectPerformanceSettings_Hook, (LPVOID*)&AutoDetectPerformanceSettings);
    HookHelper::ApplyHook((void*)addr_SetQueuedConsoleVariable, &SetQueuedConsoleVariable_Hook, (LPVOID*)&SetQueuedConsoleVariable);
    HookHelper::ApplyHook((void*)addr_SetOption, &SetOption_Hook, (LPVOID*)&SetOption);
}

static void ApplyKeyboardInputLanguageClientCheck()
{
    if (!FixKeyboardInputLanguage) return;

    DWORD addr_LoadUserProfile = ScanModuleSignature(g_State.GameClient, "53 8B 5C 24 08 84 DB 55 56 57 8B F9", "LoadUserProfile");
    DWORD addr_RestoreDefaults = ScanModuleSignature(g_State.GameClient, "57 8B F9 8B 0D ?? ?? ?? ?? 8B 01 FF 50 4C 8B 10", "RestoreDefaults");

    if (addr_LoadUserProfile == 0 ||
        addr_RestoreDefaults == 0) {
        return;
    }

    HookHelper::ApplyHook((void*)addr_LoadUserProfile, &LoadUserProfile_Hook, (LPVOID*)&LoadUserProfile);
    HookHelper::ApplyHook((void*)addr_RestoreDefaults, &RestoreDefaults_Hook, (LPVOID*)&RestoreDefaults);
}

static void ApplyWeaponFixesClientPatch()
{
    if (!WeaponFixes) return;

    DWORD addr_AimMgrCtor = ScanModuleSignature(g_State.GameClient, "8B C1 C6 00 00 C6 40 01 01 C3", "AimMgrCtor");
    DWORD addr_UpdateWeaponModel = ScanModuleSignature(g_State.GameClient, "83 EC 44 56 8B F1 57 8B 7E 08 85 FF", "UpdateWeaponModel");
    DWORD addr_AnimationClearLock = ScanModuleSignature(g_State.GameClient, "E8 BB FF FF FF C7 41 34 FF FF FF FF C7 81 58 01", "AnimationClearLock");
    DWORD addr_SetAnimProp = ScanModuleSignature(g_State.GameClient, "8B 44 24 04 83 F8 FF 74 ?? 83 F8 12 7D ?? 8B 54 24 08 89 04", "SetAnimProp");
    DWORD addr_InitAnimations = ScanModuleSignature(g_State.GameClient, "6A 08 6A 7A 8B CF FF ?? 24 8B ?? 6A 08", "InitAnimations", 3);
    DWORD addr_GetWeaponSlot = ScanModuleSignature(g_State.GameClient, "8A 51 40 32 C0 84 D2 76 23 56 8B B1 B4 00 00 00 57", "GetWeaponSlot");
    DWORD addr_NextWeapon = ScanModuleSignature(g_State.GameClient, "84 C0 0F 84 ?? 00 00 00 8B CE E8", "NextWeapon");
    DWORD addr_PreviousWeapon = ScanModuleSignature(g_State.GameClient, "8D BE ?? 57 00 00 8B CF E8 ?? ?? ?? ?? 84 C0 74 1F 8B CE E8", "PreviousWeapon");
    DWORD addr_kAP_ACT_Fire_Id = ScanModuleSignature(g_State.GameClient, "84 C0 75 1E 6A 00 68 ?? 00 00 00 6A 00 8B CF", "kAP_ACT_Fire_Id");
    addr_NextWeapon = MemoryHelper::ResolveRelativeAddress(addr_NextWeapon, 0xB);
    addr_PreviousWeapon = MemoryHelper::ResolveRelativeAddress(addr_PreviousWeapon, 0x14);

    if (addr_AimMgrCtor == 0 ||
        addr_UpdateWeaponModel == 0 ||
        addr_AnimationClearLock == 0 ||
        addr_SetAnimProp == 0 ||
        addr_InitAnimations == 0 ||
        addr_GetWeaponSlot == 0 ||
        addr_NextWeapon == 0 ||
        addr_PreviousWeapon == 0 ||
        addr_kAP_ACT_Fire_Id == 0) {
        return;
    }

    HookHelper::ApplyHook((void*)addr_AimMgrCtor, &AimMgrCtor_Hook, (LPVOID*)&AimMgrCtor);
    HookHelper::ApplyHook((void*)addr_UpdateWeaponModel, &UpdateWeaponModel_Hook, (LPVOID*)&UpdateWeaponModel);
    HookHelper::ApplyHook((void*)addr_SetAnimProp, &SetAnimProp_Hook, (LPVOID*)&SetAnimProp);
    HookHelper::ApplyHook((void*)addr_InitAnimations, &InitAnimations_Hook, (LPVOID*)&InitAnimations);
    HookHelper::ApplyHook((void*)addr_GetWeaponSlot, &GetWeaponSlot_Hook, (LPVOID*)&GetWeaponSlot);
    HookHelper::ApplyHook((void*)addr_NextWeapon, &NextWeapon_Hook, (LPVOID*)&NextWeapon);
    HookHelper::ApplyHook((void*)addr_PreviousWeapon, &PreviousWeapon_Hook, (LPVOID*)&PreviousWeapon);
	AnimationClearLock = reinterpret_cast<decltype(AnimationClearLock)>(addr_AnimationClearLock);
    g_State.kAP_ACT_Fire_Id = MemoryHelper::ReadMemory<int>(addr_kAP_ACT_Fire_Id + 0x7);
}

static void ApplyConsoleClientPatch()
{
	if (!ConsoleEnabled) return;

	DWORD addr = ScanModuleSignature(g_State.GameClient, "E8 ?? ?? ?? ?? 8A 4C 24 04 88 48 4C C2 04 00 CC", "SetInputState");

	if (addr != 0)
	{
		HookHelper::ApplyHook((void*)addr, &SetInputState_Hook, (LPVOID*)&SetInputState);
	}
}

static void ApplyClientFXHook()
{
    if (!HighFPSFixes) return;

    DWORD addr = ScanModuleSignature(g_State.GameClient, "83 EC 20 56 57 8B F1 E8 ?? ?? ?? ?? 8A 44 24 30", "LoadFxDll");

    if (addr != 0)
    {
        HookHelper::ApplyHook((void*)addr, &LoadFxDll_Hook, (LPVOID*)&LoadFxDll);
    }
}

static void ApplyDisablePunkBuster()
{
	if (!DisablePunkBuster) return;

	DWORD addr = ScanModuleSignature(g_State.GameClient, "83 EC 28 56 8B F1 8B 86 ?? ?? 00 00 85", "DisablePunkBuster");

	if (addr != 0)
	{
		MemoryHelper::WriteMemory<uint8_t>(addr, 0xC3);
	}
}

static void ApplyDisableHipFireAccuracyPenalty()
{
    if (!DisableHipFireAccuracyPenalty) return;

    DWORD addr = ScanModuleSignature(g_State.GameClient, "83 EC ?? A1 ?? ?? ?? ?? 8B 40 28 56 57 6A 00 8B F1", "DisableHipFireAccuracyPenalty");

    if (addr != 0)
    {
        HookHelper::ApplyHook((void*)addr, &AccuracyMgrUpdate_Hook, (LPVOID*)&AccuracyMgrUpdate);
    }
}

static void ApplyGameDatabaseHook()
{
    if (!HUDScaling) return;

    DWORD addr_GameDatabase = ScanModuleSignature(g_State.GameClient, "8B 5E 08 55 E8 ?? ?? ?? FF 8B 0D ?? ?? ?? ?? 8B 39 68 ?? ?? ?? ?? 6A 00 68 ?? ?? ?? ?? 53 FF 57", "HUDScaling_GameDatabase");

    if (addr_GameDatabase != 0)
    {
        int pDB = MemoryHelper::ReadMemory<int>(addr_GameDatabase + 0xB);
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
	if (!HUDScaling && !SDLGamepadSupport) return;

	DWORD addr_HUDWeaponListUpdateTriggerNames = ScanModuleSignature(g_State.GameClient, "56 32 DB 89 44 24 0C BE 1E 00 00 00", "HUDWeaponListUpdateTriggerNames");
	DWORD addr_HUDGrenadeListUpdateTriggerNames = ScanModuleSignature(g_State.GameClient, "56 32 DB 89 44 24 0C BE 28 00 00 00", "HUDGrenadeListUpdateTriggerNames");
	DWORD addr_HUDWeaponListInit = ScanModuleSignature(g_State.GameClient, "51 53 55 57 8B F9 8B 07 FF 50 20 8B 0D", "HUDWeaponListInit");
	DWORD addr_HUDGrenadeListInit = ScanModuleSignature(g_State.GameClient, "83 EC 08 53 55 57 8B F9 8B 07 FF 50 20 8B 0D", "HUDGrenadeListInit");
	DWORD addr_ScreenDimsChanged = ScanModuleSignature(g_State.GameClient, "A1 ?? ?? ?? ?? 81 EC 98 00 00 00 85 C0 56 8B F1", "ScreenDimsChanged");

	if (addr_HUDWeaponListUpdateTriggerNames == 0 ||
		addr_HUDGrenadeListUpdateTriggerNames == 0 ||
		addr_HUDWeaponListInit == 0 ||
		addr_HUDGrenadeListInit == 0 ||
		addr_ScreenDimsChanged == 0) {
		return;
	}

	HUDWeaponListUpdateTriggerNames = reinterpret_cast<decltype(HUDWeaponListUpdateTriggerNames)>(addr_HUDWeaponListUpdateTriggerNames - 0x10);
	HUDGrenadeListUpdateTriggerNames = reinterpret_cast<decltype(HUDGrenadeListUpdateTriggerNames)>(addr_HUDGrenadeListUpdateTriggerNames - 0x10);
	HookHelper::ApplyHook((void*)addr_HUDWeaponListInit, &HUDWeaponListInit_Hook, (LPVOID*)&HUDWeaponListInit);
	HookHelper::ApplyHook((void*)addr_HUDGrenadeListInit, &HUDGrenadeListInit_Hook, (LPVOID*)&HUDGrenadeListInit);
	HookHelper::ApplyHook((void*)addr_ScreenDimsChanged, &ScreenDimsChanged_Hook, (LPVOID*)&ScreenDimsChanged);
}

static void ApplyClientPatchSet2()
{
    if (!WeaponFixes && !EnableCustomMaxWeaponCapacity) return;

    DWORD addr_OnEnterWorld = ScanModuleSignature(g_State.GameClient, "8B F1 E8 ?? ?? ?? ?? DD 05 ?? ?? ?? ?? 8B 96", "OnEnterWorld", 1);

    if (addr_OnEnterWorld != 0)
    {
        HookHelper::ApplyHook((void*)addr_OnEnterWorld, &OnEnterWorld_Hook, (LPVOID*)&OnEnterWorld);
    }
}

static void ApplyClientPatchSet3()
{
	if (!SDLGamepadSupport && !ConsoleEnabled) return;

	DWORD addr_HookedWindowProc = ScanModuleSignature(g_State.GameClient, "56 8B 74 24 0C 81 FE 03 02 00 00 57 8B 7C 24 14", "HookedWindowProc");
	DWORD addr_ChangeState = ScanModuleSignature(g_State.GameClient, "8B 44 24 0C 53 8B 5C 24 0C 57 8B 7E 08", "ChangeState");
	DWORD addr_MsgBoxShow = ScanModuleSignature(g_State.GameClient, "83 EC 70 56 8B F1 8A 86 78 05 00 00 84 C0 0F 85", "MsgBoxShow");
	DWORD addr_MsgBoxHide = ScanModuleSignature(g_State.GameClient, "56 8B F1 8A 86 78 05 00 00 84 C0 0F 84", "MsgBoxHide");

	if (addr_HookedWindowProc == 0 ||
		addr_ChangeState == 0 ||
		addr_MsgBoxShow == 0 ||
		addr_MsgBoxHide == 0) {
		return;
	}

	HookHelper::ApplyHook((void*)addr_HookedWindowProc, &HookedWindowProc_Hook, (LPVOID*)&HookedWindowProc);
	HookHelper::ApplyHook((void*)(addr_ChangeState - 0x13), &ChangeState_Hook, (LPVOID*)&ChangeState);
	HookHelper::ApplyHook((void*)addr_MsgBoxShow, &MsgBoxShow_Hook, (LPVOID*)&MsgBoxShow);
	HookHelper::ApplyHook((void*)addr_MsgBoxHide, &MsgBoxHide_Hook, (LPVOID*)&MsgBoxHide);
}

void ApplyClientPatch()
{
	ApplyHighFPSFixesClientPatch();
	ApplyMouseAimMultiplierClientPatch();
	ApplyXPWidescreenClientPatch();
	ApplySkipSplashScreenClientPatch();
	ApplyDisableLetterboxClientPatch();
	ApplyPersistentWorldClientPatch();
	ApplyInfiniteFlashlightClientPatch();
	ApplyControllerClientPatch();
	ApplyHUDScalingClientPatch();
	ApplySetWeaponCapacityClientPatch();
	ApplyHighResolutionReflectionsClientPatch();
	ApplyAutoResolutionClientCheck();
	ApplyKeyboardInputLanguageClientCheck();
	ApplyWeaponFixesClientPatch();
	ApplyConsoleClientPatch();
	ApplyClientFXHook();
	ApplyDisablePunkBuster();
	ApplyDisableHipFireAccuracyPenalty();
	ApplyGameDatabaseHook();
	ApplyClientPatchSet1();
	ApplyClientPatchSet2();
	ApplyClientPatchSet3();
}

#pragma endregion