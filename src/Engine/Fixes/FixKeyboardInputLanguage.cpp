#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

bool(__thiscall* GetDeviceObjectName)(int, int, LPWSTR) = nullptr;
int(__thiscall* GetDeviceObjectDesc)(int, unsigned int, wchar_t*, unsigned int*) = nullptr;

static uintptr_t GetKeyboardDevice(int inputMgr, unsigned int deviceIndex)
{
    uintptr_t devicesStart = MemoryHelper::ReadMemory<uintptr_t>(inputMgr + 0xC);
    uintptr_t devicesEnd = MemoryHelper::ReadMemory<uintptr_t>(inputMgr + 0x10);

    if (devicesStart == 0 || deviceIndex >= (devicesEnd - devicesStart) / 0x54)
        return 0;

    uintptr_t device = devicesStart + 0x54 * deviceIndex;
    return MemoryHelper::ReadMemory<int>(device + 0x4) == 0 ? device : 0;
}

static bool FindDIKInTable(uintptr_t keyboard, uint8_t dikCode, unsigned int* ret)
{
    uintptr_t tableStart = MemoryHelper::ReadMemory<uintptr_t>(keyboard + 0x10);
    uintptr_t tableEnd = MemoryHelper::ReadMemory<uintptr_t>(keyboard + 0x14);

    if (tableStart == 0)
        return false;

    for (uintptr_t entry = tableStart; entry < tableEnd; entry += 0x20)
    {
        if (MemoryHelper::ReadMemory<uint8_t>(entry + 0x1) == dikCode)
        {
            // Object index, same as the original lookup returns
            *ret = static_cast<unsigned int>((entry - tableStart) / 0x20);
            return true;
        }
    }
    return false;
}

// ========================
// FixKeyboardInputLanguage
// ========================

static bool __fastcall GetDeviceObjectName_Hook(int thisPtr, int, int keyIndex, LPWSTR lpWideCharStr)
{
    bool result = GetDeviceObjectName(thisPtr, keyIndex, lpWideCharStr);

    if (!result || MemoryHelper::ReadMemory<int>(thisPtr + 0x4) != 0 || wcslen(lpWideCharStr) != 1)
        return result;

    uintptr_t tableBase = MemoryHelper::ReadMemory<uintptr_t>(thisPtr + 0x10);
    uint8_t dikCode = MemoryHelper::ReadMemory<uint8_t>(tableBase + 32 * keyIndex + 0x1);

    HKL layout = GetKeyboardLayout(0);
    UINT vk = MapVirtualKeyEx(dikCode, MAPVK_VSC_TO_VK, layout);
    if (vk == 0) return result;

    BYTE keyState[256] = {};
    wchar_t buf[4] = {};
    int chars = ToUnicodeEx(vk, dikCode, keyState, buf, 4, 0x4, layout);

    if (chars < 0)
    {
        // Clears the dead key on older Windows versions, which ignore 0x4
        ToUnicodeEx(vk, dikCode, keyState, buf, 4, 0x4, layout);
        return result;
    }

    if (chars == 1 && buf[0] != L'\0')
    {
        wchar_t corrected = towupper(buf[0]);
        if (corrected != towupper(lpWideCharStr[0]))
        {
            lpWideCharStr[0] = corrected;
            lpWideCharStr[1] = L'\0';
        }
    }

    return result;
}

static int FindKeyboardObject(int thisPtr, uintptr_t keyboard, unsigned int DeviceType, wchar_t* KeyName, unsigned int* ret)
{
    if (wcslen(KeyName) == 1)
    {
        wchar_t ch = KeyName[0];
        if ((ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z'))
        {
            BYTE vk = static_cast<BYTE>(towupper(ch));

            HKL layout = GetKeyboardLayout(0);
            UINT scanCode = MapVirtualKeyEx(vk, MAPVK_VK_TO_VSC, layout);

            if (scanCode != 0 && FindDIKInTable(keyboard, static_cast<uint8_t>(scanCode), ret))
            {
                return 0;
            }
        }
    }

    int result = GetDeviceObjectDesc(thisPtr, DeviceType, KeyName, ret);

    // Fallback for non-ASCII single chars the original couldn't resolve
    if (result != 61 || wcslen(KeyName) != 1)
    {
        return result;
    }

    wchar_t ch = KeyName[0];
    HKL layout = GetKeyboardLayout(0);
    SHORT vkScan = VkKeyScanExW(ch, layout);

    if (vkScan != -1)
    {
        UINT scanCode = MapVirtualKeyEx(LOBYTE(vkScan), MAPVK_VK_TO_VSC, layout);
        if (scanCode != 0 && FindDIKInTable(keyboard, static_cast<uint8_t>(scanCode), ret))
        {
            return 0;
        }
    }

    return result;
}

static int __fastcall GetDeviceObjectDesc_Hook(int thisPtr, int, unsigned int DeviceType, wchar_t* KeyName, unsigned int* ret)
{
    uintptr_t keyboard = (KeyName && ret) ? GetKeyboardDevice(thisPtr, DeviceType) : 0;
    if (keyboard == 0)
    {
        return GetDeviceObjectDesc(thisPtr, DeviceType, KeyName, ret);
    }

    int result = FindKeyboardObject(thisPtr, keyboard, DeviceType, KeyName, ret);

    if (g_State.isLoadingDefault)
    {
        g_State.defaultKeyInputMgr = thisPtr;
        g_State.isDefaultKeyUnresolved = result != 0;

        // Placeholder so the game still calls SetBinding, which drops it if the command has no default key
        if (result != 0)
        {
            *ret = 0;
            result = 0;
        }
    }

    return result;
}

static void ApplyFixKeyboardInputLanguage()
{
    if (!FixKeyboardInputLanguage) return;

    const bool checkRegion = g_State.CurrentFEARGame == FEAR;

    HookHelper::ApplyHook((void*)GetAddress(Addr::GetDeviceObjectName), &GetDeviceObjectName_Hook, (LPVOID*)&GetDeviceObjectName, checkRegion);
    HookHelper::ApplyHook((void*)GetAddress(Addr::GetDeviceObjectDesc), &GetDeviceObjectDesc_Hook, (LPVOID*)&GetDeviceObjectDesc, checkRegion);
}
