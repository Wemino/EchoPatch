#pragma once

#include <intrin.h>
#include "../../Globals.cpp"

// ILTStringEdit
static constexpr int StringEditGetStringOffset = 0x1C;

// CClientWeapon
static constexpr int ClientWeaponAmmoOffset = 0x1A8;

// IDatabaseMgr
static constexpr int DatabaseGetRecordNameOffset = 0x4C;

// Extraction Point's Training_BashDoor and WAR01_Help
static constexpr const wchar_t* DoorBashHintText = L"Press MELEE (|Alt-Fire|) to bash open nearby doors.  Grenades and other explosives can open doors from a distance.";
static constexpr const wchar_t* DoorBashLoadingTipText = L"Press MELEE (|Alt-Fire|) to bash open nearby doors.";

static DWORD stringEditAddr = 0;
static DWORD loadingTipReturnAddr = 0;
static DWORD clientDatabaseAddr = 0;
static bool doorBashHintShowing = false;
static const char* swapStringID = nullptr;
static const wchar_t* swapStringText = nullptr;
static char lastLoadingTipID[64] = {};
static DWORD lastLoadingTipTime = 0;
static bool lastLoadingTipReplaced = false;
static int lastMeleeImpactObject = 0;
static DWORD lastMeleeImpactTime = 0;

void(__thiscall* CreditsStart)(int, const char*) = nullptr;
const wchar_t*(__cdecl* CreateHelpString)(const char*) = nullptr;
const wchar_t*(__thiscall* StringEditGetString)(int, int, const char*) = nullptr;
void(__thiscall* ClientWeaponAddImpact)(int, int, const void*, const void*, const void*, const void*, int, uint32_t) = nullptr;

// ================
// Door bash hint
// ================

static const wchar_t* __fastcall StringEditGetString_Swap(int thisPtr, int, int hStringDB, const char* szStringID)
{
    if (szStringID && swapStringID && _stricmp(szStringID, swapStringID) == 0)
        return swapStringText;

    return StringEditGetString(thisPtr, hStringDB, szStringID);
}

// CreateHelpString reads the text through g_pLTIStringEdit->GetString, then puts the bound keys in place of |Action| names
static const wchar_t* CreateHelpStringWith(const char* szStringID, const wchar_t* szText)
{
    int stringEdit = MemoryHelper::ReadMemory<int>(stringEditAddr);
    if (!stringEdit)
        return CreateHelpString(szStringID);

    uintptr_t slot = *reinterpret_cast<uintptr_t*>(stringEdit) + StringEditGetStringOffset;
    StringEditGetString = *reinterpret_cast<decltype(StringEditGetString)*>(slot);
    MemoryHelper::WriteMemory<uintptr_t>(slot, reinterpret_cast<uintptr_t>(&StringEditGetString_Swap));
    swapStringID = szStringID;
    swapStringText = szText;
    const wchar_t* szResult = CreateHelpString(szStringID);
    swapStringID = nullptr;
    MemoryHelper::WriteMemory<uintptr_t>(slot, reinterpret_cast<uintptr_t>(StringEditGetString));
    return szResult;
}

// A loading screen builds its tip more than once, the pick holds for the whole load, 1 in 4
static bool ReplaceLoadingTip(const char* szStringID)
{
    DWORD nTime = GetTickCount();
    if (_stricmp(szStringID, lastLoadingTipID) != 0 || nTime - lastLoadingTipTime > 60000)
    {
        strncpy_s(lastLoadingTipID, szStringID, _TRUNCATE);
        lastLoadingTipReplaced = (__rdtsc() >> 4) % 4 == 0;
    }

    lastLoadingTipTime = nTime;
    return lastLoadingTipReplaced;
}

static const wchar_t* __cdecl CreateHelpString_Hook(const char* szStringID)
{
    if (szStringID && *szStringID)
    {
        // The hint plays through the open doors hint, same layout and timing
        if (doorBashHintShowing && _stricmp(szStringID, "Training_Action") == 0)
            return CreateHelpStringWith(szStringID, DoorBashHintText);

        if (reinterpret_cast<DWORD>(_ReturnAddress()) == loadingTipReturnAddr && _stricmp(szStringID, "<none>") != 0 && ReplaceLoadingTip(szStringID))
            return CreateHelpStringWith(szStringID, DoorBashLoadingTipText);
    }

    return CreateHelpString(szStringID);
}

// "msg player (TEXT Training_BashDoor)" from the server, the first game has no such hint
static void __fastcall CreditsStart_Hook(int thisPtr, int, const char* szOrder)
{
    if (szOrder && _stricmp(szOrder, "Training_BashDoor") == 0)
    {
        doorBashHintShowing = true;
        CreditsStart(thisPtr, "Training_Action");
        doorBashHintShowing = false;
        return;
    }

    CreditsStart(thisPtr, szOrder);
}

// ======================
// One impact per kick
// ======================

// A kick traces a hit at several animation keys, up to 67 ms apart, and a swinging door can take two, separate kicks are 100 ms or more apart
static void __fastcall ClientWeaponAddImpact_Hook(int thisPtr, int, int hObject, const void* vFrom, const void* vImpact, const void* vNormal, const void* vPath, int eSurface, uint32_t hHitNode)
{
    int hAmmo = MemoryHelper::ReadMemory<int>(thisPtr + ClientWeaponAmmoOffset);
    int pDatabase = MemoryHelper::ReadMemory<int>(clientDatabaseAddr);
    if (hObject && hHitNode == 0xFFFFFFFF && hAmmo && pDatabase)
    {
        using GetRecordName_t = const char*(__thiscall*)(int, int);
        GetRecordName_t getRecordName = reinterpret_cast<GetRecordName_t>(*reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(pDatabase) + DatabaseGetRecordNameOffset));
        const char* szAmmo = getRecordName(pDatabase, hAmmo);

        if (szAmmo && _strnicmp(szAmmo, "Melee", 5) == 0)
        {
            DWORD nTime = GetTickCount();
            bool bRepeat = hObject == lastMeleeImpactObject && nTime - lastMeleeImpactTime < 85;
            lastMeleeImpactObject = hObject;
            lastMeleeImpactTime = nTime;

            if (bRepeat)
                return;
        }
    }

    ClientWeaponAddImpact(thisPtr, hObject, vFrom, vImpact, vNormal, vPath, eSurface, hHitNode);
}

static void ApplyDoorBashingClientPatch()
{
    bool bExpansion = g_State.IsExpansion();
    if (bExpansion ? !ExpansionDoorBashFixes : (!DoorBashing || g_State.CurrentFEARGame != FEAR)) return;

    // CClientWeapon::AddImpact, after "push ecx / push esi / push edi", or "push ecx / push ebx / push esi / push edi" in Perseus Mandate
    DWORD addr_ClientWeaponAddImpact = ScanModuleSignature(g_State.GameClient, "8B F1 8B 86 A8 01 00 00 8B 0D ?? ?? ?? ?? 6A 00 50 E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 8B 39 6A 00 6A 00 68 ?? ?? ?? ?? 50 FF 57 58 8B 0D ?? ?? ?? ?? 50 FF 97 9C 00 00 00", "DoorBashing_ClientWeaponAddImpact");

    if (addr_ClientWeaponAddImpact != 0)
    {
        clientDatabaseAddr = MemoryHelper::ReadMemory<DWORD>(addr_ClientWeaponAddImpact + 0x18);
        lastMeleeImpactObject = 0;
        DWORD addr_Start = addr_ClientWeaponAddImpact - ((g_State.CurrentFEARGame == FEARXP2) ? 0x4 : 0x3);
        HookHelper::ApplyHook((void*)addr_Start, &ClientWeaponAddImpact_Hook, (LPVOID*)&ClientWeaponAddImpact);
    }

    // The hint and the loading screen tip are for the first game, the expansions have their own
    if (bExpansion) return;

    DWORD addr_CreditsStart = ScanModuleSignature(g_State.GameClient, "56 8B F1 E8 ?? ?? ?? ?? 8B 54 24 08 8B 48 24 52 51 8B C8 E8 ?? ?? ?? ?? 50 8B CE E8 ?? ?? ?? ?? 5E C2 04 00", "DoorBashing_CreditsStart");
    DWORD addr_CreateHelpString = ScanModuleSignature(g_State.GameClient, "8A 0D ?? ?? ?? ?? B8 01 00 00 00 81 EC 7C 01 00 00 84 C8 75 26 8B 15 ?? ?? ?? ?? 0B D0 B9 ?? ?? ?? ?? 89 15 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 68 ?? ?? ?? ?? E8 ?? ?? ?? ?? 83 C4 04 8B 0D ?? ?? ?? ?? 85 C9 53 56 74 2A 8B 15 ?? ?? ?? ?? 85 D2 74 20 8B 84 24 88 01 00 00 85 C0 74 0E 80 38 00 74 09 8B 31 50 52 FF 56 1C", "DoorBashing_CreateHelpString");

    // CLoadingScreen::ReadMissionInfo: "if (szHelpId[0]) m_Help.SetString(CreateHelpString(szHelpId))"
    DWORD addr_LoadingTip = ScanModuleSignature(g_State.GameClient, "8A 08 81 C6 ?? ?? ?? ?? 84 C9 6A 00 74 ?? 8B 3E 50 E8 ?? ?? ?? ?? 83 C4 04", "DoorBashing_LoadingTip");

    if (addr_CreditsStart == 0 ||
        addr_CreateHelpString == 0 ||
        addr_LoadingTip == 0)
        return;

    stringEditAddr = MemoryHelper::ReadMemory<DWORD>(addr_CreateHelpString + 0x3D);
    loadingTipReturnAddr = addr_LoadingTip + 0x16;

    HookHelper::ApplyHook((void*)addr_CreditsStart, &CreditsStart_Hook, (LPVOID*)&CreditsStart);
    HookHelper::ApplyHook((void*)addr_CreateHelpString, &CreateHelpString_Hook, (LPVOID*)&CreateHelpString);
}
