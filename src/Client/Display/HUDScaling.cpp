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

	if (elemHash == HashHelper::HUDHashes::HUDSlowMo2 || elemHash == HashHelper::HUDHashes::HUDFlashlight)
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

	if (!hAttribute || !Record || (Attribute[0] != 'T' && Attribute[0] != 'A'))
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
