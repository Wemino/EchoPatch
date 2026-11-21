#pragma once

#include "../../Globals.cpp"

// ActiveWorldModel
static constexpr int DoorPowerOffSoundOffset = 0x34C;
static constexpr int DoorOffSoundOffset = 0x368;
static constexpr uint8_t DoorTypeRotating = 2;
static constexpr uint8_t DoorStateClosed = 3;
static constexpr uint8_t DoorStateClosing = 4;

// Extraction Point's "Door" damage mask: Explode, Melee and RifleButt
static constexpr uint16_t DoorDamageMaskXP = (1 << 7) | (1 << 10) | (1 << 12);

static int doorFixCurStateOffset = 0;
static int doorFixTypeOffset = 0;
static int doorFixCantDamageOffset = 0;
static uint32_t doorFixDamageTypeCount = 0;

void(__thiscall* DoorStartSound)(int, const char*, bool) = nullptr;
const char*(__thiscall* StdStringCStr)(int) = nullptr;
uint16_t(__cdecl* DamageTypeToFlag)(uint32_t) = nullptr;

// Doors that started closing with their latch sound held back, and when
struct HeldLatch { int pDoor; DWORD nTime; };
static HeldLatch heldLatches[32] = {};

// ===================
// FixDoorCloseSound
// ===================

// Many double doors have their latch sound in "ClosingSound" instead of "ClosedSound", so it plays when the door starts closing
static bool IsLatchSound(const char* szSound)
{
    if (!szSound || !*szSound)
        return false;

    const char* szFile = szSound;
    for (const char* p = szSound; *p; p++)
    {
        if (*p == '\\' || *p == '/')
            szFile = p + 1;
    }

    for (const char* szLatch : { "door_01_close.wav", "door_02_close.wav", "door_03_close.wav", "hospitaldoor_01_close.wav", "office_close_01.wav" })
    {
        if (_stricmp(szFile, szLatch) == 0)
            return true;
    }

    return false;
}

static bool TakeHeldLatch(int pDoor)
{
    for (HeldLatch& held : heldLatches)
    {
        if (held.pDoor != pDoor)
            continue;

        bool bLive = GetTickCount() - held.nTime <= 30000;
        held = {};
        return bLive;
    }

    return false;
}

static bool HoldLatch(int pDoor)
{
    DWORD nTime = GetTickCount();
    for (HeldLatch& held : heldLatches)
    {
        if (held.pDoor == pDoor || !held.pDoor || nTime - held.nTime > 30000)
        {
            held = { pDoor, nTime };
            return true;
        }
    }

    return false;
}

// The latch is held back when a door without a closed sound starts closing, and played when it has closed
static void __fastcall DoorStartSound_Hook(int thisPtr, int, const char* szSound, bool bLoop)
{
    if (MemoryHelper::ReadMemory<uint8_t>(thisPtr + doorFixTypeOffset) == DoorTypeRotating)
    {
        uint8_t nState = MemoryHelper::ReadMemory<uint8_t>(thisPtr + doorFixCurStateOffset);
        if (nState == DoorStateClosing)
        {
            const char* szOffSound = StdStringCStr(thisPtr + DoorOffSoundOffset);
            if (IsLatchSound(szSound) && (!szOffSound || !*szOffSound) && HoldLatch(thisPtr))
            {
                // Still stops the opening sound
                DoorStartSound(thisPtr, "", bLoop);
                return;
            }
        }
        else if (nState == DoorStateClosed)
        {
            if (TakeHeldLatch(thisPtr) && (!szSound || !*szSound))
            {
                const char* szLatch = StdStringCStr(thisPtr + DoorPowerOffSoundOffset);
                if (IsLatchSound(szLatch))
                    szSound = szLatch;
            }
        }
        else
        {
            // Reopened before it closed
            TakeHeldLatch(thisPtr);
        }
    }

    DoorStartSound(thisPtr, szSound, bLoop);
}

// =====================
// FixParticleGunDoors
// =====================

// Extraction Point's 16-bit damage masks miss its last four damage types, so the particle gun gets past the door mask, Perseus Mandate uses 32 bits
static uint32_t __stdcall DamageTypeToFlagForTarget(uint32_t nType, int pDestructible)
{
    uint32_t nFlag = DamageTypeToFlag(nType);
    if (nType < 16 || nType >= doorFixDamageTypeCount)
        return nFlag;

    // Doors only allow the types below 16
    uint16_t nCantDamage = MemoryHelper::ReadMemory<uint16_t>(pDestructible + doorFixCantDamageOffset);
    if (static_cast<uint16_t>(~nCantDamage) == DoorDamageMaskXP)
        return nCantDamage;

    return nFlag;
}

static void ApplyDoorFixesServerPatch()
{
    if (g_State.CurrentFEARGame == FEARMP) return;

    for (HeldLatch& held : heldLatches) held = {};

    HMODULE hMSVCP71 = GetModuleHandleA("MSVCP71.dll");
    DWORD addr_StdStringCStr = hMSVCP71 ? reinterpret_cast<DWORD>(GetProcAddress(hMSVCP71, "?c_str@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPBDXZ")) : 0;
    DWORD addr_DoorStartSound = ScanModuleSignature(g_State.GameServer, "83 EC 24 56 8B F1 E8 ?? ?? ?? ?? 8B 44 24 2C 85 C0 0F 84", "FixDoorCloseSound_StartSound");
    DWORD addr_DoorActivate = ScanModuleSignature(g_State.GameServer, "83 EC 0C 56 8B F1 8A 86 ?? ?? ?? ?? 3C 01 74 04 3C 02 75", "FixDoorCloseSound_DoorActivate");
    DWORD addr_DoorType = ScanModuleSignature(g_State.GameServer, "80 BE ?? ?? 00 00 02 75 04 83 4F 08 08 D9 86", "FixDoorCloseSound_DoorType");

    if (addr_StdStringCStr == 0 && ShowErrors)
        MessageBoxA(NULL, "Error: Unable to find std::string::c_str in MSVCP71.dll for patch: FixDoorCloseSound", "EchoPatch", MB_ICONERROR);

    if (addr_StdStringCStr != 0 &&
        addr_DoorStartSound != 0 &&
        addr_DoorActivate != 0 &&
        addr_DoorType != 0)
    {
        StdStringCStr = reinterpret_cast<decltype(StdStringCStr)>(addr_StdStringCStr);
        doorFixCurStateOffset = MemoryHelper::ReadMemory<int>(addr_DoorActivate + 0x8);
        doorFixTypeOffset = MemoryHelper::ReadMemory<int>(addr_DoorType + 0x2);
        HookHelper::ApplyHookReplaceable((void*)addr_DoorStartSound, &DoorStartSound_Hook, (LPVOID*)&DoorStartSound);
    }

    // Perseus Mandate already has 32-bit masks, and the first game's doors have no mask
    if (g_State.CurrentFEARGame != FEARXP) return;

    // "cmp word ptr [esi + cant damage], 0 / je / push ebp / call DamageTypeToFlag / add esp, 4 / test word ptr [esi + cant damage], ax / jnz"
    DWORD addr_DamageMaskCheck = ScanModuleSignature(g_State.GameServer, "66 83 BE ?? ?? 00 00 00 74 ?? 55 E8 ?? ?? ?? ?? 83 C4 04 66 85 86 ?? ?? 00 00 0F 85", "FixParticleGunDoors_DamageMaskCheck");

    if (addr_DamageMaskCheck == 0)
        return;

    // DamageTypeToFlag: "if (type < count) return 1 << type; return 0"
    DamageTypeToFlag = reinterpret_cast<decltype(DamageTypeToFlag)>(MemoryHelper::ResolveRelativeAddress(addr_DamageMaskCheck, 0xC));
    doorFixDamageTypeCount = MemoryHelper::ReadMemory<uint8_t>(reinterpret_cast<uintptr_t>(DamageTypeToFlag) + 0xA);
    doorFixCantDamageOffset = MemoryHelper::ReadMemory<int>(addr_DamageMaskCheck + 0x3);

    // "push ebp / call DamageTypeToFlag / add esp, 4" -> "push esi / push ebp / call DamageTypeToFlagForTarget / nop"
    MemoryHelper::WriteMemory<uint16_t>(addr_DamageMaskCheck + 0xA, 0x5556);
    MemoryHelper::MakeCALL(addr_DamageMaskCheck + 0xC, reinterpret_cast<uintptr_t>(&DamageTypeToFlagForTarget));
    MemoryHelper::MakeNOP(addr_DamageMaskCheck + 0x11, 2);
}
