#pragma once

#include <windows.h>
#include <algorithm>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <optional>

namespace fs = std::filesystem;

namespace LAAPatcher
{
	// ========================
	// Error Handling
	// ========================

	inline void ShowError(const char* message)
	{
		MessageBoxA(NULL, message, "EchoPatch - LAAPatcher Error", MB_ICONERROR);
	}

	inline void ShowError(const std::string& message)
	{
		MessageBoxA(NULL, message.c_str(), "EchoPatch - LAAPatcher Error", MB_ICONERROR);
	}

	// ========================
	// Validation Helpers
	// ========================

	template<typename T>
	bool ValidateEqual(T actual, T expected, const char* fieldName, char* outError, size_t errorSize)
	{
		if (actual == expected)
			return true;

		if constexpr (sizeof(T) <= 2)
			sprintf_s(outError, errorSize, "Validation failed: %s mismatch (actual=0x%04X, expected=0x%04X).", fieldName, actual, expected);
		else
			sprintf_s(outError, errorSize, "Validation failed: %s mismatch (actual=0x%08X, expected=0x%08X).", fieldName, actual, expected);

		return false;
	}

	inline bool CheckBounds(size_t offset, size_t size, size_t bufferSize, const char* context, char* outError, size_t errorSize)
	{
		if (offset + size <= bufferSize)
			return true;

		sprintf_s(outError, errorSize, "%s exceeds bounds (offset=0x%zX, size=0x%zX, bufferSize=%zu).", context, offset, size, bufferSize);
		return false;
	}

	inline bool SectionNameEquals(const IMAGE_SECTION_HEADER& section, const char* name)
	{
		return strncmp(reinterpret_cast<const char*>(section.Name), name, IMAGE_SIZEOF_SHORT_NAME) == 0;
	}

	inline void GetSectionName(const IMAGE_SECTION_HEADER& section, char* outName)
	{
		memcpy(outName, section.Name, IMAGE_SIZEOF_SHORT_NAME);
		outName[IMAGE_SIZEOF_SHORT_NAME] = '\0';
	}

	// ========================
	// PE Header Wrapper
	// ========================

	class PEFile
	{
	public:
		explicit PEFile(std::vector<uint8_t>& buffer) : m_buffer(buffer) {}

		bool Validate(char* outError, size_t errorSize)
		{
			if (m_buffer.size() < sizeof(IMAGE_DOS_HEADER))
			{
				sprintf_s(outError, errorSize, "File too small (%zu bytes, need at least %zu).", m_buffer.size(), sizeof(IMAGE_DOS_HEADER));
				return false;
			}

			if (DosHeader()->e_magic != IMAGE_DOS_SIGNATURE)
			{
				sprintf_s(outError, errorSize, "Invalid DOS signature (0x%04X, expected 0x%04X).", DosHeader()->e_magic, IMAGE_DOS_SIGNATURE);
				return false;
			}

			if (DosHeader()->e_lfanew < 0)
			{
				sprintf_s(outError, errorSize, "Invalid e_lfanew value (%d, must be non-negative).", DosHeader()->e_lfanew);
				return false;
			}

			if (!CheckBounds(DosHeader()->e_lfanew, sizeof(IMAGE_NT_HEADERS), m_buffer.size(), "NT header", outError, errorSize))
				return false;

			if (NtHeaders()->Signature != IMAGE_NT_SIGNATURE)
			{
				sprintf_s(outError, errorSize, "Invalid NT signature (0x%08X, expected 0x%08X).", NtHeaders()->Signature, IMAGE_NT_SIGNATURE);
				return false;
			}

			if (NtHeaders()->FileHeader.NumberOfSections == 0)
			{
				sprintf_s(outError, errorSize, "PE file has zero sections.");
				return false;
			}

			size_t sectionHeadersOffset = DosHeader()->e_lfanew + sizeof(IMAGE_NT_HEADERS);
			size_t sectionHeadersSize = NtHeaders()->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);

			if (!CheckBounds(sectionHeadersOffset, sectionHeadersSize, m_buffer.size(), "Section headers", outError, errorSize))
				return false;

			return true;
		}

		PIMAGE_DOS_HEADER DosHeader() { return reinterpret_cast<PIMAGE_DOS_HEADER>(m_buffer.data()); }
		PIMAGE_NT_HEADERS NtHeaders() { return reinterpret_cast<PIMAGE_NT_HEADERS>(m_buffer.data() + DosHeader()->e_lfanew); }
		PIMAGE_SECTION_HEADER Sections() { return IMAGE_FIRST_SECTION(NtHeaders()); }
		WORD SectionCount() const { return const_cast<PEFile*>(this)->NtHeaders()->FileHeader.NumberOfSections; }

		std::optional<WORD> FindSectionIndex(const char* name)
		{
			for (WORD i = 0; i < SectionCount(); i++)
			{
				if (SectionNameEquals(Sections()[i], name))
					return i;
			}

			return std::nullopt;
		}

		IMAGE_SECTION_HEADER* FindSection(const char* name)
		{
			auto index = FindSectionIndex(name);
			return index ? &Sections()[*index] : nullptr;
		}

		bool IsLAAEnabled() const { return (const_cast<PEFile*>(this)->NtHeaders()->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) != 0; }
		void EnableLAA() { NtHeaders()->FileHeader.Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE; }
		void ClearChecksum() { NtHeaders()->OptionalHeader.CheckSum = 0; }
		size_t Size() const { return m_buffer.size(); }
		uint8_t* Data() { return m_buffer.data(); }
		std::vector<uint8_t>& Buffer() { return m_buffer; }

	private:
		std::vector<uint8_t>& m_buffer;
	};

	// ========================
	// File I/O
	// ========================

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

	// ========================
	// Hashing
	// ========================

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

	// ========================
	// Validation
	// ========================

	inline bool ValidatePatchedFile(const fs::path& origPath, const fs::path& patchPath, bool hasBindSection, DWORD textSectionFileOffset, DWORD textSectionFileSize, uint32_t expectedTextHash)
	{
		char error[512];
		std::vector<uint8_t> origData, patchData;

		if (!ReadFile(origPath, origData))
		{
			sprintf_s(error, "Validation failed: Could not read original file.\nPath: %s", origPath.string().c_str());
			ShowError(error);
			return false;
		}

		if (!ReadFile(patchPath, patchData))
		{
			sprintf_s(error, "Validation failed: Could not read patched file.\nPath: %s", patchPath.string().c_str());
			ShowError(error);
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

		PEFile origPE(origData);
		PEFile patchPE(patchData);

		if (!origPE.Validate(error, sizeof(error)))
		{
			ShowError(error);
			return false;
		}

		if (!patchPE.Validate(error, sizeof(error)))
		{
			ShowError(error);
			return false;
		}

		if (!hasBindSection)
		{
			if (origData.size() != patchData.size())
			{
				sprintf_s(error, "Validation failed: File size changed when it shouldn't have (original=%zu, patched=%zu).", origData.size(), patchData.size());
				ShowError(error);
				return false;
			}

			origPE.EnableLAA();
			origPE.ClearChecksum();
			patchPE.ClearChecksum();

			if (memcmp(origData.data(), patchData.data(), origData.size()) != 0)
			{
				ShowError("Validation failed: File differs in more than just LAA flag and checksum.");
				return false;
			}

			return true;
		}

		if (!CheckBounds(textSectionFileOffset, textSectionFileSize, patchData.size(), ".text section", error, sizeof(error)))
		{
			ShowError(error);
			return false;
		}

		uint32_t patchTextHash = MurmurHash3(patchData.data() + textSectionFileOffset, textSectionFileSize);
		if (patchTextHash != expectedTextHash)
		{
			sprintf_s(error, "Validation failed: .text section hash mismatch.\n\nExpected: 0x%08X\nGot: 0x%08X\n\nThis may indicate a different game version or corrupted executable.", expectedTextHash, patchTextHash);
			ShowError(error);
			return false;
		}

		if (memcmp(origData.data(), patchData.data(), sizeof(IMAGE_DOS_HEADER)) != 0)
		{
			ShowError("Validation failed: DOS header unexpectedly changed.");
			return false;
		}

		auto* origNt = origPE.NtHeaders();
		auto* patchNt = patchPE.NtHeaders();

		if (!ValidateEqual(origNt->Signature, patchNt->Signature, "PE signature", error, sizeof(error)) ||
			!ValidateEqual(origNt->FileHeader.Machine, patchNt->FileHeader.Machine, "Machine type", error, sizeof(error)) ||
			!ValidateEqual(origNt->FileHeader.TimeDateStamp, patchNt->FileHeader.TimeDateStamp, "TimeDateStamp", error, sizeof(error)) ||
			!ValidateEqual(origNt->FileHeader.SizeOfOptionalHeader, patchNt->FileHeader.SizeOfOptionalHeader, "SizeOfOptionalHeader", error, sizeof(error)) ||
			!ValidateEqual(origNt->OptionalHeader.Magic, patchNt->OptionalHeader.Magic, "OptionalHeader Magic", error, sizeof(error)) ||
			!ValidateEqual(origNt->OptionalHeader.SizeOfCode, patchNt->OptionalHeader.SizeOfCode, "SizeOfCode", error, sizeof(error)) ||
			!ValidateEqual(origNt->OptionalHeader.ImageBase, patchNt->OptionalHeader.ImageBase, "ImageBase", error, sizeof(error)))
		{
			ShowError(error);
			return false;
		}

		auto* origSections = origPE.Sections();
		auto* patchSections = patchPE.Sections();

		for (WORD i = 0; i < origPE.SectionCount(); i++)
		{
			char sectionName[IMAGE_SIZEOF_SHORT_NAME + 1];
			GetSectionName(origSections[i], sectionName);

			if (strcmp(sectionName, ".text") == 0 || strcmp(sectionName, ".bind") == 0)
				continue;

			bool found = false;
			for (WORD j = 0; j < patchPE.SectionCount(); j++)
			{
				if (!SectionNameEquals(patchSections[j], sectionName))
					continue;

				char fieldName[64];

				sprintf_s(fieldName, "Section '%s' VirtualAddress", sectionName);
				if (!ValidateEqual(origSections[i].VirtualAddress, patchSections[j].VirtualAddress, fieldName, error, sizeof(error)))
				{
					ShowError(error);
					return false;
				}

				sprintf_s(fieldName, "Section '%s' VirtualSize", sectionName);
				if (!ValidateEqual(origSections[i].Misc.VirtualSize, patchSections[j].Misc.VirtualSize, fieldName, error, sizeof(error)))
				{
					ShowError(error);
					return false;
				}

				sprintf_s(fieldName, "Section '%s' Characteristics", sectionName);
				if (!ValidateEqual(origSections[i].Characteristics, patchSections[j].Characteristics, fieldName, error, sizeof(error)))
				{
					ShowError(error);
					return false;
				}

				DWORD origOffset = origSections[i].PointerToRawData;
				DWORD patchOffset = patchSections[j].PointerToRawData;
				DWORD size = std::min<DWORD>(origSections[i].SizeOfRawData, patchSections[j].SizeOfRawData);

				char boundsContext[64];
				sprintf_s(boundsContext, "Section '%s' in original", sectionName);
				if (!CheckBounds(origOffset, size, origData.size(), boundsContext, error, sizeof(error)))
				{
					ShowError(error);
					return false;
				}

				sprintf_s(boundsContext, "Section '%s' in patched", sectionName);
				if (!CheckBounds(patchOffset, size, patchData.size(), boundsContext, error, sizeof(error)))
				{
					ShowError(error);
					return false;
				}

				if (memcmp(origData.data() + origOffset, patchData.data() + patchOffset, size) != 0)
				{
					sprintf_s(error, "Validation failed: Section '%s' data changed unexpectedly.", sectionName);
					ShowError(error);
					return false;
				}

				found = true;
				break;
			}

			if (!found)
			{
				sprintf_s(error, "Validation failed: Section '%s' missing in patched file.", sectionName);
				ShowError(error);
				return false;
			}
		}

		return true;
	}

	// ========================
	// Main Patching Logic
	// ========================

	// Reference: https://github.com/nipkownix/re4_tweaks/blob/master/dllmain/LAApatch.cpp
	inline bool PerformLAAPatch(HMODULE hModule, bool showConfirmation)
	{
		char error[512];

		if (!hModule)
		{
			ShowError("PerformLAAPatch called with NULL module handle.");
			return false;
		}

		char modulePathRaw[MAX_PATH];
		DWORD pathLen = GetModuleFileNameA(hModule, modulePathRaw, MAX_PATH);
		if (pathLen == 0)
		{
			sprintf_s(error, "GetModuleFileNameA failed. Error: %d", GetLastError());
			ShowError(error);
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
			sprintf_s(error, "Executable path does not exist: %s", exePath.string().c_str());
			ShowError(error);
			return false;
		}

		std::vector<uint8_t> buffer;
		if (!ReadFile(exePath, buffer))
		{
			sprintf_s(error, "Could not read executable file.\nPath: %s\nError: %d", exePath.string().c_str(), GetLastError());
			ShowError(error);
			return false;
		}

		if (buffer.empty())
		{
			sprintf_s(error, "Executable file is empty.\nPath: %s", exePath.string().c_str());
			ShowError(error);
			return false;
		}

		std::error_code ec;
		auto spaceInfo = fs::space(exePath.parent_path(), ec);
		if (!ec)
		{
			uintmax_t requiredSpace = buffer.size() * 2;
			if (spaceInfo.available < requiredSpace)
			{
				sprintf_s(error, "Not enough disk space to create backup and patched files.\n\nRequired: %llu MB\nAvailable: %llu MB", (requiredSpace / (1024 * 1024)) + 1, spaceInfo.available / (1024 * 1024));
				ShowError(error);
				return false;
			}
		}

		PEFile pe(buffer);
		if (!pe.Validate(error, sizeof(error)))
		{
			ShowError(error);
			return false;
		}

		bool isLAA = pe.IsLAAEnabled();
		auto bindSectionIndex = pe.FindSectionIndex(".bind");
		auto* textSection = pe.FindSection(".text");

		DWORD textSectionVA = textSection ? textSection->VirtualAddress : 0;
		DWORD textSectionFileOffset = textSection ? textSection->PointerToRawData : 0;
		DWORD textSectionFileSize = textSection ? textSection->SizeOfRawData : 0;

		if (isLAA && !bindSectionIndex)
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

		LONG storedElfanew = pe.DosHeader()->e_lfanew;

		pe.EnableLAA();
		pe.ClearChecksum();

		if (bindSectionIndex && textSectionFileOffset > 0)
		{
			WORD bindIdx = *bindSectionIndex;
			auto* sections = pe.Sections();

			if (!CheckBounds(textSectionFileOffset, textSectionFileSize, buffer.size(), ".text section", error, sizeof(error)))
			{
				ShowError(error);
				return false;
			}

			DWORD bindOffset = sections[bindIdx].PointerToRawData;
			DWORD bindSize = sections[bindIdx].SizeOfRawData;

			if (!CheckBounds(bindOffset, bindSize, buffer.size(), ".bind section", error, sizeof(error)))
			{
				ShowError(error);
				return false;
			}

			size_t stubSize = storedElfanew - sizeof(IMAGE_DOS_HEADER);
			if (stubSize > 0)
			{
				memset(buffer.data() + sizeof(IMAGE_DOS_HEADER), 0, stubSize);
			}

			pe.NtHeaders()->OptionalHeader.AddressOfEntryPoint = 0x13E428;

			const uint8_t* textInMemory = reinterpret_cast<const uint8_t*>(hModule) + textSectionVA;
			memcpy(buffer.data() + textSectionFileOffset, textInMemory, textSectionFileSize);

			for (WORD i = 0; i < pe.SectionCount(); i++)
			{
				if (sections[i].PointerToRawData > bindOffset)
					sections[i].PointerToRawData -= bindSize;
			}

			for (WORD i = bindIdx; i < pe.SectionCount() - 1; i++)
			{
				sections[i] = sections[i + 1];
			}

			pe.NtHeaders()->FileHeader.NumberOfSections--;
			memset(&sections[pe.SectionCount()], 0, sizeof(IMAGE_SECTION_HEADER));

			DWORD newSizeOfImage = 0;
			for (WORD i = 0; i < pe.SectionCount(); i++)
			{
				DWORD sectionEnd = sections[i].VirtualAddress + sections[i].Misc.VirtualSize;
				if (sectionEnd > newSizeOfImage)
				{
					newSizeOfImage = sectionEnd;
				}
			}

			DWORD align = pe.NtHeaders()->OptionalHeader.SectionAlignment;
			if (align == 0)
			{
				ShowError("SectionAlignment is zero, cannot calculate SizeOfImage.");
				return false;
			}
			pe.NtHeaders()->OptionalHeader.SizeOfImage = (newSizeOfImage + align - 1) & ~(align - 1);

			buffer.erase(buffer.begin() + bindOffset, buffer.begin() + bindOffset + bindSize);
		}

		if (fs::exists(newPath))
		{
			fs::remove(newPath, ec);
			if (ec)
			{
				sprintf_s(error, "Failed to remove existing .new file.\nPath: %s\nError: %s", newPath.string().c_str(), ec.message().c_str());
				ShowError(error);
				return false;
			}
		}

		{
			std::ofstream outFile(newPath, std::ios::binary);
			if (!outFile)
			{
				DWORD err = GetLastError();
				if (err == ERROR_ACCESS_DENIED)
					sprintf_s(error, "Unable to write patched file (Access Denied).\n\nYour Anti-Virus may be blocking modification.\nPlease add an exception for this folder.\nPath: %s", newPath.string().c_str());
				else
					sprintf_s(error, "Failed to create patched file.\nPath: %s\nError: %d", newPath.string().c_str(), err);

				ShowError(error);
				return false;
			}

			outFile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());

			if (!outFile.good())
			{
				sprintf_s(error, "Failed to write patched file data.\nPath: %s", newPath.string().c_str());
				ShowError(error);
				fs::remove(newPath, ec);
				return false;
			}
		}

		Sleep(100);

		auto writtenSize = fs::file_size(newPath, ec);
		if (ec || writtenSize != buffer.size())
		{
			sprintf_s(error, "Written file size mismatch (expected=%zu, actual=%zu).\nPath: %s", buffer.size(), static_cast<size_t>(writtenSize), newPath.string().c_str());
			ShowError(error);
			fs::remove(newPath, ec);
			return false;
		}

		if (!ValidatePatchedFile(exePath, newPath, bindSectionIndex.has_value(), textSectionFileOffset, textSectionFileSize, 0x96EC25CA))
		{
			fs::remove(newPath, ec);
			return false;
		}

		if (!MoveFileExA(exePath.string().c_str(), bakPath.string().c_str(), MOVEFILE_REPLACE_EXISTING))
		{
			DWORD err = GetLastError();

			if (err == ERROR_ACCESS_DENIED)
			{
				ShowError("LAA Patch failed: Access Denied (Error 5).\n\n"
					"Windows is blocking the file modification.\n"
					"Please restart the game as Administrator to apply the patch.\n\n"
					"You can disable this check by setting CheckLAAPatch=0 in EchoPatch.ini");
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
					sprintf_s(error, "LAA Patch failed: File is locked by another process.\n\nError: %d\n\nYou can disable this check by setting CheckLAAPatch=0 in EchoPatch.ini", err);
					ShowError(error);
				}

				fs::remove(newPath, ec);
				return false;
			}

			sprintf_s(error, "Failed to backup original executable.\n\nError: %d", err);
			ShowError(error);
			fs::remove(newPath, ec);
			return false;
		}

		if (!MoveFileA(newPath.string().c_str(), exePath.string().c_str()))
		{
			DWORD err = GetLastError();
			MoveFileA(bakPath.string().c_str(), exePath.string().c_str());
			sprintf_s(error, "Failed to rename patched file to original. Restored from backup.\n\nError: %d", err);
			ShowError(error);
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
			sprintf_s(error, "Patching completed successfully, but failed to restart the application.\nShellExecute returned: %d\n\nPlease restart the game manually.", static_cast<int>(shellResult));
			MessageBoxA(NULL, error, "EchoPatch - LAAPatcher Warning", MB_ICONWARNING);
			return true;
		}
	}
}