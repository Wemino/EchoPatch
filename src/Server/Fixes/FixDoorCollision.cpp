#pragma once

#include "../../Globals.cpp"

// ActiveWorldModel
static constexpr int DoorCollisionRotationPtOffset = 0x288;
static constexpr uint8_t DoorCollisionTypeSliding = 1;
static constexpr uint8_t DoorCollisionTypeRotating = 2;

static int doorCollisionTypeOffset = 0;
static int doorCollisionActivatorOffset = 0;
static int doorCollisionGetObjectPosOffset = 0;
static DWORD doorCollisionLTServerAddr = 0;

bool(__thiscall* AWMTestObjectCollision)(int, int, const float*, const float*, int*) = nullptr;

// ==================
// FixDoorCollision
// ==================

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

// SetObjectPos follows GetObjectPos in ILTCSBase
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

static bool __fastcall CalcNewPosRotTestCollision_Hook(int thisPtr, int, int hTest, const float* pTestPos, const float* pLocalRot, int*)
{
    uint8_t nType = *reinterpret_cast<uint8_t*>(thisPtr + doorCollisionTypeOffset);

    // The step is tested with the local swing only, rOutRot is the argument 0xD0 past it in the caller's frame
    const float* pTestRot = pLocalRot;
    if (nType == DoorCollisionTypeSliding || nType == DoorCollisionTypeRotating)
        pTestRot = *reinterpret_cast<const float* const*>(reinterpret_cast<uintptr_t>(pLocalRot) + 0xD0);

    // A real step moves to a position of the door itself, SetPowerOn and GetTestPosRot only preview with a temporary
    uintptr_t pFinalPos = *reinterpret_cast<const uintptr_t*>(reinterpret_cast<uintptr_t>(pLocalRot) + 0xD4);
    if (nType != DoorCollisionTypeRotating || pFinalPos < static_cast<uintptr_t>(thisPtr) || pFinalPos >= static_cast<uintptr_t>(thisPtr) + 0x400)
        return AWMTestObjectCollision(thisPtr, hTest, pTestPos, pTestRot, nullptr);

    int hDoor = *reinterpret_cast<int*>(thisPtr + 8);
    float vDoorPos[3];
    if (!DoorCollisionGetObjectPos(hDoor, vDoorPos))
        return AWMTestObjectCollision(thisPtr, hTest, pTestPos, pTestRot, nullptr);

    // A door pushing into a player can be left off its path and then fails every move, a rotating door keeps the same distance to its rotation point
    const float* vRotationPt = reinterpret_cast<const float*>(thisPtr + DoorCollisionRotationPtOffset);
    if (std::fabs(DoorCollisionDist(vDoorPos, vRotationPt) - DoorCollisionDist(pTestPos, vRotationPt)) > 1.0f)
    {
        DoorCollisionSetObjectPos(hDoor, pTestPos);
        return false;
    }

    // A blocked door retries a zero step, which float rounding can count as moving closer to its activator
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
