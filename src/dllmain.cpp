#include "Globals.cpp"
#include "Addresses.cpp"

#include "Engine/Display/FixWindow.cpp"
#include "Engine/Extra/SaveFolderRedirect.cpp"
#include "Engine/Graphics/ReducedMipMapBias.cpp"
#include "Engine/Misc/ClientHook.cpp"
#include "Engine/Misc/ConsoleVariableHook.cpp"
#include "Engine/Fixes/FixKeyboardInputLanguage.cpp"
#include "Engine/Fixes/FixAspectRatioBlur.cpp"
#include "Engine/Fixes/HighFPSFixes.cpp"
#include "Engine/Fixes/HavokPhysicsFix.cpp"
#include "Engine/Graphics/DynamicVsync.cpp"
#include "Engine/Misc/MainLoop.cpp"
#include "Engine/Display/AutoResolution.cpp"
#include "Engine/SkipIntro/SkipIntro.cpp"
#include "Engine/Fixes/OptimizeSaveSpeed.cpp"
#include "Engine/Graphics/SSAAScale.cpp"
#include "Engine/Fixes/FixNvidiaShadowCorruption.cpp"
#include "Engine/Misc/DeviceCreationHook.cpp"
#include "Engine/Fixes/FixScriptedAnimationCrash.cpp"
#include "Engine/Console/ConsoleEnabled.cpp"
#include "Engine/Fixes/FastVRAMDetection.cpp"
#include "Engine/Fixes/FixDirectInputFps.cpp"
#include "Engine/Extra/DisablePunkBuster.cpp"
#include "Engine/Fixes/DisableJoystick.cpp"

#include "Controller/Controller.cpp"

#include "Server/Graphics/EnablePersistentWorldState.cpp"
#include "Server/Extra/EnableCustomMaxWeaponCapacity.cpp"
#include "Server/Fixes/HighFPSFixes.cpp"
#include "Server/Controller/Controller.cpp"
#include "Server/ServerPatch.cpp"

#include "ClientFX/Fixes/HighFPSFixes.cpp"
#include "ClientFX/Controller/Controller.cpp"
#include "ClientFX/ClientFXPatch.cpp"

#include "Client/Client.cpp"
#include "Client/Fixes/HighFPSFixes.cpp"
#include "Client/Fixes/FixKeyboardInputLanguage.cpp"
#include "Client/Fixes/WeaponFixes.cpp"
#include "Client/Graphics/HighResolutionReflections.cpp"
#include "Client/Graphics/EnablePersistentWorldState.cpp"
#include "Client/Display/HUDScaling.cpp"
#include "Client/Graphics/SSAAScale.cpp"
#include "Client/Display/AutoResolution.cpp"
#include "Client/Controller/SDLGamepadSupport.cpp"
#include "Client/Console/ConsoleEnabled.cpp"
#include "Client/Extra/EnableCustomMaxWeaponCapacity.cpp"
#include "Client/Extra/DisableHipFireAccuracyPenalty.cpp"
#include "Client/ClientPatch.cpp"

#include "Initialization.cpp"

#pragma comment(lib, "libMinHook.x86.lib")
#pragma comment(lib, "SDL3-static.lib")
#pragma comment(lib, "dxgi.lib")

bool OnProcessAttach(HMODULE hModule)
{
    // Prevents DLL from receiving thread notifications
    DisableThreadLibraryCalls(hModule);

    uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
    g_State.BaseAddress = base;
    IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)(base);
    IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
    DWORD timestamp = nt->FileHeader.TimeDateStamp;

    switch (timestamp)
    {
        case FEAR_TIMESTAMP:
            g_State.CurrentFEARGame = FEAR;
            break;
        case FEARMP_TIMESTAMP:
            g_State.CurrentFEARGame = FEARMP;
            break;
        case FEARXP_TIMESTAMP:
        case FEARXP_TIMESTAMP2:
            g_State.CurrentFEARGame = FEARXP;
            break;
        case FEARXP2_TIMESTAMP:
            g_State.CurrentFEARGame = FEARXP2;
            break;
        default:
            MessageBoxA(NULL, "This .exe is not supported.", "EchoPatch", MB_ICONERROR);
            return false;
    }

    if (!SystemHelper::IsUALPresent())
    {
        SystemHelper::LoadProxyLibrary();
    }

    HookHelper::ApplyHookAPI(L"user32.dll", "CreateWindowExA", &CreateWindowExA_Hook, (LPVOID*)&ori_CreateWindowExA);
    return true;
}

void OnProcessDetach()
{
    MH_Uninitialize();

    if (SDLGamepadSupport)
    {
        ShutdownSDLGamepad();
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        return OnProcessAttach(hModule);
    if (ul_reason_for_call == DLL_PROCESS_DETACH)
        OnProcessDetach();
    return TRUE;
}
