#pragma once

#include "../../Globals.cpp"

// ActiveWorldModel
static constexpr int AWMDamageAggregateOffset = 0x20;
static constexpr int AWMTypeOffset = 0x1AC;
static constexpr int AWMCurStateOffset = 0x309;
static constexpr int AWMWaveformOffset = 0x30B;
static constexpr uint8_t AWMTypeRotating = 2;
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

static DWORD doorBashLTServerAddr = 0;
static int doorBashHandleToObjectOffset = 0;
static int doorBashGetFrameTimeOffset = 0;
static int doorBashGetTimeMSOffset = 0;
static DWORD doorToggleStateAddr = 0;
static int doorBashPending = 0;
static uint32_t doorBashFrameTimeMS = 0;
static std::unordered_map<int, float> doorBashFrameDamage;

void(__thiscall* DestructibleDoDamage)(int, int) = nullptr;
void(__thiscall* DoorToggleState)(int, float, int) = nullptr;
void(__thiscall* DoorHandleLinkToggleState)(int, bool, float, int) = nullptr;
float(__thiscall* AWMGetWaveformValue)(int, float, float) = nullptr;
bool(__thiscall* AWMCalcAngle)(int, float*, float, float, float, float, float) = nullptr;

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
    int pBashedDoor = doorBashPending ? doorBashPending : pDoor;

    float fLastDamage = *reinterpret_cast<float*>(pBashedDoor + AWMDamageAggregateOffset + DestructibleLastDamageOffset);
    if (static_cast<uint8_t>(nWaveform) != AWMWaveBounce || !(fLastDamage > 0.0f))
        return;

    const float* pLastDamageDir = reinterpret_cast<const float*>(pBashedDoor + AWMDamageAggregateOffset + DestructibleLastDamageDirOffset);
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
