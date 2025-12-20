#pragma once

#include "ini.hpp"
#include "Core/DInputProxy.hpp"

namespace MemoryHelper
{
	template <typename T> static bool WriteMemory(uintptr_t address, T value, bool disableProtection = true)
	{
		DWORD oldProtect;
		if (disableProtection)
		{
			if (!VirtualProtect(reinterpret_cast <LPVOID> (address), sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtect)) return false;
		}
		*reinterpret_cast <T*> (address) = value;
		if (disableProtection)
		{
			VirtualProtect(reinterpret_cast <LPVOID> (address), sizeof(T), oldProtect, &oldProtect);
		}
		return true;
	}

	static bool WriteMemoryRaw(uintptr_t address, const void* data, size_t size, bool disableProtection = true)
	{
		DWORD oldProtect;
		if (disableProtection)
		{
			if (!VirtualProtect(reinterpret_cast <LPVOID> (address), size, PAGE_EXECUTE_READWRITE, &oldProtect)) return false;
		}
		std::memcpy(reinterpret_cast <void*> (address), data, size);
		if (disableProtection)
		{
			VirtualProtect(reinterpret_cast <LPVOID> (address), size, oldProtect, &oldProtect);
		}
		return true;
	}

	static bool MakeNOP(uintptr_t address, size_t count, bool disableProtection = true)
	{
		DWORD oldProtect;
		if (disableProtection)
		{
			if (!VirtualProtect(reinterpret_cast <LPVOID> (address), count, PAGE_EXECUTE_READWRITE, &oldProtect)) return false;
		}
		std::memset(reinterpret_cast <void*> (address), 0x90, count);
		if (disableProtection)
		{
			VirtualProtect(reinterpret_cast <LPVOID> (address), count, oldProtect, &oldProtect);
		}
		return true;
	}

	static bool MakeCALL(uintptr_t srcAddress, uintptr_t destAddress, bool disableProtection = true)
	{
		DWORD oldProtect;
		if (disableProtection)
		{
			if (!VirtualProtect(reinterpret_cast <LPVOID> (srcAddress), 5, PAGE_EXECUTE_READWRITE, &oldProtect)) return false;
		}
		uintptr_t relativeAddress = destAddress - srcAddress - 5; *reinterpret_cast <uint8_t*> (srcAddress) = 0xE8; // CALL opcode
		*reinterpret_cast <uintptr_t*> (srcAddress + 1) = relativeAddress;
		if (disableProtection)
		{
			VirtualProtect(reinterpret_cast <LPVOID> (srcAddress), 5, oldProtect, &oldProtect);
		}
		return true;
	}

	static bool MakeJMP(uintptr_t srcAddress, uintptr_t destAddress, bool disableProtection = true)
	{
		DWORD oldProtect;
		if (disableProtection)
		{
			if (!VirtualProtect(reinterpret_cast <LPVOID> (srcAddress), 5, PAGE_EXECUTE_READWRITE, &oldProtect)) return false;
		}
		uintptr_t relativeAddress = destAddress - srcAddress - 5; *reinterpret_cast <uint8_t*> (srcAddress) = 0xE9; // JMP opcode
		*reinterpret_cast <uintptr_t*> (srcAddress + 1) = relativeAddress;
		if (disableProtection)
		{
			VirtualProtect(reinterpret_cast <LPVOID> (srcAddress), 5, oldProtect, &oldProtect);
		}
		return true;
	}

	template <typename T> static T ReadMemory(uintptr_t address, bool disableProtection = false)
	{
		DWORD oldProtect;
		if (disableProtection)
		{
			if (!VirtualProtect(reinterpret_cast <LPVOID> (address), sizeof(T), PAGE_EXECUTE_READ, &oldProtect)) return T();
		}
		T value = *reinterpret_cast <T*> (address);
		if (disableProtection)
		{
			VirtualProtect(reinterpret_cast <LPVOID> (address), sizeof(T), oldProtect, &oldProtect);
		}
		return value;
	}

	inline DWORD PatternScan(HMODULE hModule, std::string_view signature)
	{
		auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
		if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
			return 0;

		auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<BYTE*>(hModule) + dosHeader->e_lfanew);
		if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
			return 0;

		DWORD sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
		BYTE* baseAddress = reinterpret_cast<BYTE*>(hModule);

		// Parse pattern
		std::vector<uint8_t> patternBytes;
		std::vector<bool> mask;

		for (size_t i = 0; i < signature.length(); i++)
		{
			if (signature[i] == ' ') continue;

			if (signature[i] == '?')
			{
				patternBytes.push_back(0);
				mask.push_back(true);
				if (i + 1 < signature.length() && signature[i + 1] == '?')
					i++;
			}
			else
			{
				auto hexChar = [](char c) noexcept -> uint8_t 
				{
					if (c >= '0' && c <= '9') return c - '0';
					if (c >= 'A' && c <= 'F') return c - 'A' + 10;
					if (c >= 'a' && c <= 'f') return c - 'a' + 10;
					return 0;
				};

				uint8_t byte = (hexChar(signature[i]) << 4) | hexChar(signature[i + 1]);
				patternBytes.push_back(byte);
				mask.push_back(false);
				i++;
			}
		}

		size_t patternSize = patternBytes.size();
		if (patternSize == 0) return reinterpret_cast<DWORD>(baseAddress);

		// Find first non-wildcard for optimized search
		size_t firstCheck = 0;
		while (firstCheck < patternSize && mask[firstCheck])
			firstCheck++;

		if (firstCheck == patternSize)
			return reinterpret_cast<DWORD>(baseAddress);

		// Use memchr with bounds checking
		BYTE* scanStart = baseAddress;
		BYTE* scanEnd = baseAddress + sizeOfImage - patternSize;
		uint8_t firstByte = patternBytes[firstCheck];

		while (scanStart <= scanEnd)
		{
			// Find next candidate using optimized memchr
			BYTE* candidate = reinterpret_cast<BYTE*>(std::memchr(scanStart + firstCheck, firstByte, (scanEnd - scanStart) - firstCheck + 1));

			if (!candidate) break;

			candidate -= firstCheck;

			// Verify full pattern
			bool found = true;
			for (size_t j = 0; j < patternSize; ++j)
			{
				if (!mask[j] && candidate[j] != patternBytes[j])
				{
					found = false;
					break;
				}
			}

			if (found)
				return reinterpret_cast<DWORD>(candidate);

			scanStart = candidate + 1;
		}

		return 0;
	}

	inline DWORD FindSignatureAddress(HMODULE Module, std::string_view Signature, int FunctionStartCheckCount = -1)
	{
		DWORD Address = static_cast<DWORD>(PatternScan(Module, Signature));
		if (Address == 0) 
			return 0;

		if (FunctionStartCheckCount >= 0)
		{
			// After a RET, compilers pad with INT3 (0xCC) to align the next function (often on a 16-byte boundary).
			// This padding also acts as a breakpoint if execution runs off the end of a function.
			// We backtrack past any 0xCC bytes to find the real function start.
			for (DWORD ScanAddress = Address; ScanAddress > Address - 0x1000; ScanAddress--)
			{
				bool IsValid = true;
				for (int OffsetIndex = 1; OffsetIndex <= FunctionStartCheckCount; OffsetIndex++)
				{
					if (ReadMemory<uint8_t>(ScanAddress - OffsetIndex) != 0xCC)
					{
						IsValid = false;
						break;
					}
				}
				if (IsValid)
					return ScanAddress;
			}
		}

		return Address;
	}

	inline DWORD ResolveRelativeAddress(uintptr_t BaseAddress, std::size_t InstructionOffset)
	{
		int RelativeOffset = ReadMemory<int>(BaseAddress + InstructionOffset);
		return BaseAddress + InstructionOffset + sizeof(RelativeOffset) + RelativeOffset;
	}
};

namespace SystemHelper
{
	inline void SimulateSpacebarPress(HWND phWnd)
	{
		if (phWnd)
		{
			PostMessage(phWnd, WM_KEYDOWN, VK_SPACE, 0);
			PostMessage(phWnd, WM_KEYUP, VK_SPACE, 0);
		}
	}

	static DWORD GetCurrentDisplayFrequency()
	{
		DEVMODE devMode = {};
		devMode.dmSize = sizeof(DEVMODE);

		if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode))
		{
			return devMode.dmDisplayFrequency;
		}
		return 60;
	}

	static std::pair<DWORD, DWORD> GetScreenResolution()
	{
		DEVMODE devMode = {};
		devMode.dmSize = sizeof(DEVMODE);

		if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode))
		{
			return { devMode.dmPelsWidth, devMode.dmPelsHeight };
		}
		return { 0, 0 };
	}

	inline bool FileExists(const std::string& path)
	{
		struct stat buffer;
		return (stat(path.c_str(), &buffer) == 0);
	}

	inline void LoadProxyLibrary()
	{
		// Attempt to load the chain-load DLL from the game's directory
		wchar_t modulePath[MAX_PATH];
		if (GetModuleFileNameW(NULL, modulePath, MAX_PATH))
		{
			wchar_t* lastBackslash = wcsrchr(modulePath, L'\\');
			if (lastBackslash != NULL)
			{
				*lastBackslash = L'\0';
				lstrcatW(modulePath, L"\\dinput8_hook.dll");

				HINSTANCE hChain = LoadLibraryExW(modulePath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
				if (hChain)
				{
					// Set up proxies to use the chain-loaded DLL
					if (g_dinput8.ProxySetup(hChain))
					{
						return; // Successfully chained
					}
					else
					{
						// Handle missing exports in chain DLL
						FreeLibrary(hChain);
						// Fall through to system DLL
					}
				}
			}
		}

		// Fallback to system dinput8.dll
		wchar_t systemPath[MAX_PATH];
		GetSystemDirectoryW(systemPath, MAX_PATH);
		lstrcatW(systemPath, L"\\dinput8.dll");

		HINSTANCE hOriginal = LoadLibraryExW(systemPath, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
		if (!hOriginal)
		{
			DWORD errorCode = GetLastError();
			wchar_t errorMessage[512];

			FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorCode, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), errorMessage, sizeof(errorMessage) / sizeof(wchar_t), NULL);
			MessageBoxW(NULL, errorMessage, L"Error Loading dinput8.dll", MB_ICONERROR);
			return;
		}

		// Set up proxies to system DLL
		g_dinput8.ProxySetup(hOriginal);
	}
};

namespace IniHelper
{
	inline std::unique_ptr<mINI::INIFile> iniFile;
	inline mINI::INIStructure iniReader;

	inline void Init(bool lookUp)
	{
		std::string iniPath = "EchoPatch.ini";
		if (!SystemHelper::FileExists(iniPath) && lookUp)
		{
			std::string parentPath = "..\\EchoPatch.ini";
			if (SystemHelper::FileExists(parentPath))
			{
				iniPath = parentPath;
			}
		}
		iniFile = std::make_unique<mINI::INIFile>(iniPath);
		iniFile->read(iniReader);
	}

	inline void Save()
	{
		if (iniFile)
		{
			iniFile->write(iniReader);
		}
	}

	inline char* ReadString(const char* sectionName, const char* valueName, const char* defaultValue)
	{
		char* result = new char[255];
		try
		{
			if (iniReader.has(sectionName) && iniReader.get(sectionName).has(valueName))
			{
				std::string value = iniReader.get(sectionName).get(valueName);

				if (!value.empty() && (value.front() == '\"' || value.front() == '\''))
					value.erase(0, 1);
				if (!value.empty() && (value.back() == '\"' || value.back() == '\''))
					value.erase(value.size() - 1);

					strncpy_s(result, 255, value.c_str(), _TRUNCATE);
					return result;
			}
		}
		catch (...) {}

			strncpy_s(result, 255, defaultValue, _TRUNCATE);
			return result;
	}

	inline float ReadFloat(const char* sectionName, const char* valueName, float defaultValue)
	{
		try
		{
			if (iniReader.has(sectionName) && iniReader.get(sectionName).has(valueName))
			{
				const std::string& s = iniReader.get(sectionName).get(valueName);
				if (!s.empty())
					return std::stof(s);
			}
		}
		catch (...) {}
		return defaultValue;
	}

	inline int ReadInteger(const char* sectionName, const char* valueName, int defaultValue)
	{
		try
		{
			if (iniReader.has(sectionName) && iniReader.get(sectionName).has(valueName))
			{
				const std::string& s = iniReader.get(sectionName).get(valueName);
				if (!s.empty())
					return std::stoi(s);
			}
		}
		catch (...) {}
		return defaultValue;
	}
};

namespace HookHelper
{
	inline bool isMHInitialized = false;

	static bool InitializeMinHook()
	{
		if (!isMHInitialized)
		{
			if (MH_Initialize() == MH_OK)
			{
				isMHInitialized = true;
			}
			else
			{
				MessageBoxA(NULL, "Failed to initialize MinHook!", "Error", MB_ICONERROR | MB_OK);
				return false;
			}
		}
		return true;
	}

	// FEAR: For the no-cd version that removes the EXECUTE flag
	static bool EnsureExecutable(void* addr)
	{
		MEMORY_BASIC_INFORMATION mbi;
		if (!VirtualQuery(addr, &mbi, sizeof(mbi))) return false;

		// Check if it already has any execution rights
		if (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) return true;

		// Add EXECUTE flag if needed
		DWORD oldProtect;
		return VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READWRITE, &oldProtect);
	}

	static void ApplyHook(void* addr, LPVOID hookFunc, LPVOID* originalFunc, bool checkRegion = false)
	{
		if (!InitializeMinHook()) return;

		if (checkRegion && !EnsureExecutable(addr))
		{
			char msg[0x100];
			sprintf_s(msg, "Could not make address %p executable.", addr);
			MessageBoxA(NULL, msg, "Error", MB_ICONERROR);
			return;
		}

		MH_STATUS status = MH_CreateHook(addr, hookFunc, originalFunc);
		if (status != MH_OK)
		{
			char errorMsg[0x100];
			sprintf_s(errorMsg, "Failed to create hook at address: %p\nError: %s", addr, MH_StatusToString(status));
			MessageBoxA(NULL, errorMsg, "Error", MB_ICONERROR | MB_OK);
			return;
		}

		status = MH_EnableHook(addr);
		if (status != MH_OK)
		{
			char errorMsg[0x100];
			sprintf_s(errorMsg, "Failed to enable hook at address: %p\nError: %s", addr, MH_StatusToString(status));
			MessageBoxA(NULL, errorMsg, "Error", MB_ICONERROR | MB_OK);
		}
	}

	static void ApplyHookAPI(LPCWSTR moduleName, LPCSTR apiName, LPVOID hookFunc, LPVOID* originalFunc)
	{
		if (!InitializeMinHook()) return;

		MH_STATUS status = MH_CreateHookApi(moduleName, apiName, hookFunc, originalFunc);
		if (status != MH_OK)
		{
			char errorMsg[0x100];
			sprintf_s(errorMsg, "Failed to create hook for API: %s\nError: %s", apiName, MH_StatusToString(status));
			MessageBoxA(NULL, errorMsg, "Error", MB_ICONERROR | MB_OK);
			return;
		}

		status = MH_EnableHook(MH_ALL_HOOKS);
		if (status != MH_OK)
		{
			char errorMsg[0x100];
			sprintf_s(errorMsg, "Failed to enable hook for API: %s\nError: %s", apiName, MH_StatusToString(status));
			MessageBoxA(NULL, errorMsg, "Error", MB_ICONERROR | MB_OK);
		}
	}
};
