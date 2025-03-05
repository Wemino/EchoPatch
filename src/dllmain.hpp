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

        if (!DirectInput8Create)
        {
            return false;
        }

        return true;
    }
} dinput8;

__declspec(naked) void Hook_DirectInput8Create() { _asm { jmp[dinput8.DirectInput8Create] } }
__declspec(naked) void Hook_DllCanUnloadNow() { _asm { jmp[dinput8.DllCanUnloadNow] } }
__declspec(naked) void Hook_DllGetClassObject() { _asm { jmp[dinput8.DllGetClassObject] } }
__declspec(naked) void Hook_DllRegisterServer() { _asm { jmp[dinput8.DllRegisterServer] } }
__declspec(naked) void Hook_DllUnregisterServer() { _asm { jmp[dinput8.DllUnregisterServer] } }
__declspec(naked) void Hook_GetdfDIJoystick() { _asm { jmp[dinput8.GetdfDIJoystick] } }