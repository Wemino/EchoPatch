#pragma once

#include "../../Globals.cpp"
#include "../../Controller/Controller.hpp"
#include "../../Controller/ScreenJoystickHook.hpp"
#include "../../ClientFX/ClientFX.hpp"
#include "../../Server/Server.hpp"

// What a persistent effect is attached to
enum class PersistentFXParent : uint8_t
{
	None,
	Object,
	Node,
	RigidBody
};

enum class PersistentShape : uint32_t
{
	Sphere,
	Box,
	Capsule
};

struct PersistentFXInfo
{
	std::string name;
	uint32_t flags;
	int props;
	uint32_t type;
	uint32_t key;
	float random;
	DWORD worldModel;
	float worldModelTransform[7];
};

struct PersistentFXKeyRecord
{
	uint16_t key;
	float random[2];
};

struct PersistentRigidBodyRecord
{
	PersistentShape shape;
	uint32_t group;
	uint32_t system;
	uint32_t body;
	float shapeData[7];
	float mass;
	float density;
	float friction;
	float cor;
	float transform[7];
	float velocity[3];
	float angularVelocity[3];
};

struct PersistentFXRecord
{
	std::string name;
	uint32_t flags;
	float transform[7];
	std::vector<PersistentFXKeyRecord> keys;
	PersistentFXParent parent;
	float parentPos[3];
	float parentDims[3];
	std::string node;
	uint32_t socket;
	float offset[7];
	PersistentRigidBodyRecord rigidBody;
};

struct PersistentShellCasingRecord
{
	std::string category;
	std::string ammo;
	uint32_t flags;
	float transform[7];
};

struct PersistentShatterInfo
{
	uint32_t seed;
	uint32_t dataId;
	float transform[7];
	float hitPos[3];
	float hitDir[3];
};

struct PersistentShatterPieceRecord
{
	float transform[7];
	float motion[7];
};

struct PersistentShatterRecord
{
	std::string category;
	std::string type;
	PersistentShatterInfo info;
	float elapsed;
	std::vector<PersistentShatterPieceRecord> pieces;
};

struct PersistentWorldState
{
	std::vector<PersistentFXRecord> fx;
	std::vector<PersistentShellCasingRecord> shellCasings;
	std::vector<uint8_t> decalData;
	uint32_t decalBits = 0;
	std::vector<uint8_t> decalMaterials;
	std::vector<PersistentShatterRecord> shatters;
};

static std::unordered_map<int, PersistentFXInfo> persistentFXInfos;
static std::unordered_map<int, PersistentShatterInfo> persistentShatterInfos;
static std::unordered_map<DWORD, uint8_t> persistentDecalMaterials;
static std::vector<uint8_t> restoredDecalMaterials;
static std::vector<PersistentFXRecord> pendingFX;
static ULONGLONG pendingFXTime = 0;
static PersistentWorldState persistentWorldState;
static bool isRestoringWorldState = false;
static const PersistentFXRecord* restoringFXRecord = nullptr;
static const void* restoringCreateStruct = nullptr;
static float bulletHoleCount = 1.0f;
static DWORD pGameClientShellAddr = 0;
static DWORD pLTDatabaseAddr = 0;
static DWORD pPhysicsLTAddr = 0;
static DWORD simulationFXMgrOffset = 0;
static DWORD sfxMgrOffset = 0;
static DWORD modelDecalMgrOffset = 0;
static DWORD shatterEffectMgrOffset = 0;
static DWORD findObjectsInBoxOffset = 0;
static DWORD randImportAddr = 0;
static uint32_t shatterRandState = 0;
static uint8_t shellCasingSFXId = 0;

// ==========================
// EnablePersistentWorldState
// ==========================

static float __stdcall GetShatterLifetime_Hook(int shatterType)
{
	return FLT_MAX;
}

// The rand of the game, the random values are picked the same way the game does
static int GameRand()
{
	return reinterpret_cast<int(__cdecl*)()>(*(DWORD*)randImportAddr)();
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

static void CapturePersistentDecalMaterials(int modelDecalMgr)
{
	std::unordered_map<DWORD, uint8_t> liveDecalMaterials;

	// 'm_aDecals', in the same order as the saved decals
	for (DWORD decal = *(DWORD*)(modelDecalMgr + 0x18); decal && decal != *(DWORD*)(modelDecalMgr + 0x1C); decal += 0x28)
	{
		auto it = persistentDecalMaterials.find(*(DWORD*)(decal + 0x14));
		persistentWorldState.decalMaterials.push_back(it != persistentDecalMaterials.end() ? it->second : 0xFF);

		if (it != persistentDecalMaterials.end())
		{
			liveDecalMaterials.emplace(*it);
		}
	}

	persistentDecalMaterials = std::move(liveDecalMaterials);
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
		CapturePersistentDecalMaterials(modelDecalMgr);
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

	// 'nCurrKey' of the effect group being started
	uint32_t key = *(uint32_t*)(fxData + 0x70);
	const PersistentFXKeyRecord* restoredKey = nullptr;

	// Only the parts still in the world when saving
	if (restoringFXRecord && *(const void**)(fxData + 0x74) == restoringCreateStruct)
	{
		auto it = std::find_if(restoringFXRecord->keys.begin(), restoringFXRecord->keys.end(), [key](const PersistentFXKeyRecord& record) { return record.key == key; });
		if (it == restoringFXRecord->keys.end()) return 0;

		restoredKey = &*it;
	}

	if (isPersistent)
	{
		MemoryHelper::WriteMemory<float>(prop + 0x8, FLT_MAX, false); // m_tmEnd
		MemoryHelper::WriteMemory<float>(prop + 0xC, FLT_MAX, false); // m_tmLifetime
	}

	uint32_t type = *reinterpret_cast<uint32_t*>(effectType);
	bool isRestoredSprite = restoredKey && type == 0x69727053;
	float minScale = isRestoredSprite ? *(float*)(prop + 0x58) : 0.0f;
	float maxScale = isRestoredSprite ? *(float*)(prop + 0x5C) : 0.0f;

	// Sprite, the same random scale as when saved, 'm_fMinScale' and 'm_fMaxScale'
	if (isRestoredSprite)
	{
		*(float*)(prop + 0x58) = restoredKey->random[1];
		*(float*)(prop + 0x5C) = restoredKey->random[1];
	}

	int fx = CreateFX(effectType, fxData, prop);

	if (isRestoredSprite)
	{
		*(float*)(prop + 0x58) = minScale;
		*(float*)(prop + 0x5C) = maxScale;

		// 'm_fCurrRotationRad'
		if (fx)
		{
			*(float*)(fx + 0x90) = restoredKey->random[0];
		}
	}

	// Keep the name of the effect group to recreate it from a save
	if (isPersistent && fx && SavePersistentWorldState)
	{
		const char* groupName = *(const char**)(fxData + 0x74);
		float random = restoredKey ? restoredKey->random[0] : 0.0f;

		// Decal, the random scale is picked here to be saved, 'm_fScaleMin' and 'm_fScaleMax'
		if (type == 0x61636544 && !restoredKey)
		{
			float scaleMin = *(float*)(prop + 0x4C);
			float scaleMax = *(float*)(prop + 0x50);
			random = scaleMin + (scaleMax - scaleMin) * (GameRand() / (float)RAND_MAX);
		}

		persistentFXInfos[fx] = { std::string(groupName, strnlen(groupName, 127)), *(uint32_t*)(fxData + 0x44), prop, type, key, random };
	}

	return fx;
}

// ========================
// SavePersistentWorldState
// ========================

// IShapeTraversalCallback, gives the shape of a debris piece to recreate it
class PersistentShapeTraversal
{
public:
	explicit PersistentShapeTraversal(PersistentRigidBodyRecord& record) : m_record(record) {}
	virtual ~PersistentShapeTraversal() = default;

	virtual void HandleSphere(const float* center, float radius, float mass, float density)
	{
		SetShape(PersistentShape::Sphere, mass, density, IsAtOrigin(center));
		m_record.shapeData[0] = radius;
	}

	virtual void HandleCapsule(const float* point1, const float* point2, float radius, float mass, float density)
	{
		SetShape(PersistentShape::Capsule, mass, density, true);
		memcpy(m_record.shapeData, point1, sizeof(float) * 3);
		memcpy(m_record.shapeData + 3, point2, sizeof(float) * 3);
		m_record.shapeData[6] = radius;
	}

	virtual void HandleOBB(const float* transform, const float* halfDims, float mass, float density)
	{
		SetShape(PersistentShape::Box, mass, density, IsAtOrigin(transform) && fabsf(transform[6]) > 0.9999f);
		memcpy(m_record.shapeData, halfDims, sizeof(float) * 3);
	}

	// Only the single shapes of the debris are recreated
	bool IsValid() const { return m_shapeCount == 1 && m_isAtOrigin; }

private:
	static bool IsAtOrigin(const float* position)
	{
		return fabsf(position[0]) < 0.01f && fabsf(position[1]) < 0.01f && fabsf(position[2]) < 0.01f;
	}

	void SetShape(PersistentShape shape, float mass, float density, bool isAtOrigin)
	{
		m_record.shape = shape;
		m_record.mass = mass;
		m_record.density = density;
		m_isAtOrigin = isAtOrigin;
		m_shapeCount++;
	}

	PersistentRigidBodyRecord& m_record;
	int m_shapeCount = 0;
	bool m_isAtOrigin = false;
};

static int GetSimulationFXMgr()
{
	int pGameClientShell = *(int*)pGameClientShellAddr;
	return pGameClientShell ? pGameClientShell + simulationFXMgrOffset : 0;
}

static DWORD GetPhysicsSim()
{
	DWORD client = EngineInterface::GetClient();
	return client ? EngineInterface::Call<DWORD>(client, EngineInterface::Slot::PhysicsSim) : 0;
}

static bool IsServerObject(DWORD hObject)
{
	// 'm_ObjectID', the objects created by the client have none
	return hObject && *(uint16_t*)(hObject + 0x66) != 0xFFFF;
}

// Used by the ClientFX decal hook
bool GetPersistentDecalScale(int decal, float& scale)
{
	auto it = persistentFXInfos.find(decal);
	if (it == persistentFXInfos.end() || *(int*)(decal + 0x14) != it->second.props)
		return false;

	scale = it->second.random;
	return true;
}

void SetPersistentDecalWorldModel(int decal)
{
	DWORD client = EngineInterface::GetClient();
	auto it = persistentFXInfos.find(decal);
	if (!client || it == persistentFXInfos.end() || *(int*)(decal + 0x14) != it->second.props)
		return;

	// Not attached and only projected on a world model it follows, 'm_nNumWMDecals', 'm_hAttachedWM' and 'm_bDisableFollowWM'
	DWORD hWorldModel = *(DWORD*)(decal + 0x7C + 0xC);
	if (*(DWORD*)(decal + 0x5C) || *(uint32_t*)(decal + 0x1EC) != 1 || !hWorldModel || *(bool*)(decal + 0x7C + 0x54))
		return;

	// Its transform when projected, the decal is saved relative to it
	it->second.worldModel = hWorldModel;
	EngineInterface::Call<int>(client, EngineInterface::Slot::GetObjectPos, hWorldModel, (void*)it->second.worldModelTransform);
	EngineInterface::Call<int>(client, EngineInterface::Slot::GetObjectRotation, hWorldModel, (void*)(it->second.worldModelTransform + 3));
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

static void CapturePersistentShatters()
{
	int pGameClientShell = *(int*)pGameClientShellAddr;
	DWORD database = *(DWORD*)pLTDatabaseAddr;
	if (!pGameClientShell || !database) return;

	// 'm_ShatterList'
	int shatterMgr = pGameClientShell + shatterEffectMgrOffset;
	DWORD* shatterBegin = *(DWORD**)(shatterMgr + 0x4);
	DWORD* shatterEnd = *(DWORD**)(shatterMgr + 0x8);
	std::unordered_map<int, PersistentShatterInfo> liveShatterInfos;

	for (DWORD* it = shatterBegin; it && it != shatterEnd; it++)
	{
		int shatter = *it;
		auto info = persistentShatterInfos.find(shatter);
		if (info == persistentShatterInfos.end()) continue;

		liveShatterInfos.emplace(*info);

		// 'm_hShatterType'
		DWORD hType = *(DWORD*)(shatter + 0x8);
		DWORD hCategory = hType ? EngineInterface::CallOffset<DWORD>(database, EngineInterface::Database_GetRecordParent, hType) : 0;
		const char* categoryName = hCategory ? EngineInterface::CallOffset<const char*>(database, EngineInterface::Database_GetCategoryName, hCategory) : nullptr;
		const char* typeName = hType ? EngineInterface::CallOffset<const char*>(database, EngineInterface::Database_GetRecordName, hType) : nullptr;
		if (!categoryName || !typeName) continue;

		// 'm_fTotalElapsed'
		PersistentShatterRecord record{ categoryName, typeName, info->second, *(float*)shatter };

		// 'm_DebrisList', where each piece is now and how it moves
		uint32_t pieceCount = *(uint32_t*)(shatter + 0x14);
		DWORD* pieces = *(DWORD**)(shatter + 0x1C);
		if (!pieces || pieceCount > (*(DWORD*)(shatter + 0x20) - (DWORD)pieces) / 4) continue;

		record.pieces.resize(pieceCount);
		for (uint32_t i = 0; i < pieceCount; i++)
		{
			memcpy(record.pieces[i].transform, (void*)pieces[i], sizeof(record.pieces[i].transform)); // m_tTransform
			memcpy(record.pieces[i].motion, (void*)(pieces[i] + 0x20), sizeof(record.pieces[i].motion)); // m_fUpdateDelayS, m_vLinearVel, m_vAngularVel
		}

		persistentWorldState.shatters.push_back(std::move(record));
	}

	persistentShatterInfos = std::move(liveShatterInfos);
}

static bool CapturePersistentRigidBody(PersistentRigidBodyRecord& record, DWORD hRigidBody)
{
	DWORD physicsSim = GetPhysicsSim();
	DWORD hShape = 0;
	if (!physicsSim || EngineInterface::CallOffset<int>(physicsSim, EngineInterface::PhysicsSim_GetRigidBodyShape, hRigidBody, &hShape) != 0 || !hShape)
		return false;

	const float identity[7] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
	PersistentShapeTraversal traversal(record);
	EngineInterface::CallOffset<int>(physicsSim, EngineInterface::PhysicsSim_TraverseShapeHierarchy, hShape, (void*)&traversal, identity);
	EngineInterface::CallOffset<int>(physicsSim, EngineInterface::PhysicsSim_ReleaseShape, hShape);
	if (!traversal.IsValid()) return false;

	EngineInterface::CallOffset<int>(physicsSim, EngineInterface::PhysicsSim_GetRigidBodyTransform, hRigidBody, (void*)record.transform);
	EngineInterface::CallOffset<int>(physicsSim, EngineInterface::PhysicsSim_GetRigidBodyCollisionInfo, hRigidBody, &record.group, &record.system);
	EngineInterface::CallOffset<int>(physicsSim, EngineInterface::PhysicsSim_GetRigidBodyFriction, hRigidBody, &record.friction);
	EngineInterface::CallOffset<int>(physicsSim, EngineInterface::PhysicsSim_GetRigidBodyCOR, hRigidBody, &record.cor);
	EngineInterface::CallOffset<int>(physicsSim, EngineInterface::PhysicsSim_GetRigidBodyVelocity, hRigidBody, (void*)record.velocity);
	EngineInterface::CallOffset<int>(physicsSim, EngineInterface::PhysicsSim_GetRigidBodyAngularVelocity, hRigidBody, (void*)record.angularVelocity);
	return true;
}

static void CapturePersistentDecalWorldModel(PersistentFXRecord& record, int fx, const PersistentFXInfo& info)
{
	DWORD client = EngineInterface::GetClient();
	DWORD physics = *(DWORD*)pPhysicsLTAddr;

	// Still on the world model it was projected on, 'm_hAttachedWM'
	if (!info.worldModel || *(DWORD*)(fx + 0x7C + 0xC) != info.worldModel || !IsServerObject(info.worldModel))
		return;

	// Relative to the world model when it was projected, 'm_tParentTransform'
	const float* transform = (const float*)(fx + 0x24);
	const float* worldModelTransform = info.worldModelTransform;
	EngineInterface::Rotation inverse = { -worldModelTransform[3], -worldModelTransform[4], -worldModelTransform[5], worldModelTransform[6] };
	EngineInterface::Vector3 pos = inverse.Rotate({ transform[0] - worldModelTransform[0], transform[1] - worldModelTransform[1], transform[2] - worldModelTransform[2] });
	EngineInterface::Rotation rot = inverse * EngineInterface::Rotation{ transform[3], transform[4], transform[5], transform[6] };

	float offset[7] = { pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, rot.w };
	memcpy(record.offset, offset, sizeof(record.offset));

	EngineInterface::Call<int>(client, EngineInterface::Slot::GetObjectPos, info.worldModel, (void*)record.parentPos);
	EngineInterface::CallOffset<int>(physics, EngineInterface::Physics_GetObjectDims, info.worldModel, (void*)record.parentDims);
	record.socket = (uint32_t)-1;
	record.parent = PersistentFXParent::Object;
}

static bool CapturePersistentFXParent(PersistentFXRecord& record, int fx, const PersistentFXInfo& info, DWORD hPlayer)
{
	DWORD client = EngineInterface::GetClient();
	DWORD physics = *(DWORD*)pPhysicsLTAddr;
	DWORD hParent = *(DWORD*)(fx + 0x5C);
	DWORD hRigidBody = *(DWORD*)(fx + 0x60);
	int hNode = *(int*)(fx + 0x64);

	// 'm_tParentOffset'
	memcpy(record.offset, (void*)(fx + 0x40), sizeof(record.offset));
	record.parent = PersistentFXParent::None;

	if (!client || !physics)
		return true;

	// Debris simulated by the client, without its physics it's only left at its current transform
	if (hRigidBody)
	{
		// The effects on the same debris share its body
		record.rigidBody.body = hRigidBody;

		if (CapturePersistentRigidBody(record.rigidBody, hRigidBody))
		{
			record.parent = PersistentFXParent::RigidBody;
		}

		return true;
	}

	// Not attached, a decal can still follow the world model it was projected on
	if (!hParent)
	{
		CapturePersistentDecalWorldModel(record, fx, info);
		return true;
	}

	// Attached to the world
	if (EngineInterface::CallOffset<int>(physics, EngineInterface::Physics_IsWorldObject, hParent) == 86)
		return true;

	// Skip if attached to the player or to an object of the client, recreated by its owner
	if (hParent == hPlayer || !IsServerObject(hParent))
		return false;

	// Fixed, it stayed where it was created when its parent moved, 'm_eFollowType'
	// A decal still follows the world model it's projected on, 'm_hAttachedWM' and 'm_bDisableFollowWM'
	if (*(uint8_t*)(info.props + 0x14) == 0 && (info.type != 0x61636544 || *(DWORD*)(fx + 0x7C + 0xC) != hParent || *(bool*)(fx + 0x7C + 0x54)))
		return true;

	// Attached to a model node, the node is found by its name
	if (hNode != -1)
	{
		char nodeName[128] = {};
		DWORD modelLT = EngineInterface::Call<DWORD>(client, EngineInterface::Slot::GetModelLT);

		// Left where it is without it
		if (!modelLT || EngineInterface::CallOffset<int>(modelLT, EngineInterface::Model_GetNodeName, hParent, hNode, nodeName, (uint32_t)sizeof(nodeName)) != 0 || !nodeName[0])
			return true;

		record.node = nodeName;
		record.parent = PersistentFXParent::Node;
	}
	else
	{
		record.socket = *(uint32_t*)(fx + 0x68);
		record.parent = PersistentFXParent::Object;
	}

	// Found again by its position and dims once loaded, like the model decals
	EngineInterface::Call<int>(client, EngineInterface::Slot::GetObjectPos, hParent, (void*)record.parentPos);
	EngineInterface::CallOffset<int>(physics, EngineInterface::Physics_GetObjectDims, hParent, (void*)record.parentDims);
	return true;
}

// The parts of an effect group can be attached differently, the ones attached the same way are saved together
static bool IsSamePersistentFXParent(const PersistentFXRecord& record, const PersistentFXRecord& other)
{
	if (record.parent != other.parent || memcmp(record.offset, other.offset, sizeof(record.offset)) != 0)
		return false;

	switch (record.parent)
	{
		case PersistentFXParent::None:
			return memcmp(record.transform, other.transform, sizeof(record.transform)) == 0;

		case PersistentFXParent::Object:
		case PersistentFXParent::Node:
			return record.socket == other.socket && record.node == other.node
				&& memcmp(record.parentPos, other.parentPos, sizeof(record.parentPos)) == 0
				&& memcmp(record.parentDims, other.parentDims, sizeof(record.parentDims)) == 0;

		case PersistentFXParent::RigidBody:
			return record.rigidBody.body == other.rigidBody.body;
	}

	return false;
}

static void CapturePersistentWorldState()
{
	persistentWorldState = {};

	CapturePersistentShellCasings();
	CapturePersistentShatters();

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

			std::vector<PersistentFXRecord> records;
			int fxListHead = instance + 0x4;

			for (int fxLink = *(int*)(fxListHead + 0x4); fxLink != fxListHead; fxLink = *(int*)(fxLink + 0x4))
			{
				int fx = *(int*)(fxLink + 0x8);
				auto it = persistentFXInfos.find(fx);

				// Skip if not persistent or shutting down
				if (it == persistentFXInfos.end() || *(int*)(fx + 0x14) != it->second.props || (*(DWORD*)(fx + 0x18) & 0x4)) continue;

				liveFXInfos.emplace(fx, it->second);

				// At the current world transform of the effect
				PersistentFXRecord record{ it->second.name, it->second.flags };
				memcpy(record.transform, (void*)(fx + 0x24), sizeof(record.transform));
				if (!CapturePersistentFXParent(record, fx, it->second, hPlayer)) continue;

				auto group = std::find_if(records.begin(), records.end(), [&record](const PersistentFXRecord& other) { return IsSamePersistentFXParent(record, other); });
				if (group == records.end())
				{
					group = records.insert(records.end(), std::move(record));
				}

				// Each part still in the world, the sprites keep their 'm_fCurrRotationRad' and 'm_fScale'
				bool isSprite = it->second.type == 0x69727053;
				PersistentFXKeyRecord key{ (uint16_t)it->second.key, { isSprite ? *(float*)(fx + 0x90) : it->second.random, isSprite ? *(float*)(fx + 0x98) : 0.0f } };

				if (group->keys.size() < 255)
				{
					group->keys.push_back(key);
				}
			}

			// Save the group once for each way its parts are attached
			for (auto& record : records)
			{
				persistentWorldState.fx.push_back(std::move(record));
			}
		}
	}

	persistentFXInfos = std::move(liveFXInfos);

	// Not restored yet, still waiting for their object
	persistentWorldState.fx.insert(persistentWorldState.fx.begin(), pendingFX.begin(), pendingFX.end());

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

static bool IsFinite(const float* values, size_t count)
{
	return std::all_of(values, values + count, [](float value) { return std::isfinite(value); });
}

static bool ReadFloats(std::ifstream& file, float* values, size_t count)
{
	return file.read((char*)values, sizeof(float) * count) && IsFinite(values, count);
}

static bool ReadTransform(std::ifstream& file, float* transform)
{
	return ReadFloats(file, transform, 7);
}

static bool ReadPersistentFXParent(std::ifstream& file, PersistentFXRecord& record)
{
	// The values after the shape and the collision info are all floats
	size_t rigidBodyFloats = (sizeof(PersistentRigidBodyRecord) - offsetof(PersistentRigidBodyRecord, shapeData)) / sizeof(float);

	switch (record.parent)
	{
		case PersistentFXParent::None:
			return true;

		case PersistentFXParent::Object:
			return ReadFloats(file, record.parentPos, 3) && ReadFloats(file, record.parentDims, 3) && ReadValue(file, record.socket) && ReadTransform(file, record.offset);

		case PersistentFXParent::Node:
			return ReadFloats(file, record.parentPos, 3) && ReadFloats(file, record.parentDims, 3) && ReadString(file, record.node) && ReadTransform(file, record.offset);

		case PersistentFXParent::RigidBody:
			return ReadValue(file, record.rigidBody) && record.rigidBody.shape <= PersistentShape::Capsule && IsFinite(record.rigidBody.shapeData, rigidBodyFloats) && ReadTransform(file, record.offset);
	}

	return false;
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
		WriteValue(file, (uint8_t)record.keys.size());

		for (const auto& key : record.keys)
		{
			WriteValue(file, key.key);
			file.write((char*)key.random, sizeof(key.random));
		}

		WriteValue(file, record.parent);

		if (record.parent == PersistentFXParent::Object || record.parent == PersistentFXParent::Node)
		{
			file.write((char*)record.parentPos, sizeof(record.parentPos));
			file.write((char*)record.parentDims, sizeof(record.parentDims));

			if (record.parent == PersistentFXParent::Node)
				WriteString(file, record.node);
			else
				WriteValue(file, record.socket);
		}
		else if (record.parent == PersistentFXParent::RigidBody)
		{
			WriteValue(file, record.rigidBody);
		}

		if (record.parent != PersistentFXParent::None)
		{
			file.write((char*)record.offset, sizeof(record.offset));
		}
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

	WriteValue(file, (uint32_t)persistentWorldState.decalMaterials.size());
	file.write((char*)persistentWorldState.decalMaterials.data(), persistentWorldState.decalMaterials.size());

	WriteValue(file, (uint32_t)persistentWorldState.shatters.size());

	for (const auto& record : persistentWorldState.shatters)
	{
		WriteString(file, record.category);
		WriteString(file, record.type);
		WriteValue(file, record.info);
		WriteValue(file, record.elapsed);
		WriteValue(file, (uint32_t)record.pieces.size());
		file.write((char*)record.pieces.data(), record.pieces.size() * sizeof(PersistentShatterPieceRecord));
	}

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
		uint8_t keyCount = 0;

		if (!ReadString(file, record.name) || record.name.size() > 127 || !ReadValue(file, record.flags) || !ReadTransform(file, record.transform) || !ReadValue(file, keyCount))
			return false;

		record.keys.resize(keyCount);

		for (auto& key : record.keys)
		{
			if (!ReadValue(file, key.key) || !ReadFloats(file, key.random, 2))
				return false;
		}

		if (!ReadValue(file, record.parent) || !ReadPersistentFXParent(file, record))
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
	if (!state.decalData.empty() && !file.read((char*)state.decalData.data(), state.decalData.size()))
		return false;

	uint32_t decalMaterialCount = 0;
	if (!ReadValue(file, decalMaterialCount) || decalMaterialCount > 0x100000)
		return false;

	state.decalMaterials.resize(decalMaterialCount);
	if (!state.decalMaterials.empty() && !file.read((char*)state.decalMaterials.data(), state.decalMaterials.size()))
		return false;

	uint32_t shatterCount = 0;
	if (!ReadValue(file, shatterCount) || shatterCount > 0xFFFF)
		return false;

	for (uint32_t i = 0; i < shatterCount; i++)
	{
		PersistentShatterRecord record{};
		uint32_t pieceCount = 0;

		if (!ReadString(file, record.category) || !ReadString(file, record.type) || !ReadValue(file, record.info) || !IsFinite(record.info.transform, 13) || !ReadFloats(file, &record.elapsed, 1) || !ReadValue(file, pieceCount) || pieceCount > 0xFFFF)
			return false;

		record.pieces.resize(pieceCount);
		if (!record.pieces.empty() && !ReadFloats(file, (float*)record.pieces.data(), record.pieces.size() * 14))
			return false;

		state.shatters.push_back(std::move(record));
	}

	return true;
}

static DWORD CreatePersistentRigidBody(const PersistentRigidBodyRecord& record, std::unordered_map<uint32_t, uint32_t>& systems)
{
	DWORD physicsSim = GetPhysicsSim();
	if (!physicsSim) return 0;

	DWORD hShape = 0;

	switch (record.shape)
	{
		case PersistentShape::Sphere:
			hShape = EngineInterface::CallOffset<DWORD>(physicsSim, EngineInterface::PhysicsSim_CreateSphereShape, record.shapeData[0], record.mass, record.density);
			break;

		case PersistentShape::Box:
			hShape = EngineInterface::CallOffset<DWORD>(physicsSim, EngineInterface::PhysicsSim_CreateBoxShape, (const float*)record.shapeData, record.mass, record.density, 0.0f);
			break;

		case PersistentShape::Capsule:
			hShape = EngineInterface::CallOffset<DWORD>(physicsSim, EngineInterface::PhysicsSim_CreateCapsuleShape, (const float*)record.shapeData, (const float*)(record.shapeData + 3), record.shapeData[6], record.mass, record.density);
			break;
	}

	if (!hShape) return 0;

	// The pieces of the same debris don't collide with each other
	uint32_t system = record.system;
	if (system)
	{
		auto it = systems.find(system);
		if (it == systems.end())
		{
			uint32_t uniqueSystem = 0;
			EngineInterface::CallOffset<int>(physicsSim, EngineInterface::PhysicsSim_GetUniqueSystemGroup, &uniqueSystem);
			it = systems.emplace(system, uniqueSystem).first;
		}

		system = it->second;
	}

	DWORD hRigidBody = EngineInterface::CallOffset<DWORD>(physicsSim, EngineInterface::PhysicsSim_CreateRigidBody, hShape, (const float*)record.transform, false, record.group, system, record.friction, record.cor);
	EngineInterface::CallOffset<int>(physicsSim, EngineInterface::PhysicsSim_ReleaseShape, hShape);

	if (hRigidBody)
	{
		EngineInterface::CallOffset<int>(physicsSim, EngineInterface::PhysicsSim_SetRigidBodyVelocity, hRigidBody, (const float*)record.velocity);
		EngineInterface::CallOffset<int>(physicsSim, EngineInterface::PhysicsSim_SetRigidBodyAngularVelocity, hRigidBody, (const float*)record.angularVelocity);
	}

	return hRigidBody;
}

static void CreatePersistentFX(const PersistentFXRecord& record, DWORD hParent, int hNode, DWORD hRigidBody)
{
	int fxMgr = GetSimulationFXMgr();
	if (!fxMgr) return;

	// CLIENTFX_CREATESTRUCT
	alignas(4) uint8_t createStruct[0xC4];

	memset(createStruct, 0, sizeof(createStruct));
	memcpy(createStruct, record.name.data(), record.name.size()); // m_sName
	*(uint32_t*)(createStruct + 0x80) = record.flags; // m_dwFlags
	*(DWORD*)(createStruct + 0x84) = hParent; // m_hParentObject
	*(DWORD*)(createStruct + 0x88) = hRigidBody; // m_hParentRigidBody
	*(int*)(createStruct + 0x8C) = hNode; // m_hNode
	*(int*)(createStruct + 0x90) = hParent && record.parent == PersistentFXParent::Object ? (int)record.socket : -1; // m_hSocket
	memcpy(createStruct + 0x94, hParent || hRigidBody ? record.offset : record.transform, sizeof(record.transform)); // m_tTransform

	isRestoringWorldState = true;
	restoringFXRecord = &record;
	restoringCreateStruct = createStruct;

	CreateClientFX(fxMgr, 0, createStruct, true, false);

	isRestoringWorldState = false;
	restoringFXRecord = nullptr;
	restoringCreateStruct = nullptr;
}

struct PersistentFXParentSearch
{
	const PersistentFXRecord* record;
	DWORD hPlayer;
	DWORD hResult;
	int hNode;
	float distance;
};

static void FindPersistentFXParentCB(DWORD hObject, void* pUserData)
{
	PersistentFXParentSearch* search = (PersistentFXParentSearch*)pUserData;
	const PersistentFXRecord& record = *search->record;
	DWORD client = EngineInterface::GetClient();
	DWORD physics = *(DWORD*)pPhysicsLTAddr;
	if (hObject == search->hPlayer || !IsServerObject(hObject) || !client || !physics) return;

	float pos[3] = {}, dims[3] = {};
	EngineInterface::Call<int>(client, EngineInterface::Slot::GetObjectPos, hObject, (void*)pos);
	EngineInterface::CallOffset<int>(physics, EngineInterface::Physics_GetObjectDims, hObject, (void*)dims);

	// About the same position and dims, a bit more lenient than the model decals for the objects still moving when saving, the closest one is used
	float distance = 0.0f;
	for (int i = 0; i < 3; i++)
	{
		if (fabsf(dims[i] - record.parentDims[i]) > 4.0f || fabsf(pos[i] - record.parentPos[i]) > 4.0f) return;

		distance += (pos[i] - record.parentPos[i]) * (pos[i] - record.parentPos[i]);
	}

	if (search->hResult && distance >= search->distance) return;

	int hNode = -1;
	if (record.parent == PersistentFXParent::Node)
	{
		DWORD modelLT = EngineInterface::Call<DWORD>(client, EngineInterface::Slot::GetModelLT);
		if (!modelLT || EngineInterface::CallOffset<int>(modelLT, EngineInterface::Model_GetNode, hObject, record.node.c_str(), &hNode) != 0) return;
	}

	*search = { search->record, search->hPlayer, hObject, hNode, distance };
}

static void RestorePersistentFX()
{
	DWORD client = EngineInterface::GetClient();
	DWORD hPlayer = client ? EngineInterface::CallMember<DWORD>(client, EngineInterface::Client_GetClientObject) : 0;
	DWORD physicsSim = GetPhysicsSim();

	// Only while in the world
	if (!hPlayer) return;

	// The objects still not found after a few seconds are gone
	if (!pendingFXTime)
	{
		pendingFXTime = GetTickCount64();
	}

	bool isLastTry = GetTickCount64() - pendingFXTime > 3000;
	std::unordered_map<uint32_t, DWORD> rigidBodies;
	std::unordered_map<uint32_t, uint32_t> systems;
	std::vector<PersistentFXRecord> notFoundFX;

	for (const auto& record : pendingFX)
	{
		if (record.parent == PersistentFXParent::None)
		{
			CreatePersistentFX(record, 0, -1, 0);
			continue;
		}

		// Debris, its body is created once for all its effects
		if (record.parent == PersistentFXParent::RigidBody)
		{
			auto it = rigidBodies.find(record.rigidBody.body);
			if (it == rigidBodies.end())
			{
				it = rigidBodies.emplace(record.rigidBody.body, CreatePersistentRigidBody(record.rigidBody, systems)).first;
			}

			CreatePersistentFX(record, 0, -1, it->second);
			continue;
		}

		PersistentFXParentSearch search{ &record, hPlayer, 0, -1, 0.0f };
		float halfDims[3] = { record.parentDims[0] + 4.0f, record.parentDims[1] + 4.0f, record.parentDims[2] + 4.0f };

		EngineInterface::CallOffset<int>(client, findObjectsInBoxOffset, (const float*)record.parentPos, (const float*)halfDims, &FindPersistentFXParentCB, (void*)&search);

		if (search.hResult)
		{
			CreatePersistentFX(record, search.hResult, search.hNode, 0);
		}
		else if (isLastTry)
		{
			// The object is gone, left where it was
			CreatePersistentFX(record, 0, -1, 0);
		}
		else
		{
			notFoundFX.push_back(record);
		}
	}

	// The effects keep their own reference
	for (const auto& [body, hRigidBody] : rigidBodies)
	{
		if (hRigidBody)
		{
			EngineInterface::CallOffset<int>(physicsSim, EngineInterface::PhysicsSim_ReleaseRigidBody, hRigidBody);
		}
	}

	pendingFX = std::move(notFoundFX);
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

// Replaces the rand of the game while a shatter effect is created, same algorithm as the CRT
static int __cdecl ShatterRand()
{
	shatterRandState = shatterRandState * 214013 + 2531011;
	return (shatterRandState >> 16) & 0x7FFF;
}

static bool CreateSeededShatterEffect(int shatterMgr, const PersistentShatterInfo& info, DWORD hShatterType)
{
	// The pieces are randomly cut, the same seed gives back the same pieces
	DWORD gameRand = *(DWORD*)randImportAddr;
	shatterRandState = info.seed;
	MemoryHelper::WriteMemory<DWORD>(randImportAddr, (DWORD)&ShatterRand);

	bool result = CreateShatterEffect(shatterMgr, info.dataId, info.transform, info.hitPos, info.hitDir, hShatterType);

	MemoryHelper::WriteMemory<DWORD>(randImportAddr, gameRand);

	// Added at the end of 'm_ShatterList'
	if (result)
	{
		persistentShatterInfos[*(*(int**)(shatterMgr + 0x8) - 1)] = info;
	}

	return result;
}

static void RestorePersistentShatters(const std::vector<PersistentShatterRecord>& records)
{
	int pGameClientShell = *(int*)pGameClientShellAddr;
	DWORD database = *(DWORD*)pLTDatabaseAddr;
	int decalCategory = GetModelDecalCategory();
	if (!pGameClientShell || !database || !decalCategory || records.empty()) return;

	// Same database as the model decals
	DWORD hDatabase = EngineInterface::CallOffset<DWORD>(database, EngineInterface::Database_GetCategoryParent, *(DWORD*)(decalCategory + 0x24));
	if (!hDatabase) return;

	int shatterMgr = pGameClientShell + shatterEffectMgrOffset;

	for (const auto& record : records)
	{
		DWORD hCategory = EngineInterface::CallOffset<DWORD>(database, EngineInterface::Database_GetCategory, hDatabase, record.category.c_str());
		DWORD hType = hCategory ? EngineInterface::CallOffset<DWORD>(database, EngineInterface::Database_GetRecord, hCategory, record.type.c_str()) : 0;
		if (!hType || !CreateSeededShatterEffect(shatterMgr, record.info, hType)) continue;

		int shatter = *(*(int**)(shatterMgr + 0x8) - 1);
		uint32_t pieceCount = *(uint32_t*)(shatter + 0x14);
		DWORD* pieces = *(DWORD**)(shatter + 0x1C);
		if (!pieces || pieceCount != record.pieces.size()) continue;

		// Put the pieces back where they were, 'm_fTotalElapsed' and 'm_DebrisList'
		*(float*)shatter = record.elapsed;

		for (uint32_t i = 0; i < pieceCount; i++)
		{
			memcpy((void*)pieces[i], record.pieces[i].transform, sizeof(record.pieces[i].transform));
			memcpy((void*)(pieces[i] + 0x20), record.pieces[i].motion, sizeof(record.pieces[i].motion));
		}
	}
}

static void RestorePersistentWorldState()
{
	std::string savePath = std::move(g_State.loadedSaveFilePath);
	g_State.loadedSaveFilePath.clear();
	pendingFX.clear();
	pendingFXTime = 0;
	restoredDecalMaterials.clear();

	PersistentWorldState state;
	if (!savePath.ends_with(".sav") || !ReadPersistentWorldStateFile(savePath, state)) return;

	// Restored on the next update, once the objects are back in the world, like the model decals
	pendingFX = std::move(state.fx);
	RestorePersistentShellCasings(state.shellCasings);
	RestorePersistentDecals(state.decalData, state.decalBits);
	RestorePersistentShatters(state.shatters);
	restoredDecalMaterials = std::move(state.decalMaterials);
}

static void __fastcall BuildClientSaveMsg_Hook(int thisPtr, int, int* pMsg, int eSaveDataState)
{
	// Written next to the save file once the server is done with it
	CapturePersistentWorldState();
	BuildClientSaveMsg(thisPtr, pMsg, eSaveDataState);
}

static void __fastcall ModelDecalMgrAddDecal_Hook(int thisPtr, int, DWORD hObject, uint32_t nDecalType, void* pProjection)
{
	DWORD typesBegin = *(DWORD*)(thisPtr + 0x8);
	DWORD typesEnd = *(DWORD*)(thisPtr + 0xC);

	// 'm_aMaterials' of the decal type
	DWORD* pMaterialsBegin = typesBegin && nDecalType < (typesEnd - typesBegin) / 0x28 ? (DWORD*)(typesBegin + nDecalType * 0x28 + 0x4) : nullptr;
	DWORD materialsBegin = pMaterialsBegin ? pMaterialsBegin[0] : 0;
	DWORD materialsEnd = pMaterialsBegin ? pMaterialsBegin[1] : 0;
	uint32_t materialCount = materialsBegin ? (materialsEnd - materialsBegin) / 4 : 0;

	if (materialCount == 0)
	{
		ModelDecalMgrAddDecal(thisPtr, hObject, nDecalType, pProjection);
		return;
	}

	uint32_t material = GameRand() % materialCount;

	// Loaded from a save, 'm_aLoadRecords' are in the same order as the saved decals
	DWORD recordsBegin = *(DWORD*)(thisPtr + 0x38);
	DWORD record = (DWORD)pProjection - 0x1C;

	if (recordsBegin && record >= recordsBegin && record < *(DWORD*)(thisPtr + 0x3C))
	{
		uint32_t index = (record - recordsBegin) / 0x4C;
		if (index < restoredDecalMaterials.size() && restoredDecalMaterials[index] < materialCount)
		{
			material = restoredDecalMaterials[index];
		}
	}

	// Only leave the material to use, it's then saved with the decal
	DWORD decalCount = (*(DWORD*)(thisPtr + 0x1C) - *(DWORD*)(thisPtr + 0x18)) / 0x28;
	pMaterialsBegin[0] = materialsBegin + material * 4;
	pMaterialsBegin[1] = pMaterialsBegin[0] + 4;

	ModelDecalMgrAddDecal(thisPtr, hObject, nDecalType, pProjection);

	pMaterialsBegin[0] = materialsBegin;
	pMaterialsBegin[1] = materialsEnd;

	// 'm_hDecal' of the added decal
	if ((*(DWORD*)(thisPtr + 0x1C) - *(DWORD*)(thisPtr + 0x18)) / 0x28 > decalCount)
	{
		persistentDecalMaterials[*(DWORD*)(*(DWORD*)(thisPtr + 0x1C) - 0x28 + 0x14)] = (uint8_t)material;
	}
}

static void __fastcall ModelDecalMgrUpdate_Hook(int thisPtr, int)
{
	// The decals loaded from the save are added here
	ModelDecalMgrUpdate(thisPtr);
	restoredDecalMaterials.clear();

	// The effects too, retried until the objects they are attached to are back in the world
	if (!pendingFX.empty())
	{
		RestorePersistentFX();
	}
}

static bool __fastcall CreateShatterEffect_Hook(int thisPtr, int, uint32_t nShatterDataID, const float* tObjTransform, const float* vHitPos, const float* vHitDir, DWORD hShatterType)
{
	// Everything needed to shatter it again from a save
	PersistentShatterInfo info{ ((uint32_t)GameRand() << 16) ^ (uint32_t)GameRand(), nShatterDataID };
	memcpy(info.transform, tObjTransform, sizeof(info.transform));
	memcpy(info.hitPos, vHitPos, sizeof(info.hitPos));
	memcpy(info.hitDir, vHitDir, sizeof(info.hitDir));

	return CreateSeededShatterEffect(thisPtr, info, hShatterType);
}
