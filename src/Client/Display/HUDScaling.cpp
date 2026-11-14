#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"
#include "../../Controller/ScreenJoystickHook.hpp"
#include "../../ClientFX/ClientFX.hpp"
#include "../../Server/Server.hpp"

static bool crosshairSliderUpdated = false;

struct PendingDBScale
{
	int hAttribute;
	int valueIndex;
	float scalingFactor;
};

static thread_local PendingDBScale pendingDBScale = {};

static int ScaleValue(int value, float scalingFactor)
{
	return static_cast<int>(std::lround(value * scalingFactor));
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
	g_State.isRenderingHUD = true;
	HUDRender(thisPtr, eHUDRenderLayer);
	g_State.isRenderingHUD = false;

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
	DWORD* result = LayoutDBGetPosition(a1, Record, Attribute, a4);

	if (!Record)
		return result;

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
			result[0] = static_cast<DWORD>(ScaleValue(static_cast<int>(result[0]), scalingFactor));
			result[1] = static_cast<DWORD>(ScaleValue(static_cast<int>(result[1]), scalingFactor));
		}
	}

	return result;
}

// Scale the rectangles of the flashlight and slowmo bars
static float* __stdcall GetRectF_Hook(DWORD* a1, int Record, char* Attribute, int a4)
{
	float* result = GetRectF(a1, Record, Attribute, a4);

	if (!Record || HashHelper::FNV1aRuntime(Attribute) != HashHelper::HUDHashes::AdditionalRect)
		return result;

	const char* hudRecordString = *reinterpret_cast<const char**>(Record);
	const uint32_t elemHash = HashHelper::FNV1aRuntime(hudRecordString);

	if (elemHash == HashHelper::HUDHashes::HUDSlowMo2 || elemHash == HashHelper::HUDHashes::HUDFlashlight || elemHash == HashHelper::HUDHashes::HUDDialogue)
	{
		result[0] *= g_State.scalingFactor;
		result[1] *= g_State.scalingFactor;
		result[2] *= g_State.scalingFactor;
		result[3] *= g_State.scalingFactor;
	}

	return result;
}

// Remember which attribute must be scaled, its value is scaled once it is read by 'DBGetInt32' or 'DBGetFloat'
static int __fastcall DBGetRecord_Hook(int thisPtr, int, int Record, char* Attribute)
{
	int hAttribute = DBGetRecord(thisPtr, Record, Attribute);
	pendingDBScale.hAttribute = 0;

	if (!HUDScaling || !hAttribute || !Record || (Attribute[0] != 'T' && Attribute[0] != 'A'))
		return hAttribute;

	const char* hudRecordString = *reinterpret_cast<const char**>(Record);
	const uint32_t attrHash = HashHelper::FNV1aRuntime(Attribute);

	if (attrHash == HashHelper::HUDHashes::TextSize)
	{
		auto it = g_State.textDataMap.find(hudRecordString);
		if (it != g_State.textDataMap.end())
		{
			float scalingFactor = g_State.scalingFactor;
			switch (it->second)
			{
				case 1: scalingFactor *= 0.95f; break;
				case 2: scalingFactor = g_State.scalingFactorText; break;
				case 3: scalingFactor = g_State.scalingFactorScores; break;
			}
			pendingDBScale = { hAttribute, -1, scalingFactor };
		}
	}
	else if (attrHash == HashHelper::HUDHashes::AdditionalFloat)
	{
		const uint32_t elemHash = HashHelper::FNV1aRuntime(hudRecordString);
		if (elemHash == HashHelper::HUDHashes::HUDSlowMo2 || elemHash == HashHelper::HUDHashes::HUDFlashlight)
		{
			pendingDBScale = { hAttribute, 0, g_State.scalingFactor };
		}
	}
	// HUDHealth medkit prompt
	else if (attrHash == HashHelper::HUDHashes::AdditionalInt && HashHelper::FNV1aRuntime(hudRecordString) == HashHelper::HUDHashes::HUDHealth)
	{
		pendingDBScale = { hAttribute, 2, g_State.scalingFactor };
	}
	// HUDScores columns widths and header text size
	else if (attrHash == HashHelper::HUDHashes::AdditionalInt && HashHelper::FNV1aRuntime(hudRecordString) == HashHelper::HUDHashes::HUDScores)
	{
		pendingDBScale = { hAttribute, -1, g_State.scalingFactorScores };
	}
	// HUDRespawn timer and penalty text sizes
	else if (attrHash == HashHelper::HUDHashes::AdditionalInt && HashHelper::FNV1aRuntime(hudRecordString) == HashHelper::HUDHashes::HUDRespawn)
	{
		pendingDBScale = { hAttribute, -1, g_State.scalingFactor };
	}
	// HUDControlPointBar meters and icon rectangles
	else if (attrHash == HashHelper::HUDHashes::AdditionalRect && HashHelper::FNV1aRuntime(hudRecordString) == HashHelper::HUDHashes::HUDControlPointBar)
	{
		pendingDBScale = { hAttribute, -1, g_State.scalingFactor };
	}
	// Nav markers range, its texture is made at this size
	else if (attrHash == HashHelper::HUDHashes::NavMarkerTextSize && g_State.isInitializingNavMarkers)
	{
		pendingDBScale = { hAttribute, -1, g_State.scalingFactor };
	}

	return hAttribute;
}

static bool IsPendingDBScale(int hAttribute, unsigned int nValueIndex)
{
	if (!hAttribute || hAttribute != pendingDBScale.hAttribute)
		return false;

	return pendingDBScale.valueIndex == -1 || pendingDBScale.valueIndex == static_cast<int>(nValueIndex);
}

static int __fastcall DBGetInt32_Hook(int thisPtr, int, int hAttribute, unsigned int nValueIndex, int nDefault)
{
	int result = DBGetInt32(thisPtr, hAttribute, nValueIndex, nDefault);

	if (!IsPendingDBScale(hAttribute, nValueIndex))
		return result;

	pendingDBScale.hAttribute = 0;
	return ScaleValue(result, pendingDBScale.scalingFactor);
}

static float __fastcall DBGetFloat_Hook(int thisPtr, int, int hAttribute, unsigned int nValueIndex, float fDefault)
{
	float result = DBGetFloat(thisPtr, hAttribute, nValueIndex, fDefault);

	if (!IsPendingDBScale(hAttribute, nValueIndex))
		return result;

	pendingDBScale.hAttribute = 0;
	return result * pendingDBScale.scalingFactor;
}

// Read by 'GetRect' for the rectangles
static float* __fastcall DBGetVector4_Hook(int thisPtr, int, float* pResult, int hAttribute, unsigned int nValueIndex, float* vDefault)
{
	float* result = DBGetVector4(thisPtr, pResult, hAttribute, nValueIndex, vDefault);

	if (!IsPendingDBScale(hAttribute, nValueIndex))
		return result;

	pendingDBScale.hAttribute = 0;
	result[0] *= pendingDBScale.scalingFactor;
	result[1] *= pendingDBScale.scalingFactor;
	result[2] *= pendingDBScale.scalingFactor;
	result[3] *= pendingDBScale.scalingFactor;
	return result;
}

static const char* __fastcall DBGetString_Hook(int thisPtr, int, int hAttribute, unsigned int nValueIndex, int pszDefault)
{
	return DBGetString(thisPtr, hAttribute, nValueIndex, pszDefault);
}

static void __fastcall SliderSetSliderPos_Hook(int thisPtr, int, int nPos)
{
	const char* sliderName = *reinterpret_cast<const char**>(thisPtr + 8);
	const uint32_t nameHash = HashHelper::FNV1aRuntime(sliderName);

	if (HUDScaling)
	{
		if (nameHash == HashHelper::StringHashes::IDS_HELP_PICKUP_MSG_DUR)
		{
			crosshairSliderUpdated = false;
		}

		if (nameHash == HashHelper::StringHashes::ScreenCrosshair_Size_Help && !crosshairSliderUpdated && g_State.scalingFactorCrosshair > 1.0f)
		{
			float unscaledIndex = nPos / g_State.scalingFactorCrosshair;
			int newIndex = static_cast<int>((unscaledIndex / 2.0f) + 0.5f) * 2;
			nPos = std::clamp(newIndex, 4, 16);
			crosshairSliderUpdated = true;
		}
	}

	if (SDLGamepadSupport && !g_State.isBuildingCScreenJoystick)
	{
		if (nameHash == HashHelper::StringHashes::IDS_HELP_CONTROLLER_SENSITIVITY)
		{
			float newValue = nPos / 25.0f;
			if (newValue != GPadAimSensitivity)
			{
				GPadAimSensitivity = newValue;
				SetConsoleVariableFloat("GPadAimSensitivity", newValue);
				IniHelper::iniReader["Controller"]["GPadAimSensitivity"] = std::to_string(GPadAimSensitivity);
				IniHelper::Save();
			}
		}
		else if (nameHash == HashHelper::StringHashes::IDS_HELP_EDGE_ACCELERATION)
		{
			float newValue = (nPos / 100.0f) * 1.4f + 1.0f;
			if (newValue != GPadAimEdgeMultiplier)
			{
				GPadAimEdgeMultiplier = newValue;
				SetConsoleVariableFloat("GPadAimEdgeMultiplier", newValue);
				IniHelper::iniReader["Controller"]["GPadAimEdgeMultiplier"] = std::to_string(GPadAimEdgeMultiplier);
				IniHelper::Save();
			}
		}
		else if (nameHash == HashHelper::StringHashes::IDS_HELP_GYRO_SENSITIVITY)
		{
			float newValue = nPos / 50.0f;
			if (newValue != GyroSensitivity)
			{
				GyroSensitivity = newValue;
				SetGyroSensitivity(GyroSensitivity);
				IniHelper::iniReader["Controller"]["GyroSensitivity"] = std::to_string(GyroSensitivity);
				IniHelper::Save();
			}
		}
		else if (nameHash == HashHelper::StringHashes::IDS_HELP_GYRO_SMOOTHING)
		{
			float newValue = nPos / 1000.0f;
			if (newValue != GyroSmoothing)
			{
				GyroSmoothing = newValue;
				SetGyroSmoothing(GyroSmoothing);
				IniHelper::iniReader["Controller"]["GyroSmoothing"] = std::to_string(GyroSmoothing);
				IniHelper::Save();
			}
		}
		else if (nameHash == HashHelper::StringHashes::IDS_HELP_HOLD_DURATION)
		{
			ScreenJoystickHook::OnHoldDurationChanged(nPos);
		}
	}

	SliderSetSliderPos(thisPtr, nPos);
}

static void __fastcall CycleCtrlSetSelIndex_Hook(int thisPtr, int, unsigned __int8 index)
{
	const char* cycleName = *reinterpret_cast<const char**>(thisPtr + 8);
	const uint32_t nameHash = HashHelper::FNV1aRuntime(cycleName);

	if (SDLGamepadSupport && !g_State.isBuildingCScreenJoystick)
	{
		if (nameHash == HashHelper::StringHashes::IDS_HELP_RUMBLE)
		{
			bool newValue = (index == 1);
			if (newValue != RumbleEnabled)
			{
				RumbleEnabled = newValue;
				SetRumbleEnabled(RumbleEnabled);
				IniHelper::iniReader["Controller"]["RumbleEnabled"] = std::to_string(RumbleEnabled ? 1 : 0);
				IniHelper::Save();
			}
		}
		else if (nameHash == HashHelper::StringHashes::IDS_HELP_GYRO_ENABLED)
		{
			bool newValue = (index == 1);
			if (newValue != GyroEnabled)
			{
				GyroEnabled = newValue;
				SetGyroEnabled(GyroEnabled);
				IniHelper::iniReader["Controller"]["GyroEnabled"] = std::to_string(GyroEnabled ? 1 : 0);
				IniHelper::Save();
			}
		}
		else if (nameHash == HashHelper::StringHashes::IDS_HELP_GYRO_TYPE)
		{
			if (index != GyroAimingMode)
			{
				GyroAimingMode = index;
				IniHelper::iniReader["Controller"]["GyroAimingMode"] = std::to_string(GyroAimingMode);
				IniHelper::Save();
			}
		}
		else if (nameHash == HashHelper::StringHashes::IDS_HELP_GYRO_PERSISTENCE_ENABLED)
		{
			bool newValue = (index == 1);
			if (newValue != GyroCalibrationPersistence)
			{
				GyroCalibrationPersistence = newValue;
				SetGyroCalibrationPersistence(GyroCalibrationPersistence);
				IniHelper::iniReader["Controller"]["GyroCalibrationPersistence"] = std::to_string(GyroCalibrationPersistence ? 1 : 0);
				IniHelper::Save();
			}
		}
		else if (nameHash == HashHelper::StringHashes::IDS_HELP_TOUCHPAD)
		{
			bool newValue = (index == 1);
			if (newValue != TouchpadEnabled)
			{
				TouchpadEnabled = newValue;
				IniHelper::iniReader["Controller"]["TouchpadEnabled"] = std::to_string(TouchpadEnabled ? 1 : 0);
				IniHelper::Save();
			}
		}
	}

	CycleCtrlSetSelIndex(thisPtr, index);
}

// Scale the 'ImageRect' read from the texture data list, it is only written when the record has one
static void __stdcall InitAdditionalTextureData_Hook(int a1, int a2, int* a3, DWORD* vPos, DWORD* vSize, float a6)
{
	const DWORD defaultPosX = vPos[0];
	vPos[0] = 0x80000000;

	InitAdditionalTextureData(a1, a2, a3, vPos, vSize, a6);

	if (vPos[0] == 0x80000000)
	{
		vPos[0] = defaultPosX;
		return;
	}

	vPos[0] = static_cast<DWORD>(ScaleValue(static_cast<int>(vPos[0]), g_State.scalingFactor));
	vPos[1] = static_cast<DWORD>(ScaleValue(static_cast<int>(vPos[1]), g_State.scalingFactor));
	vSize[0] = static_cast<DWORD>(ScaleValue(static_cast<int>(vSize[0]), g_State.scalingFactor));
	vSize[1] = static_cast<DWORD>(ScaleValue(static_cast<int>(vSize[1]), g_State.scalingFactor));
}

static void __fastcall HUDPausedInit_Hook(int thisPtr, int)
{
	g_State.CHUDPaused = thisPtr;
	HUDPausedInit(thisPtr);
}

// Scale the drop shadow offset of the HUD texts
static int __fastcall GUIStringRender_Hook(int thisPtr, int)
{
	BYTE dropShadow = *(BYTE*)(thisPtr + 0x54);

	if (!g_State.isRenderingHUD || dropShadow == 0)
		return GUIStringRender(thisPtr);

	*(BYTE*)(thisPtr + 0x54) = static_cast<BYTE>(std::clamp(ScaleValue(dropShadow, g_State.scalingFactor), 1, 255));
	int result = GUIStringRender(thisPtr);
	*(BYTE*)(thisPtr + 0x54) = dropShadow;
	return result;
}

// The nav markers sizes are in pixels, those of the teammates are scaled by the X and Y ratios
static float GetNavMarkerScalingFactor(int thisPtr)
{
	// Only the games scaling the teammates have the flag
	if (g_State.interfaceResMgrAddr == 0 || !*(bool*)(thisPtr + 0x2D4))
		return g_State.scalingFactor;

	// The size they have in 4:3, with the custom scaling
	int interfaceResMgr = MemoryHelper::ReadMemory<int>(g_State.interfaceResMgrAddr);
	float ratio = std::min(*(float*)(interfaceResMgr + 0x10), *(float*)(interfaceResMgr + 0x14));
	return ratio * g_State.scalingFactor / std::max(1.0f, std::min(g_State.currentWidth / BASE_WIDTH, g_State.currentHeight / BASE_HEIGHT));
}

// Scale the icon, arrow, range and text offsets of a nav marker while it renders
static void RenderScaledNavMarker(void(__thiscall* render)(int), int thisPtr)
{
	float* vectors = reinterpret_cast<float*>(thisPtr + 0x25C);
	float defaultVectors[10];
	memcpy(defaultVectors, vectors, sizeof(defaultVectors));

	float scalingFactor = GetNavMarkerScalingFactor(thisPtr);
	for (int i = 0; i < 10; i++)
	{
		vectors[i] *= scalingFactor;
	}

	// Already scaled, the ratios aren't applied
	bool* isScaled = reinterpret_cast<bool*>(thisPtr + 0x2D4);
	bool wasScaled = false;
	if (g_State.interfaceResMgrAddr != 0)
	{
		wasScaled = *isScaled;
		*isScaled = false;
	}

	render(thisPtr);

	if (g_State.interfaceResMgrAddr != 0) *isScaled = wasScaled;
	memcpy(vectors, defaultVectors, sizeof(defaultVectors));
}

static void __fastcall HUDNavMarkerRender_Hook(int thisPtr, int)
{
	if (!HUDScaling)
	{
		HUDNavMarkerRender(thisPtr);
		return;
	}

	RenderScaledNavMarker(HUDNavMarkerRender, thisPtr);
}

static void __fastcall HUDNavMarkerRenderArrow_Hook(int thisPtr, int)
{
	if (!HUDScaling)
	{
		HUDNavMarkerRenderArrow(thisPtr);
		return;
	}

	RenderScaledNavMarker(HUDNavMarkerRenderArrow, thisPtr);
}

// Scale the text of the nav markers, it is only sized by the Y ratio for those of the teammates
static void __fastcall HUDNavMarkerScaleChanged_Hook(int thisPtr, int)
{
	if (!HUDScaling)
	{
		HUDNavMarkerScaleChanged(thisPtr);
		return;
	}

	float* yRatio = reinterpret_cast<float*>(MemoryHelper::ReadMemory<int>(g_State.interfaceResMgrAddr) + 0x14);
	bool* isScaled = reinterpret_cast<bool*>(thisPtr + 0x2D4);
	const float defaultYRatio = *yRatio;
	const bool wasScaled = *isScaled;

	*yRatio = GetNavMarkerScalingFactor(thisPtr);
	*isScaled = true;
	HUDNavMarkerScaleChanged(thisPtr);
	*yRatio = defaultYRatio;
	*isScaled = wasScaled;
}

static bool __fastcall HUDNavMarkerMgrInit_Hook(int thisPtr, int)
{
	g_State.CHUDNavMarkerMgr = thisPtr;
	g_State.isInitializingNavMarkers = true;
	bool result = HUDNavMarkerMgrInit(thisPtr);
	g_State.isInitializingNavMarkers = false;
	return result;
}

// ======================
// HUD Reinitialization
// ======================

// What the game gave to the HUD items, they are made again when the HUD is reinitialized
struct HUDItemsState
{
	std::vector<int> navMarkers;
	std::vector<std::array<BYTE, 0x12>> timers;
	std::vector<bool> ctfFlags;
	bool isPausedVisible = false;
};

// The HUD items made by this Init, in the order of CHUDMgr::Init
static std::vector<int> FindHUDItems(DWORD init)
{
	std::vector<int> items;
	int* item = *reinterpret_cast<int**>(g_State.CHUDMgr + 0x47C);
	int* end = *reinterpret_cast<int**>(g_State.CHUDMgr + 0x480);

	for (; item != end; item++)
	{
		if (init != 0 && MemoryHelper::ReadMemory<DWORD>(*reinterpret_cast<int*>(*item) + 0x4) == init)
			items.push_back(*item);
	}

	return items;
}

static HUDItemsState SaveHUDItemsState()
{
	HUDItemsState state;

	if (g_State.CHUDNavMarkerMgr != 0)
	{
		int* begin = *reinterpret_cast<int**>(g_State.CHUDNavMarkerMgr + 0x1C4);
		int* end = *reinterpret_cast<int**>(g_State.CHUDNavMarkerMgr + 0x1C8);
		state.navMarkers.assign(begin, end);
	}

	// Time, time left, paused and drawn, set by the server
	for (int timer : FindHUDItems(reinterpret_cast<DWORD>(HUDTimerInit)))
	{
		std::array<BYTE, 0x12> timerState;
		memcpy(timerState.data(), reinterpret_cast<void*>(timer + 0x1C0), timerState.size());
		state.timers.push_back(timerState);
	}

	// Carrying the flag
	for (int ctfFlag : FindHUDItems(reinterpret_cast<DWORD>(HUDCTFFlagInit)))
	{
		state.ctfFlags.push_back(*(bool*)(ctfFlag + 0x1C1));
	}

	if (g_State.CHUDPaused != 0)
	{
		state.isPausedVisible = *(bool*)(g_State.CHUDPaused + 0x202);
	}

	return state;
}

static void RestoreHUDItemsState(const HUDItemsState& state)
{
	if (g_State.CHUDNavMarkerMgr != 0 && HUDNavMarkerMgrAddMarker && GUIStringSetSourceString)
	{
		int rangeTexture = *(int*)(g_State.CHUDNavMarkerMgr + 0x1D4);
		for (int navMarker : state.navMarkers)
		{
			HUDNavMarkerMgrAddMarker(g_State.CHUDNavMarkerMgr, navMarker);
			GUIStringSetSourceString(navMarker + 0xF4, rangeTexture);
		}

		// ScaleChanged of the markers
		auto scaleChanged = reinterpret_cast<void(__thiscall*)(int)>(MemoryHelper::ReadMemory<DWORD>(*(int*)g_State.CHUDNavMarkerMgr + 0x18));
		scaleChanged(g_State.CHUDNavMarkerMgr);
	}

	std::vector<int> timers = FindHUDItems(reinterpret_cast<DWORD>(HUDTimerInit));
	for (size_t i = 0; i < timers.size() && i < state.timers.size(); i++)
	{
		memcpy(reinterpret_cast<void*>(timers[i] + 0x1C0), state.timers[i].data(), state.timers[i].size());

		// UpdateLayout, like CHUDTimer::SetTime
		auto updateLayout = reinterpret_cast<void(__thiscall*)(int)>(MemoryHelper::ReadMemory<DWORD>(*(int*)timers[i] + 0x20));
		updateLayout(timers[i]);
	}

	std::vector<int> ctfFlags = FindHUDItems(reinterpret_cast<DWORD>(HUDCTFFlagInit));
	for (size_t i = 0; i < ctfFlags.size() && i < state.ctfFlags.size(); i++)
	{
		*(bool*)(ctfFlags[i] + 0x1C1) = state.ctfFlags[i];

		// Update, like CHUDCTFFlag::SetHasFlag
		auto update = reinterpret_cast<void(__thiscall*)(int)>(MemoryHelper::ReadMemory<DWORD>(*(int*)ctfFlags[i] + 0x14));
		update(ctfFlags[i]);
	}

	// Shown by its Init, only hidden once the game is unpaused
	if (g_State.CHUDPaused != 0)
	{
		*(bool*)(g_State.CHUDPaused + 0x202) = state.isPausedVisible;
	}
}
