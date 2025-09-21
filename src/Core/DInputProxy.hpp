#pragma once

#include <Windows.h>

struct dinput8
{
    FARPROC DirectInput8Create;
    FARPROC DllCanUnloadNow;
    FARPROC DllGetClassObject;
    FARPROC DllRegisterServer;
    FARPROC DllUnregisterServer;
    FARPROC GetdfDIJoystick;

    bool ProxySetup(HINSTANCE hL)
    {
        DirectInput8Create = GetProcAddress(hL, "DirectInput8Create");
        DllCanUnloadNow = GetProcAddress(hL, "DllCanUnloadNow");
        DllGetClassObject = GetProcAddress(hL, "DllGetClassObject");
        DllRegisterServer = GetProcAddress(hL, "DllRegisterServer");
        DllUnregisterServer = GetProcAddress(hL, "DllUnregisterServer");
        GetdfDIJoystick = GetProcAddress(hL, "GetdfDIJoystick");
        return DirectInput8Create != nullptr;
    }
};

extern dinput8 g_dinput8;

extern "C" 
{
    void Hook_DirectInput8Create();
    void Hook_DllCanUnloadNow();
    void Hook_DllGetClassObject();
    void Hook_DllRegisterServer();
    void Hook_DllUnregisterServer();
    void Hook_GetdfDIJoystick();
}