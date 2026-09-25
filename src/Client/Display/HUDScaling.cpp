#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"
#include "../../Controller/ScreenJoystickHook.hpp"
#include "../../ClientFX/ClientFX.hpp"
#include "../../Server/Server.hpp"

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

static void __fastcall SliderSetSliderPos_Hook(int thisPtr, int, int nPos)
{
	const char* sliderName = *reinterpret_cast<const char**>(thisPtr + 8);
	const uint32_t nameHash = HashHelper::FNV1aRuntime(sliderName);

	if (HUDScaling)
	{
		if (nameHash == HashHelper::StringHashes::IDS_HELP_PICKUP_MSG_DUR)
		{
			g_State.crosshairSliderUpdated = false;
		}

		if (nameHash == HashHelper::StringHashes::ScreenCrosshair_Size_Help && !g_State.crosshairSliderUpdated && g_State.scalingFactorCrosshair > 1.0f)
		{
			float unscaledIndex = nPos / g_State.scalingFactorCrosshair;
			int newIndex = static_cast<int>((unscaledIndex / 2.0f) + 0.5f) * 2;
			nPos = std::clamp(newIndex, 4, 16);
			g_State.crosshairSliderUpdated = true;
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
