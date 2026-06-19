#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"
#include "../../Controller/ScreenJoystickHook.hpp"
#include "../../ClientFX/ClientFX.hpp"
#include "../../Server/Server.hpp"

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
