#pragma once

#include <io.h>
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

	inline uint32_t MurmurHash3(const uint8_t* data, size_t size, uint32_t seed = 0)
	{
		uint32_t h1 = seed;
		const uint32_t c1 = 0xCC9E2D51;
		const uint32_t c2 = 0x1B873593;
		int remaining = (int)size & 3; // mod 4
		int numBlocks = (int)size >> 2; // div 4

		// Body
		for (int i = 0; i < numBlocks; i++)
		{
			uint32_t k1;
			memcpy(&k1, data + i * 4, sizeof(uint32_t));

			k1 *= c1;
			k1 = (k1 << 15) | (k1 >> 17);
			k1 *= c2;

			h1 ^= k1;
			h1 = (h1 << 13) | (h1 >> 19);
			h1 = h1 * 5 + 0xE6546B64;
		}

		// Tail
		if (remaining > 0)
		{
			uint32_t k1 = 0;
			for (int i = 0; i < remaining; i++)
			{
				k1 |= (uint32_t)data[numBlocks * 4 + i] << (8 * i);
			}

			k1 *= c1;
			k1 = (k1 << 15) | (k1 >> 17);
			k1 *= c2;
			h1 ^= k1;
		}

		// Finalization
		h1 ^= (uint32_t)size;
		h1 ^= h1 >> 16;
		h1 *= 0x85EBCA6B;
		h1 ^= h1 >> 13;
		h1 *= 0xC2B2AE35;
		h1 ^= h1 >> 16;
		return h1;
	}

	inline bool ValidatePatchedFile(const std::string& originalPath, const std::string& patchedPath, bool hasBindSection, DWORD textSectionFileOffset, DWORD textSectionFileSize, uint32_t expectedTextHash)
	{
		// Read original file
		std::ifstream origFile(originalPath, std::ios::binary);
		if (!origFile.is_open())
		{
			MessageBoxA(NULL, "Failed to open original file for validation", "EchoPatch - LAAPatcher Validation Error", MB_ICONERROR);
			return false;
		}
		std::vector<uint8_t> origData((std::istreambuf_iterator<char>(origFile)), std::istreambuf_iterator<char>());
		origFile.close();

		// Read patched file
		std::ifstream patchFile(patchedPath, std::ios::binary);
		if (!patchFile.is_open())
		{
			MessageBoxA(NULL, "Failed to open patched file for validation", "EchoPatch - LAAPatcher Validation Error", MB_ICONERROR);
			return false;
		}
		std::vector<uint8_t> patchData((std::istreambuf_iterator<char>(patchFile)), std::istreambuf_iterator<char>());
		patchFile.close();

		if (origData.empty() || patchData.empty())
		{
			MessageBoxA(NULL, "Failed to read files for validation", "EchoPatch - LAAPatcher Validation Error", MB_ICONERROR);
			return false;
		}

		PIMAGE_DOS_HEADER origDosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(origData.data());
		PIMAGE_NT_HEADERS origNtHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(origData.data() + origDosHeader->e_lfanew);

		PIMAGE_DOS_HEADER patchDosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(patchData.data());
		PIMAGE_NT_HEADERS patchNtHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(patchData.data() + patchDosHeader->e_lfanew);

		if (!hasBindSection)
		{
			// Only LAA flag and checksum should change

			// Check file sizes match
			if (origData.size() != patchData.size())
			{
				MessageBoxA(NULL, "Validation failed: File size changed when it shouldn't have", "EchoPatch - LAAPatcher Validation Error", MB_ICONERROR);
				return false;
			}

			// Create temporary copies to compare
			std::vector<uint8_t> origCopy = origData;
			std::vector<uint8_t> patchCopy = patchData;

			PIMAGE_DOS_HEADER origDosHeaderCopy = reinterpret_cast<PIMAGE_DOS_HEADER>(origCopy.data());
			PIMAGE_NT_HEADERS origNtHeadersCopy = reinterpret_cast<PIMAGE_NT_HEADERS>(origCopy.data() + origDosHeaderCopy->e_lfanew);

			PIMAGE_DOS_HEADER patchDosHeaderCopy = reinterpret_cast<PIMAGE_DOS_HEADER>(patchCopy.data());
			PIMAGE_NT_HEADERS patchNtHeadersCopy = reinterpret_cast<PIMAGE_NT_HEADERS>(patchCopy.data() + patchDosHeaderCopy->e_lfanew);

			// Normalize the fields we expect to change
			origNtHeadersCopy->FileHeader.Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
			origNtHeadersCopy->OptionalHeader.CheckSum = 0;
			patchNtHeadersCopy->OptionalHeader.CheckSum = 0;

			// Compare entire files
			if (memcmp(origCopy.data(), patchCopy.data(), origCopy.size()) != 0)
			{
				MessageBoxA(NULL, "Validation failed: File differs in more than just LAA flag and checksum", "EchoPatch - LAAPatcher Validation Error", MB_ICONERROR);
				return false;
			}
		}
		else
		{
			// 1. Hash the .text section in both files
			if (textSectionFileOffset + textSectionFileSize > origData.size())
			{
				MessageBoxA(NULL, "Validation failed: Invalid .text section offset/size", "EchoPatch - LAAPatcher Validation Error", MB_ICONERROR);
				return false;
			}

			uint32_t origTextHash = MurmurHash3(origData.data() + textSectionFileOffset, textSectionFileSize);

			if (textSectionFileOffset + textSectionFileSize > patchData.size())
			{
				MessageBoxA(NULL, "Validation failed: Invalid patched .text section offset/size", "EchoPatch - LAAPatcher Validation Error", MB_ICONERROR);
				return false;
			}

			uint32_t patchTextHash = MurmurHash3(patchData.data() + textSectionFileOffset, textSectionFileSize);

			// .text section must be different
			if (origTextHash == patchTextHash)
			{
				MessageBoxA(NULL, "Validation failed: .text section was not updated", "EchoPatch - LAAPatcher Validation Error", MB_ICONERROR);
				return false;
			}

			// Verify the patched .text section matches the expected hash
			if (patchTextHash != expectedTextHash)
			{
				char errorMsg[512];
				sprintf_s(errorMsg, "Validation failed: Decrypted .text section hash mismatch!\n\nExpected: 0x%08X\nGot: 0x%08X\n\nThis may indicate a different version of F.E.A.R. or corrupted executable.", expectedTextHash, patchTextHash);
				MessageBoxA(NULL, errorMsg, "EchoPatch - LAAPatcher Validation Error", MB_ICONERROR);
				return false;
			}

			// 2. Verify other regions that shouldn't change

			// Region 1: DOS header (before DOS stub which gets zeroed)
			if (memcmp(origData.data(), patchData.data(), sizeof(IMAGE_DOS_HEADER)) != 0)
			{
				MessageBoxA(NULL, "Validation failed: DOS header unexpectedly changed", "EchoPatch - LAAPatcher Validation Error", MB_ICONERROR);
				return false;
			}

			// Region 2: PE headers (excluding fields we intentionally modified)
			size_t ntHeadersOffset = origDosHeader->e_lfanew;

			// Check PE signature
			if (origNtHeaders->Signature != patchNtHeaders->Signature)
			{
				MessageBoxA(NULL, "Validation failed: PE signature changed", "EchoPatch - LAAPatcher Validation Error", MB_ICONERROR);
				return false;
			}

			// Check fields in FileHeader (except Characteristics and NumberOfSections)
			if (origNtHeaders->FileHeader.Machine != patchNtHeaders->FileHeader.Machine ||
				origNtHeaders->FileHeader.TimeDateStamp != patchNtHeaders->FileHeader.TimeDateStamp ||
				origNtHeaders->FileHeader.PointerToSymbolTable != patchNtHeaders->FileHeader.PointerToSymbolTable ||
				origNtHeaders->FileHeader.NumberOfSymbols != patchNtHeaders->FileHeader.NumberOfSymbols ||
				origNtHeaders->FileHeader.SizeOfOptionalHeader != patchNtHeaders->FileHeader.SizeOfOptionalHeader)
			{
				MessageBoxA(NULL, "Validation failed: Unexpected FileHeader changes", "EchoPatch - LAAPatcher Validation Error", MB_ICONERROR);
				return false;
			}

			// Check OptionalHeader fields (excluding CheckSum, AddressOfEntryPoint, and SizeOfImage)
			if (origNtHeaders->OptionalHeader.Magic != patchNtHeaders->OptionalHeader.Magic ||
				origNtHeaders->OptionalHeader.MajorLinkerVersion != patchNtHeaders->OptionalHeader.MajorLinkerVersion ||
				origNtHeaders->OptionalHeader.MinorLinkerVersion != patchNtHeaders->OptionalHeader.MinorLinkerVersion ||
				origNtHeaders->OptionalHeader.SizeOfCode != patchNtHeaders->OptionalHeader.SizeOfCode ||
				origNtHeaders->OptionalHeader.SizeOfInitializedData != patchNtHeaders->OptionalHeader.SizeOfInitializedData ||
				origNtHeaders->OptionalHeader.BaseOfCode != patchNtHeaders->OptionalHeader.BaseOfCode ||
				origNtHeaders->OptionalHeader.ImageBase != patchNtHeaders->OptionalHeader.ImageBase)
			{
				MessageBoxA(NULL, "Validation failed: Unexpected OptionalHeader changes", "EchoPatch - LAAPatcher Validation Error", MB_ICONERROR);
				return false;
			}

			// Region 3: Compare sections that aren't .text or .bind
			PIMAGE_SECTION_HEADER origSections = IMAGE_FIRST_SECTION(origNtHeaders);
			PIMAGE_SECTION_HEADER patchSections = IMAGE_FIRST_SECTION(patchNtHeaders);

			for (WORD i = 0; i < origNtHeaders->FileHeader.NumberOfSections; i++)
			{
				char sectionName[9] = { 0 };
				memcpy(sectionName, origSections[i].Name, IMAGE_SIZEOF_SHORT_NAME);

				if (strcmp(sectionName, ".text") == 0 || strcmp(sectionName, ".bind") == 0)
					continue; // Skip .text (already validated) and .bind (removed)

				// Find corresponding section in patched file
				bool foundMatch = false;
				for (WORD j = 0; j < patchNtHeaders->FileHeader.NumberOfSections; j++)
				{
					char patchSectionName[9] = { 0 };
					memcpy(patchSectionName, patchSections[j].Name, IMAGE_SIZEOF_SHORT_NAME);

					if (strcmp(sectionName, patchSectionName) == 0)
					{
						// Found matching section - verify properties match
						if (origSections[i].VirtualAddress != patchSections[j].VirtualAddress ||
							origSections[i].Misc.VirtualSize != patchSections[j].Misc.VirtualSize ||
							origSections[i].Characteristics != patchSections[j].Characteristics)
						{
							char errorMsg[256];
							sprintf_s(errorMsg, "Validation failed: Section %s properties changed unexpectedly", sectionName);
							MessageBoxA(NULL, errorMsg, "EchoPatch - LAAPatcher Validation Error", MB_ICONERROR);
							return false;
						}

						// Compare actual section data
						DWORD origOffset = origSections[i].PointerToRawData;
						DWORD patchOffset = patchSections[j].PointerToRawData;
						DWORD size = min(origSections[i].SizeOfRawData, patchSections[j].SizeOfRawData);

						if (origOffset + size <= origData.size() && patchOffset + size <= patchData.size())
						{
							if (memcmp(origData.data() + origOffset, patchData.data() + patchOffset, size) != 0)
							{
								char errorMsg[256];
								sprintf_s(errorMsg, "Validation failed: Section %s data changed unexpectedly", sectionName);
								MessageBoxA(NULL, errorMsg, "EchoPatch - LAAPatcher Validation Error", MB_ICONERROR);
								return false;
							}
						}

						foundMatch = true;
						break;
					}
				}

				if (!foundMatch)
				{
					char errorMsg[256];
					sprintf_s(errorMsg, "Validation failed: Section %s missing in patched file", sectionName);
					MessageBoxA(NULL, errorMsg, "EchoPatch - LAAPatcher Validation Error", MB_ICONERROR);
					return false;
				}
			}
		}

		return true;
	}

	// Reference: https://github.com/nipkownix/re4_tweaks/blob/master/dllmain/LAApatch.cpp
	inline bool PerformLAAPatch(HMODULE hModule, bool showConfirmation)
	{
		if (!hModule)
		{
			MessageBoxA(NULL, "Invalid module handle", "EchoPatch - LAAPatcher Error", MB_ICONERROR);
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
		DWORD textSectionFileOffset = 0;
		DWORD textSectionFileSize = 0;
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

			// Validate file size before accessing headers
			if (fileBuffer.size() < sizeof(IMAGE_DOS_HEADER))
			{
				MessageBoxA(NULL, "File too small to be a valid PE", "EchoPatch - LAAPatcher Error", MB_ICONERROR);
				return false;
			}

			PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(fileBuffer.data());
			if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
			{
				MessageBoxA(NULL, "Invalid DOS header", "EchoPatch - LAAPatcher Error", MB_ICONERROR);
				return false;
			}

			// Validate NT headers offset and size
			if (dosHeader->e_lfanew < 0 || fileBuffer.size() < static_cast<size_t>(dosHeader->e_lfanew) + sizeof(IMAGE_NT_HEADERS))
			{
				MessageBoxA(NULL, "Invalid or corrupt PE headers", "EchoPatch - LAAPatcher Error", MB_ICONERROR);
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
				memcpy(sectionName, sectionHeader[i].Name, IMAGE_SIZEOF_SHORT_NAME);

				if (strcmp(sectionName, ".bind") == 0)
				{
					hasBindSection = true;
				}

				if (strcmp(sectionName, ".text") == 0)
				{
					textSectionRVA = sectionHeader[i].VirtualAddress;
					textSectionSize = sectionHeader[i].Misc.VirtualSize;
					textSectionFileOffset = sectionHeader[i].PointerToRawData;
					textSectionFileSize = sectionHeader[i].SizeOfRawData;
				}
			}
		}

		if (isLAA && !hasBindSection)
		{
			return true; // No need to modify
		}

		// Prompt user before patching if confirmation is enabled
		if (showConfirmation)
		{
			char promptMsg[1024];
			sprintf_s(promptMsg, sizeof(promptMsg),
				"Your game executable is missing the 4GB/LAA patch. This allows the game to use 4GB of memory instead of 2GB, which prevents crashes when loading levels.\n\n"
				"EchoPatch will patch %s and create a backup.\n\n"
				"Apply patch and restart the game?\n\n"
				"(This check can be disabled by setting CheckLAAPatch=0 in the [Fixes] section of EchoPatch.ini)",
				executableName);

			if (MessageBoxA(NULL, promptMsg, "4GB/Large Address Aware patch missing!", MB_YESNO | MB_ICONEXCLAMATION) != IDYES)
			{
				return false;
			}
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

		// Handle .bind section if present (integrity check over FileHeader.Characteristics)
		if (hasBindSection)
		{
			// Zero out the DOS stub data (between DOS header and NT headers)
			DWORD dosStubOffset = sizeof(IMAGE_DOS_HEADER);
			DWORD dosStubSize = dosHeader->e_lfanew - dosStubOffset;
			if (dosStubSize > 0)
			{
				memset(exeData.data() + dosStubOffset, 0, dosStubSize);
			}

			// Set the AddressOfEntryPoint to 0x13E428 for F.E.A.R. (Steam)
			ntHeaders->OptionalHeader.AddressOfEntryPoint = 0x13E428;

			PIMAGE_SECTION_HEADER sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
			int bindSectionIndex = -1;
			PIMAGE_SECTION_HEADER textSection = nullptr;

			// Find the .bind and .text sections
			for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++)
			{
				char sectionName[9] = { 0 };
				memcpy(sectionName, sectionHeader[i].Name, IMAGE_SIZEOF_SHORT_NAME);

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
				// Save .bind section info before we modify anything
				DWORD bindSectionOffset = sectionHeader[bindSectionIndex].PointerToRawData;
				DWORD bindSectionSize = sectionHeader[bindSectionIndex].SizeOfRawData;

				// Copy the decrypted .text section from memory to the file buffer
				BYTE* textSectionInMemory = reinterpret_cast<BYTE*>(hModule) + textSection->VirtualAddress;
				memcpy(exeData.data() + textSection->PointerToRawData, textSectionInMemory, textSection->SizeOfRawData);

				// 'Remove' the .bind section by updating the section count
				ntHeaders->FileHeader.NumberOfSections--;

				// Shift section headers to remove the .bind section header
				for (int i = bindSectionIndex; i < ntHeaders->FileHeader.NumberOfSections; i++)
				{
					sectionHeader[i] = sectionHeader[i + 1];
				}

				// Zero out the last section header slot (where .bind metadata was)
				memset(&sectionHeader[ntHeaders->FileHeader.NumberOfSections], 0, sizeof(IMAGE_SECTION_HEADER));

				// Adjust PointerToRawData for all sections after .bind
				for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++)
				{
					if (sectionHeader[i].PointerToRawData > bindSectionOffset)
					{
						sectionHeader[i].PointerToRawData -= bindSectionSize;
					}
				}

				// Update SizeOfImage
				DWORD newSizeOfImage = 0;
				for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++)
				{
					DWORD sectionEnd = sectionHeader[i].VirtualAddress + sectionHeader[i].Misc.VirtualSize;
					if (sectionEnd > newSizeOfImage)
					{
						newSizeOfImage = sectionEnd;
					}
				}

				// Round up to SectionAlignment
				DWORD sectionAlignment = ntHeaders->OptionalHeader.SectionAlignment;
				newSizeOfImage = (newSizeOfImage + sectionAlignment - 1) & ~(sectionAlignment - 1);
				ntHeaders->OptionalHeader.SizeOfImage = newSizeOfImage;

				// Remove the .bind section data from the buffer
				exeData.erase(exeData.begin() + bindSectionOffset, exeData.begin() + bindSectionOffset + bindSectionSize);
				fileSize -= bindSectionSize;
			}
		}

		// Write modified data back to file
		fseek(file, 0, SEEK_SET);
		if (fwrite(exeData.data(), 1, fileSize, file) != fileSize)
		{
			MessageBoxA(NULL, "Failed to write modified file", "EchoPatch - LAAPatcher Error", MB_ICONERROR);
			fclose(file);
			DeleteFileA(modulePathNew.c_str());
			return false;
		}

		// Truncate file to new size
		_chsize_s(_fileno(file), fileSize);

		// Close the file
		fflush(file);
		fclose(file);

		// Verify the patched file before replacing original
		if (!ValidatePatchedFile(modulePath, modulePathNew, hasBindSection, textSectionFileOffset, textSectionFileSize, 0x96EC25CA))
		{
			MessageBoxA(NULL, "Patching validation failed. The patched file does not match expected changes. Aborting.", "EchoPatch - LAAPatcher Validation Error", MB_ICONERROR);
			DeleteFileA(modulePathNew.c_str());
			return false;
		}

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

		// Relaunch the application
		if ((int)ShellExecuteA(NULL, "open", moduleFilePathA, NULL, NULL, SW_SHOWDEFAULT) > 32)
		{
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
