#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

struct SaveBuffer
{
    HANDLE handle = INVALID_HANDLE_VALUE;
    std::vector<uint8_t> buffer{};
    LONGLONG position = 0;
    LONGLONG size = 0;
    bool flushed = false;

    void Reset()
    {
        handle = INVALID_HANDLE_VALUE;
        buffer.clear();
        position = 0;
        size = 0;
        flushed = false;
    }

    bool IsActive() const { return handle != INVALID_HANDLE_VALUE; }
};

static SaveBuffer saveBuffer{};

bool(__thiscall* FileWrite)(DWORD*, LPCVOID, DWORD) = nullptr;
bool(__thiscall* FileOpen)(DWORD*, LPCSTR, char) = nullptr;
bool(__thiscall* FileSeek)(DWORD*, LARGE_INTEGER) = nullptr;
bool(__thiscall* FileSeekEnd)(DWORD*) = nullptr;
bool(__thiscall* FileTell)(DWORD*, DWORD*) = nullptr;
bool(__thiscall* FileClose)(DWORD*) = nullptr;

// =========================
// OptimizeSaveSpeed
// =========================

static bool __fastcall FileWrite_Hook(DWORD* thisp, int, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite)
{
    HANDLE h = reinterpret_cast<HANDLE>(thisp[1]);

    if (saveBuffer.handle == h && !saveBuffer.flushed)
    {
        LONGLONG endPos = saveBuffer.position + nNumberOfBytesToWrite;

        if (endPos > (LONGLONG)saveBuffer.buffer.size())
        {
            saveBuffer.buffer.resize((size_t)endPos, 0);
        }

        memcpy(saveBuffer.buffer.data() + saveBuffer.position, lpBuffer, nNumberOfBytesToWrite);
        saveBuffer.position = endPos;

        if (endPos > saveBuffer.size)
        {
            saveBuffer.size = endPos;
        }

        return true;
    }

    return FileWrite(thisp, lpBuffer, nNumberOfBytesToWrite);
}

static bool __fastcall FileOpen_Hook(DWORD* thisp, int, LPCSTR lpFileName, char a3)
{
    bool result = FileOpen(thisp, lpFileName, a3);

    if (result && lpFileName)
    {
        HANDLE h = reinterpret_cast<HANDLE>(thisp[1]);

        if (h != INVALID_HANDLE_VALUE)
        {
            std::string_view filename(lpFileName);

            if (filename.ends_with(".sav"))
            {
                if (saveBuffer.IsActive())
                {
                    saveBuffer.Reset();
                }

                saveBuffer.handle = h;
                saveBuffer.position = 0;
                saveBuffer.size = 0;
                saveBuffer.flushed = false;
                saveBuffer.buffer.resize(6 * 1024 * 1024);
            }
        }
    }

    return result;
}

static bool __fastcall FileSeek_Hook(DWORD* thisp, int, LARGE_INTEGER liDistanceToMove)
{
    HANDLE h = reinterpret_cast<HANDLE>(thisp[1]);

    if (saveBuffer.handle == h && !saveBuffer.flushed)
    {
        if (liDistanceToMove.QuadPart < 0)
            return false;

        saveBuffer.position = liDistanceToMove.QuadPart;
        return true;
    }

    return FileSeek(thisp, liDistanceToMove);
}

static bool __fastcall FileSeekEnd_Hook(DWORD* thisp, int)
{
    HANDLE h = reinterpret_cast<HANDLE>(thisp[1]);

    if (saveBuffer.handle == h && !saveBuffer.flushed)
    {
        saveBuffer.position = saveBuffer.size;
        return true;
    }

    return FileSeekEnd(thisp);
}

static bool __fastcall FileTell_Hook(DWORD* thisp, int, DWORD* a2)
{
    HANDLE h = reinterpret_cast<HANDLE>(thisp[1]);

    if (saveBuffer.handle == h && !saveBuffer.flushed)
    {
        a2[0] = (DWORD)saveBuffer.position;
        a2[1] = (DWORD)(saveBuffer.position >> 32);
        return 1;
    }

    return FileTell(thisp, a2);
}

static bool __fastcall FileClose_Hook(DWORD* thisp, int)
{
    HANDLE h = reinterpret_cast<HANDLE>(thisp[1]);

    if (saveBuffer.handle == h)
    {
        if (!saveBuffer.flushed && saveBuffer.size > 0)
        {
            saveBuffer.flushed = true;

            LARGE_INTEGER zero = { 0 };
            FileSeek(thisp, zero);
            FileWrite(thisp, saveBuffer.buffer.data(), (DWORD)saveBuffer.size);
            SetEndOfFile(h);
        }

        saveBuffer.Reset();
    }

    return FileClose(thisp);
}

static void ApplyOptimizeSaveSpeed()
{
    if (!OptimizeSaveSpeed) return;

    const bool checkRegion = g_State.CurrentFEARGame == FEAR;

    HookHelper::ApplyHook((void*)GetAddress(Addr::FileWrite), &FileWrite_Hook, (LPVOID*)&FileWrite, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::FileOpen), &FileOpen_Hook, (LPVOID*)&FileOpen, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::FileSeek), &FileSeek_Hook, (LPVOID*)&FileSeek, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::FileSeekEnd), &FileSeekEnd_Hook, (LPVOID*)&FileSeekEnd, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::FileTell), &FileTell_Hook, (LPVOID*)&FileTell, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::FileClose), &FileClose_Hook, (LPVOID*)&FileClose, checkRegion);
}
