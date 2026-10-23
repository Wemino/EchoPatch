#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

bool(__thiscall* GetDeviceObjectName)(int, int, LPWSTR) = nullptr;
int(__thiscall* GetDeviceObjectDesc)(int, unsigned int, wchar_t*, unsigned int*) = nullptr;

static uintptr_t keyboardNamesDevice = 0;
static HKL keyboardNamesLayout = nullptr;
static std::vector<std::wstring> keyboardNames;

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

static wchar_t GetLayoutChar(uint8_t dikCode, HKL layout)
{
    UINT vk = MapVirtualKeyEx(dikCode, MAPVK_VSC_TO_VK, layout);
    if (vk == 0) return 0;

    BYTE keyState[256] = {};
    wchar_t buf[4] = {};
    int chars = ToUnicodeEx(vk, dikCode, keyState, buf, 4, 0x4, layout);

    if (chars < 0)
    {
        // Clears the dead key on older Windows versions, which ignore 0x4
        ToUnicodeEx(vk, dikCode, keyState, buf, 4, 0x4, layout);
        return 0;
    }

    return chars == 1 ? buf[0] : 0;
}

static bool IsInAnsiCodePage(wchar_t ch)
{
    if (GetACP() == CP_UTF8)
        return true;

    char converted[8];
    BOOL usedDefaultChar = FALSE;
    int size = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, &ch, 1, converted, sizeof(converted), nullptr, &usedDefaultChar);
    return size > 0 && !usedDefaultChar;
}

// Key FindKeyboardObject finds for a name, before its fallback for other characters
static int FindKeyboardName(uintptr_t keyboard, const std::wstring& name, HKL layout)
{
    if (name.size() == 1)
    {
        wchar_t ch = name[0];
        if ((ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z'))
        {
            UINT scanCode = MapVirtualKeyEx(static_cast<BYTE>(towupper(ch)), MAPVK_VK_TO_VSC, layout);
            unsigned int object = 0;

            if (scanCode != 0 && FindDIKInTable(keyboard, static_cast<uint8_t>(scanCode), &object))
                return static_cast<int>(object);
        }
    }

    for (size_t i = 0; i < keyboardNames.size(); i++)
    {
        if (CompareStringOrdinal(keyboardNames[i].c_str(), -1, name.c_str(), -1, TRUE) == CSTR_EQUAL)
            return static_cast<int>(i);
    }
    return -1;
}

// Gives every key a profile name that loads back to that key with the active layout
static void BuildKeyboardNames(uintptr_t keyboard, HKL layout)
{
    keyboardNamesDevice = keyboard;
    keyboardNamesLayout = layout;
    keyboardNames.clear();

    uintptr_t tableStart = MemoryHelper::ReadMemory<uintptr_t>(keyboard + 0x10);
    uintptr_t tableEnd = MemoryHelper::ReadMemory<uintptr_t>(keyboard + 0x14);
    size_t count = tableStart ? (tableEnd - tableStart) / 0x20 : 0;

    for (size_t i = 0; i < count; i++)
    {
        wchar_t desc[530] = L"";
        GetDeviceObjectName(static_cast<int>(keyboard), static_cast<int>(i), desc);
        std::wstring name = desc;

        // DirectInput names are ANSI, a character missing from the code page gets a lossy name (Đ is named D)
        if (name.size() == 1)
        {
            wchar_t ch = GetLayoutChar(MemoryHelper::ReadMemory<uint8_t>(tableStart + 0x20 * i + 0x1), layout);
            if (ch != 0 && !IsInAnsiCodePage(ch))
                name.assign(1, static_cast<wchar_t>(towupper(ch)));
        }

        keyboardNames.push_back(name);
    }

    // A name that loads another key (numpad - and -) gets the key's scan code added
    for (size_t i = 0; i < count; i++)
    {
        if (!keyboardNames[i].empty() && FindKeyboardName(keyboard, keyboardNames[i], layout) == static_cast<int>(i))
            continue;

        wchar_t suffix[16];
        swprintf_s(suffix, _countof(suffix), L" (0x%02X)", MemoryHelper::ReadMemory<uint8_t>(tableStart + 0x20 * i + 0x1));
        keyboardNames[i] = (keyboardNames[i].empty() ? L"Key" : keyboardNames[i]) + suffix;
    }
}

static bool __fastcall GetDeviceObjectName_Hook(int thisPtr, int, int keyIndex, LPWSTR lpWideCharStr)
{
    bool result = GetDeviceObjectName(thisPtr, keyIndex, lpWideCharStr);

    if (!result || MemoryHelper::ReadMemory<int>(thisPtr + 0x4) != 0)
        return result;

    uintptr_t keyboard = static_cast<uintptr_t>(thisPtr);
    uintptr_t tableStart = MemoryHelper::ReadMemory<uintptr_t>(keyboard + 0x10);
    uintptr_t tableEnd = MemoryHelper::ReadMemory<uintptr_t>(keyboard + 0x14);
    size_t count = tableStart ? (tableEnd - tableStart) / 0x20 : 0;
    HKL layout = GetKeyboardLayout(0);

    if (keyboard != keyboardNamesDevice || layout != keyboardNamesLayout || count != keyboardNames.size())
        BuildKeyboardNames(keyboard, layout);

    if (keyIndex >= 0 && static_cast<size_t>(keyIndex) < keyboardNames.size())
        wcsncpy_s(lpWideCharStr, 261, keyboardNames[keyIndex].c_str(), _TRUNCATE);

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
        g_State.isDefaultKeyUnresolved = result != 0;

        // Placeholder so the game still calls SetBinding, which drops it if the command has no default key
        if (result != 0)
        {
            *ret = 0;
            result = 0;
        }

        g_State.defaultKeyInputMgr = thisPtr;
        g_State.defaultKeyDevice = DeviceType;
        g_State.defaultKeyObject = *ret;
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