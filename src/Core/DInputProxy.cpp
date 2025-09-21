#include "DInputProxy.hpp"

dinput8 g_dinput8{};

extern "C" 
{
	__declspec(naked) void Hook_DirectInput8Create() { __asm { jmp g_dinput8.DirectInput8Create } }
	__declspec(naked) void Hook_DllCanUnloadNow() { __asm { jmp g_dinput8.DllCanUnloadNow } }
	__declspec(naked) void Hook_DllGetClassObject() { __asm { jmp g_dinput8.DllGetClassObject } }
	__declspec(naked) void Hook_DllRegisterServer() { __asm { jmp g_dinput8.DllRegisterServer } }
	__declspec(naked) void Hook_DllUnregisterServer() { __asm { jmp g_dinput8.DllUnregisterServer } }
	__declspec(naked) void Hook_GetdfDIJoystick() { __asm { jmp g_dinput8.GetdfDIJoystick } }
}