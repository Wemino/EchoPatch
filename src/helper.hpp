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

	DWORD64 PatternScan(HMODULE hModule, std::string_view signature)
	{
		auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
		if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
			return 0;

		auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<BYTE*>(hModule) + dosHeader->e_lfanew);

		if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
			return 0;

		DWORD sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
		DWORD64 baseAddress = reinterpret_cast<DWORD64>(hModule);

		// Convert pattern to byte array
		std::vector<uint8_t> patternBytes;
		std::vector<bool> mask;

		for (size_t i = 0; i < signature.length(); ++i)
		{
			if (signature[i] == ' ')
				continue;

			if (signature[i] == '?')
			{
				patternBytes.push_back(0);
				mask.push_back(true);

				if (i + 1 < signature.length() && signature[i + 1] == '?')
				{
					i++;
				}
			}
			else
			{
				patternBytes.push_back(static_cast<uint8_t>(std::strtol(signature.data() + i, nullptr, 16)));
				mask.push_back(false);
				i++;
			}
		}

		size_t patternSize = patternBytes.size();
		BYTE* data = reinterpret_cast<BYTE*>(baseAddress);

		for (DWORD64 i = 0; i <= sizeOfImage - patternSize; ++i)
		{
			size_t j = 0;

			while (j < patternSize && (mask[j] || data[i + j] == patternBytes[j]))
			{
				j++;
			}

			if (j == patternSize)
			{
				return baseAddress + i;
			}
		}

		return 0;
	}
};

namespace SystemHelper
{
	void SimulateSpacebarPress(HWND phWnd)
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

	bool FileExists(const std::string& path)
	{
		struct stat buffer;
		return (stat(path.c_str(), &buffer) == 0);
	}

	static void LoadProxyLibrary()
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
					if (dinput8.ProxySetup(hChain))
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
		dinput8.ProxySetup(hOriginal);
	}
};

namespace IniHelper
{
	std::unique_ptr<mINI::INIFile> iniFile;
	mINI::INIStructure iniReader;

	void Init(bool lookUp)
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

	void Save()
	{
		if (iniFile)
		{
			iniFile->write(iniReader);
		}
	}

	char* ReadString(const char* sectionName, const char* valueName, const char* defaultValue)
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

				strncpy(result, value.c_str(), 254);
				result[254] = '\0';
				return result;
			}
		}
		catch (...) {}

		strncpy(result, defaultValue, 254);
		result[254] = '\0';
		return result;
	}

	float ReadFloat(const char* sectionName, const char* valueName, float defaultValue)
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

	int ReadInteger(const char* sectionName, const char* valueName, int defaultValue)
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
	bool isMHInitialized = false;

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

	static void ApplyHook(void* addr, LPVOID hookFunc, LPVOID* originalFunc)
	{
		if (!InitializeMinHook()) return;

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
			return;
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
			return;
		}
	}

};

namespace StringHelper
{
	const char* IntegerToCString(int value) 
	{
		static thread_local char buffer[32];
		snprintf(buffer, sizeof(buffer), "%d", value);
		return buffer;
	}

	const char* FloatToCString(float value) 
	{
		static thread_local char buffer[32];
		snprintf(buffer, sizeof(buffer), "%.0f", value);
		return buffer;
	}

	const char* BoolToCString(bool value)
	{
		return value ? "1" : "0";
	}

	bool stricmp(const char* str1, const char* str2) 
	{
		if (!str1 || !str2) 
		{
			return false;
		}

		while (*str1 && *str2) 
		{
			if (tolower(static_cast<unsigned char>(*str1)) != tolower(static_cast<unsigned char>(*str2))) 
			{
				return false;
			}
			++str1;
			++str2;
		}

		return *str1 == '\0' && *str2 == '\0';
	}
};