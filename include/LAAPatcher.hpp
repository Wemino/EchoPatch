#pragma once

#include <windows.h>
#include <algorithm>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace LAAPatcher
{
	inline void ShowError(const char* message)
	{
		MessageBoxA(NULL, message, "EchoPatch - LAAPatcher Error", MB_ICONERROR);
	}

	inline void ShowError(const std::string& message)
	{
		MessageBoxA(NULL, message.c_str(), "EchoPatch - LAAPatcher Error", MB_ICONERROR);
	}

	inline bool ReadFile(const fs::path& path, std::vector<uint8_t>& buffer)
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file)
			return false;

		std::streamsize size = file.tellg();
		if (size <= 0)
			return false;

		file.seekg(0, std::ios::beg);

		try
		{
			buffer.resize(static_cast<size_t>(size));
		}
		catch (...)
		{
			return false;
		}

		return file.read(reinterpret_cast<char*>(buffer.data()), size).good();
	}

	inline uint32_t MurmurHash3(const uint8_t* data, size_t size, uint32_t seed = 0)
	{
		uint32_t h1 = seed;
		constexpr uint32_t c1 = 0xCC9E2D51;
		constexpr uint32_t c2 = 0x1B873593;

		const size_t numBlocks = size / 4;

		for (size_t i = 0; i < numBlocks; i++)
		{
			uint32_t k1;
			memcpy(&k1, data + i * 4, sizeof(uint32_t));

			k1 *= c1;
			k1 = _rotl(k1, 15);
			k1 *= c2;

			h1 ^= k1;
			h1 = _rotl(h1, 13);
			h1 = h1 * 5 + 0xE6546B64;
		}

		const uint8_t* tail = data + numBlocks * 4;
		uint32_t k1 = 0;

		switch (size & 3)
		{
			case 3: k1 ^= static_cast<uint32_t>(tail[2]) << 16; [[fallthrough]];
			case 2: k1 ^= static_cast<uint32_t>(tail[1]) << 8; [[fallthrough]];
			case 1: k1 ^= static_cast<uint32_t>(tail[0]);
			k1 *= c1;
			k1 = _rotl(k1, 15);
			k1 *= c2;
			h1 ^= k1;
		}

		h1 ^= static_cast<uint32_t>(size);
		h1 ^= h1 >> 16;
		h1 *= 0x85EBCA6B;
		h1 ^= h1 >> 13;
		h1 *= 0xC2B2AE35;
		h1 ^= h1 >> 16;
		return h1;
	}

	inline bool ValidatePatchedFile(const fs::path& origPath, const fs::path& patchPath, bool hasBindSection, DWORD textSectionFileOffset, DWORD textSectionFileSize, uint32_t expectedTextHash)
	{
		std::vector<uint8_t> origData, patchData;

		if (!ReadFile(origPath, origData))
		{
			char msg[512];
			sprintf_s(msg, "Validation failed: Could not read original file.\nPath: %s", origPath.string().c_str());
			ShowError(msg);
			return false;
		}

		if (!ReadFile(patchPath, patchData))
		{
			char msg[512];
			sprintf_s(msg, "Validation failed: Could not read patched file.\nPath: %s", patchPath.string().c_str());
			ShowError(msg);
			return false;
		}

		if (origData.empty())
		{
			ShowError("Validation failed: Original file is empty.");
			return false;
		}

		if (patchData.empty())
		{
			ShowError("Validation failed: Patched file is empty.");
			return false;
		}

		if (origData.size() < sizeof(IMAGE_DOS_HEADER))
		{
			char msg[256];
			sprintf_s(msg, "Validation failed: Original file too small (%zu bytes, need at least %zu).", origData.size(), sizeof(IMAGE_DOS_HEADER));
			ShowError(msg);
			return false;
		}

		if (patchData.size() < sizeof(IMAGE_DOS_HEADER))
		{
			char msg[256];
			sprintf_s(msg, "Validation failed: Patched file too small (%zu bytes, need at least %zu).", patchData.size(), sizeof(IMAGE_DOS_HEADER));
			ShowError(msg);
			return false;
		}

		auto* origDos = reinterpret_cast<PIMAGE_DOS_HEADER>(origData.data());
		auto* patchDos = reinterpret_cast<PIMAGE_DOS_HEADER>(patchData.data());

		if (origDos->e_magic != IMAGE_DOS_SIGNATURE)
		{
			char msg[256];
			sprintf_s(msg, "Validation failed: Original file has invalid DOS signature (0x%04X, expected 0x%04X).", origDos->e_magic, IMAGE_DOS_SIGNATURE);
			ShowError(msg);
			return false;
		}

		if (patchDos->e_magic != IMAGE_DOS_SIGNATURE)
		{
			char msg[256];
			sprintf_s(msg, "Validation failed: Patched file has invalid DOS signature (0x%04X, expected 0x%04X).", patchDos->e_magic, IMAGE_DOS_SIGNATURE);
			ShowError(msg);
			return false;
		}

		if (static_cast<size_t>(origDos->e_lfanew) + sizeof(IMAGE_NT_HEADERS) > origData.size())
		{
			char msg[256];
			sprintf_s(msg, "Validation failed: Original file NT header offset out of bounds (e_lfanew=0x%X, filesize=%zu).", origDos->e_lfanew, origData.size());
			ShowError(msg);
			return false;
		}

		if (static_cast<size_t>(patchDos->e_lfanew) + sizeof(IMAGE_NT_HEADERS) > patchData.size())
		{
			char msg[256];
			sprintf_s(msg, "Validation failed: Patched file NT header offset out of bounds (e_lfanew=0x%X, filesize=%zu).", patchDos->e_lfanew, patchData.size());
			ShowError(msg);
			return false;
		}

		auto* origNt = reinterpret_cast<PIMAGE_NT_HEADERS>(origData.data() + origDos->e_lfanew);
		auto* patchNt = reinterpret_cast<PIMAGE_NT_HEADERS>(patchData.data() + patchDos->e_lfanew);

		if (origNt->Signature != IMAGE_NT_SIGNATURE)
		{
			char msg[256];
			sprintf_s(msg, "Validation failed: Original file has invalid NT signature (0x%08X, expected 0x%08X).", origNt->Signature, IMAGE_NT_SIGNATURE);
			ShowError(msg);
			return false;
		}

		if (patchNt->Signature != IMAGE_NT_SIGNATURE)
		{
			char msg[256];
			sprintf_s(msg, "Validation failed: Patched file has invalid NT signature (0x%08X, expected 0x%08X).", patchNt->Signature, IMAGE_NT_SIGNATURE);
			ShowError(msg);
			return false;
		}

		if (!hasBindSection)
		{
			if (origData.size() != patchData.size())
			{
				char msg[256];
				sprintf_s(msg, "Validation failed: File size changed when it shouldn't have (original=%zu, patched=%zu).", origData.size(), patchData.size());
				ShowError(msg);
				return false;
			}

			origNt->FileHeader.Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
			origNt->OptionalHeader.CheckSum = 0;
			patchNt->OptionalHeader.CheckSum = 0;

			if (memcmp(origData.data(), patchData.data(), origData.size()) != 0)
			{
				ShowError("Validation failed: File differs in more than just LAA flag and checksum.");
				return false;
			}

			return true;
		}

		if (textSectionFileOffset + textSectionFileSize > patchData.size())
		{
			char msg[256];
			sprintf_s(msg, "Validation failed: .text section bounds exceed file size (offset=0x%X, size=0x%X, filesize=%zu).", textSectionFileOffset, textSectionFileSize, patchData.size());
			ShowError(msg);
			return false;
		}

		uint32_t patchTextHash = MurmurHash3(patchData.data() + textSectionFileOffset, textSectionFileSize);
		if (patchTextHash != expectedTextHash)
		{
			char msg[512];
			sprintf_s(msg, "Validation failed: .text section hash mismatch.\n\nExpected: 0x%08X\nGot: 0x%08X\n\nThis may indicate a different game version or corrupted executable.", expectedTextHash, patchTextHash);
			ShowError(msg);
			return false;
		}

		if (memcmp(origData.data(), patchData.data(), sizeof(IMAGE_DOS_HEADER)) != 0)
		{
			ShowError("Validation failed: DOS header unexpectedly changed.");
			return false;
		}

		if (origNt->Signature != patchNt->Signature)
		{
			char msg[256];
			sprintf_s(msg, "Validation failed: PE signature changed (original=0x%08X, patched=0x%08X).", origNt->Signature, patchNt->Signature);
			ShowError(msg);
			return false;
		}

		if (origNt->FileHeader.Machine != patchNt->FileHeader.Machine)
		{
			char msg[256];
			sprintf_s(msg, "Validation failed: Machine type changed (original=0x%04X, patched=0x%04X).", origNt->FileHeader.Machine, patchNt->FileHeader.Machine);
			ShowError(msg);
			return false;
		}

		if (origNt->FileHeader.TimeDateStamp != patchNt->FileHeader.TimeDateStamp)
		{
			char msg[256];
			sprintf_s(msg, "Validation failed: TimeDateStamp changed (original=0x%08X, patched=0x%08X).", origNt->FileHeader.TimeDateStamp, patchNt->FileHeader.TimeDateStamp);
			ShowError(msg);
			return false;
		}

		if (origNt->FileHeader.SizeOfOptionalHeader != patchNt->FileHeader.SizeOfOptionalHeader)
		{
			char msg[256];
			sprintf_s(msg, "Validation failed: SizeOfOptionalHeader changed (original=0x%04X, patched=0x%04X).", origNt->FileHeader.SizeOfOptionalHeader, patchNt->FileHeader.SizeOfOptionalHeader);
			ShowError(msg);
			return false;
		}

		if (origNt->OptionalHeader.Magic != patchNt->OptionalHeader.Magic)
		{
			char msg[256];
			sprintf_s(msg, "Validation failed: OptionalHeader Magic changed (original=0x%04X, patched=0x%04X).", origNt->OptionalHeader.Magic, patchNt->OptionalHeader.Magic);
			ShowError(msg);
			return false;
		}

		if (origNt->OptionalHeader.SizeOfCode != patchNt->OptionalHeader.SizeOfCode)
		{
			char msg[256];
			sprintf_s(msg, "Validation failed: SizeOfCode changed (original=0x%08X, patched=0x%08X).", origNt->OptionalHeader.SizeOfCode, patchNt->OptionalHeader.SizeOfCode);
			ShowError(msg);
			return false;
		}

		if (origNt->OptionalHeader.ImageBase != patchNt->OptionalHeader.ImageBase)
		{
			char msg[256];
			sprintf_s(msg, "Validation failed: ImageBase changed (original=0x%08X, patched=0x%08X).", origNt->OptionalHeader.ImageBase, patchNt->OptionalHeader.ImageBase);
			ShowError(msg);
			return false;
		}

		auto* origSections = IMAGE_FIRST_SECTION(origNt);
		auto* patchSections = IMAGE_FIRST_SECTION(patchNt);

		for (WORD i = 0; i < origNt->FileHeader.NumberOfSections; i++)
		{
			char sectionName[IMAGE_SIZEOF_SHORT_NAME + 1] = {};
			memcpy(sectionName, origSections[i].Name, IMAGE_SIZEOF_SHORT_NAME);

			if (strcmp(sectionName, ".text") == 0 || strcmp(sectionName, ".bind") == 0)
				continue;

			bool found = false;
			for (WORD j = 0; j < patchNt->FileHeader.NumberOfSections; j++)
			{
				char patchName[IMAGE_SIZEOF_SHORT_NAME + 1] = {};
				memcpy(patchName, patchSections[j].Name, IMAGE_SIZEOF_SHORT_NAME);

				if (strcmp(sectionName, patchName) != 0)
					continue;

				if (origSections[i].VirtualAddress != patchSections[j].VirtualAddress)
				{
					char msg[256];
					sprintf_s(msg, "Validation failed: Section '%s' VirtualAddress changed (original=0x%08X, patched=0x%08X).", sectionName, origSections[i].VirtualAddress, patchSections[j].VirtualAddress);
					ShowError(msg);
					return false;
				}

				if (origSections[i].Misc.VirtualSize != patchSections[j].Misc.VirtualSize)
				{
					char msg[256];
					sprintf_s(msg, "Validation failed: Section '%s' VirtualSize changed (original=0x%08X, patched=0x%08X).", sectionName, origSections[i].Misc.VirtualSize, patchSections[j].Misc.VirtualSize);
					ShowError(msg);
					return false;
				}

				if (origSections[i].Characteristics != patchSections[j].Characteristics)
				{
					char msg[256];
					sprintf_s(msg, "Validation failed: Section '%s' Characteristics changed (original=0x%08X, patched=0x%08X).", sectionName, origSections[i].Characteristics, patchSections[j].Characteristics);
					ShowError(msg);
					return false;
				}

				DWORD origOffset = origSections[i].PointerToRawData;
				DWORD patchOffset = patchSections[j].PointerToRawData;
				DWORD size = std::min<DWORD>(origSections[i].SizeOfRawData, patchSections[j].SizeOfRawData);

				if (origOffset + size > origData.size())
				{
					char msg[256];
					sprintf_s(msg, "Validation failed: Section '%s' exceeds original file bounds (offset=0x%X, size=0x%X, filesize=%zu).", sectionName, origOffset, size, origData.size());
					ShowError(msg);
					return false;
				}

				if (patchOffset + size > patchData.size())
				{
					char msg[256];
					sprintf_s(msg, "Validation failed: Section '%s' exceeds patched file bounds (offset=0x%X, size=0x%X, filesize=%zu).", sectionName, patchOffset, size, patchData.size());
					ShowError(msg);
					return false;
				}

				if (memcmp(origData.data() + origOffset, patchData.data() + patchOffset, size) != 0)
				{
					char msg[256];
					sprintf_s(msg, "Validation failed: Section '%s' data changed unexpectedly.", sectionName);
					ShowError(msg);
					return false;
				}

				found = true;
				break;
			}

			if (!found)
			{
				char msg[256];
				sprintf_s(msg, "Validation failed: Section '%s' missing in patched file.", sectionName);
				ShowError(msg);
				return false;
			}
		}

		return true;
	}

	// Reference: https://github.com/nipkownix/re4_tweaks/blob/master/dllmain/LAApatch.cpp
	inline bool PerformLAAPatch(HMODULE hModule, bool showConfirmation)
	{
		if (!hModule)
		{
			ShowError("PerformLAAPatch called with NULL module handle.");
			return false;
		}

		char modulePathRaw[MAX_PATH];
		DWORD pathLen = GetModuleFileNameA(hModule, modulePathRaw, MAX_PATH);
		if (pathLen == 0)
		{
			char msg[256];
			sprintf_s(msg, "GetModuleFileNameA failed. Error: %d", GetLastError());
			ShowError(msg);
			return false;
		}

		if (pathLen >= MAX_PATH)
		{
			ShowError("Module path exceeds MAX_PATH. Path may be truncated.");
			return false;
		}

		fs::path exePath = modulePathRaw;
		fs::path exeName = exePath.filename();
		fs::path newPath = exePath; newPath += ".new";
		fs::path bakPath = exePath; bakPath += ".bak";

		if (!fs::exists(exePath))
		{
			char msg[512];
			sprintf_s(msg, "Executable path does not exist: %s", exePath.string().c_str());
			ShowError(msg);
			return false;
		}

		std::vector<uint8_t> buffer;
		if (!ReadFile(exePath, buffer))
		{
			char msg[512];
			sprintf_s(msg, "Could not read executable file.\nPath: %s\nError: %d", exePath.string().c_str(), GetLastError());
			ShowError(msg);
			return false;
		}

		if (buffer.empty())
		{
			char msg[512];
			sprintf_s(msg, "Executable file is empty.\nPath: %s", exePath.string().c_str());
			ShowError(msg);
			return false;
		}

		std::error_code ec;
		auto spaceInfo = fs::space(exePath.parent_path(), ec);
		if (!ec)
		{
			uintmax_t requiredSpace = buffer.size() * 2;
			if (spaceInfo.available < requiredSpace)
			{
				char msg[512];
				sprintf_s(msg, "Not enough disk space to create backup and patched files.\n\nRequired: %llu MB\nAvailable: %llu MB", (requiredSpace / (1024 * 1024)) + 1, spaceInfo.available / (1024 * 1024));
				ShowError(msg);
				return false;
			}
		}

		if (buffer.size() < sizeof(IMAGE_DOS_HEADER))
		{
			char msg[256];
			sprintf_s(msg, "File too small to be a valid PE (%zu bytes, need at least %zu).", buffer.size(), sizeof(IMAGE_DOS_HEADER));
			ShowError(msg);
			return false;
		}

		auto* dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(buffer.data());
		if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		{
			char msg[256];
			sprintf_s(msg, "Invalid DOS signature (0x%04X, expected 0x%04X).", dosHeader->e_magic, IMAGE_DOS_SIGNATURE);
			ShowError(msg);
			return false;
		}

		if (dosHeader->e_lfanew < 0)
		{
			char msg[256];
			sprintf_s(msg, "Invalid e_lfanew value (%d, must be non-negative).", dosHeader->e_lfanew);
			ShowError(msg);
			return false;
		}

		if (static_cast<size_t>(dosHeader->e_lfanew) + sizeof(IMAGE_NT_HEADERS) > buffer.size())
		{
			char msg[256];
			sprintf_s(msg, "NT header offset out of bounds (e_lfanew=0x%X, filesize=%zu).", dosHeader->e_lfanew, buffer.size());
			ShowError(msg);
			return false;
		}

		auto* ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(buffer.data() + dosHeader->e_lfanew);
		if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
		{
			char msg[256];
			sprintf_s(msg, "Invalid NT signature (0x%08X, expected 0x%08X).", ntHeaders->Signature, IMAGE_NT_SIGNATURE);
			ShowError(msg);
			return false;
		}

		if (ntHeaders->FileHeader.NumberOfSections == 0)
		{
			ShowError("PE file has zero sections.");
			return false;
		}

		size_t sectionHeadersOffset = dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS);
		size_t sectionHeadersSize = ntHeaders->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
		if (sectionHeadersOffset + sectionHeadersSize > buffer.size())
		{
			char msg[256];
			sprintf_s(msg, "Section headers exceed file bounds (offset=0x%zX, size=0x%zX, filesize=%zu).", sectionHeadersOffset, sectionHeadersSize, buffer.size());
			ShowError(msg);
			return false;
		}

		bool isLAA = (ntHeaders->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) != 0;
		bool hasBindSection = false;
		int bindSectionIndex = -1;
		DWORD textSectionVA = 0;
		DWORD textSectionFileOffset = 0;
		DWORD textSectionFileSize = 0;

		auto* sectionHeaders = IMAGE_FIRST_SECTION(ntHeaders);
		for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++)
		{
			char name[IMAGE_SIZEOF_SHORT_NAME + 1] = {};
			memcpy(name, sectionHeaders[i].Name, IMAGE_SIZEOF_SHORT_NAME);

			if (strcmp(name, ".bind") == 0)
			{
				hasBindSection = true;
				bindSectionIndex = i;
			}
			else if (strcmp(name, ".text") == 0)
			{
				textSectionVA = sectionHeaders[i].VirtualAddress;
				textSectionFileOffset = sectionHeaders[i].PointerToRawData;
				textSectionFileSize = sectionHeaders[i].SizeOfRawData;
			}
		}

		if (isLAA && !hasBindSection)
			return true;

		if (showConfirmation)
		{
			std::string msg = "Your game executable is missing the 4GB/LAA patch. "
				"This allows the game to use 4GB of memory instead of 2GB, which prevents crashes when loading levels.\n\n"
				"EchoPatch will patch " + exeName.string() + " and create a backup.\n\n"
				"Apply patch and restart the game?\n\n"
				"(This check can be disabled by setting CheckLAAPatch=0 in the [Fixes] section of EchoPatch.ini)";

			if (MessageBoxA(NULL, msg.c_str(), "4GB/Large Address Aware patch missing!", MB_YESNO | MB_ICONEXCLAMATION) != IDYES)
				return false;
		}

		LONG storedElfanew = dosHeader->e_lfanew;

		ntHeaders->FileHeader.Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
		ntHeaders->OptionalHeader.CheckSum = 0;

		if (hasBindSection && textSectionFileOffset > 0 && bindSectionIndex >= 0)
		{
			if (textSectionFileOffset + textSectionFileSize > buffer.size())
			{
				char msg[256];
				sprintf_s(msg, ".text section exceeds file bounds (offset=0x%X, size=0x%X, filesize=%zu).", textSectionFileOffset, textSectionFileSize, buffer.size());
				ShowError(msg);
				return false;
			}

			DWORD bindOffset = sectionHeaders[bindSectionIndex].PointerToRawData;
			DWORD bindSize = sectionHeaders[bindSectionIndex].SizeOfRawData;
			if (bindOffset + bindSize > buffer.size())
			{
				char msg[256];
				sprintf_s(msg, ".bind section exceeds file bounds (offset=0x%X, size=0x%X, filesize=%zu).", bindOffset, bindSize, buffer.size());
				ShowError(msg);
				return false;
			}

			size_t stubSize = storedElfanew - sizeof(IMAGE_DOS_HEADER);
			if (stubSize > 0)
				memset(buffer.data() + sizeof(IMAGE_DOS_HEADER), 0, stubSize);

			ntHeaders->OptionalHeader.AddressOfEntryPoint = 0x13E428;

			const uint8_t* textInMemory = reinterpret_cast<const uint8_t*>(hModule) + textSectionVA;
			memcpy(buffer.data() + textSectionFileOffset, textInMemory, textSectionFileSize);

			for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++)
			{
				if (sectionHeaders[i].PointerToRawData > bindOffset)
					sectionHeaders[i].PointerToRawData -= bindSize;
			}

			for (int i = bindSectionIndex; i < ntHeaders->FileHeader.NumberOfSections - 1; i++)
				sectionHeaders[i] = sectionHeaders[i + 1];

			ntHeaders->FileHeader.NumberOfSections--;
			memset(&sectionHeaders[ntHeaders->FileHeader.NumberOfSections], 0, sizeof(IMAGE_SECTION_HEADER));

			DWORD newSizeOfImage = 0;
			for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++)
			{
				DWORD sectionEnd = sectionHeaders[i].VirtualAddress + sectionHeaders[i].Misc.VirtualSize;
				if (sectionEnd > newSizeOfImage)
					newSizeOfImage = sectionEnd;
			}

			DWORD align = ntHeaders->OptionalHeader.SectionAlignment;
			if (align == 0)
			{
				ShowError("SectionAlignment is zero, cannot calculate SizeOfImage.");
				return false;
			}
			ntHeaders->OptionalHeader.SizeOfImage = (newSizeOfImage + align - 1) & ~(align - 1);

			buffer.erase(buffer.begin() + bindOffset, buffer.begin() + bindOffset + bindSize);

			dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(buffer.data());
			ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(buffer.data() + storedElfanew);
			sectionHeaders = IMAGE_FIRST_SECTION(ntHeaders);
		}

		if (fs::exists(newPath))
		{
			fs::remove(newPath, ec);
			if (ec)
			{
				char msg[512];
				sprintf_s(msg, "Failed to remove existing .new file.\nPath: %s\nError: %s", newPath.string().c_str(), ec.message().c_str());
				ShowError(msg);
				return false;
			}
		}

		{
			std::ofstream outFile(newPath, std::ios::binary);
			if (!outFile)
			{
				DWORD err = GetLastError();
				char msg[512];
				if (err == ERROR_ACCESS_DENIED)
					sprintf_s(msg, "Unable to write patched file (Access Denied).\n\nYour Anti-Virus may be blocking modification.\nPlease add an exception for this folder.\nPath: %s", newPath.string().c_str());
				else
					sprintf_s(msg, "Failed to create patched file.\nPath: %s\nError: %d", newPath.string().c_str(), err);
				ShowError(msg);
				return false;
			}

			outFile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());

			if (!outFile.good())
			{
				char msg[512];
				sprintf_s(msg, "Failed to write patched file data.\nPath: %s", newPath.string().c_str());
				ShowError(msg);
				fs::remove(newPath, ec);
				return false;
			}
		}

		Sleep(100);

		auto writtenSize = fs::file_size(newPath, ec);
		if (ec || writtenSize != buffer.size())
		{
			char msg[512];
			sprintf_s(msg, "Written file size mismatch (expected=%zu, actual=%zu).\nPath: %s", buffer.size(), static_cast<size_t>(writtenSize), newPath.string().c_str());
			ShowError(msg);
			fs::remove(newPath, ec);
			return false;
		}

		if (!ValidatePatchedFile(exePath, newPath, hasBindSection, textSectionFileOffset, textSectionFileSize, 0x96EC25CA))
		{
			fs::remove(newPath, ec);
			return false;
		}

		if (!MoveFileExA(exePath.string().c_str(), bakPath.string().c_str(), MOVEFILE_REPLACE_EXISTING))
		{
			DWORD err = GetLastError();

			if (err == ERROR_ACCESS_DENIED)
			{
				char msg[512];
				sprintf_s(msg, "LAA Patch failed: Access Denied (Error 5).\n\n"
					"Windows is blocking the file modification.\n"
					"Please restart the game as Administrator to apply the patch.\n\n"
					"You can disable this check by setting CheckLAAPatch=0 in EchoPatch.ini");
				ShowError(msg);
				fs::remove(newPath, ec);
				return false;
			}

			if (err == ERROR_SHARING_VIOLATION || err == ERROR_LOCK_VIOLATION)
			{
				uint32_t exeHash = MurmurHash3(buffer.data(), buffer.size());
				constexpr uint32_t NOCD_HASH = 0x080D7FDF;

				if (exeHash == NOCD_HASH)
				{
					ShowError("LAA Patch failed: Detected obsolete NO-CD version.\n\nThis version cannot be patched while running.");
				}
				else
				{
					char msg[512];
					sprintf_s(msg, "LAA Patch failed: File is locked by another process.\n\nError: %d\n\nYou can disable this check by setting CheckLAAPatch=0 in EchoPatch.ini", err);
					ShowError(msg);
				}

				fs::remove(newPath, ec);
				return false;
			}

			char msg[512];
			sprintf_s(msg, "Failed to backup original executable.\n\nError: %d", err);
			ShowError(msg);
			fs::remove(newPath, ec);
			return false;
		}

		if (!MoveFileA(newPath.string().c_str(), exePath.string().c_str()))
		{
			DWORD err = GetLastError();

			MoveFileA(bakPath.string().c_str(), exePath.string().c_str());

			char msg[512];
			sprintf_s(msg, "Failed to rename patched file to original. Restored from backup.\n\nError: %d", err);
			ShowError(msg);
			return false;
		}

		Sleep(50);

		INT_PTR shellResult = reinterpret_cast<INT_PTR>(ShellExecuteA(NULL, "open", exePath.string().c_str(), NULL, NULL, SW_SHOWDEFAULT));
		if (shellResult > 32)
		{
			ExitProcess(0);
		}
		else
		{
			char msg[512];
			sprintf_s(msg, "Patching completed successfully, but failed to restart the application.\nShellExecute returned: %d\n\nPlease restart the game manually.", static_cast<int>(shellResult));
			MessageBoxA(NULL, msg, "EchoPatch - LAAPatcher Warning", MB_ICONWARNING);
			return true;
		}
	}
}