#include "../Core/Core.hpp"
#include "MinHook.hpp"
#include "../helper.hpp"

namespace ScreenJoystickHook
{
    // Local settings
    static uint8_t g_RumbleEnabled = 0;
    static uint8_t g_GyroEnabled = 0;
    static uint32_t g_GyroType = 0;
    static int g_GyroSensitivityInt = 50;
    static int g_GyroSmoothingInt = 16;
    static uint8_t g_TouchpadEnabled = 1;
    static int g_ControllerSensitivityInt = 50;
    static int g_EdgeAccelerationInt = 43;

    // Address storage
    static uintptr_t CreateTitleByName_Addr = 0;
    static uintptr_t AddControl_Addr = 0;
    static uintptr_t AddToggleByName_Addr = 0;
    static uintptr_t AddSlider_Addr = 0;
    static uintptr_t AddCycleByName_Addr = 0;
    static uintptr_t SetOnString_Addr = 0;
    static uintptr_t SetOffString_Addr = 0;
    static uintptr_t BaseScreenBuild_Addr = 0;
    static uintptr_t UseBack_Addr = 0;
    static uintptr_t FrameConstructor_Addr = 0;
    static uintptr_t FrameCreate_Addr = 0;
    static uintptr_t TextureMgr_Addr = 0;

    static bool(__thiscall* CScreenJoystick_Build)(int*) = nullptr;

    using AddString_t = void(__thiscall*)(int*, const wchar_t*);
    using CreateTexRef_t = void(__thiscall*)(int, int*, const char*);

    // =========================================================================
    // Wrapper Functions
    // =========================================================================

    static void __stdcall CallCreateTitleByName(int* screen, const wchar_t* title)
    {
        __asm
        {
            push title
            mov ecx, screen
            call CreateTitleByName_Addr
        }
    }

    static __int16 __stdcall CallAddControl(int* screen, int* control)
    {
        __int16 result = 0;
        __asm
        {
            mov ecx, screen
            push control
            call AddControl_Addr
            mov result, ax
        }

        return result;
    }

    static int* __stdcall CallAddToggleByName(int* screen, const wchar_t* label, int structData[16], int fontSize)
    {
        int* result = nullptr;

        __asm
        {
            push fontSize
            push 0
            push 0

            mov eax, structData
            push dword ptr[eax + 60]
            push dword ptr[eax + 56]
            push dword ptr[eax + 52]
            push dword ptr[eax + 48]
            push dword ptr[eax + 44]
            push dword ptr[eax + 40]
            push dword ptr[eax + 36]
            push dword ptr[eax + 32]
            push dword ptr[eax + 28]
            push dword ptr[eax + 24]
            push dword ptr[eax + 20]
            push dword ptr[eax + 16]
            push dword ptr[eax + 12]
            push dword ptr[eax + 8]
            push dword ptr[eax + 4]
            push dword ptr[eax]

            push label

            mov ecx, screen
            call AddToggleByName_Addr
            mov result, eax
        }

        return result;
    }

    static int* __stdcall CallAddSlider(int* screen, const char* labelStringID, int sliderStruct[25], int fontSize)
    {
        int* result = nullptr;

        __asm
        {
            push fontSize
            push 0
            push 0

            mov eax, sliderStruct
            push dword ptr[eax + 96]
            push dword ptr[eax + 92]
            push dword ptr[eax + 88]
            push dword ptr[eax + 84]
            push dword ptr[eax + 80]
            push dword ptr[eax + 76]
            push dword ptr[eax + 72]
            push dword ptr[eax + 68]
            push dword ptr[eax + 64]
            push dword ptr[eax + 60]
            push dword ptr[eax + 56]
            push dword ptr[eax + 52]
            push dword ptr[eax + 48]
            push dword ptr[eax + 44]
            push dword ptr[eax + 40]
            push dword ptr[eax + 36]
            push dword ptr[eax + 32]
            push dword ptr[eax + 28]
            push dword ptr[eax + 24]
            push dword ptr[eax + 20]
            push dword ptr[eax + 16]
            push dword ptr[eax + 12]
            push dword ptr[eax + 8]
            push dword ptr[eax + 4]
            push dword ptr[eax]

            push labelStringID

            mov ecx, screen
            call AddSlider_Addr
            mov result, eax
        }

        return result;
    }

    static int* __stdcall CallAddCycleByName(int* screen, const wchar_t* label, int structData[15], int fontSize)
    {
        int* result = nullptr;

        __asm
        {
            push fontSize
            push 0
            push 0

            mov eax, structData
            push dword ptr[eax + 56]
            push dword ptr[eax + 52]
            push dword ptr[eax + 48]
            push dword ptr[eax + 44]
            push dword ptr[eax + 40]
            push dword ptr[eax + 36]
            push dword ptr[eax + 32]
            push dword ptr[eax + 28]
            push dword ptr[eax + 24]
            push dword ptr[eax + 20]
            push dword ptr[eax + 16]
            push dword ptr[eax + 12]
            push dword ptr[eax + 8]
            push dword ptr[eax + 4]
            push dword ptr[eax]

            push label

            mov ecx, screen
            call AddCycleByName_Addr
            mov result, eax
        }

        return result;
    }

    static void __stdcall CallSetOnString(int* toggle, const wchar_t* str)
    {
        __asm
        {
            push str
            mov ecx, toggle
            call SetOnString_Addr
        }
    }

    static void __stdcall CallSetOffString(int* toggle, const wchar_t* str)
    {
        __asm
        {
            push str
            mov ecx, toggle
            call SetOffString_Addr
        }
    }

    static bool __stdcall CallBaseScreenBuild(int* screen)
    {
        bool result = false;
        __asm
        {
            mov ecx, screen
            call BaseScreenBuild_Addr
            mov result, al
        }

        return result;
    }

    static void __stdcall CallUseBack(int* screen, char a2, char a3, char a4)
    {
        __asm
        {
            mov ecx, screen
            movzx eax, a4
            push eax
            movzx eax, a3
            push eax
            movzx eax, a2
            push eax
            call UseBack_Addr
        }
    }

    static int* CreateFrame(int* screen, int left, int top, int right, int bottom)
    {
        const char* texturePath = "Interface\\menu\\frame_n.dds";

        int* pTexMgr = (int*)TextureMgr_Addr;
        if (!pTexMgr || !*pTexMgr)
            return nullptr;

        int texMgr = *pTexMgr;
        int texMgrVtable = *(int*)texMgr;

        int textureRef = 0;
        CreateTexRef_t createTexRef = (CreateTexRef_t)(*(int*)(texMgrVtable + 4));
        createTexRef(texMgr, &textureRef, texturePath);

        if (!textureRef)
            return nullptr;

        void* frame = operator new(0x4C8);
        if (!frame)
            return nullptr;

        void* constructed = nullptr;
        __asm
        {
            mov ecx, frame
            call FrameConstructor_Addr
            mov constructed, eax
        }

        int frameStruct[13];
        memset(frameStruct, 0, sizeof(frameStruct));
        frameStruct[5] = left;
        frameStruct[6] = top;
        frameStruct[7] = right;
        frameStruct[8] = bottom;
        frameStruct[9] = 1;
        frameStruct[10] = 0x3E800000;
        frameStruct[11] = 0x3F99999A;
        frameStruct[12] = 0x3F99999A;

        __asm
        {
            push 0
            lea eax, frameStruct
            push eax
            push textureRef
            mov ecx, constructed
            call FrameCreate_Addr
        }

        CallAddControl(screen, (int*)constructed);
        return (int*)constructed;
    }

    // =========================================================================
    // Logic & Hooks
    // =========================================================================

    static void InitFromGlobals()
    {
        g_RumbleEnabled = RumbleEnabled ? 1 : 0;
        g_GyroEnabled = GyroEnabled ? 1 : 0;
        g_GyroType = (uint32_t)GyroAimingMode;
        g_TouchpadEnabled = TouchpadEnabled ? 1 : 0;

        // GPadAimSensitivity: 0.0-4.0 -> 0-100
        g_ControllerSensitivityInt = (int)(GPadAimSensitivity * 25.0f);
        if (g_ControllerSensitivityInt > 100) g_ControllerSensitivityInt = 100;
        if (g_ControllerSensitivityInt < 0) g_ControllerSensitivityInt = 0;

        // GPadAimEdgeMultiplier: 1.0-2.4 -> 0-100
        g_EdgeAccelerationInt = (int)((GPadAimEdgeMultiplier - 1.0f) / 1.4f * 100.0f);
        if (g_EdgeAccelerationInt > 100) g_EdgeAccelerationInt = 100;
        if (g_EdgeAccelerationInt < 0) g_EdgeAccelerationInt = 0;

        // GyroSensitivity: 0.0-2.0 -> 0-100
        g_GyroSensitivityInt = (int)(GyroSensitivity * 50.0f);
        if (g_GyroSensitivityInt > 100) g_GyroSensitivityInt = 100;
        if (g_GyroSensitivityInt < 0) g_GyroSensitivityInt = 0;

        // GyroSmoothing: 0.0-0.1 -> 0-100
        g_GyroSmoothingInt = (int)(GyroSmoothing * 1000.0f);
        if (g_GyroSmoothingInt > 100) g_GyroSmoothingInt = 100;
        if (g_GyroSmoothingInt < 0) g_GyroSmoothingInt = 0;
    }

    static bool __fastcall CScreenJoystick_Build_Hook(int* thisPtr, int)
    {
        InitFromGlobals();

        uint32_t* pThis = (uint32_t*)thisPtr;

        int screenLeft = pThis[60];
        int screenTop = pThis[61];
        int screenRight = pThis[62];
        int screenBottom = pThis[63];

        CallCreateTitleByName(thisPtr, L"Controller");

        int kGap = 200;
        int kWidth = 200;
        int fontSize = 16;
        int numControls = 8;

        int frameLeft = screenLeft + kGap - 5;
        int frameTop = screenTop - 5;
        int frameRight = screenLeft + kGap + kWidth + 10;
        int frameBottom = screenTop + (fontSize + 8) * numControls + 13;

        CreateFrame(thisPtr, frameLeft, frameTop, frameRight, frameBottom);

        pThis[69] = pThis[60];
        pThis[70] = pThis[61];

        int toggleStruct[16];
        memset(toggleStruct, 0, sizeof(toggleStruct));
        toggleStruct[7] = kGap + kWidth;
        toggleStruct[8] = fontSize;
        toggleStruct[9] = 1;
        toggleStruct[10] = 0x3E800000;
        toggleStruct[11] = 0x3F99999A;
        toggleStruct[12] = 0x3F99999A;
        toggleStruct[13] = kGap;

        int sliderStruct[25];
        memset(sliderStruct, 0, sizeof(sliderStruct));
        sliderStruct[7] = kGap + kWidth;
        sliderStruct[8] = fontSize;
        sliderStruct[9] = 1;
        sliderStruct[10] = 0x3E800000;
        sliderStruct[11] = 0x3F99999A;
        sliderStruct[12] = 0x3F99999A;
        sliderStruct[15] = kGap;
        sliderStruct[19] = 3;

        int cycleStruct[15];
        memset(cycleStruct, 0, sizeof(cycleStruct));
        cycleStruct[7] = kGap + kWidth;
        cycleStruct[8] = fontSize;
        cycleStruct[9] = 1;
        cycleStruct[10] = 0x3E800000;
        cycleStruct[11] = 0x3F99999A;
        cycleStruct[12] = 0x3F99999A;
        cycleStruct[13] = kGap;

        // 1. Controller Sensitivity
        sliderStruct[1] = (int)"IDS_HELP_CONTROLLER_SENSITIVITY";
        sliderStruct[17] = 0;
        sliderStruct[18] = 100;
        sliderStruct[20] = (int)&g_ControllerSensitivityInt;
        CallAddSlider(thisPtr, "IDS_CONTROLLER_SENSITIVITY", sliderStruct, fontSize);

        // 2. Edge Acceleration
        sliderStruct[1] = (int)"IDS_HELP_EDGE_ACCELERATION";
        sliderStruct[17] = 0;
        sliderStruct[18] = 100;
        sliderStruct[20] = (int)&g_EdgeAccelerationInt;
        CallAddSlider(thisPtr, "IDS_EDGE_ACCELERATION", sliderStruct, fontSize);

        // 3. Rumble
        toggleStruct[1] = (int)"IDS_HELP_RUMBLE";
        toggleStruct[15] = (int)&g_RumbleEnabled;
        int* rumbleToggle = CallAddToggleByName(thisPtr, L"Rumble", toggleStruct, fontSize);
        if (rumbleToggle)
        {
            CallAddControl(thisPtr, rumbleToggle);
            CallSetOnString(rumbleToggle, L"On");
            CallSetOffString(rumbleToggle, L"Off");
        }

        // 4. Gyro Enabled
        toggleStruct[1] = (int)"IDS_HELP_GYRO_ENABLED";
        toggleStruct[15] = (int)&g_GyroEnabled;
        int* gyroToggle = CallAddToggleByName(thisPtr, L"Gyro", toggleStruct, fontSize);
        if (gyroToggle)
        {
            CallAddControl(thisPtr, gyroToggle);
            CallSetOnString(gyroToggle, L"On");
            CallSetOffString(gyroToggle, L"Off");
        }

        // 5. Gyro Type
        cycleStruct[1] = (int)"IDS_HELP_GYRO_TYPE";
        cycleStruct[14] = (int)&g_GyroType;
        int* gyroTypeCycle = CallAddCycleByName(thisPtr, L"Gyro Type", cycleStruct, fontSize);
        if (gyroTypeCycle)
        {
            CallAddControl(thisPtr, gyroTypeCycle);
            auto vtable = *(uintptr_t*)gyroTypeCycle;
            AddString_t addStringFn = (AddString_t)(*(uintptr_t*)(vtable + 220));
            addStringFn(gyroTypeCycle, L"Always On");
            addStringFn(gyroTypeCycle, L"Aiming Only");
            addStringFn(gyroTypeCycle, L"Hip Fire Only");
        }

        // 6. Gyro Sensitivity
        sliderStruct[1] = (int)"IDS_HELP_GYRO_SENSITIVITY";
        sliderStruct[17] = 0;
        sliderStruct[18] = 100;
        sliderStruct[20] = (int)&g_GyroSensitivityInt;
        CallAddSlider(thisPtr, "IDS_GYRO_SENSITIVITY", sliderStruct, fontSize);

        // 7. Gyro Smoothing
        sliderStruct[1] = (int)"IDS_HELP_GYRO_SMOOTHING";
        sliderStruct[17] = 0;
        sliderStruct[18] = 100;
        sliderStruct[20] = (int)&g_GyroSmoothingInt;
        CallAddSlider(thisPtr, "IDS_GYRO_SMOOTHING", sliderStruct, fontSize);

        // 8. Touchpad
        toggleStruct[1] = (int)"IDS_HELP_TOUCHPAD";
        toggleStruct[15] = (int)&g_TouchpadEnabled;
        int* touchpadToggle = CallAddToggleByName(thisPtr, L"Touchpad", toggleStruct, fontSize);
        if (touchpadToggle)
        {
            CallAddControl(thisPtr, touchpadToggle);
            CallSetOnString(touchpadToggle, L"On");
            CallSetOffString(touchpadToggle, L"Off");
        }

        bool buildResult = CallBaseScreenBuild(thisPtr);

        if (!buildResult)
            return false;

        CallUseBack(thisPtr, 1, 1, 0);
        return true;
    }

    // =========================================================================
    // Initialization
    // =========================================================================

    static bool InitializeFunctionPointers()
    {
        DWORD addr_CreateTitleByName = ScanModuleSignature(g_State.GameClient, "83 EC 30 56 8B F1 8B 86 C8 00 00 00 DB 86 C8 00 00 00 85 C0", "CreateTitleByName");
        DWORD addr_AddControl = ScanModuleSignature(g_State.GameClient, "83 EC 08 53 55 56 8B F1 57 8D 44 24 1C", "AddControl");
        DWORD addr_AddToggleByName = ScanModuleSignature(g_State.GameClient, "68 44 01 00 00 8B E9 E8", "AddToggleByName", 2);
        DWORD addr_AddSlider = ScanModuleSignature(g_State.GameClient, "8B 44 24 74 8B 54 24 6C 56 57 50 8B F9", "AddSlider");
        DWORD addr_AddCycleByName = ScanModuleSignature(g_State.GameClient, "68 3C 01 00 00 8B E9 E8", "AddCycleByName", 2);
        DWORD addr_SetOnString = ScanModuleSignature(g_State.GameClient, "8B 81 E4 00 00 00 8B 4C 24 04 6A 00 83 C0 04", "SetOnString");
        DWORD addr_SetOffString = ScanModuleSignature(g_State.GameClient, "8B 89 E4 00 00 00 8B 44 24 04 8B 09 6A 00", "SetOffString");
        DWORD addr_BaseScreenBuild = ScanModuleSignature(g_State.GameClient, "55 56 57 6A 00 8B F1 6A 00 C6 46 05 01 E8 ", "BaseScreenBuild");
        DWORD addr_UseBack = ScanModuleSignature(g_State.GameClient, "53 8A 5C 24 08 84 DB 56 8B F1 74 56 8B 44 24 14", "UseBack");
        DWORD addr_FrameConstructor = ScanModuleSignature(g_State.GameClient, "53 56 57 8B F9 E8 ?? ?? ?? ?? C7 07 ?? ?? ?? ?? 8D 77 54", "FrameConstructor");
        DWORD addr_FrameCreate = ScanModuleSignature(g_State.GameClient, "56 57 8B 7C 24 0C 85 FF 8B F1 75 07 5F 32 C0 5E C2 0C 00 8B 86 B4 03 00 00", "FrameCreate");

        if (addr_CreateTitleByName == 0 ||
            addr_AddControl == 0 ||
            addr_AddToggleByName == 0 ||
            addr_AddSlider == 0 ||
            addr_AddCycleByName == 0 ||
            addr_SetOnString == 0 ||
            addr_SetOffString == 0 ||
            addr_BaseScreenBuild == 0 ||
            addr_UseBack == 0 ||
            addr_FrameConstructor == 0 ||
            addr_FrameCreate == 0)
        {
            return false;
        }

        CreateTitleByName_Addr = addr_CreateTitleByName;
        AddControl_Addr = addr_AddControl;
        AddToggleByName_Addr = addr_AddToggleByName;
        AddSlider_Addr = addr_AddSlider;
        AddCycleByName_Addr = addr_AddCycleByName;
        SetOnString_Addr = addr_SetOnString;
        SetOffString_Addr = addr_SetOffString;
        BaseScreenBuild_Addr = addr_BaseScreenBuild;
        UseBack_Addr = addr_UseBack;
        FrameConstructor_Addr = addr_FrameConstructor;
        FrameCreate_Addr = addr_FrameCreate;

        TextureMgr_Addr = MemoryHelper::ReadMemory<int>(addr_AddSlider + 0x49);
        if (TextureMgr_Addr == 0) return false;

        return true;
    }

    static void InstallHook()
    {
        if (InitializeFunctionPointers())
        {
            DWORD addr_CScreenJoystick_Build = ScanModuleSignature(g_State.GameClient, "81 EC 6C 04 00 00 53 55 56 57", "CScreenJoystick_Build");

            if (addr_CScreenJoystick_Build)
            {
                HookHelper::ApplyHook((void*)addr_CScreenJoystick_Build, &CScreenJoystick_Build_Hook, (LPVOID*)&CScreenJoystick_Build);
            }
        }
    }
}