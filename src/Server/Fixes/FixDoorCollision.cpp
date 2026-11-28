#pragma once

#include "../../Globals.cpp"

static int doorCollisionTypeOffset = 0;
static int doorCollisionActivatorOffset = 0;
static int doorCollisionGetObjectPosOffset = 0;
static DWORD doorCollisionLTServerAddr = 0;

bool(__thiscall* AWMTestObjectCollision)(int, int, const float*, const float*, int*) = nullptr;

// ================
// FixDoorCollision
// ================

static uintptr_t DoorCollisionLTServerMethod(int ltServer, int nOffset)
{
    return *reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(ltServer) + nOffset);
}

static bool DoorCollisionGetObjectPos(int hObject, float* pPos)
{
    using GetObjectPos_t = int(__thiscall*)(int, int, float*);
    int ltServer = MemoryHelper::ReadMemory<int>(doorCollisionLTServerAddr);
    return reinterpret_cast<GetObjectPos_t>(DoorCollisionLTServerMethod(ltServer, doorCollisionGetObjectPosOffset))(ltServer, hObject, pPos) == 0;
}

// SetObjectPos follows GetObjectPos in ILTCSBase, it teleports the object
static void DoorCollisionSetObjectPos(int hObject, const float* pPos)
{
    using SetObjectPos_t = int(__thiscall*)(int, int, const float*);
    int ltServer = MemoryHelper::ReadMemory<int>(doorCollisionLTServerAddr);
    reinterpret_cast<SetObjectPos_t>(DoorCollisionLTServerMethod(ltServer, doorCollisionGetObjectPosOffset + 4))(ltServer, hObject, pPos);
}

static float DoorCollisionDist(const float* a, const float* b)
{
    float x = a[0] - b[0], y = a[1] - b[1], z = a[2] - b[2];
    return sqrtf(x * x + y * y + z * z);
}

// Replaces the call to TestObjectCollision in ActiveWorldModel::CalculateNewPosRot, which tests every object (hTest is NULL) and passes no collision object
static bool __fastcall CalcNewPosRotTestCollision_Hook(int thisPtr, int, int hTest, const float* pTestPos, const float* pLocalRot, int*)
{
    uint8_t nType = *reinterpret_cast<uint8_t*>(thisPtr + doorCollisionTypeOffset);

    // The step is tested with the local swing only, a door placed with a rotation is tested at the wrong orientation
    // Use rOutRot instead, its reference is the argument at [esp+110h] of the caller, 0xD0 past rRotation
    const float* pTestRot = pLocalRot;
    if (nType == AWMTypeSliding || nType == AWMTypeRotating)
    {
        pTestRot = *reinterpret_cast<const float* const*>(reinterpret_cast<uintptr_t>(pLocalRot) + 0xD0);
    }

    // A real movement step passes one of the door's own positions (m_vOnPos, m_vOffPos, m_vMoveToPos) as vFinalPos, at [esp+114h] of the caller
    // SetPowerOn and GetTestPosRot also call CalculateNewPosRot, only to preview poses, and pass a temporary
    uintptr_t pFinalPos = *reinterpret_cast<const uintptr_t*>(reinterpret_cast<uintptr_t>(pLocalRot) + 0xD4);
    if (nType != AWMTypeRotating || pFinalPos < static_cast<uintptr_t>(thisPtr) || pFinalPos >= static_cast<uintptr_t>(thisPtr) + 0x400)
        return AWMTestObjectCollision(thisPtr, hTest, pTestPos, pTestRot, nullptr);

    int hDoor = *reinterpret_cast<int*>(thisPtr + 8);
    float vDoorPos[3];
    if (!DoorCollisionGetObjectPos(hDoor, vDoorPos))
        return AWMTestObjectCollision(thisPtr, hTest, pTestPos, pTestRot, nullptr);

    // When a door pushes into a player, the engine can leave the door at the player's position, off its hinge
    // Every later move of the door then fails and the player is stuck in it, so put it back on its path
    // A rotating door always keeps the same distance to its rotation point
    const float* vRotationPt = reinterpret_cast<const float*>(thisPtr + AWMRotationPtOffset);
    if (std::fabs(DoorCollisionDist(vDoorPos, vRotationPt) - DoorCollisionDist(pTestPos, vRotationPt)) > 1.0f)
    {
        DoorCollisionSetObjectPos(hDoor, pTestPos);
        return false;
    }

    // A rotating door touching its player activator is blocked when the step brings it closer, a blocked door retries the same step
    // At the start of a move that step is zero, so float rounding can keep the door shut until the player backs away
    // A step that doesn't move the door can't push the player
    int hPlayer = 0;
    bool bBlocked = AWMTestObjectCollision(thisPtr, hTest, pTestPos, pTestRot, &hPlayer);
    if (bBlocked && hPlayer && hPlayer == *reinterpret_cast<int*>(thisPtr + doorCollisionActivatorOffset) && DoorCollisionDist(vDoorPos, pTestPos) < 0.01f)
        return false;

    return bBlocked;
}

static void ApplyFixDoorCollisionServerPatch()
{
    if (!FixDoorCollision || g_State.CurrentFEARGame == FEARMP) return;

    DWORD addr_TestCollisionCall = ScanModuleSignature(g_State.GameServer, "6A 00 8D 44 24 44 50 ?? 6A 00 8B ?? E8 ?? ?? ?? ?? 8A 8C 24 20 01 00 00", "FixDoorCollision_TestCollisionCall");
    DWORD addr_ActivatorCheck = ScanModuleSignature(g_State.GameServer, "80 BF ?? ?? 00 00 02 0F 85 ?? ?? 00 00 85 ED 75 0C 3B B7 ?? ?? 00 00 0F 85 ?? ?? 00 00 8B 0D ?? ?? ?? ?? 8B 01 8D 54 24 1C 52 56 FF 50 ??", "FixDoorCollision_ActivatorCheck");

    if (addr_TestCollisionCall == 0 ||
        addr_ActivatorCheck == 0)
        return;

    doorCollisionTypeOffset = MemoryHelper::ReadMemory<int>(addr_ActivatorCheck + 0x2);
    doorCollisionActivatorOffset = MemoryHelper::ReadMemory<int>(addr_ActivatorCheck + 0x13);
    doorCollisionLTServerAddr = MemoryHelper::ReadMemory<DWORD>(addr_ActivatorCheck + 0x1F);
    doorCollisionGetObjectPosOffset = MemoryHelper::ReadMemory<uint8_t>(addr_ActivatorCheck + 0x2D);
    AWMTestObjectCollision = reinterpret_cast<decltype(AWMTestObjectCollision)>(MemoryHelper::ResolveRelativeAddress(addr_TestCollisionCall, 0xD));

    MemoryHelper::MakeCALL(addr_TestCollisionCall + 0xC, reinterpret_cast<uintptr_t>(&CalcNewPosRotTestCollision_Hook));
}
