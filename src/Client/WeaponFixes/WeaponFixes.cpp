#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"
#include "../../Controller/ScreenJoystickHook.hpp"
#include "../../ClientFX/ClientFX.hpp"
#include "../../Server/Server.hpp"

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
