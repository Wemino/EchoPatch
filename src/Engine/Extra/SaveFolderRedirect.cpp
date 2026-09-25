#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

HRESULT(WINAPI* ori_SHGetFolderPathA)(HWND, int, HANDLE, DWORD, LPSTR);

static HRESULT WINAPI SHGetFolderPathA_Hook(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath)
{
    BOOL shouldRedirect = (csidl == (CSIDL_COMMON_DOCUMENTS | CSIDL_FLAG_CREATE));

    HRESULT hr = ori_SHGetFolderPathA(hwnd, shouldRedirect ? (CSIDL_MYDOCUMENTS | CSIDL_FLAG_CREATE) : csidl, hToken, dwFlags, pszPath);

    if (SUCCEEDED(hr) && shouldRedirect)
    {
        if (PathAppendA(pszPath, "My Games") == FALSE)
        {
            return E_FAIL;
        }
    }

    return hr;
}

static void ApplySaveFolderRedirect()
{
    if (!RedirectSaveFolder) return;

    HookHelper::ApplyHookAPI(L"shell32.dll", "SHGetFolderPathA", &SHGetFolderPathA_Hook, (LPVOID*)&ori_SHGetFolderPathA);
}
