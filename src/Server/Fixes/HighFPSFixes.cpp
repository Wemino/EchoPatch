#pragma once

#include "../../Globals.cpp"

static int phSlowMoRecord = 0;
static int lastHSlowMoRecord = 0;
static bool slowMoChargeObserved = false;
static DWORD pPlayerObjListBegin = 0;
static DWORD pPlayerObjListEnd = 0;
static DWORD pLTServerAddr = 0;
static int serverGetFrameTimeOffset = 0;
static int proximityFlippingOffset = 0;

struct AIRoll
{
    float value;
    double time;
    unsigned int frame;
};
static std::unordered_map<uint64_t, AIRoll> aiRolls;
static std::unordered_map<int, double> proximityGrenadeUpdateTimes;
static std::unordered_map<int, double> deathFireUpdateTimes;
static std::unordered_map<int, double> flickerUpdateTimes;
static std::unordered_map<uint64_t, double> progressiveDamageTimes;
static std::unordered_map<uint64_t, double> triggerTouchTimes;

void(__thiscall* ServerUpdateSlowMo)(int*) = nullptr;
void(__thiscall* ServerExitSlowMo)(int*, bool, float) = nullptr;
void(__thiscall* PlayerInventoryUpdate)(int) = nullptr;
bool(__thiscall* CheckForSever)(int*, int, char, int) = nullptr;
void(__thiscall* AI_SetNextServerUpdate)(DWORD, float) = nullptr;
bool(__thiscall* ProximityGrenadeUpdate)(int) = nullptr;
void(__thiscall* UpdateFireWeaponDuringDeath)(int) = nullptr;
float(__cdecl* AIGetRandom)(float, float) = nullptr;
void(__thiscall* FlickerUpdate)(int) = nullptr;
bool(__thiscall* DamageStructDoDamage)(int, int, int) = nullptr;
void(__thiscall* TriggerObjectTouch)(int, int) = nullptr;

// ========================
// HighFPSFixes
// ========================

// The local client is in this process, so a lone player on the server is the local player
static bool IsLocalPlayerAlone()
{
    DWORD playerObjListBegin = *(DWORD*)pPlayerObjListBegin;
    DWORD playerObjListEnd = *(DWORD*)pPlayerObjListEnd;
    return playerObjListEnd - playerObjListBegin == sizeof(DWORD);
}

void __fastcall ServerUpdateSlowMo_Hook(int* thisPtr, int)
{
    if (!IsLocalPlayerAlone())
    {
        ServerUpdateSlowMo(thisPtr);
        return;
    }

    int hSlowMoRecord = *(int*)((char*)thisPtr + phSlowMoRecord);

    // New activation, reset observation
    if (hSlowMoRecord != lastHSlowMoRecord)
    {
        slowMoChargeObserved = false;
        lastHSlowMoRecord = hSlowMoRecord;
    }

    // Latch: once we've seen a real charge, we trust future zero readings
    if (g_State.clientSlowMoCharge > 0.01)
    {
        slowMoChargeObserved = true;
    }

    // Only force-exit if we've confirmed charge was real and it's now depleted
    if (hSlowMoRecord != 0 && slowMoChargeObserved && g_State.clientSlowMoCharge <= 0.01)
    {
        ServerExitSlowMo(thisPtr, true, 0.0f);
        return;
    }
    ServerUpdateSlowMo(thisPtr);
}

// The server charge follows the charge displayed by the client, only with a single player as the client also drains its charge when another player uses slowmo
void __fastcall PlayerInventoryUpdate_Hook(int thisPtr, int)
{
    PlayerInventoryUpdate(thisPtr);

    if (!IsLocalPlayerAlone())
        return;

    float fMaxCharge = *(float*)(thisPtr + 0x64);
    *(float*)(thisPtr + 0x5C) = std::min(static_cast<float>(g_State.clientSlowMoCharge), fMaxCharge);
}

bool __fastcall CheckForSever_Hook(int* thisPtr, int, int a2, char a3, int a4)
{
    bool isSevered = CheckForSever(thisPtr, a2, a3, a4);

    if (isSevered)
    {
        DWORD pCharacter = *reinterpret_cast<DWORD*>(reinterpret_cast<DWORD>(thisPtr) + 0x4);
        if (pCharacter)
        {
            AI_SetNextServerUpdate(pCharacter, 0.01f); // prevent erratic ragdoll
        }
    }

    return isSevered;
}

void __fastcall AI_SetNextServerUpdate_Hook(DWORD thisPtr, int, float a2)
{
    AI_SetNextServerUpdate(thisPtr, 0.00000001f);
}

static float GetServerFrameTime()
{
    using GetFrameTime_t = float(__thiscall*)(int);
    int ltServer = MemoryHelper::ReadMemory<int>(pLTServerAddr);
    auto vtable = *(uintptr_t*)ltServer;
    GetFrameTime_t getFrameTime = (GetFrameTime_t)(*(uintptr_t*)(vtable + serverGetFrameTimeOffset));
    return getFrameTime(ltServer);
}

static bool __fastcall ProximityGrenadeUpdate_Hook(int thisPtr, int)
{
    BYTE* pFlipping = reinterpret_cast<BYTE*>(thisPtr + proximityFlippingOffset);
    if (IsTargetFrameDue(proximityGrenadeUpdateTimes.try_emplace(thisPtr, -1.0).first->second) || *pFlipping)
        return ProximityGrenadeUpdate(thisPtr);

    *pFlipping = 2;
    bool result = ProximityGrenadeUpdate(thisPtr);
    if (*pFlipping == 2)
        *pFlipping = 0;

    return result;
}

static void __fastcall UpdateFireWeaponDuringDeath_Hook(int thisPtr, int)
{
    if (IsTargetFrameDue(deathFireUpdateTimes.try_emplace(thisPtr, -1.0).first->second))
        UpdateFireWeaponDuringDeath(thisPtr);
}

static float GetAIRoll(uint64_t key, float min, float max)
{
    // Goals and actions with a chance roll on every update while they are considered, keep the roll for a 60 FPS frame
    auto [entry, isNew] = aiRolls.try_emplace(key, AIRoll{ 0.0f, -1.0, 0 });
    AIRoll& roll = entry->second;

    if ((!isNew && roll.frame == g_State.frameCount) || IsTargetFrameDue(roll.time))
    {
        roll.value = AIGetRandom(min, max);
        roll.frame = g_State.frameCount;
    }

    return roll.value;
}

static float __cdecl GoalRoll(int goal, float min, float max)
{
    return GetAIRoll(static_cast<uint32_t>(goal), min, max);
}

static float __cdecl ActionRoll(int ai, int action, float min, float max)
{
    return GetAIRoll((static_cast<uint64_t>(static_cast<uint32_t>(ai)) << 32) | static_cast<uint32_t>(action), min, max);
}

// Replaces the activate chance roll of a goal, the goal is in esi
static __declspec(naked) void GoalRoll_Hook()
{
    __asm
    {
        push dword ptr [esp + 8]
        push dword ptr [esp + 8]
        push esi
        call GoalRoll
        add esp, 12
        ret
    }
}

// Replaces the probability roll of an action, the action is in esi and the planner in edi
static __declspec(naked) void ActionRoll_Hook()
{
    __asm
    {
        push dword ptr [esp + 8]
        push dword ptr [esp + 8]
        push esi
        mov eax, [edi + 0x10]
        push eax
        call ActionRoll
        add esp, 16
        ret
    }
}

static void __fastcall FlickerUpdate_Hook(int thisPtr, int)
{
    bool bEnabled = *reinterpret_cast<bool*>(thisPtr + 0x55);
    if (!bEnabled || IsTargetFrameDue(flickerUpdateTimes.try_emplace(thisPtr, -1.0).first->second))
    {
        FlickerUpdate(thisPtr);
        return;
    }

    // A flicker without delay picks a new value and sends it on every update, keep its timer expired until the next 60 FPS frame
    float* pNextValueTimer = reinterpret_cast<float*>(thisPtr + 0x30);
    float nextValueTimer = *pNextValueTimer;
    float frameTime = GetServerFrameTime();
    if (nextValueTimer - frameTime > 0.0f)
    {
        FlickerUpdate(thisPtr);
        return;
    }

    *pNextValueTimer = frameTime + 0.0001f;
    FlickerUpdate(thisPtr);
    *pNextValueTimer = nextValueTimer - frameTime;
}

static bool __fastcall DamageStructDoDamage_Hook(int thisPtr, int, int hDamager, int hVictim)
{
    int hContainer = *reinterpret_cast<int*>(thisPtr + 0x28);
    float fDuration = *reinterpret_cast<float*>(thisPtr + 0x34);

    // Damage volumes and lingering explosions send their damage over time again on every update, the victim only keeps the latest one
    if (hContainer && hVictim && fDuration > 0.0f)
    {
        uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(hContainer)) << 32) | static_cast<uint32_t>(hVictim);
        if (!IsTargetFrameDue(progressiveDamageTimes.try_emplace(key, -1.0).first->second))
            return true;
    }

    return DamageStructDoDamage(thisPtr, hDamager, hVictim);
}

static void __fastcall TriggerObjectTouch_Hook(int thisPtr, int, int hToucher)
{
    // A trigger without delay accepts every touch, an object standing in it touches it on every update
    float fTriggerDelay = *reinterpret_cast<float*>(thisPtr + 0x24);
    if (fTriggerDelay <= 0.0f)
    {
        uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(thisPtr)) << 32) | static_cast<uint32_t>(hToucher);
        if (!IsTargetFrameDue(triggerTouchTimes.try_emplace(key, -1.0).first->second))
            return;
    }

    TriggerObjectTouch(thisPtr, hToucher);
}

static void ApplyHighFPSFixesServerPatch()
{
    if (!HighFPSFixes) return;

    DWORD addr_ServerUpdateSlowMo = ScanModuleSignature(g_State.GameServer, "56 8B F1 8B 86 ?? ?? ?? ?? 85 C0 74 6B", "ServerUpdateSlowMo");
    DWORD addr_ServerExitSlowMo = ScanModuleSignature(g_State.GameServer, "51 53 8B D9 8B 83 ?? ?? ?? ?? 85 C0 0F 84 ?? ?? ?? ?? DD 05", "ServerExitSlowMo");
    DWORD addr_CheckForSever = ScanModuleSignature(g_State.GameServer, "83 EC 18 8B 44 24 1C 89 4C 24 08", "CheckForSever");
    DWORD addr_AI_SetNextServerUpdate = ScanModuleSignature(g_State.GameServer, "8B 44 24 10 83 C4 08 89 86", "AI_SetNextServerUpdate");
    DWORD addr_PlayerInventoryUpdate = ScanModuleSignature(g_State.GameServer, "51 A1 ?? ?? ?? ?? 56 8B F1 8B 88 ?? ?? 00 00 85 C9 74 ?? 8A 46 ?? 84 C0 74 ?? 8A 46 ?? 84 C0 74 ?? E8", "PlayerInventoryUpdate");
    DWORD addr_PlayerObjList = ScanModuleSignature(g_State.GameServer, "8B 35 ?? ?? ?? ?? 3B 35 ?? ?? ?? ?? 74 25 8B 87 ?? ?? 00 00 8B 0E 33 D2", "PlayerObjList");
    DWORD addr_ProximityGrenadeFlipping = ScanModuleSignature(g_State.GameServer, "8A 86 ?? 02 00 00 84 C0 0F 85 02 02 00 00 8A 86 ?? 02 00 00 84 C0 0F 85 F4 01 00 00", "ProximityGrenadeFlipping");
    DWORD addr_ProximityGrenadeUpdate = ScanModuleSignature(g_State.GameServer, "8A 86 ?? 02 00 00 84 C0 0F 85 02 02 00 00 8A 86 ?? 02 00 00 84 C0 0F 85 F4 01 00 00", "ProximityGrenadeUpdate", 4);
    DWORD addr_UpdateFireWeaponDuringDeath = ScanModuleSignature(g_State.GameServer, "81 EC 80 00 00 00 57 8B F9 8B 87 ?? ?? 00 00 83 F8 ?? 0F 84 71 02 00 00 83 F8 ?? 0F 84 68 02 00 00 8B 87", "UpdateFireWeaponDuringDeath");
    DWORD addr_GoalRoll = ScanModuleSignature(g_State.GameServer, "8B 06 8B CE FF 50 2C D9 5C 24 ?? 68 00 00 80 3F 53 E8 ?? ?? ?? ?? D8 5C 24 ?? 83 C4 08", "GoalRoll");
    DWORD addr_ActionRoll = ScanModuleSignature(g_State.GameServer, "8B 47 10 8B 16 50 8B CE FF 52 1C D9 5C 24 ?? 68 00 00 80 3F 6A 00 E8 ?? ?? ?? ?? D8 5C 24", "ActionRoll");
    DWORD addr_FlickerUpdate = ScanModuleSignature(g_State.GameServer, "56 8B F1 8A 46 55 84 C0 0F 84 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 8B 01 FF 90 ?? ?? 00 00 D9 C0 D8 46 28 D9 5E 28 D8 6E 30 D9 56 30", "FlickerUpdate");
    DWORD addr_DamageStructDoDamage = ScanModuleSignature(g_State.GameServer, "53 8B 5C 24 0C 85 DB 57 8B F9 75 07 5F 32 C0 5B C2 08 00 8B 0D ?? ?? ?? ?? 8B 01 56 8D 54 24 14 52 33 F6", "DamageStructDoDamage");
    DWORD addr_TriggerObjectTouch = ScanModuleSignature(g_State.GameServer, "53 55 57 8B 7C 24 10 57 8B E9 E8 ?? ?? ?? ?? 84 C0 57 0F 95", "TriggerObjectTouch");

    if (addr_ServerUpdateSlowMo == 0 || addr_ServerExitSlowMo == 0 || addr_CheckForSever == 0 || addr_AI_SetNextServerUpdate == 0 || addr_PlayerInventoryUpdate == 0 || addr_PlayerObjList == 0 ||
        addr_ProximityGrenadeFlipping == 0 || addr_ProximityGrenadeUpdate == 0 || addr_UpdateFireWeaponDuringDeath == 0 || addr_GoalRoll == 0 || addr_ActionRoll == 0 ||
        addr_FlickerUpdate == 0 || addr_DamageStructDoDamage == 0 || addr_TriggerObjectTouch == 0)
        return;

    // Needs the client charge, not available on a dedicated server
    if (g_State.isClientSlowMoTracked)
    {
        pPlayerObjListBegin = MemoryHelper::ReadMemory<DWORD>(addr_PlayerObjList + 0x2);
        pPlayerObjListEnd = MemoryHelper::ReadMemory<DWORD>(addr_PlayerObjList + 0x8);
        phSlowMoRecord = MemoryHelper::ReadMemory<int>(addr_ServerUpdateSlowMo + 0x5);
        HookHelper::ApplyHookReplaceable((void*)addr_ServerUpdateSlowMo, &ServerUpdateSlowMo_Hook, (LPVOID*)&ServerUpdateSlowMo);
        HookHelper::ApplyHookReplaceable((void*)addr_PlayerInventoryUpdate, &PlayerInventoryUpdate_Hook, (LPVOID*)&PlayerInventoryUpdate);
        ServerExitSlowMo = reinterpret_cast<decltype(ServerExitSlowMo)>((int)addr_ServerExitSlowMo);
    }

    HookHelper::ApplyHookReplaceable((void*)addr_CheckForSever, &CheckForSever_Hook, (LPVOID*)&CheckForSever);
    HookHelper::ApplyHookReplaceable((void*)(addr_AI_SetNextServerUpdate - 0x15), &AI_SetNextServerUpdate_Hook, (LPVOID*)&AI_SetNextServerUpdate);

    proximityFlippingOffset = MemoryHelper::ReadMemory<int>(addr_ProximityGrenadeFlipping + 0x10);
    pLTServerAddr = MemoryHelper::ReadMemory<DWORD>(addr_FlickerUpdate + 0x10);
    serverGetFrameTimeOffset = MemoryHelper::ReadMemory<int>(addr_FlickerUpdate + 0x18);
    AIGetRandom = reinterpret_cast<decltype(AIGetRandom)>(MemoryHelper::ResolveRelativeAddress(addr_GoalRoll, 0x12));
    MemoryHelper::MakeCALL(addr_GoalRoll + 0x11, reinterpret_cast<uintptr_t>(&GoalRoll_Hook));
    MemoryHelper::MakeCALL(addr_ActionRoll + 0x16, reinterpret_cast<uintptr_t>(&ActionRoll_Hook));
    HookHelper::ApplyHookReplaceable((void*)addr_ProximityGrenadeUpdate, &ProximityGrenadeUpdate_Hook, (LPVOID*)&ProximityGrenadeUpdate);
    HookHelper::ApplyHookReplaceable((void*)addr_UpdateFireWeaponDuringDeath, &UpdateFireWeaponDuringDeath_Hook, (LPVOID*)&UpdateFireWeaponDuringDeath);
    HookHelper::ApplyHookReplaceable((void*)addr_FlickerUpdate, &FlickerUpdate_Hook, (LPVOID*)&FlickerUpdate);
    HookHelper::ApplyHookReplaceable((void*)addr_DamageStructDoDamage, &DamageStructDoDamage_Hook, (LPVOID*)&DamageStructDoDamage);
    HookHelper::ApplyHookReplaceable((void*)addr_TriggerObjectTouch, &TriggerObjectTouch_Hook, (LPVOID*)&TriggerObjectTouch);
}
