#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

bool(__thiscall* GetDeviceObjectName)(int, int, LPWSTR) = nullptr;
int(__thiscall* GetDeviceObjectDesc)(int, unsigned int, wchar_t*, unsigned int*) = nullptr;

static bool FindDIKInTable(int thisPtr, uint8_t dikCode, unsigned int* ret)
{
    int KB_DIK_Table = MemoryHelper::ReadMemory<int>(thisPtr + 0xC);
    int tableStart = MemoryHelper::ReadMemory<int>(KB_DIK_Table + 0x10);
    int tableEnd = MemoryHelper::ReadMemory<int>(KB_DIK_Table + 0x14);

    while (tableStart < tableEnd)
    {
        if (MemoryHelper::ReadMemory<uint8_t>(tableStart + 0x1) == dikCode)
        {
            *ret = MemoryHelper::ReadMemory<int>(tableStart + 0x1C);
            return true;
        }
        tableStart += 0x20;
    }
    return false;
}

// ========================
// FixKeyboardInputLanguage
// ========================

static bool __fastcall GetDeviceObjectName_Hook(int thisPtr, int, int keyIndex, LPWSTR lpWideCharStr)
{
    bool result = GetDeviceObjectName(thisPtr, keyIndex, lpWideCharStr);

    if (!result || wcslen(lpWideCharStr) != 1)
        return result;

    int tableBase = MemoryHelper::ReadMemory<int>(thisPtr + 0x10);
    uint8_t dikCode = MemoryHelper::ReadMemory<uint8_t>(tableBase + 32 * keyIndex + 0x1);

    HKL layout = GetKeyboardLayout(0);
    UINT vk = MapVirtualKeyEx(dikCode, MAPVK_VSC_TO_VK, layout);
    if (vk == 0) return result;

    BYTE keyState[256] = {};
    wchar_t buf[4] = {};
    int chars = ToUnicodeEx(vk, dikCode, keyState, buf, 4, 0, layout);

    if (chars < 0)
    {
        ToUnicodeEx(vk, dikCode, keyState, buf, 4, 0, layout);
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

static int __fastcall GetDeviceObjectDesc_Hook(int thisPtr, int, unsigned int DeviceType, wchar_t* KeyName, unsigned int* ret)
{
    if (g_State.isLoadingDefault && DeviceType == 0) // Initialization of the keyboard layout
    {
        // Index to DirectInput Key Id mapping
        static const std::unordered_map<int, unsigned int> keyMap =
        {
            {0, 0x11},   // W
            {1, 0x1F},   // S
            {2, 0x1E},   // A
            {3, 0x20},   // D
            {4, 0xCB},   // Left
            {5, 0xCD},   // Right
            {6, 0x9D},   // Right Ctrl
            {8, 0x39},   // Space
            {9, 0x2E},   // C
            {10, 0x10},  // Q
            {11, 0x12},  // E
            {13, 0x22},  // G
            {14, 0x21},  // F
            {15, 0x13},  // R
            {16, 0x2A},  // Shift
            {17, 0x1D},  // Ctrl
            {18, 0x2D},  // X
            {19, 0x0F},  // Tab
            {20, 0x32},  // M
            {21, 0x14},  // T
            {22, 0x15},  // Y
            {23, 0x2F},  // V
            {24, 0xC8},  // Up
            {25, 0xD0},  // Down
            {26, 0xCF},  // End
            {31, 0x30},  // B
            {36, 0x2C},  // Z
            {37, 0x23}   // H
        };

        auto it = keyMap.find(g_State.currentKeyIndex);
        g_State.currentKeyIndex++;

        if (it != keyMap.end() && FindDIKInTable(thisPtr, static_cast<uint8_t>(it->second), ret))
        {
            return 0;
        }
    }

    if (DeviceType == 0 && KeyName && wcslen(KeyName) == 1)
    {
        wchar_t ch = KeyName[0];
        if ((ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z'))
        {
            BYTE vk = static_cast<BYTE>(towupper(ch));

            HKL layout = GetKeyboardLayout(0);
            UINT scanCode = MapVirtualKeyEx(vk, MAPVK_VK_TO_VSC, layout);

            if (scanCode != 0 && FindDIKInTable(thisPtr, static_cast<uint8_t>(scanCode), ret))
            {
                return 0;
            }
        }
    }

    int result = GetDeviceObjectDesc(thisPtr, DeviceType, KeyName, ret);

    // Fallback for non-ASCII single chars the original couldn't resolve
    if (result != 61 || DeviceType != 0 || !KeyName || wcslen(KeyName) != 1)
    {
        return result;
    }

    wchar_t ch = KeyName[0];
    HKL layout = GetKeyboardLayout(0);
    SHORT vkScan = VkKeyScanExW(ch, layout);

    if (vkScan != -1)
    {
        UINT scanCode = MapVirtualKeyEx(LOBYTE(vkScan), MAPVK_VK_TO_VSC, layout);
        if (scanCode != 0 && FindDIKInTable(thisPtr, static_cast<uint8_t>(scanCode), ret))
        {
            return 0;
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
