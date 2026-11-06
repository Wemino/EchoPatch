#pragma once

#include "../Globals.cpp"

namespace ScreenHelper
{
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
    static uintptr_t Default_Addr = 0;
    static uintptr_t OperatorNew_Addr = 0;
    static uintptr_t CreateTextItem_Addr = 0;

    using AddString_t = void(__thiscall*)(int*, const wchar_t*);
    using CreateTexRef_t = void(__thiscall*)(int, int*, const char*);
    using ReleaseTexRef_t = void(__thiscall*)(int, int);
    using OperatorNew_t = void* (__cdecl*)(size_t);
    using ScreenInit_t = bool(__thiscall*)(int*, int);
    using ScreenTerm_t = void(__thiscall*)(int*);
    using ScreenUpdateData_t = void(__thiscall*)(int*, bool);
    using SetCurrentScreen_t = bool(__thiscall*)(int, int);

    // CBaseScreen virtual functions
    static constexpr int VTBL_SCREEN_ON_COMMAND = 1;
    static constexpr int VTBL_SCREEN_INIT = 2;
    static constexpr int VTBL_SCREEN_TERM = 3;
    static constexpr int VTBL_SCREEN_ON_FOCUS = 8;
    static constexpr int VTBL_SCREEN_UPDATE_DATA = 26;

    // CScreenMgr virtual functions
    static constexpr int VTBL_SCREENMGR_SET_CURRENT_SCREEN = 17;

    // CBaseScreen members
    static constexpr int SCREEN_BUILT_OFFSET = 0x5;
    static constexpr int SCREEN_LAYOUT_INDEX = 2;
    static constexpr int SCREEN_MGR_INDEX = 3;
    static constexpr int SCREEN_ID_INDEX = 4;
    static constexpr int SCREEN_ITEM_SPACING_INDEX = 57;
    static constexpr int SCREEN_RECT_LEFT_INDEX = 60;
    static constexpr int SCREEN_RECT_TOP_INDEX = 61;
    static constexpr int SCREEN_DEFAULT_POS_X_INDEX = 69;
    static constexpr int SCREEN_DEFAULT_POS_Y_INDEX = 70;

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

    static int* __stdcall CallCreateTextItem(int* screen, const wchar_t* label, int structData[13], int fontSize)
    {
        int* result = nullptr;

        __asm
        {
            push fontSize
            push 0
            push 0

            mov eax, structData
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
            call CreateTextItem_Addr
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

    static void ReleaseTextureRef(int textureRef)
    {
        if (!textureRef)
            return;

        int* pTexMgr = (int*)TextureMgr_Addr;
        if (!pTexMgr || !*pTexMgr)
            return;

        int texMgr = *pTexMgr;
        int texMgrVtable = *(int*)texMgr;
        ReleaseTexRef_t releaseTexRef = (ReleaseTexRef_t)(*(int*)(texMgrVtable + 0x10));
        releaseTexRef(texMgr, textureRef);
    }

    static int CreateFrame(int* screen, int left, int top, int right, int bottom)
    {
        const char* texturePath = "Interface\\menu\\frame_n.dds";

        int* pTexMgr = (int*)TextureMgr_Addr;
        if (!pTexMgr || !*pTexMgr)
            return 0;

        int texMgr = *pTexMgr;
        int texMgrVtable = *(int*)texMgr;

        int textureRef = 0;
        CreateTexRef_t createTexRef = (CreateTexRef_t)(*(int*)(texMgrVtable + 4));
        createTexRef(texMgr, &textureRef, texturePath);

        if (!textureRef)
            return 0;

        void* frame = nullptr;
        size_t allocSz = 0x4C8;
        __asm
        {
            push allocSz
            mov eax, OperatorNew_Addr
            call eax
            add esp, 4
            mov frame, eax
        }

        if (!frame)
        {
            ReleaseTextureRef(textureRef);
            return 0;
        }

        void* constructed = nullptr;
        __asm
        {
            mov ecx, frame
            call FrameConstructor_Addr
            mov constructed, eax
        }

        int frameStruct[13];
        frameStruct[0] = 0;
        frameStruct[1] = (int)Default_Addr;
        frameStruct[2] = 0;
        frameStruct[3] = 0;
        frameStruct[4] = 0;
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
        return textureRef;
    }

    // =========================================================================
    // Screen Helpers
    // =========================================================================

    static uintptr_t GetVirtualFunction(const void* object, int index)
    {
        return (*reinterpret_cast<uintptr_t* const*>(object))[index];
    }

    static bool IsScreenBuilt(int* screen)
    {
        return *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(screen) + SCREEN_BUILT_OFFSET);
    }

    static void CallScreenUpdateData(int* screen, bool saveAndValidate)
    {
        reinterpret_cast<ScreenUpdateData_t>(GetVirtualFunction(screen, VTBL_SCREEN_UPDATE_DATA))(screen, saveAndValidate);
    }

    static void InitControlStruct(int* structData, int count, int width, int fontSize)
    {
        memset(structData, 0, count * sizeof(int));
        structData[7] = width;
        structData[8] = fontSize;
        structData[9] = 1;
        structData[10] = 0x3E800000;
        structData[11] = 0x3F99999A;
        structData[12] = 0x3F99999A;
    }

    static int* AddTextItem(int* screen, const wchar_t* label, const char* helpId, uint32_t commandId, uint32_t param, int width, int fontSize)
    {
        int textStruct[13];
        InitControlStruct(textStruct, 13, width, fontSize);
        textStruct[0] = commandId;
        textStruct[1] = (int)helpId;
        textStruct[3] = param;

        int* textItem = CallCreateTextItem(screen, label, textStruct, fontSize);
        if (textItem)
        {
            CallAddControl(screen, textItem);
        }

        return textItem;
    }

    // =========================================================================
    // Initialization
    // =========================================================================

    static bool s_isInitialized = false;
    static bool s_isAvailable = false;

    // Shared by the menus, the signatures are only scanned once
    static bool Initialize()
    {
        if (s_isInitialized)
            return s_isAvailable;

        s_isInitialized = true;

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
        DWORD addr_DefaultAddr = ScanModuleSignature(g_State.GameClient, "C7 84 24 8C 00 00 00 ?? ?? ?? ?? 89 9C 24 90 00 00 00", "DefaultAddr");
        DWORD addr_OperatorNew = ScanModuleSignature(g_State.GameClient, "56 8B 74 24 08 EB 11", "OperatorNew", -1, false);

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
            addr_FrameCreate == 0 ||
            addr_DefaultAddr == 0 ||
            addr_OperatorNew == 0)
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
        OperatorNew_Addr = addr_OperatorNew;

        Default_Addr = MemoryHelper::ReadMemory<int>(addr_DefaultAddr + 0x7);
        TextureMgr_Addr = MemoryHelper::ReadMemory<int>(addr_AddSlider + 0x49);

        // Only needed by the menus made of text items
        CreateTextItem_Addr = ScanModuleSignature(g_State.GameClient, "83 EC 30 56 57 68 EC 00 00 00 8B F1 E8", "CreateTextItem");

        s_isAvailable = true;
        return true;
    }
}