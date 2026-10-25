#pragma once

#include "../../Globals.cpp"
#include "../../Client/Client.hpp"

bool(__thiscall* FinishSaveGame)(int) = nullptr;
bool(__thiscall* LoadSaveFile)(int, const char*, const char*, const char*) = nullptr;
const char*(__thiscall* SaveFileCStr)(int) = nullptr;
void(__cdecl* GetUserDirectory)(char*, uint32_t) = nullptr;

static void ApplyPersistentWorldServerPatch()
{
    if (!EnablePersistentWorldState) return;

    DWORD addr_BodyFading = ScanModuleSignature(g_State.GameServer, "8A 86 ?? ?? 00 00 84 C0 74 A1 8D 8E", "BodyFading");

    if (addr_BodyFading != 0)
    {
        MemoryHelper::WriteMemory<uint8_t>(addr_BodyFading + 0x22, 0xEB);
    }
}

// ========================
// SavePersistentWorldState
// ========================

static std::string GetSaveFilePath(const char* relativePath)
{
    char userDirectory[MAX_PATH * 2] = {};
    GetUserDirectory(userDirectory, sizeof(userDirectory));
    return std::string(userDirectory) + relativePath;
}

static bool __fastcall FinishSaveGame_Hook(int thisPtr, int)
{
    bool result = FinishSaveGame(thisPtr);

    // Write it next to 'm_sSaveGameFile'
    if (result)
    {
        WritePersistentWorldStateFile(GetSaveFilePath(SaveFileCStr(thisPtr + 0x7C)));
    }

    return result;
}

static bool __fastcall LoadSaveFile_Hook(int thisPtr, int, const char* pszSaveIniKey, const char* pszSaveDir, const char* pszSaveFile)
{
    bool result = LoadSaveFile(thisPtr, pszSaveIniKey, pszSaveDir, pszSaveFile);

    // Restored when entering the world
    g_State.loadedSaveFilePath = result ? GetSaveFilePath(pszSaveFile) : "";

    return result;
}

static void ApplySavePersistentWorldStateServerPatch()
{
    if (!SavePersistentWorldState) return;

    DWORD addr_FinishSaveGame = ScanModuleSignature(g_State.GameServer, "A1 ?? ?? ?? ?? 81 EC 08 02 00 00 85 C0 57 8B F9 74 15 8D 4F 60 FF 15", "FinishSaveGame");
    DWORD addr_LoadSaveFile = ScanModuleSignature(g_State.GameServer, "81 EC 08 02 00 00 53 8B 9C 24 10 02 00 00 85 DB 55 56 57 8B F1 0F 84 C9 00 00 00 80 3B 00", "LoadSaveFile");
    DWORD addr_GetUserDirectory = ScanModuleSignature(g_State.GameServer, "B9 81 00 00 00 8D 7C 24 11 F3 AB 66 AB 8D 54 24 10 68 08 02 00 00 52 AA E8", "GetUserDirectory");

    if (addr_FinishSaveGame == 0 || addr_LoadSaveFile == 0 || addr_GetUserDirectory == 0)
        return;

    SaveFileCStr = reinterpret_cast<decltype(SaveFileCStr)>(MemoryHelper::ReadMemory<DWORD>(MemoryHelper::ReadMemory<DWORD>(addr_FinishSaveGame + 0x17)));
    GetUserDirectory = reinterpret_cast<decltype(GetUserDirectory)>(MemoryHelper::ResolveRelativeAddress(addr_GetUserDirectory, 0x19));

    HookHelper::ApplyHookReplaceable((void*)addr_FinishSaveGame, &FinishSaveGame_Hook, (LPVOID*)&FinishSaveGame);
    HookHelper::ApplyHookReplaceable((void*)addr_LoadSaveFile, &LoadSaveFile_Hook, (LPVOID*)&LoadSaveFile);
}
