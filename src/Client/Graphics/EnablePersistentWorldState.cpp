#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"
#include "../../Controller/ScreenJoystickHook.hpp"
#include "../../ClientFX/ClientFX.hpp"
#include "../../Server/Server.hpp"

struct PersistentFXInfo
{
	std::string name;
	uint32_t flags;
	int props;
};

struct PersistentFXRecord
{
	std::string name;
	uint32_t flags;
	float transform[7];
};

struct PersistentShellCasingRecord
{
	std::string category;
	std::string ammo;
	uint32_t flags;
	float transform[7];
};

struct PersistentWorldState
{
	std::vector<PersistentFXRecord> fx;
	std::vector<PersistentShellCasingRecord> shellCasings;
	std::vector<uint8_t> decalData;
	uint32_t decalBits = 0;
};

static std::unordered_map<int, PersistentFXInfo> persistentFXInfos;
static PersistentWorldState persistentWorldState;
static bool isRestoringWorldState = false;
static float bulletHoleCount = 1.0f;
static DWORD pGameClientShellAddr = 0;
static DWORD pLTDatabaseAddr = 0;
static DWORD simulationFXMgrOffset = 0;
static DWORD sfxMgrOffset = 0;
static DWORD modelDecalMgrOffset = 0;
static uint8_t shellCasingSFXId = 0;

// ==========================
// EnablePersistentWorldState
// ==========================

static float __stdcall GetShatterLifetime_Hook(int shatterType)
{
	return FLT_MAX;
}

static DWORD CreateEngineMessage()
{
	DWORD common = EngineInterface::GetClientCommon();
	DWORD message = 0;

	if (!common || EngineInterface::CallOffset<int>(common, EngineInterface::Common_CreateMessage, &message) != 0 || !message)
		return 0;

	EngineInterface::CallOffset<void>(message, EngineInterface::Message_AddRef);
	return message;
}

static bool CapturePersistentDecals(int modelDecalMgr, int eSaveDataState)
{
	DWORD message = CreateEngineMessage();
	if (!message) return false;

	ModelDecalMgrSave(modelDecalMgr, (int*)message, eSaveDataState);

	DWORD bits = EngineInterface::CallOffset<DWORD>(message, EngineInterface::Message_Size);
	DWORD readMessage = EngineInterface::CallOffset<DWORD>(message, EngineInterface::Message_Read);

	if (readMessage)
	{
		EngineInterface::CallOffset<void>(readMessage, EngineInterface::Message_AddRef);
		persistentWorldState.decalData.assign((bits + 7) / 8, 0);
		persistentWorldState.decalBits = bits;
		EngineInterface::CallOffset<void>(readMessage, EngineInterface::Message_ReadData, (void*)persistentWorldState.decalData.data(), (uint32_t)bits);
		EngineInterface::CallOffset<void>(readMessage, EngineInterface::Message_Release);
	}

	EngineInterface::CallOffset<void>(message, EngineInterface::Message_Release);
	return readMessage != 0;
}

static void __fastcall ModelDecalMgrSave_Hook(int thisPtr, int, int* pMsg, int eSaveDataState)
{
	DWORD* pBegin = (DWORD*)(thisPtr + 0x18);
	DWORD* pEnd = (DWORD*)(thisPtr + 0x1C);

	// Remove the decals of the deleted objects, one pass can skip some
	DWORD decalsSize;
	do
	{
		decalsSize = *pEnd - *pBegin;
		ModelDecalMgrCleanDecalList(thisPtr);
	} while (*pEnd - *pBegin != decalsSize);

	// Saved in the persistent world state file instead, the client save data only gets an empty list
	if (SavePersistentWorldState && CapturePersistentDecals(thisPtr, eSaveDataState))
	{
		EngineInterface::CallOffset<void>((DWORD)pMsg, EngineInterface::Message_WriteBits, (uint32_t)0, (uint32_t)32);
		EngineInterface::CallOffset<void>((DWORD)pMsg, EngineInterface::Message_WriteBits, (uint32_t)0, (uint32_t)32);
		return;
	}

	// The client save data is stored with a 16 bits size, 608 bits per decal
	DWORD typeCount = (*(DWORD*)(thisPtr + 0xC) - *(DWORD*)(thisPtr + 0x8)) / 0x28;
	DWORD usedBits = EngineInterface::CallOffset<DWORD>((DWORD)pMsg, EngineInterface::Message_Size) + typeCount * 512 + 1088;
	DWORD maxDecals = usedBits < 0xFFFF ? (0xFFFF - usedBits) / 608 : 0;

	// Only save the newest decals
	DWORD realBegin = *pBegin;
	if ((*pEnd - *pBegin) / 0x28 > maxDecals)
	{
		*pBegin = *pEnd - maxDecals * 0x28;
	}

	ModelDecalMgrSave(thisPtr, pMsg, eSaveDataState);

	*pBegin = realBegin;
}

static bool IsPersistentFX(char* effectType, int fxData)
{
	// Decal
	if (*reinterpret_cast<uint32_t*>(effectType) == 0x61636544)
	{
		return true;
	}

	// LTBModel
	if (*reinterpret_cast<uint32_t*>(effectType) == 0x4D42544C)
	{
		int fxName = *(DWORD*)(fxData + 0x74);

		// Skip HRocket_Debris
		return *reinterpret_cast<uint64_t*>(fxName + 0x2) != 0x65445F74656B636F;
	}

	// Sprite
	if (*reinterpret_cast<uint32_t*>(effectType) == 0x69727053)
	{
		int fxName = *(DWORD*)(fxData + 0x74);

		return *reinterpret_cast<uint64_t*>(fxName) == 0x75625F656E6F7453 // Stone_bullethole
			|| *reinterpret_cast<uint64_t*>(fxName) == 0x455F736972626544 // Debris_Electronic_Chunk
			|| *reinterpret_cast<uint64_t*>(fxName) == 0x575F736972626544 // Debris_Wood_Chunk
			|| *reinterpret_cast<uint64_t*>(fxName) == 0x4D5F736972626544 // Debris_Mug_Chunk
			|| *reinterpret_cast<uint64_t*>(fxName) == 0x565F736972626544 // Debris_Vase1_Chunk
			|| *reinterpret_cast<uint64_t*>(fxName + 0x4) == 0x3674616C70735F68; // Flesh_splat6
	}

	return false;
}

static int __stdcall CreateFX_Hook(char* effectType, int fxData, int prop)
{
	bool isPersistent = prop && IsPersistentFX(effectType, fxData);

	// Only recreate the persistent parts of the effect
	if (isRestoringWorldState && !isPersistent)
	{
		return 0;
	}

	if (isPersistent)
	{
		MemoryHelper::WriteMemory<float>(prop + 0x8, FLT_MAX, false); // m_tmEnd
		MemoryHelper::WriteMemory<float>(prop + 0xC, FLT_MAX, false); // m_tmLifetime
	}

	int fx = CreateFX(effectType, fxData, prop);

	// Keep the name of the effect group to recreate it from a save
	if (isPersistent && fx && SavePersistentWorldState)
	{
		const char* groupName = *(const char**)(fxData + 0x74);
		persistentFXInfos[fx] = { std::string(groupName, strnlen(groupName, 127)), *(uint32_t*)(fxData + 0x44), prop };
	}

	return fx;
}

// ========================
// SavePersistentWorldState
// ========================

static int GetSimulationFXMgr()
{
	int pGameClientShell = *(int*)pGameClientShellAddr;
	return pGameClientShell ? pGameClientShell + simulationFXMgrOffset : 0;
}

static void CapturePersistentShellCasings()
{
	int pGameClientShell = *(int*)pGameClientShellAddr;
	DWORD database = *(DWORD*)pLTDatabaseAddr;
	DWORD client = EngineInterface::GetClient();
	if (!pGameClientShell || !database || !client) return;

	// 'm_dynSFXLists'
	int sfxList = pGameClientShell + sfxMgrOffset + shellCasingSFXId * 0x10;
	DWORD* sfxArray = *(DWORD**)sfxList;
	uint32_t sfxArraySize = *(uint32_t*)(sfxList + 0x8);
	if (!sfxArray) return;

	for (uint32_t i = 0; i < sfxArraySize; i++)
	{
		DWORD shellCasing = sfxArray[i];
		if (!shellCasing) continue;

		DWORD hObject = *(DWORD*)(shellCasing + 0x14);
		DWORD hAmmo = *(DWORD*)(shellCasing + 0x78);

		// Skip if 'm_bWantRemove' or not 'm_bResting'
		if (!hObject || !hAmmo || *(bool*)(shellCasing + 0x40) || !*(bool*)(shellCasing + 0x82)) continue;

		DWORD hCategory = EngineInterface::CallOffset<DWORD>(database, EngineInterface::Database_GetRecordParent, hAmmo);
		const char* categoryName = hCategory ? EngineInterface::CallOffset<const char*>(database, EngineInterface::Database_GetCategoryName, hCategory) : nullptr;
		const char* ammoName = EngineInterface::CallOffset<const char*>(database, EngineInterface::Database_GetRecordName, hAmmo);
		if (!categoryName || !ammoName) continue;

		PersistentShellCasingRecord record{ categoryName, ammoName, *(uint32_t*)(shellCasing + 0x7C) };
		EngineInterface::Call<int>(client, EngineInterface::Slot::GetObjectPos, hObject, (void*)record.transform);
		EngineInterface::Call<int>(client, EngineInterface::Slot::GetObjectRotation, hObject, (void*)(record.transform + 3));
		persistentWorldState.shellCasings.push_back(std::move(record));
	}
}

static void CapturePersistentWorldState()
{
	persistentWorldState = {};

	CapturePersistentShellCasings();

	int fxMgr = GetSimulationFXMgr();
	if (!fxMgr) return;

	DWORD client = EngineInterface::GetClient();
	DWORD hPlayer = client ? EngineInterface::CallMember<DWORD>(client, EngineInterface::Client_GetClientObject) : 0;
	std::unordered_map<int, PersistentFXInfo> liveFXInfos;

	// 'm_FXInstanceList' and 'm_NextUpdateFXList'
	for (int listHead : { fxMgr + 0x20, fxMgr + 0x28 })
	{
		for (int link = *(int*)(listHead + 0x4); link != listHead; link = *(int*)(link + 0x4))
		{
			int instance = *(int*)(link + 0x8);

			// Skip if 'm_bShutdown'
			if (*(bool*)(instance + 0x42)) continue;

			bool isRecorded = false;
			int fxListHead = instance + 0x4;

			for (int fxLink = *(int*)(fxListHead + 0x4); fxLink != fxListHead; fxLink = *(int*)(fxLink + 0x4))
			{
				int fx = *(int*)(fxLink + 0x8);
				auto it = persistentFXInfos.find(fx);

				// Skip if not persistent or shutting down
				if (it == persistentFXInfos.end() || *(int*)(fx + 0x14) != it->second.props || (*(DWORD*)(fx + 0x18) & 0x4)) continue;

				liveFXInfos.emplace(fx, it->second);

				// Skip if attached to the player or to a model node/socket
				if (isRecorded || (hPlayer && *(DWORD*)(fx + 0x5C) == hPlayer) || *(int*)(fx + 0x64) != -1 || *(int*)(fx + 0x68) != -1) continue;

				// Save the group once, at the current world transform of the effect
				PersistentFXRecord record{ it->second.name, it->second.flags };
				memcpy(record.transform, (void*)(fx + 0x24), sizeof(record.transform));
				persistentWorldState.fx.push_back(std::move(record));
				isRecorded = true;
			}
		}
	}

	persistentFXInfos = std::move(liveFXInfos);

	// Keep the newest effects
	if (persistentWorldState.fx.size() > 4096)
	{
		persistentWorldState.fx.erase(persistentWorldState.fx.begin(), persistentWorldState.fx.end() - 4096);
	}
}

template <typename T>
static void WriteValue(std::ofstream& file, const T& value)
{
	file.write((const char*)&value, sizeof(T));
}

static void WriteString(std::ofstream& file, const std::string& text)
{
	uint8_t length = (uint8_t)std::min<size_t>(text.size(), 255);
	WriteValue(file, length);
	file.write(text.data(), length);
}

template <typename T>
static bool ReadValue(std::ifstream& file, T& value)
{
	return (bool)file.read((char*)&value, sizeof(T));
}

static bool ReadString(std::ifstream& file, std::string& text)
{
	uint8_t length = 0;
	if (!ReadValue(file, length) || length == 0) return false;

	text.resize(length);
	return (bool)file.read(text.data(), length);
}

static bool ReadTransform(std::ifstream& file, float* transform)
{
	return file.read((char*)transform, sizeof(float) * 7) && std::all_of(transform, transform + 7, [](float value) { return std::isfinite(value); });
}

void WritePersistentWorldStateFile(const std::string& savePath)
{
	if (!savePath.ends_with(".sav")) return;

	std::ofstream file(savePath.substr(0, savePath.size() - 4) + ".pws", std::ios::binary | std::ios::trunc);
	if (!file) return;

	// "EPWS", version, count
	uint32_t header[3] = { 0x53575045, 1, (uint32_t)persistentWorldState.fx.size() };
	file.write((char*)header, sizeof(header));

	for (const auto& record : persistentWorldState.fx)
	{
		WriteString(file, record.name);
		WriteValue(file, record.flags);
		file.write((char*)record.transform, sizeof(record.transform));
	}

	WriteValue(file, (uint32_t)persistentWorldState.shellCasings.size());

	for (const auto& record : persistentWorldState.shellCasings)
	{
		WriteString(file, record.category);
		WriteString(file, record.ammo);
		WriteValue(file, record.flags);
		file.write((char*)record.transform, sizeof(record.transform));
	}

	WriteValue(file, persistentWorldState.decalBits);
	file.write((char*)persistentWorldState.decalData.data(), persistentWorldState.decalData.size());

	persistentWorldState = {};
}

static bool ReadPersistentWorldStateFile(const std::string& savePath, PersistentWorldState& state)
{
	std::string path = savePath.substr(0, savePath.size() - 4) + ".pws";

	WIN32_FILE_ATTRIBUTE_DATA saveData, stateData;
	if (!GetFileAttributesExA(savePath.c_str(), GetFileExInfoStandard, &saveData) || !GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &stateData))
		return false;

	// Skip if older than the save, the save was overwritten without this option
	ULARGE_INTEGER saveTime = { saveData.ftLastWriteTime.dwLowDateTime, saveData.ftLastWriteTime.dwHighDateTime };
	ULARGE_INTEGER stateTime = { stateData.ftLastWriteTime.dwLowDateTime, stateData.ftLastWriteTime.dwHighDateTime };
	if (stateTime.QuadPart + 20000000 < saveTime.QuadPart)
		return false;

	std::ifstream file(path, std::ios::binary);
	uint32_t header[3] = {};
	if (!file.read((char*)header, sizeof(header)) || header[0] != 0x53575045 || header[1] != 1 || header[2] > 4096)
		return false;

	for (uint32_t i = 0; i < header[2]; i++)
	{
		PersistentFXRecord record{};

		if (!ReadString(file, record.name) || record.name.size() > 127 || !ReadValue(file, record.flags) || !ReadTransform(file, record.transform))
			return false;

		state.fx.push_back(std::move(record));
	}

	uint32_t shellCasingCount = 0;
	if (!ReadValue(file, shellCasingCount) || shellCasingCount > 0xFFFF)
		return false;

	for (uint32_t i = 0; i < shellCasingCount; i++)
	{
		PersistentShellCasingRecord record{};

		if (!ReadString(file, record.category) || !ReadString(file, record.ammo) || !ReadValue(file, record.flags) || !ReadTransform(file, record.transform))
			return false;

		state.shellCasings.push_back(std::move(record));
	}

	if (!ReadValue(file, state.decalBits) || state.decalBits > 0x10000000)
		return false;

	state.decalData.resize((state.decalBits + 7) / 8);
	return state.decalData.empty() || (bool)file.read((char*)state.decalData.data(), state.decalData.size());
}

static void RestorePersistentFX(const std::vector<PersistentFXRecord>& records)
{
	int fxMgr = GetSimulationFXMgr();
	if (!fxMgr) return;

	// CLIENTFX_CREATESTRUCT
	alignas(4) uint8_t createStruct[0xC4];

	isRestoringWorldState = true;

	for (const auto& record : records)
	{
		memset(createStruct, 0, sizeof(createStruct));
		memcpy(createStruct, record.name.data(), record.name.size()); // m_sName
		*(uint32_t*)(createStruct + 0x80) = record.flags; // m_dwFlags
		*(int*)(createStruct + 0x8C) = -1; // m_hNode
		*(int*)(createStruct + 0x90) = -1; // m_hSocket
		memcpy(createStruct + 0x94, record.transform, sizeof(record.transform)); // m_tTransform

		CreateClientFX(fxMgr, 0, createStruct, true, false);
	}

	isRestoringWorldState = false;
}

static void RestorePersistentShellCasings(const std::vector<PersistentShellCasingRecord>& records)
{
	int pGameClientShell = *(int*)pGameClientShellAddr;
	DWORD database = *(DWORD*)pLTDatabaseAddr;
	int decalCategory = GetModelDecalCategory();
	if (!pGameClientShell || !database || !decalCategory || records.empty()) return;

	// Same database as the model decals
	DWORD hDatabase = EngineInterface::CallOffset<DWORD>(database, EngineInterface::Database_GetCategoryParent, *(DWORD*)(decalCategory + 0x24));
	if (!hDatabase) return;

	// SHELLCREATESTRUCT
	alignas(4) uint8_t createStruct[0x40];

	for (const auto& record : records)
	{
		DWORD hCategory = EngineInterface::CallOffset<DWORD>(database, EngineInterface::Database_GetCategory, hDatabase, record.category.c_str());
		DWORD hAmmo = hCategory ? EngineInterface::CallOffset<DWORD>(database, EngineInterface::Database_GetRecord, hCategory, record.ammo.c_str()) : 0;
		if (!hAmmo) continue;

		memset(createStruct, 0, sizeof(createStruct));
		memcpy(createStruct + 0x8, record.transform + 3, sizeof(float) * 4); // rRot
		memcpy(createStruct + 0x18, record.transform, sizeof(float) * 3); // vStartPos
		*(DWORD*)(createStruct + 0x34) = hAmmo; // hAmmo
		*(uint32_t*)(createStruct + 0x38) = record.flags; // dwFlags
		createStruct[0x3C] = 1; // b3rdPerson

		int shellCasing = CreateSFX(pGameClientShell + sfxMgrOffset, shellCasingSFXId, createStruct, 0, 0);

		// Already on the ground, 'm_bResting' and full scale
		if (shellCasing)
		{
			*(bool*)(shellCasing + 0x82) = true;
			*(float*)(shellCasing + 0x8C) = 1000.0f;
		}
	}
}

static void RestorePersistentDecals(const std::vector<uint8_t>& decalData, uint32_t decalBits)
{
	int pGameClientShell = *(int*)pGameClientShellAddr;
	int modelDecalMgr = pGameClientShell ? *(int*)(pGameClientShell + modelDecalMgrOffset) : 0;
	if (!modelDecalMgr || decalBits == 0) return;

	DWORD message = CreateEngineMessage();
	if (!message) return;

	EngineInterface::CallOffset<void>(message, EngineInterface::Message_WriteData, (const void*)decalData.data(), decalBits);
	DWORD readMessage = EngineInterface::CallOffset<DWORD>(message, EngineInterface::Message_Read);

	if (readMessage)
	{
		EngineInterface::CallOffset<void>(readMessage, EngineInterface::Message_AddRef);
		ModelDecalMgrLoad(modelDecalMgr, readMessage, 0);
		EngineInterface::CallOffset<void>(readMessage, EngineInterface::Message_Release);
	}

	EngineInterface::CallOffset<void>(message, EngineInterface::Message_Release);
}

static void RestorePersistentWorldState()
{
	std::string savePath = std::move(g_State.loadedSaveFilePath);
	g_State.loadedSaveFilePath.clear();

	PersistentWorldState state;
	if (!savePath.ends_with(".sav") || !ReadPersistentWorldStateFile(savePath, state)) return;

	RestorePersistentFX(state.fx);
	RestorePersistentShellCasings(state.shellCasings);
	RestorePersistentDecals(state.decalData, state.decalBits);
}

static void __fastcall BuildClientSaveMsg_Hook(int thisPtr, int, int* pMsg, int eSaveDataState)
{
	// Written next to the save file once the server is done with it
	CapturePersistentWorldState();
	BuildClientSaveMsg(thisPtr, pMsg, eSaveDataState);
}
