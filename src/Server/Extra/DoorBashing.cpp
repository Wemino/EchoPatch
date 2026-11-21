#pragma once

#include "../../Globals.cpp"

// ActiveWorldModel
static constexpr int AWMDamageAggregateOffset = 0x20;
static constexpr int AWMTypeOffset = 0x1AC;
static constexpr int AWMCurStateOffset = 0x309;
static constexpr int AWMWaveformOffset = 0x30B;
static constexpr uint8_t AWMTypeRotating = 2;
static constexpr uint8_t AWMStatePowerOn = 2;
static constexpr uint8_t AWMStateOff = 3;
static constexpr uint8_t AWMStatePowerOff = 4;
static constexpr uint8_t AWMWaveBounce = 4;

// CDestructible, and DamageStruct
static constexpr int DestructibleObjectOffset = 0x18;
static constexpr int DestructibleLastDamageOffset = 0x34;
static constexpr int DestructibleLastDamageDirOffset = 0x54;
static constexpr int DamageTypeOffset = 0x4;
static constexpr int DamageDamagerOffset = 0x14;
static constexpr int DamageAmountOffset = 0x2C;
static constexpr int DamageTypeExplode = 5;
static constexpr int DamageTypeMelee = 8;
static constexpr int DamageTypeRifleButt = 10;

// ILTServer, ILTCommon, ILTMessage_Write and ILTMessage_Read
static constexpr int ServerGetObjectNameOffset = 0x1BC;
static constexpr int CommonCreateMessageOffset = 0x38;
static constexpr int MessageAddRefOffset = 0x4;
static constexpr int MessageReleaseOffset = 0x8;
static constexpr int MessageReadOffset = 0x1C;
static constexpr int MessageWriteBitsOffset = 0x24;
static constexpr int MessageWriteDataOffset = 0x2C;
static constexpr int MessageWriteStringOffset = 0x38;
static constexpr int MessageWriteCompRotationOffset = 0x48;
static constexpr uint32_t SFXClientFXGroupInstant = 44;
static constexpr uint32_t MIDInitialUpdate = 1;
static constexpr uint32_t MIDUpdate = 2;
static constexpr uint32_t MIDLoadObject = 7;
static constexpr uint32_t MIDAllObjectsCreated = 14;

// ILTDatabase
static constexpr int DatabaseGetAttributeOffset = 0x58;
static constexpr int DatabaseGetStringOffset = 0x84;
static constexpr int DatabaseGetRecordLinkOffset = 0x9C;

// The first double doors of the first level
static constexpr const char* DoorBashHintDoorNames[2] = { "Door-Db_industrial_glass0100.Door1", "Door-Db_industrial_glass0100.Door2" };

struct DoorVec3 { float x, y, z; };
struct DoorQuat { float x, y, z, w; };
struct KickedDoor { int pDoor; int hKicker; DWORD nTime; };

struct DoorIntersectQuery
{
    DoorVec3 vFrom, vTo;
    uint32_t dwFlags;
    bool(__cdecl* pFilterFn)(int hObject, void* pUserData);
    void* pActualFilterFn;
    void* pPolyFilterFn;
    void* pUserData;
    void* pActualUserData;
    uint32_t nReserved;
};

struct DoorIntersectInfo
{
    DoorVec3 vPoint;
    DoorVec3 vNormal;
    float fDist;
    int hObject;
    uint32_t hPoly[2];
    uint32_t nSurfaceFlags;
};

static_assert(sizeof(DoorIntersectQuery) == 0x34, "IntersectQuery");
static_assert(sizeof(DoorIntersectInfo) == 0x2C, "IntersectInfo");

static DWORD doorBashLTServerAddr = 0;
static int doorBashHandleToObjectOffset = 0;
static int doorBashGetFrameTimeOffset = 0;
static int doorBashGetTimeMSOffset = 0;
static DWORD doorToggleStateAddr = 0;
static int doorBashPending = 0;
static uint32_t doorBashFrameTimeMS = 0;
static std::unordered_map<int, float> doorBashFrameDamage;

static DWORD doorLTServerAddr = 0;
static DWORD doorCommonLTAddr = 0;
static DWORD doorLTBaseAddr = 0;
static DWORD doorSurfaceDBAddr = 0;
static DWORD doorDatabaseAddr = 0;
static int doorGetObjectPosOffset = 0;
static int doorSendSFXMessageOffset = 0;
static int doorIntersectSegmentOffset = 0;
static uint32_t doorIntersectFlags = 0;
static int doorObjectOffset = 0;
static int doorCurStateOffset = 0;
static int doorTypeOffset = 0;
static int doorLastDamageOffset = 0;
static int doorLastDamageDirOffset = 0;
static int doorLinkSource = 0;
static bool doorLinkSourceKicked = false;
static KickedDoor kickedDoors[16] = {};
static int kickGoalMgrOffset = 0;
static int kickCurrentGoalOffset = 0;
static int kickActionRecordOffset = 0;
static uint32_t kickActionType = 0;
static int lastKicker = 0;
static DWORD lastKickTime = 0;
static int doorBashHintDoors[2] = {};
static bool doorBashHintShown = false;

void(__thiscall* DestructibleDoDamage)(int, int) = nullptr;
void(__thiscall* DoorToggleState)(int, float, int) = nullptr;
void(__thiscall* DoorHandleLinkToggleState)(int, bool, float, int) = nullptr;
float(__thiscall* AWMGetWaveformValue)(int, float, float) = nullptr;
bool(__thiscall* AWMCalcAngle)(int, float*, float, float, float, float, float) = nullptr;
uint32_t(__cdecl* GetSurfaceType)(const DoorIntersectInfo*) = nullptr;
int(__thiscall* SurfaceDBGetSurface)(int, uint32_t) = nullptr;
uint32_t(__thiscall* DoorSendNamedMessage)(int, int, int, const char*) = nullptr;
int(__thiscall* AIGoalGetCurrentAction)(int) = nullptr;
void(__thiscall* DoorSetPowerOn)(int, double, uint8_t) = nullptr;
void(__thiscall* DoorHandleLinkToggleStateHook)(int, bool, float, int) = nullptr;
void(__thiscall* DoorFXHandler)(int, int, const void*) = nullptr;
uint32_t(__thiscall* DoorEngineMessageFn)(int, uint32_t, void*, float) = nullptr;
void(__thiscall* DoorDestructor)(int) = nullptr;
uint32_t(__thiscall* PlayerEngineMessageFn)(int, uint32_t, void*, float) = nullptr;
void(__thiscall* PlayerHandleTextMsg)(int, int, const void*) = nullptr;


// =============
// DoorBashing
// =============

static int DoorBashHandleToObject(int hObject)
{
    using HandleToObject_t = int(__cdecl*)(int);
    int ltServer = MemoryHelper::ReadMemory<int>(doorBashLTServerAddr);
    HandleToObject_t handleToObject = *reinterpret_cast<HandleToObject_t*>(ltServer + doorBashHandleToObjectOffset);
    return handleToObject(hObject);
}

static float DoorBashGetFrameTime()
{
    using GetFrameTime_t = float(__thiscall*)(int);
    int ltServer = MemoryHelper::ReadMemory<int>(doorBashLTServerAddr);
    auto vtable = *reinterpret_cast<uintptr_t*>(ltServer);
    GetFrameTime_t getFrameTime = reinterpret_cast<GetFrameTime_t>(*reinterpret_cast<uintptr_t*>(vtable + doorBashGetFrameTimeOffset));
    return getFrameTime(ltServer);
}

static uint32_t DoorBashGetTimeMS()
{
    using GetTimeMS_t = uint32_t(__thiscall*)(int);
    int ltServer = MemoryHelper::ReadMemory<int>(doorBashLTServerAddr);
    auto vtable = *reinterpret_cast<uintptr_t*>(ltServer);
    GetTimeMS_t getTimeMS = reinterpret_cast<GetTimeMS_t>(*reinterpret_cast<uintptr_t*>(vtable + doorBashGetTimeMSOffset));
    return getTimeMS(ltServer);
}

// Extraction Point's Bounce waveform, the door swings open 4x faster then rebounds three times
static float GetBounceSpeed(float fRate, float fPercent)
{
    if (fPercent <= 0.09f) return fRate * 12.566371f;
    if (fPercent <= 0.4f) return std::cos((fPercent - 0.09f) * 10.13417f) * fRate * -1.2566371f;
    if (fPercent <= 0.8f) return std::cos((fPercent - 0.4f) * 7.853982f) * fRate * -0.62831855f;
    if (fPercent <= 1.0f) return std::cos((fPercent - 0.8f) * 15.707965f) * fRate * -0.31415927f;
    return 0.0f;
}

static float __fastcall AWMGetWaveformValue_Hook(int thisPtr, int, float fRate, float fPercent)
{
    if (*reinterpret_cast<uint8_t*>(thisPtr + AWMWaveformOffset) == AWMWaveBounce)
        return GetBounceSpeed(fRate, fPercent);

    return AWMGetWaveformValue(thisPtr, fRate, fPercent);
}

static bool __fastcall AWMCalcAngle_Hook(int thisPtr, int, float* pAngle, float fInitial, float fTarget, float fDir, float fTotalTime, float fPercent)
{
    uint8_t nWaveform = *reinterpret_cast<uint8_t*>(thisPtr + AWMWaveformOffset);
    uint8_t nType = *reinterpret_cast<uint8_t*>(thisPtr + AWMTypeOffset);

    if (nWaveform != AWMWaveBounce || nType != AWMTypeRotating || fPercent >= 1.0f || fDir == 0.0f || *pAngle != fTarget)
        return AWMCalcAngle(thisPtr, pAngle, fInitial, fTarget, fDir, fTotalTime, fPercent);

    float fRate = fTarget - fInitial;
    if (fTotalTime > 0.001f)
    {
        fRate /= fTotalTime;
    }

    *pAngle += GetBounceSpeed(fRate, fPercent) * DoorBashGetFrameTime();
    if (fDir > 0.0f ? *pAngle > fTarget : *pAngle < fTarget)
    {
        *pAngle = fTarget;
    }

    return false;
}

static void __fastcall SetBashActivatorPos(int pDoor, float* pActivatorPos, const float* pDoorPos, int nWaveform)
{
    float fLastDamage = *reinterpret_cast<float*>(pDoor + AWMDamageAggregateOffset + DestructibleLastDamageOffset);
    if (static_cast<uint8_t>(nWaveform) != AWMWaveBounce || !(fLastDamage > 0.0f))
        return;

    const float* pLastDamageDir = reinterpret_cast<const float*>(pDoor + AWMDamageAggregateOffset + DestructibleLastDamageDirOffset);
    pActivatorPos[0] = pDoorPos[0] - pLastDamageDir[0];
    pActivatorPos[1] = pDoorPos[1] - pLastDamageDir[1];
    pActivatorPos[2] = pDoorPos[2] - pLastDamageDir[2];
}

// Replaces 'mov eax, [esi + 260h]' in ActiveWorldModel::SetPowerOn after the positions of the activator and the door are read, the door is in esi
static __declspec(naked) void SetPowerOnActivatorPos_Hook()
{
    __asm
    {
        pushad
        movzx eax, byte ptr [esp + 0xAC]
        push eax
        lea eax, [esp + 0x88]
        push eax
        lea edx, [esp + 0x80]
        mov ecx, esi
        call SetBashActivatorPos
        popad
        mov eax, [esi + 0x260]
        ret
    }
}

// Replaces the toggle of a door being bashed, the original drops the time and the waveform
static void __fastcall DoorToggleState_Hook(int thisPtr, int, float fTime, int nWaveform)
{
    if (thisPtr == doorBashPending)
    {
        DoorHandleLinkToggleState(thisPtr, true, 2.0f, AWMWaveBounce);
        return;
    }

    DoorToggleState(thisPtr, fTime, nWaveform);
}

static void __fastcall DestructibleDoDamage_Hook(int thisPtr, int, int pDamage)
{
    DestructibleDoDamage(thisPtr, pDamage);

    int eType = *reinterpret_cast<int*>(pDamage + DamageTypeOffset);
    float fDamage = *reinterpret_cast<float*>(pDamage + DamageAmountOffset);
    if (!(fDamage > 0.0f) || (eType != DamageTypeMelee && eType != DamageTypeRifleButt && eType != DamageTypeExplode))
        return;

    int hDamager = *reinterpret_cast<int*>(pDamage + DamageDamagerOffset);
    int pDoor = DoorBashHandleToObject(*reinterpret_cast<int*>(thisPtr + DestructibleObjectOffset));
    if (!hDamager || !pDoor || pDoor + AWMDamageAggregateOffset != thisPtr)
        return;

    // Rotating doors only, RotatingDoor and SlidingDoor keep the toggle of Door
    uintptr_t vtable = *reinterpret_cast<uintptr_t*>(pDoor);
    if (*reinterpret_cast<DWORD*>(vtable + 0xBC) != doorToggleStateAddr || *reinterpret_cast<uint8_t*>(pDoor + AWMTypeOffset) != AWMTypeRotating)
        return;

    uint32_t nTimeMS = DoorBashGetTimeMS();
    if (nTimeMS != doorBashFrameTimeMS)
    {
        doorBashFrameDamage.clear();
        doorBashFrameTimeMS = nTimeMS;
    }

    float& fFrameDamage = doorBashFrameDamage[pDoor];
    fFrameDamage += fDamage;
    if (!(fFrameDamage > 99.0f))
        return;

    // Closed or closing
    uint8_t nState = *reinterpret_cast<uint8_t*>(pDoor + AWMCurStateOffset);
    if (nState != AWMStateOff && nState != AWMStatePowerOff)
        return;

    // Opens like a use by the damager, the door stays shut when locked or when it can't be used
    using Activate_t = void(__thiscall*)(int, int);
    Activate_t activate = reinterpret_cast<Activate_t>(*reinterpret_cast<uintptr_t*>(vtable + 0xC4));
    doorBashPending = pDoor;
    activate(pDoor, hDamager);
    doorBashPending = 0;
}

static void ApplyDoorBashingServerPatch()
{
    // Extraction Point and Perseus Mandate already have it
    if (!DoorBashing || g_State.CurrentFEARGame != FEAR) return;

    doorBashPending = 0;
    doorBashFrameTimeMS = 0;
    doorBashFrameDamage.clear();

    DWORD addr_GetWaveformValue = ScanModuleSignature(g_State.GameServer, "51 8A 81 0B 03 00 00 84 C0 75 08 D9 44 24 08 59 C2 08 00", "DoorBashing_GetWaveformValue");
    DWORD addr_CalcAngle = ScanModuleSignature(g_State.GameServer, "80 B9 AC 01 00 00 02 75 1B D9 44 24 18 D8 1D", "DoorBashing_CalcAngle");
    DWORD addr_SetPowerOnActivatorPos = ScanModuleSignature(g_State.GameServer, "8D 7C 24 54 57 50 FF 52 20 8B 0D ?? ?? ?? ?? 8B 01 8D 54 24 60 52 8B 56 08 52 FF 50 20 8B 86 60 02 00 00", "DoorBashing_SetPowerOnActivatorPos");
    DWORD addr_DoDamage = ScanModuleSignature(g_State.GameServer, "83 EC 20 53 55 56 57 8B 7C 24 34 8B 6F 04 8B 5F 14 8B F1 8D 47 44", "DoorBashing_DoDamage");
    DWORD addr_DoorToggleState = ScanModuleSignature(g_State.GameServer, "68 FF 00 00 00 68 00 00 80 BF 6A 01 E8 ?? ?? ?? ?? C2 08 00", "DoorBashing_DoorToggleState");

    if (addr_GetWaveformValue == 0 ||
        addr_CalcAngle == 0 ||
        addr_SetPowerOnActivatorPos == 0 ||
        addr_DoDamage == 0 ||
        addr_DoorToggleState == 0)
        return;

    doorBashLTServerAddr = MemoryHelper::ReadMemory<DWORD>(addr_CalcAngle + 0x66);
    doorBashGetFrameTimeOffset = MemoryHelper::ReadMemory<int>(addr_CalcAngle + 0x6E);
    doorBashHandleToObjectOffset = MemoryHelper::ReadMemory<uint8_t>(addr_DoDamage + 0x48);
    doorBashGetTimeMSOffset = MemoryHelper::ReadMemory<int>(addr_DoDamage + 0x64);
    doorToggleStateAddr = addr_DoorToggleState;
    DoorHandleLinkToggleState = reinterpret_cast<decltype(DoorHandleLinkToggleState)>(MemoryHelper::ResolveRelativeAddress(addr_DoorToggleState, 0xD));

    HookHelper::ApplyHookReplaceable((void*)addr_GetWaveformValue, &AWMGetWaveformValue_Hook, (LPVOID*)&AWMGetWaveformValue);
    HookHelper::ApplyHookReplaceable((void*)addr_CalcAngle, &AWMCalcAngle_Hook, (LPVOID*)&AWMCalcAngle);
    HookHelper::ApplyHookReplaceable((void*)addr_DoDamage, &DestructibleDoDamage_Hook, (LPVOID*)&DestructibleDoDamage);
    HookHelper::ApplyHookReplaceable((void*)addr_DoorToggleState, &DoorToggleState_Hook, (LPVOID*)&DoorToggleState);
    MemoryHelper::MakeCALL(addr_SetPowerOnActivatorPos + 0x1D, reinterpret_cast<uintptr_t>(&SetPowerOnActivatorPos_Hook));
    MemoryHelper::MakeNOP(addr_SetPowerOnActivatorPos + 0x22, 1);
}

// ==================
// Door bash helpers
// ==================

static bool DoorGetObjectPos(int hObject, DoorVec3& vPos)
{
    int ltServer = MemoryHelper::ReadMemory<int>(doorLTServerAddr);
    if (!ltServer || !hObject)
        return false;

    using GetObjectPos_t = uint32_t(__thiscall*)(int, int, DoorVec3*);
    GetObjectPos_t getObjectPos = reinterpret_cast<GetObjectPos_t>(*reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(ltServer) + doorGetObjectPosOffset));
    return getObjectPos(ltServer, hObject, &vPos) == 0;
}

static bool __cdecl DoorOnlyThisObject(int hObject, void* pUserData)
{
    return hObject && hObject == reinterpret_cast<int>(pUserData);
}

// The point of the door nearest vFrom, rays sweep across the door from its hinge, coarse then fine
static bool FindDoorImpact(int pDoor, const DoorVec3& vFrom, DoorIntersectInfo& bestInfo, DoorVec3& vBestDir)
{
    int hDoor = MemoryHelper::ReadMemory<int>(pDoor + doorObjectOffset);
    int ltBase = MemoryHelper::ReadMemory<int>(doorLTBaseAddr);
    DoorVec3 vHinge;
    if (!ltBase || !DoorGetObjectPos(hDoor, vHinge))
        return false;

    using IntersectSegment_t = bool(__thiscall*)(int, DoorIntersectQuery*, DoorIntersectInfo*, int);
    IntersectSegment_t intersectSegment = reinterpret_cast<IntersectSegment_t>(*reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(ltBase) + doorIntersectSegmentOffset));

    // Sideways: horizontal and across the line from vFrom to the hinge
    DoorVec3 vSide = { vHinge.z - vFrom.z, 0.0f, vFrom.x - vHinge.x };
    float fSideLen = std::sqrt(vSide.x * vSide.x + vSide.z * vSide.z);
    vSide = (fSideLen > 1.0f) ? DoorVec3{ vSide.x / fSideLen, 0.0f, vSide.z / fSideLen } : DoorVec3{ 1.0f, 0.0f, 0.0f };

    DoorIntersectQuery query = {};
    query.dwFlags = doorIntersectFlags;
    query.pFilterFn = &DoorOnlyThisObject;
    query.pUserData = reinterpret_cast<void*>(hDoor);

    bool bFound = false;
    float fBestDist = 0.0f;
    DoorVec3 vBestTarget = {};

    auto sweep = [&](DoorVec3 vCenter, float fStep, int nSide, int nDown, int nUp)
    {
        for (int i = -nSide; i <= nSide; i++)
        {
            for (int j = -nDown; j <= nUp; j++)
            {
                DoorVec3 vTarget = { vCenter.x + vSide.x * fStep * i, vCenter.y + fStep * j, vCenter.z + vSide.z * fStep * i };
                DoorVec3 vDir = { vTarget.x - vFrom.x, vTarget.y - vFrom.y, vTarget.z - vFrom.z };
                float fLen = std::sqrt(vDir.x * vDir.x + vDir.y * vDir.y + vDir.z * vDir.z);
                if (!(fLen > 1.0f))
                    continue;

                vDir = { vDir.x / fLen, vDir.y / fLen, vDir.z / fLen };
                query.vFrom = { vFrom.x - vDir.x * 32.0f, vFrom.y - vDir.y * 32.0f, vFrom.z - vDir.z * 32.0f };
                query.vTo = { vTarget.x + vDir.x * 256.0f, vTarget.y + vDir.y * 256.0f, vTarget.z + vDir.z * 256.0f };

                DoorIntersectInfo info = {};
                if (!intersectSegment(ltBase, &query, &info, hDoor))
                    continue;

                DoorVec3 vToHit = { info.vPoint.x - vFrom.x, info.vPoint.y - vFrom.y, info.vPoint.z - vFrom.z };
                float fDist = vToHit.x * vToHit.x + vToHit.y * vToHit.y + vToHit.z * vToHit.z;
                if (!bFound || fDist < fBestDist)
                {
                    bFound = true;
                    fBestDist = fDist;
                    bestInfo = info;
                    vBestDir = vDir;
                    vBestTarget = vTarget;
                }
            }
        }
    };

    // 32 units apart around the hinge, then 8 units apart around the closest hit
    sweep(vHinge, 32.0f, 4, 4, 6);
    if (!bFound)
        return false;

    sweep(vBestTarget, 8.0f, 2, 2, 2);
    return true;
}

// The ClientFX of a melee hit on the surface: SurfaceDB record -> WeaponFX -> Melee -> NormalFX
static const char* GetMeleeImpactFX(const DoorIntersectInfo& info)
{
    int pDatabase = MemoryHelper::ReadMemory<int>(doorDatabaseAddr);
    int pSurfaceDB = MemoryHelper::ReadMemory<int>(doorSurfaceDBAddr);
    if (!pDatabase || !pSurfaceDB)
        return nullptr;

    using GetAttribute_t = int(__thiscall*)(int, int, const char*);
    using GetRecordLink_t = int(__thiscall*)(int, int, uint32_t, int);
    using GetString_t = const char*(__thiscall*)(int, int, uint32_t, const char*);
    uintptr_t vtable = *reinterpret_cast<uintptr_t*>(pDatabase);
    GetAttribute_t getAttribute = reinterpret_cast<GetAttribute_t>(*reinterpret_cast<uintptr_t*>(vtable + DatabaseGetAttributeOffset));
    GetRecordLink_t getRecordLink = reinterpret_cast<GetRecordLink_t>(*reinterpret_cast<uintptr_t*>(vtable + DatabaseGetRecordLinkOffset));
    GetString_t getString = reinterpret_cast<GetString_t>(*reinterpret_cast<uintptr_t*>(vtable + DatabaseGetStringOffset));

    int hRecord = SurfaceDBGetSurface(pSurfaceDB, GetSurfaceType(&info));
    for (const char* szLink : { "WeaponFX", "Melee" })
    {
        int hAttribute = hRecord ? getAttribute(pDatabase, hRecord, szLink) : 0;
        hRecord = hAttribute ? getRecordLink(pDatabase, hAttribute, 0, 0) : 0;
    }

    int hAttribute = hRecord ? getAttribute(pDatabase, hRecord, "NormalFX") : 0;
    const char* szFX = hAttribute ? getString(pDatabase, hAttribute, 0, "") : nullptr;
    return (szFX && *szFX) ? szFX : nullptr;
}

// Rotation whose forward axis points along vDir
static DoorQuat DoorRotationFromForward(DoorVec3 f)
{
    DoorVec3 up = (std::fabs(f.y) > 0.99f) ? DoorVec3{ 0.0f, 0.0f, 1.0f } : DoorVec3{ 0.0f, 1.0f, 0.0f };
    DoorVec3 r = { up.y * f.z - up.z * f.y, up.z * f.x - up.x * f.z, up.x * f.y - up.y * f.x };
    float fLen = std::sqrt(r.x * r.x + r.y * r.y + r.z * r.z);
    r = { r.x / fLen, r.y / fLen, r.z / fLen };
    DoorVec3 u = { f.y * r.z - f.z * r.y, f.z * r.x - f.x * r.z, f.x * r.y - f.y * r.x };

    float fTrace = r.x + u.y + f.z;
    if (fTrace > 0.0f)
    {
        float s = std::sqrt(fTrace + 1.0f) * 2.0f;
        return { (u.z - f.y) / s, (f.x - r.z) / s, (r.y - u.x) / s, 0.25f * s };
    }
    if (r.x > u.y && r.x > f.z)
    {
        float s = std::sqrt(1.0f + r.x - u.y - f.z) * 2.0f;
        return { 0.25f * s, (u.x + r.y) / s, (f.x + r.z) / s, (u.z - f.y) / s };
    }
    if (u.y > f.z)
    {
        float s = std::sqrt(1.0f + u.y - r.x - f.z) * 2.0f;
        return { (u.x + r.y) / s, 0.25f * s, (f.y + u.z) / s, (f.x - r.z) / s };
    }

    float s = std::sqrt(1.0f + f.z - r.x - u.y) * 2.0f;
    return { (f.x + r.z) / s, (f.y + u.z) / s, 0.25f * s, (r.y - u.x) / s };
}

// Same message as CDestructibleModel::CreateDestroyedFX: SFX_CLIENTFXGROUPINSTANT, sent to every client
static void SpawnDoorClientFX(const char* szFX, const DoorVec3& vPos, const DoorQuat& rRot)
{
    int ltServer = MemoryHelper::ReadMemory<int>(doorLTServerAddr);
    int commonLT = MemoryHelper::ReadMemory<int>(doorCommonLTAddr);
    if (!ltServer || !commonLT)
        return;

    auto vfunc = [](int pObject, int nOffset) { return *reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(pObject) + nOffset); };

    int pWrite = 0;
    if (reinterpret_cast<int(__thiscall*)(int, int*)>(vfunc(commonLT, CommonCreateMessageOffset))(commonLT, &pWrite) != 0 || !pWrite)
        return;

    auto writeBits = reinterpret_cast<void(__thiscall*)(int, uint32_t, uint32_t)>(vfunc(pWrite, MessageWriteBitsOffset));
    reinterpret_cast<void(__thiscall*)(int)>(vfunc(pWrite, MessageAddRefOffset))(pWrite);
    writeBits(pWrite, SFXClientFXGroupInstant, 8);
    reinterpret_cast<void(__thiscall*)(int, const char*)>(vfunc(pWrite, MessageWriteStringOffset))(pWrite, szFX);
    writeBits(pWrite, 0, 1); // Loop
    writeBits(pWrite, 0, 1); // Smooth shutdown
    writeBits(pWrite, 0, 1); // No special parent
    reinterpret_cast<void(__thiscall*)(int, const void*, uint32_t)>(vfunc(pWrite, MessageWriteDataOffset))(pWrite, &vPos, 96);
    reinterpret_cast<void(__thiscall*)(int, const DoorQuat*)>(vfunc(pWrite, MessageWriteCompRotationOffset))(pWrite, &rRot);
    writeBits(pWrite, 0, 1); // No target

    int pRead = reinterpret_cast<int(__thiscall*)(int)>(vfunc(pWrite, MessageReadOffset))(pWrite);
    if (pRead)
    {
        reinterpret_cast<void(__thiscall*)(int)>(vfunc(pRead, MessageAddRefOffset))(pRead);
        reinterpret_cast<int(__thiscall*)(int, int, uint32_t)>(vfunc(ltServer, doorSendSFXMessageOffset))(ltServer, pRead, 0);
        reinterpret_cast<void(__thiscall*)(int)>(vfunc(pRead, MessageReleaseOffset))(pRead);
    }

    reinterpret_cast<void(__thiscall*)(int)>(vfunc(pWrite, MessageReleaseOffset))(pWrite);
}

// ======================
// Door kicks and links
// ======================

// A kick and a hand push both open doors with the "DOOR" key, a kick is the blocked door traversal action, as in CAI::KickDoor
static bool IsKicking(int pAI)
{
    int pGoalMgr = MemoryHelper::ReadMemory<int>(pAI + kickGoalMgrOffset);
    int pGoal = pGoalMgr ? MemoryHelper::ReadMemory<int>(pGoalMgr + kickCurrentGoalOffset) : 0;
    int pAction = pGoal ? AIGoalGetCurrentAction(pGoal) : 0;
    int pRecord = pAction ? MemoryHelper::ReadMemory<int>(pAction + kickActionRecordOffset) : 0;
    return pRecord && MemoryHelper::ReadMemory<uint32_t>(pRecord) == kickActionType;
}

static int TakeKickedDoor(int pDoor)
{
    DWORD nTime = GetTickCount();
    for (KickedDoor& kicked : kickedDoors)
    {
        if (kicked.pDoor != pDoor)
            continue;

        int hKicker = (nTime - kicked.nTime <= 1000) ? kicked.hKicker : 0;
        kicked = {};
        return hKicker;
    }

    return 0;
}

// The impact sound of a melee hit on the door's surface, where the kick meets the door, once per kick
static void PlayKickImpact(int pDoor, int hKicker)
{
    DoorVec3 vFrom;
    if (!DoorGetObjectPos(hKicker, vFrom))
        return;

    // The kick lands low, below the height of handles and push bars
    vFrom.y -= 50.0f;

    DoorIntersectInfo info = {};
    DoorVec3 vDir;
    if (!FindDoorImpact(pDoor, vFrom, info, vDir))
        return;

    // Skip edges, faces turned away and slanted parts like push bars
    DoorVec3 vToKicker = { vFrom.x - info.vPoint.x, vFrom.y - info.vPoint.y, vFrom.z - info.vPoint.z };
    float fLen = std::sqrt(vToKicker.x * vToKicker.x + vToKicker.y * vToKicker.y + vToKicker.z * vToKicker.z);
    float fFacing = (fLen > 1.0f) ? (info.vNormal.x * vToKicker.x + info.vNormal.y * vToKicker.y + info.vNormal.z * vToKicker.z) / fLen : 0.0f;
    if (!(std::fabs(fFacing) > 0.5f) || !(std::fabs(info.vNormal.y) < 0.5f) || GetSurfaceType(&info) == 0)
        return;

    DWORD nTime = GetTickCount();
    if (hKicker == lastKicker && nTime - lastKickTime < 500)
        return;

    if (const char* szFX = GetMeleeImpactFX(info))
    {
        lastKicker = hKicker;
        lastKickTime = nTime;
        SpawnDoorClientFX(szFX, info.vPoint, DoorRotationFromForward({ -vDir.x, -vDir.y, -vDir.z }));
    }
}

// Replaces the call that sends a door "ON" in the door link's open code
static uint32_t __fastcall DoorSendNamedMessage_Hook(int thisPtr, int, int pSender, int pTarget, const char* szMessage)
{
    if (pSender && pTarget && IsKicking(pSender))
    {
        KickedDoor* pSlot = &kickedDoors[0];
        for (KickedDoor& kicked : kickedDoors)
        {
            if (kicked.pDoor == pTarget || !kicked.pDoor) { pSlot = &kicked; break; }
            if (kicked.nTime - pSlot->nTime > 0x80000000u) pSlot = &kicked;
        }

        *pSlot = { pTarget, MemoryHelper::ReadMemory<int>(pSender + doorObjectOffset), GetTickCount() };
    }

    return DoorSendNamedMessage(thisPtr, pSender, pTarget, szMessage);
}

// A bouncing door opens away from its last damage, or from its activator when it has none
static void BouncingDoorSetPowerOn(int pDoor, double fTime, uint8_t nWaveform, float fDamage, const DoorVec3& vDamageDir)
{
    float* pLastDamage = reinterpret_cast<float*>(pDoor + doorLastDamageOffset);
    DoorVec3* pLastDamageDir = reinterpret_cast<DoorVec3*>(pDoor + doorLastDamageDirOffset);
    float fLastDamage = *pLastDamage;
    DoorVec3 vLastDamageDir = *pLastDamageDir;

    *pLastDamage = fDamage;
    *pLastDamageDir = vDamageDir;
    DoorSetPowerOn(pDoor, fTime, nWaveform);
    *pLastDamage = fLastDamage;
    *pLastDamageDir = vLastDamageDir;
}

static void __fastcall DoorSetPowerOn_Hook(int thisPtr, int, double fTime, uint8_t nWaveform)
{
    int hKicker = TakeKickedDoor(thisPtr);
    bool bRotating = MemoryHelper::ReadMemory<uint8_t>(thisPtr + doorTypeOffset) == AWMTypeRotating;

    // A kicked door bounces open away from the kicker
    if (hKicker && bRotating)
    {
        BouncingDoorSetPowerOn(thisPtr, 2.0, AWMWaveBounce, 0.0f, {});
        if (thisPtr == doorLinkSource)
            doorLinkSourceKicked = true;

        if (MemoryHelper::ReadMemory<uint8_t>(thisPtr + doorCurStateOffset) == AWMStatePowerOn)
            PlayKickImpact(thisPtr, hKicker);
        return;
    }

    // The other half of a double door swings the same way as the half that was hit
    if (doorLinkSource && doorLinkSource != thisPtr && bRotating && nWaveform == AWMWaveBounce)
    {
        float fDamage = doorLinkSourceKicked ? 0.0f : MemoryHelper::ReadMemory<float>(doorLinkSource + doorLastDamageOffset);
        BouncingDoorSetPowerOn(thisPtr, fTime, nWaveform, fDamage, MemoryHelper::ReadMemory<DoorVec3>(doorLinkSource + doorLastDamageDirOffset));
        return;
    }

    DoorSetPowerOn(thisPtr, fTime, nWaveform);
}

// Door::HandleLinkToggleState toggles the door, then its link
static void __fastcall DoorHandleLinkToggleState_Hook(int thisPtr, int, bool bTriggerLink, float fTime, int nWaveform)
{
    int pOuterSource = doorLinkSource;
    bool bOuterKicked = doorLinkSourceKicked;
    doorLinkSource = thisPtr;
    doorLinkSourceKicked = false;
    DoorHandleLinkToggleStateHook(thisPtr, bTriggerLink, fTime, nWaveform);
    doorLinkSource = pOuterSource;
    doorLinkSourceKicked = bOuterKicked;
}

// ================
// Door bash hint
// ================

// Extraction Point's hint, shown before the first double doors of the first level

// Same as the levels' hint triggers: "msg player (TEXT off)" then "msg player (TEXT Training_BashDoor)"
static void SendDoorBashHint(int pPlayer)
{
    struct Token { uint32_t nHash; const char* szValue; };
    struct ParsedMsg { uint32_t nArgs; Token args[2]; };
    ParsedMsg off = { 2, { { 0, "TEXT" }, { 0, "off" } } };
    ParsedMsg hint = { 2, { { 0, "TEXT" }, { 0, "Training_BashDoor" } } };
    PlayerHandleTextMsg(pPlayer, 0, &off);
    PlayerHandleTextMsg(pPlayer, 0, &hint);
}

// The doors are reached from their -Z side
static void CheckDoorBashHint(int pPlayer)
{
    if (doorBashHintShown || !doorBashHintDoors[0] || !doorBashHintDoors[1])
        return;

    if (MemoryHelper::ReadMemory<uint8_t>(doorBashHintDoors[0] + doorCurStateOffset) != AWMStateOff ||
        MemoryHelper::ReadMemory<uint8_t>(doorBashHintDoors[1] + doorCurStateOffset) != AWMStateOff)
        return;

    DoorVec3 vDoor1, vDoor2, vPlayer;
    if (!DoorGetObjectPos(MemoryHelper::ReadMemory<int>(doorBashHintDoors[0] + doorObjectOffset), vDoor1) ||
        !DoorGetObjectPos(MemoryHelper::ReadMemory<int>(doorBashHintDoors[1] + doorObjectOffset), vDoor2) ||
        !DoorGetObjectPos(MemoryHelper::ReadMemory<int>(pPlayer + doorObjectOffset), vPlayer))
        return;

    float fInFront = vDoor1.z - vPlayer.z;
    if (fInFront > 0.0f && fInFront < 400.0f &&
        std::fabs(vPlayer.x - (vDoor1.x + vDoor2.x) * 0.5f) < 300.0f &&
        std::fabs(vPlayer.y - vDoor1.y) < 300.0f)
    {
        doorBashHintShown = true;
        SendDoorBashHint(pPlayer);
    }
}

// Remembers the hint's doors when they're created or loaded
static uint32_t __fastcall DoorEngineMessageFn_Hook(int thisPtr, int, uint32_t messageID, void* pData, float fData)
{
    uint32_t result = DoorEngineMessageFn(thisPtr, messageID, pData, fData);

    if (messageID == MIDInitialUpdate || messageID == MIDLoadObject || messageID == MIDAllObjectsCreated)
    {
        int ltServer = MemoryHelper::ReadMemory<int>(doorLTServerAddr);
        int hObject = MemoryHelper::ReadMemory<int>(thisPtr + doorObjectOffset);
        char szName[128] = {};

        using GetObjectName_t = uint32_t(__thiscall*)(int, int, char*, uint32_t);
        if (ltServer && hObject &&
            reinterpret_cast<GetObjectName_t>(*reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(ltServer) + ServerGetObjectNameOffset))(ltServer, hObject, szName, sizeof(szName)) == 0)
        {
            for (int i = 0; i < 2; i++)
            {
                if (_stricmp(szName, DoorBashHintDoorNames[i]) == 0 && doorBashHintDoors[i] != thisPtr)
                {
                    doorBashHintDoors[i] = thisPtr;
                    doorBashHintShown = false;
                }
            }
        }
    }

    return result;
}

static void __fastcall DoorDestructor_Hook(int thisPtr, int)
{
    for (int& pDoor : doorBashHintDoors)
    {
        if (pDoor == thisPtr)
            pDoor = 0;
    }

    DoorDestructor(thisPtr);
}

static uint32_t __fastcall PlayerEngineMessageFn_Hook(int thisPtr, int, uint32_t messageID, void* pData, float fData)
{
    uint32_t result = PlayerEngineMessageFn(thisPtr, messageID, pData, fData);

    if (messageID == MIDUpdate)
        CheckDoorBashHint(thisPtr);

    return result;
}

// Kicks, double doors and the hint, the Bounce waveform comes from DoorBashing in the first game
static void ApplyDoorBashingExtrasServerPatch()
{
    bool bExpansion = g_State.IsExpansion();
    if (bExpansion ? !ExpansionDoorBashFixes : (!DoorBashing || g_State.CurrentFEARGame != FEAR || AWMCalcAngle == nullptr)) return;

    for (KickedDoor& kicked : kickedDoors) kicked = {};
    lastKicker = 0;
    lastKickTime = 0;
    doorLinkSource = 0;
    doorLinkSourceKicked = false;
    doorBashHintDoors[0] = 0;
    doorBashHintDoors[1] = 0;
    doorBashHintShown = false;

    // Door::Activate: "mov al, [esi + m_nCurState]" ... "push [esi + m_hObject] / call [ILTServer + GetObjectPos]"
    DWORD addr_DoorActivate = ScanModuleSignature(g_State.GameServer, "83 EC 0C 56 8B F1 8A 86 ?? ?? ?? ?? 3C 01 74 04 3C 02 75 ?? 8B 0D ?? ?? ?? ?? 8B 01 8D 54 24 04 52 8B 56 ?? 52 FF 50 ??", "DoorBashing_DoorActivate");
    DWORD addr_DoorType = ScanModuleSignature(g_State.GameServer, "80 BE ?? ?? 00 00 02 75 04 83 4F 08 08 D9 86", "DoorBashing_DoorType");
    DWORD addr_CreateDestroyedFX = ScanModuleSignature(g_State.GameServer, "83 EC 20 57 8B F9 8B 87 ?? 00 00 00 85 C0 0F 84 15 01 00 00 80 38 00 0F 84 0C 01 00 00", "DoorBashing_CreateDestroyedFX");

    // The game's "where does this ray meet this object" helper, then its surface lookup
    DWORD addr_IntersectObject = ScanModuleSignature(g_State.GameServer, "83 7C 24 10 02 8B 84 24 ?? ?? ?? ?? 75 19 C7 44 24 70 ?? 00 00 00 C7 44 24 74 ?? ?? ?? ?? 89 84 24 80 00 00 00 EB 08 C7 44 24 70 ?? 00 00 00 8B 0D ?? ?? ?? ?? 8B 11 50 8D 44 24 30 50 8D 44 24 60 50 FF 92", "DoorBashing_IntersectObject");
    DWORD addr_SurfaceLookup = ScanModuleSignature(g_State.GameServer, "8D 4C 24 2C 51 E8 ?? ?? ?? ?? 83 C4 04 3B C7 74 ?? 8B 0D ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 3B C7 75 ?? 5F 5E 5D 33 C0 5B 83 C4 ?? C3 8B 0D", "DoorBashing_SurfaceLookup");

    // "push "ON" / push door / push AI / mov ecx, [g_pCmdMgr] / call SendNamedMessage", for each door on the path
    DWORD addr_SendDoorOn = ScanModuleSignature(g_State.GameServer, "68 ?? ?? ?? ?? 50 51 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 45 20 47 83 C3 10 3B F8 7C", "DoorBashing_SendDoorOn");
    DWORD addr_KickDoor = ScanModuleSignature(g_State.GameServer, "56 8B F1 E8 ?? ?? ?? ?? 8B 86 ?? ?? 00 00 8B 48 ?? 85 C9 74 ?? E8 ?? ?? ?? ?? 85 C0 74 ?? 8B 8E ?? ?? 00 00 8B 49 ?? E8 ?? ?? ?? ?? 8B 90 ?? ?? 00 00 83 3A ?? 75", "DoorBashing_KickDoor");
    DWORD addr_DoorSetPowerOn = ScanModuleSignature(g_State.GameServer, "8B 44 24 0C DD 44 24 04 56 50 83 EC 08 DD 1C 24 8B F1 E8", "DoorBashing_DoorSetPowerOn");
    DWORD addr_DoorHandleLinkToggleState = ScanModuleSignature(g_State.GameServer, "8B 44 24 08 55 8B 6C 24 10 57 55 50 8B F9 E8 ?? ?? ?? ?? 8B 87 ?? ?? 00 00 F6 C4 01 75", "DoorBashing_DoorHandleLinkToggleState");

    // The expansions' "a bouncing door opens away from its last damage": "cmp bl, 4 / jne / fld [esi + last damage]"
    DWORD addr_LastDamage = bExpansion ? ScanModuleSignature(g_State.GameServer, "80 FB 04 75 ?? D9 46 ?? D8 1D ?? ?? ?? ?? DF E0 F6 C4 41 75 ?? D9 44 24 ?? 8D 46", "DoorBashing_LastDamage") : 1;

    // The hint, first game only
    DWORD addr_DoorEngineMessageFn = !bExpansion ? ScanModuleSignature(g_State.GameServer, "83 EC 08 53 8B 5C 24 10 83 FB 0E 55 56 57 8B 7C 24 20 8B F1 0F 87 DD 00 00 00", "DoorBashing_DoorEngineMessageFn") : 1;
    DWORD addr_DoorDestructor = !bExpansion ? ScanModuleSignature(g_State.GameServer, "53 56 57 8B F9 C7 07 ?? ?? ?? ?? C7 47 10 ?? ?? ?? ?? 8D B7 ?? ?? 00 00 BB 02 00 00 00", "DoorBashing_DoorDestructor") : 1;
    DWORD addr_PlayerEngineMessageFn = !bExpansion ? ScanModuleSignature(g_State.GameServer, "53 8B 5C 24 08 83 FB 0E 55 56 57 8B 7C 24 18 8B F1 0F 87 07 01 00 00", "DoorBashing_PlayerEngineMessageFn") : 1;
    DWORD addr_PlayerHandleTextMsg = !bExpansion ? ScanModuleSignature(g_State.GameServer, "53 57 8B 7C 24 10 83 3F 02 8B D9 0F 82 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 8B 01 56 8D 54 24 14 52 33 F6 FF 50 38 85 C0 75 0F 8B 4C 24 14 85 C9 74 07 8B 01 8B F1 FF 50 04 8B 16 6A 08 6A 7E", "DoorBashing_PlayerHandleTextMsg") : 1;

    if (addr_DoorActivate == 0 ||
        addr_DoorType == 0 ||
        addr_CreateDestroyedFX == 0 ||
        addr_IntersectObject == 0 ||
        addr_SurfaceLookup == 0 ||
        addr_SendDoorOn == 0 ||
        addr_KickDoor == 0 ||
        addr_DoorSetPowerOn == 0 ||
        addr_DoorHandleLinkToggleState == 0 ||
        addr_LastDamage == 0 ||
        addr_DoorEngineMessageFn == 0 ||
        addr_DoorDestructor == 0 ||
        addr_PlayerEngineMessageFn == 0 ||
        addr_PlayerHandleTextMsg == 0)
        return;

    // CreateDestroyedFX sends its effect through the same g_pLTServer
    if (MemoryHelper::ReadMemory<DWORD>(addr_CreateDestroyedFX + 0x107) != MemoryHelper::ReadMemory<DWORD>(addr_DoorActivate + 0x16))
    {
        if (ShowErrors)
            MessageBoxA(NULL, "Error: Unexpected engine pointers for patch: DoorBashing_CreateDestroyedFX", "EchoPatch", MB_ICONERROR);
        return;
    }

    doorCurStateOffset = MemoryHelper::ReadMemory<int>(addr_DoorActivate + 0x8);
    doorLTServerAddr = MemoryHelper::ReadMemory<DWORD>(addr_DoorActivate + 0x16);
    doorObjectOffset = MemoryHelper::ReadMemory<uint8_t>(addr_DoorActivate + 0x23);
    doorGetObjectPosOffset = MemoryHelper::ReadMemory<uint8_t>(addr_DoorActivate + 0x27);
    doorTypeOffset = MemoryHelper::ReadMemory<int>(addr_DoorType + 0x2);
    doorCommonLTAddr = MemoryHelper::ReadMemory<DWORD>(addr_CreateDestroyedFX + 0x6F);
    doorSendSFXMessageOffset = MemoryHelper::ReadMemory<int>(addr_CreateDestroyedFX + 0x112);
    doorIntersectFlags = MemoryHelper::ReadMemory<uint8_t>(addr_IntersectObject + 0x12);
    doorLTBaseAddr = MemoryHelper::ReadMemory<DWORD>(addr_IntersectObject + 0x31);
    doorIntersectSegmentOffset = MemoryHelper::ReadMemory<int>(addr_IntersectObject + 0x44);
    doorSurfaceDBAddr = MemoryHelper::ReadMemory<DWORD>(addr_SurfaceLookup + 0x13);
    doorDatabaseAddr = MemoryHelper::ReadMemory<DWORD>(addr_SurfaceLookup + 0x2D);
    GetSurfaceType = reinterpret_cast<decltype(GetSurfaceType)>(MemoryHelper::ResolveRelativeAddress(addr_SurfaceLookup, 0x6));
    SurfaceDBGetSurface = reinterpret_cast<decltype(SurfaceDBGetSurface)>(MemoryHelper::ResolveRelativeAddress(addr_SurfaceLookup, 0x19));
    kickGoalMgrOffset = MemoryHelper::ReadMemory<int>(addr_KickDoor + 0xA);
    kickCurrentGoalOffset = MemoryHelper::ReadMemory<uint8_t>(addr_KickDoor + 0x10);
    kickActionRecordOffset = MemoryHelper::ReadMemory<int>(addr_KickDoor + 0x2E);
    kickActionType = MemoryHelper::ReadMemory<uint8_t>(addr_KickDoor + 0x34);
    AIGoalGetCurrentAction = reinterpret_cast<decltype(AIGoalGetCurrentAction)>(MemoryHelper::ResolveRelativeAddress(addr_KickDoor, 0x16));
    DoorSendNamedMessage = reinterpret_cast<decltype(DoorSendNamedMessage)>(MemoryHelper::ResolveRelativeAddress(addr_SendDoorOn, 0xE));
    doorLastDamageOffset = bExpansion ? MemoryHelper::ReadMemory<uint8_t>(addr_LastDamage + 0x7) : AWMDamageAggregateOffset + DestructibleLastDamageOffset;
    doorLastDamageDirOffset = bExpansion ? MemoryHelper::ReadMemory<uint8_t>(addr_LastDamage + 0x1B) : AWMDamageAggregateOffset + DestructibleLastDamageDirOffset;

    HookHelper::ApplyHookReplaceable((void*)addr_DoorSetPowerOn, &DoorSetPowerOn_Hook, (LPVOID*)&DoorSetPowerOn);
    HookHelper::ApplyHookReplaceable((void*)addr_DoorHandleLinkToggleState, &DoorHandleLinkToggleState_Hook, (LPVOID*)&DoorHandleLinkToggleStateHook);
    MemoryHelper::MakeCALL(addr_SendDoorOn + 0xD, reinterpret_cast<uintptr_t>(&DoorSendNamedMessage_Hook));

    if (bExpansion)
        return;

    PlayerHandleTextMsg = reinterpret_cast<decltype(PlayerHandleTextMsg)>(addr_PlayerHandleTextMsg);
    HookHelper::ApplyHookReplaceable((void*)addr_DoorEngineMessageFn, &DoorEngineMessageFn_Hook, (LPVOID*)&DoorEngineMessageFn);
    HookHelper::ApplyHookReplaceable((void*)addr_DoorDestructor, &DoorDestructor_Hook, (LPVOID*)&DoorDestructor);
    HookHelper::ApplyHookReplaceable((void*)addr_PlayerEngineMessageFn, &PlayerEngineMessageFn_Hook, (LPVOID*)&PlayerEngineMessageFn);
}


// ==================
// Door bash sparks
// ==================

// The levels' bash effect, sparks and a metal thump at a fixed spot whatever the door is made of
static void __fastcall DoorFXHandler_Hook(int thisPtr, int, int hSender, const void* pParsedMsg)
{
    // CParsedMsg: the argument count, then { hash, string } for each argument
    uint32_t nArgs = MemoryHelper::ReadMemory<uint32_t>(reinterpret_cast<uintptr_t>(pParsedMsg));
    const char* szFX = (nArgs > 1) ? MemoryHelper::ReadMemory<const char*>(reinterpret_cast<uintptr_t>(pParsedMsg) + 0x10) : nullptr;
    if (szFX && _stricmp(szFX, "SparkFX_small_sound") == 0)
        return;

    DoorFXHandler(thisPtr, hSender, pParsedMsg);
}

static void ApplyDoorBashSparksServerPatch()
{
    if (!ExpansionDoorBashFixes || !g_State.IsExpansion()) return;

    DWORD addr_DoorFXHandler = ScanModuleSignature(g_State.GameServer, "A0 ?? ?? ?? ?? 83 EC 2C A8 01 53 56 57 8B F9 75", "DoorBashing_DoorFXHandler");

    if (addr_DoorFXHandler == 0)
        return;

    HookHelper::ApplyHookReplaceable((void*)addr_DoorFXHandler, &DoorFXHandler_Hook, (LPVOID*)&DoorFXHandler);
}
