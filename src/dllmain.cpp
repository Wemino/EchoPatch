#include "Globals.cpp"
#include "Addresses.cpp"

#include "Engine/FixWindow/FixWindow.cpp"
#include "Engine/SaveFolderRedirect/SaveFolderRedirect.cpp"
#include "Engine/ReducedMipMapBias/ReducedMipMapBias.cpp"
#include "Engine/ClientHook/ClientHook.cpp"
#include "Engine/ConsoleVariableHook/ConsoleVariableHook.cpp"
#include "Engine/FixKeyboardInputLanguage/FixKeyboardInputLanguage.cpp"
#include "Engine/FixAspectRatioBlur/FixAspectRatioBlur.cpp"
#include "Engine/HighFPSFixes/HighFPSFixes.cpp"
#include "Engine/DynamicVsync/DynamicVsync.cpp"
#include "Engine/MainLoop/MainLoop.cpp"
#include "Engine/AutoResolution/AutoResolution.cpp"
#include "Engine/SkipIntro/SkipIntro.cpp"
#include "Engine/OptimizeSaveSpeed/OptimizeSaveSpeed.cpp"
#include "Engine/SSAAScale/SSAAScale.cpp"
#include "Engine/FixNvidiaShadowCorruption/FixNvidiaShadowCorruption.cpp"
#include "Engine/DeviceCreationHook/DeviceCreationHook.cpp"
#include "Engine/FixScriptedAnimationCrash/FixScriptedAnimationCrash.cpp"
#include "Engine/ConsoleEnabled/ConsoleEnabled.cpp"
#include "Engine/FastVRAMDetection/FastVRAMDetection.cpp"
#include "Engine/FixDirectInputFps/FixDirectInputFps.cpp"
#include "Engine/DisablePunkBuster/DisablePunkBuster.cpp"
#include "Engine/DisableJoystick/DisableJoystick.cpp"

#include "Controller/Controller.cpp"

#include "Server/EnablePersistentWorldState/EnablePersistentWorldState.cpp"
#include "Server/EnableCustomMaxWeaponCapacity/EnableCustomMaxWeaponCapacity.cpp"
#include "Server/HighFPSFixes/HighFPSFixes.cpp"
#include "Server/Controller/Controller.cpp"
#include "Server/ServerPatch.cpp"

#include "ClientFX/HighFPSFixes/HighFPSFixes.cpp"
#include "ClientFX/Controller/Controller.cpp"
#include "ClientFX/ClientFXPatch.cpp"

#include "Client/Client.cpp"
#include "Client/HighFPSFixes/HighFPSFixes.cpp"
#include "Client/FixKeyboardInputLanguage/FixKeyboardInputLanguage.cpp"
#include "Client/WeaponFixes/WeaponFixes.cpp"
#include "Client/HighResolutionReflections/HighResolutionReflections.cpp"
#include "Client/EnablePersistentWorldState/EnablePersistentWorldState.cpp"
#include "Client/HUDScaling/HUDScaling.cpp"
#include "Client/SSAAScale/SSAAScale.cpp"
#include "Client/AutoResolution/AutoResolution.cpp"
#include "Client/SDLGamepadSupport/SDLGamepadSupport.cpp"
#include "Client/ConsoleEnabled/ConsoleEnabled.cpp"
#include "Client/EnableCustomMaxWeaponCapacity/EnableCustomMaxWeaponCapacity.cpp"
#include "Client/DisableHipFireAccuracyPenalty/DisableHipFireAccuracyPenalty.cpp"
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
