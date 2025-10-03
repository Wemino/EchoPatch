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

		for (size_t i = 0; i < signature.length(); ++i)
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

	// Reference: https://github.com/nipkownix/re4_tweaks/blob/master/dllmain/LAApatch.cpp
	inline bool PerformLAAPatch(HMODULE hModule)
	{
		if (!hModule)
		{
			MessageBox(NULL, L"Invalid module handle", L"EchoPatch - LAAPatcher Error", MB_ICONERROR);
			return false;
		}

		// Get the module filename
		char moduleFilePathA[MAX_PATH];
		if (!GetModuleFileNameA(hModule, moduleFilePathA, MAX_PATH))
		{
			char errorMsg[256];
			sprintf_s(errorMsg, "Failed to get module filename. Error: %d", GetLastError());
			MessageBoxA(NULL, errorMsg, "EchoPatch - LAAPatcher Error", MB_ICONERROR);
			return false;
		}

		// Check if LAA flag is already set
		bool isLAA = false;
		bool hasBindSection = false;
		PIMAGE_SECTION_HEADER textSectionHeader = nullptr;
		DWORD textSectionRVA = 0;
		DWORD textSectionSize = 0;
		char executableName[MAX_PATH];

		// Extract the executable name for later use
		const char* lastSlash = strrchr(moduleFilePathA, '\\');
		if (lastSlash)
		{
			strcpy_s(executableName, sizeof(executableName), lastSlash + 1);
		}
		else
		{
			strcpy_s(executableName, sizeof(executableName), moduleFilePathA);
		}

		{
			std::ifstream file(moduleFilePathA, std::ios::binary);
			if (!file.is_open())
			{
				MessageBoxA(NULL, "Failed to open executable file for checking", "EchoPatch - LAAPatcher Error", MB_ICONERROR);
				return false;
			}

			std::vector<uint8_t> fileBuffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			file.close();

			if (fileBuffer.empty())
			{
				MessageBoxA(NULL, "Failed to read file or file is empty", "EchoPatch - LAAPatcher Error", MB_ICONERROR);
				return false;
			}

			PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(fileBuffer.data());
			if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
			{
				MessageBoxA(NULL, "Invalid DOS header", "EchoPatch - LAAPatcher Error", MB_ICONERROR);
				return false;
			}

			PIMAGE_NT_HEADERS ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(fileBuffer.data() + dosHeader->e_lfanew);
			if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
			{
				MessageBoxA(NULL, "Invalid NT header", "EchoPatch - LAAPatcher Error", MB_ICONERROR);
				return false;
			}

			isLAA = (ntHeaders->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) != 0;

			// Check for .bind section and locate .text section
			PIMAGE_SECTION_HEADER sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
			for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++)
			{
				char sectionName[9] = { 0 };
				memcpy(sectionName, sectionHeader[i].Name, 8);

				if (strcmp(sectionName, ".bind") == 0)
				{
					hasBindSection = true;
				}

				if (strcmp(sectionName, ".text") == 0)
				{
					textSectionRVA = sectionHeader[i].VirtualAddress;
					textSectionSize = sectionHeader[i].Misc.VirtualSize;
				}
			}
		}

		if (isLAA && !hasBindSection)
		{
			return true; // No need to modify
		}

		// If SteamDRM (.bind section) is detected
		if (hasBindSection)
		{
			char steamDrmMessage[512];
			sprintf_s(steamDrmMessage,
				"SteamDRM protection has been detected in '%s'.\n\n"
				"You have two options:\n"
				"- Press YES to continue with the current patcher (less reliable)\n"
				"- Press NO to abort and use Steamless to remove the DRM first (recommended)\n\n"
				"Using Steamless for proper unpacking before applying the LAA patch is recommended for better compatibility.",
				executableName);

			int result = MessageBoxA(NULL, steamDrmMessage, "EchoPatch - LAA Patcher", MB_YESNO | MB_ICONWARNING);
			if (result == IDNO)
			{
				// User chose to use Steamless instead
				MessageBoxA(NULL,
					"Operation aborted. Please use Steamless first to unpack the executable, then run this patcher again.",
					"EchoPatch - Operation Cancelled",
					MB_ICONINFORMATION);
				return false;
			}
			// If user pressed Yes, continue with current patcher
		}

		// Setup file paths
		std::string modulePath = moduleFilePathA;
		std::string modulePathNew = modulePath + ".new";
		std::string modulePathBak = modulePath + ".bak";

		// Clean up any existing temporary files
		if (GetFileAttributesA(modulePathNew.c_str()) != INVALID_FILE_ATTRIBUTES)
		{
			DeleteFileA(modulePathNew.c_str());
		}

		if (GetFileAttributesA(modulePathBak.c_str()) != INVALID_FILE_ATTRIBUTES)
		{
			DeleteFileA(modulePathBak.c_str());
		}

		// Copy the original file
		if (!CopyFileA(modulePath.c_str(), modulePathNew.c_str(), FALSE))
		{
			DWORD error = GetLastError();
			if (error == ERROR_ACCESS_DENIED)
			{
				MessageBoxA(NULL, "Administrator rights are required to modify the executable. Please run the application as administrator.", "EchoPatch - LAAPatcher Admin Rights Required", MB_ICONWARNING);
				return false;
			}

			char errorMsg[256];
			sprintf_s(errorMsg, "Failed to create a copy of the executable. Error: %d", error);
			MessageBoxA(NULL, errorMsg, "EchoPatch - LAAPatcher Error", MB_ICONERROR);
			return false;
		}

		// Open the copied file for patching
		FILE* file;
		int errorCode = fopen_s(&file, modulePathNew.c_str(), "rb+");
		if (errorCode != 0)
		{
			char errorMsg[256];
			sprintf_s(errorMsg, "Failed to open file for patching. Error: %d", errorCode);
			MessageBoxA(NULL, errorMsg, "EchoPatch - LAAPatcher Error", MB_ICONERROR);
			DeleteFileA(modulePathNew.c_str());
			return false;
		}

		// Read the file into memory
		fseek(file, 0, SEEK_END);
		size_t fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);

		std::vector<uint8_t> exeData(fileSize);
		if (fread(exeData.data(), 1, fileSize, file) != fileSize)
		{
			MessageBoxA(NULL, "Failed to read file data", "EchoPatch - LAAPatcher Error", MB_ICONERROR);
			fclose(file);
			DeleteFileA(modulePathNew.c_str());
			return false;
		}

		// Modify the PE header
		PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(exeData.data());
		PIMAGE_NT_HEADERS ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(exeData.data() + dosHeader->e_lfanew);

		// Set LAA flag
		ntHeaders->FileHeader.Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;

		// Set checksum to 0 and skip calculation
		ntHeaders->OptionalHeader.CheckSum = 0;

		// Handle .bind section if present
		if (hasBindSection)
		{
			// Set the AddressOfEntryPoint to 0x13E428 for F.E.A.R. (Steam)
			ntHeaders->OptionalHeader.AddressOfEntryPoint = 0x13E428;

			PIMAGE_SECTION_HEADER sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
			int bindSectionIndex = -1;
			PIMAGE_SECTION_HEADER textSection = nullptr;

			// Find the .bind and .text sections
			for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++)
			{
				char sectionName[9] = { 0 };
				memcpy(sectionName, sectionHeader[i].Name, 8);

				if (strcmp(sectionName, ".bind") == 0)
				{
					bindSectionIndex = i;
				}

				if (strcmp(sectionName, ".text") == 0)
				{
					textSection = &sectionHeader[i];
				}
			}

			if (bindSectionIndex >= 0 && textSection)
			{
				// Copy the decrypted .text section from memory to the file
				BYTE* textSectionInMemory = reinterpret_cast<BYTE*>(hModule) + textSection->VirtualAddress;

				// Write the in-memory .text section to the file
				fseek(file, textSection->PointerToRawData, SEEK_SET);
				if (fwrite(textSectionInMemory, 1, textSection->SizeOfRawData, file) != textSection->SizeOfRawData)
				{
					MessageBoxA(NULL, "Failed to write .text section from memory", "EchoPatch - LAAPatcher Error", MB_ICONERROR);
					fclose(file);
					DeleteFileA(modulePathNew.c_str());
					return false;
				}

				// 'Remove' the .bind section by updating the section count
				ntHeaders->FileHeader.NumberOfSections--;
			}
		}

		// Write modified headers back to file
		fseek(file, 0, SEEK_SET);
		if (fwrite(exeData.data(), 1, dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS), file) != dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS))
		{
			MessageBoxA(NULL, "Failed to write modified headers", "EchoPatch - LAAPatcher Error", MB_ICONERROR);
			fclose(file);
			DeleteFileA(modulePathNew.c_str());
			return false;
		}

		// Write section headers
		PIMAGE_SECTION_HEADER sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
		fseek(file, dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS), SEEK_SET);
		if (fwrite(sectionHeader, sizeof(IMAGE_SECTION_HEADER), ntHeaders->FileHeader.NumberOfSections, file) !=
			ntHeaders->FileHeader.NumberOfSections)
		{
			MessageBoxA(NULL, "Failed to write section headers", "EchoPatch - LAAPatcher Error", MB_ICONERROR);
			fclose(file);
			DeleteFileA(modulePathNew.c_str());
			return false;
		}

		// Close the file
		fflush(file);
		fclose(file);

		// Replace the original file with the patched one
		BOOL moveResult1 = MoveFileExA(modulePath.c_str(), modulePathBak.c_str(), MOVEFILE_REPLACE_EXISTING);
		if (!moveResult1)
		{
			DWORD error = GetLastError();
			if (error == ERROR_ACCESS_DENIED)
			{
				MessageBoxA(NULL, "Administrator rights are required to replace the executable. Please run the application as administrator.", "EchoPatch - LAAPatcher Admin Rights Required", MB_ICONWARNING);
				DeleteFileA(modulePathNew.c_str());
				return false;
			}

			char errorMsg[256];
			sprintf_s(errorMsg, "Failed to backup original file. Error: %d", error);
			MessageBoxA(NULL, errorMsg, "EchoPatch - LAAPatcher Error", MB_ICONERROR);
			DeleteFileA(modulePathNew.c_str());
			return false;
		}

		BOOL moveResult2 = MoveFileA(modulePathNew.c_str(), modulePath.c_str());
		if (!moveResult2)
		{
			char errorMsg[256];
			sprintf_s(errorMsg, "Failed to replace original file. Error: %d", GetLastError());
			MessageBoxA(NULL, errorMsg, "EchoPatch - LAAPatcher Error", MB_ICONERROR);

			// Try to restore the original file
			MoveFileA(modulePathBak.c_str(), modulePath.c_str());
			return false;
		}

		MessageBoxA(NULL, "Successfully patched the executable with LAA flag (a backup has been created).\n\nPress OK to relaunch the application for the patch to take effect!", "EchoPatch - LAAPatcher Successful", MB_ICONINFORMATION);

		wchar_t moduleFilePathW[MAX_PATH];
		MultiByteToWideChar(CP_ACP, 0, moduleFilePathA, -1, moduleFilePathW, MAX_PATH);

		// Relaunch the application
		if ((int)ShellExecuteW(NULL, L"open", moduleFilePathW, NULL, NULL, SW_SHOWDEFAULT) > 32)
		{
			// Exit the current instance
			ExitProcess(0);
		}
		else
		{
			MessageBoxA(NULL, "Failed to restart the application. Please restart it manually.", "EchoPatch - LAAPatcher Warning", MB_ICONWARNING);
			return true;
		}
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
