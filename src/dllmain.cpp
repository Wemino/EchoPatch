#include "Core/Core.hpp"

#pragma comment(lib, "libMinHook.x86.lib")
#pragma comment(lib, "Xinput.lib")

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        return OnProcessAttach(hModule);
    if (ul_reason_for_call == DLL_PROCESS_DETACH)
        OnProcessDetach();
    return TRUE;
}