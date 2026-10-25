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
	constexpr DWORD STEAM_ENTRY_POINT = 0x13E428;
	constexpr uint32_t STEAM_TEXT_HASH = 0x96EC25CA;
	constexpr uint32_t STEAM_PATCHED_HASH = 0x8786B8E8;

	constexpr wchar_t RESTART_ENV_VAR[] = L"ECHOPATCH_LAA_RESTART";

	enum class PatchResult
	{
		Patched,
		Declined,
		Failed
	};

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

	// fs::path::string() throws if the path has characters that don't exist in the ANSI code page
	inline std::string PathToString(const fs::path& path)
	{
		std::wstring widePath = path.wstring();
		int size = WideCharToMultiByte(CP_ACP, 0, widePath.c_str(), -1, NULL, 0, NULL, NULL);
		if (size <= 0)
			return "?";

		std::string result(size, '\0');
		WideCharToMultiByte(CP_ACP, 0, widePath.c_str(), -1, result.data(), size, NULL, NULL);
		result.resize(size - 1);
		return result;
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
		if (offset <= bufferSize && size <= bufferSize - offset)
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

			if (DosHeader()->e_lfanew < static_cast<LONG>(sizeof(IMAGE_DOS_HEADER)))
			{
				sprintf_s(outError, errorSize, "Invalid e_lfanew value (%d, must be at least %zu).", DosHeader()->e_lfanew, sizeof(IMAGE_DOS_HEADER));
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

			size_t sectionHeadersOffset = DosHeader()->e_lfanew + offsetof(IMAGE_NT_HEADERS, OptionalHeader) + NtHeaders()->FileHeader.SizeOfOptionalHeader;
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

	inline bool MoveFileWithRetry(const fs::path& from, const fs::path& to, DWORD flags)
	{
		for (int i = 0; i < 20; i++)
		{
			if (MoveFileExW(from.wstring().c_str(), to.wstring().c_str(), flags))
				return true;

			DWORD err = GetLastError();
			if (err != ERROR_SHARING_VIOLATION && err != ERROR_LOCK_VIOLATION && err != ERROR_ACCESS_DENIED)
				return false;

			Sleep(100);
		}

		return false;
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

	inline bool ValidatePatchedFile(const fs::path& origPath, const fs::path& patchPath, bool hasBindSection, DWORD textSectionFileOffset, DWORD textSectionFileSize, uint32_t expectedTextHash, uint32_t expectedFileHash)
	{
		char error[512];
		std::vector<uint8_t> origData, patchData;

		if (!ReadFile(origPath, origData))
		{
			sprintf_s(error, "Validation failed: Could not read original file.\nPath: %s", PathToString(origPath).c_str());
			ShowError(error);
			return false;
		}

		if (!ReadFile(patchPath, patchData))
		{
			sprintf_s(error, "Validation failed: Could not read patched file.\nPath: %s", PathToString(patchPath).c_str());
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

		uint32_t patchFileHash = MurmurHash3(patchData.data(), patchData.size());
		if (patchFileHash != expectedFileHash)
		{
			sprintf_s(error, "Validation failed: Patched file hash mismatch.\n\nExpected: 0x%08X\nGot: 0x%08X\n\nThis may indicate a different game version or corrupted executable.", expectedFileHash, patchFileHash);
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
	// File Replacement
	// ========================

	// Writes the patched file next to the executable, validates it, then swaps them (the original is kept as a backup)
	inline bool ReplaceExecutable(const fs::path& exePath, const std::vector<uint8_t>& buffer, bool hasBindSection, DWORD textSectionFileOffset, DWORD textSectionFileSize)
	{
		char error[512];
		std::error_code ec;

		std::string exeName = PathToString(exePath.filename());
		fs::path newPath = exePath; newPath += ".new";
		fs::path bakPath = exePath; bakPath += ".bak";

		if (fs::exists(newPath, ec))
		{
			fs::remove(newPath, ec);
			if (ec)
			{
				sprintf_s(error, "Failed to remove existing .new file.\nPath: %s\nError: %d", PathToString(newPath).c_str(), ec.value());
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
					sprintf_s(error, "Unable to write patched file (Access Denied).\n\nYour Anti-Virus may be blocking modification.\nPlease add an exception for this folder.\nPath: %s", PathToString(newPath).c_str());
				else
					sprintf_s(error, "Failed to create patched file.\nPath: %s\nError: %d", PathToString(newPath).c_str(), err);

				ShowError(error);
				return false;
			}

			outFile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());

			if (!outFile.good())
			{
				sprintf_s(error, "Failed to write patched file data.\nPath: %s", PathToString(newPath).c_str());
				ShowError(error);
				fs::remove(newPath, ec);
				return false;
			}
		}

		// Make sure the patched file is on disk before replacing the original
		HANDLE hNewFile = CreateFileW(newPath.wstring().c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hNewFile != INVALID_HANDLE_VALUE)
		{
			FlushFileBuffers(hNewFile);
			CloseHandle(hNewFile);
		}

		auto writtenSize = fs::file_size(newPath, ec);
		if (ec || writtenSize != buffer.size())
		{
			sprintf_s(error, "Written file size mismatch (expected=%zu, actual=%zu).\nPath: %s", buffer.size(), static_cast<size_t>(writtenSize), PathToString(newPath).c_str());
			ShowError(error);
			fs::remove(newPath, ec);
			return false;
		}

		if (!ValidatePatchedFile(exePath, newPath, hasBindSection, textSectionFileOffset, textSectionFileSize, STEAM_TEXT_HASH, STEAM_PATCHED_HASH))
		{
			fs::remove(newPath, ec);
			return false;
		}

		// Don't overwrite an existing backup unless it's identical to the current executable
		std::vector<uint8_t> bakData, exeData;
		if (fs::exists(bakPath, ec) && !(ReadFile(bakPath, bakData) && ReadFile(exePath, exeData) && bakData == exeData))
		{
			for (int i = 1; fs::exists(bakPath, ec); i++)
			{
				bakPath = exePath;
				bakPath += ".bak" + std::to_string(i);
			}
		}

		if (!MoveFileWithRetry(exePath, bakPath, MOVEFILE_REPLACE_EXISTING))
		{
			DWORD err = GetLastError();

			if (err == ERROR_ACCESS_DENIED)
			{
				sprintf_s(error, "LAA Patch failed: Access Denied (Error 5).\n\n"
					"Windows is blocking the modification of %s.\n"
					"Please restart the game as Administrator to apply the patch.\n\n"
					"You can disable this check by setting CheckLAAPatch=0 in EchoPatch.ini", exeName.c_str());
				ShowError(error);
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
					sprintf_s(error, "LAA Patch failed: %s is locked by another process.\n\nError: %d\n\nYou can disable this check by setting CheckLAAPatch=0 in EchoPatch.ini", exeName.c_str(), err);
					ShowError(error);
				}

				fs::remove(newPath, ec);
				return false;
			}

			sprintf_s(error, "Failed to backup %s.\n\nError: %d", exeName.c_str(), err);
			ShowError(error);
			fs::remove(newPath, ec);
			return false;
		}

		if (!MoveFileWithRetry(newPath, exePath, 0))
		{
			DWORD err = GetLastError();

			if (!MoveFileWithRetry(bakPath, exePath, 0))
			{
				sprintf_s(error, "Failed to rename patched file to %s, and restoring the backup also failed.\n\nError: %d\n\nPlease close the game and rename %s back to %s.", exeName.c_str(), err, PathToString(bakPath.filename()).c_str(), exeName.c_str());
				ShowError(error);
				return false;
			}

			sprintf_s(error, "Failed to rename patched file to %s. Restored from backup.\n\nError: %d", exeName.c_str(), err);
			ShowError(error);
			fs::remove(newPath, ec);
			return false;
		}

		return true;
	}

	// ========================
	// Restart Handling
	// ========================

	// The game only allows one instance, wait for the previous one to exit after a restart (called from DLL_PROCESS_ATTACH)
	inline void WaitForPreviousInstance()
	{
		wchar_t value[16];
		DWORD len = GetEnvironmentVariableW(RESTART_ENV_VAR, value, 16);
		if (len == 0 || len >= 16)
			return;

		SetEnvironmentVariableW(RESTART_ENV_VAR, NULL);

		DWORD pid = wcstoul(value, NULL, 10);
		HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, pid);
		if (hProcess)
		{
			WaitForSingleObject(hProcess, 10000);
			CloseHandle(hProcess);
		}
	}

	// ========================
	// Main Patching Logic
	// ========================

	inline void PatchExpansion(const fs::path& exePath)
	{
		char error[512];
		std::error_code ec;

		if (!fs::exists(exePath, ec))
			return;

		std::vector<uint8_t> buffer;
		if (!ReadFile(exePath, buffer))
		{
			sprintf_s(error, "Could not read executable file.\nPath: %s", PathToString(exePath).c_str());
			ShowError(error);
			return;
		}

		PEFile pe(buffer);
		if (!pe.Validate(error, sizeof(error)))
		{
			ShowError(PathToString(exePath.filename()) + ": " + error);
			return;
		}

		if (pe.IsLAAEnabled())
			return;

		pe.EnableLAA();
		pe.ClearChecksum();

		ReplaceExecutable(exePath, buffer, false, 0, 0);
	}

	// Reference: https://github.com/nipkownix/re4_tweaks/blob/master/dllmain/LAApatch.cpp
	inline PatchResult PerformLAAPatch(HMODULE hModule, bool showConfirmation, bool patchExpansions)
	{
		char error[512];

		if (!hModule)
		{
			ShowError("PerformLAAPatch called with NULL module handle.");
			return PatchResult::Failed;
		}

		wchar_t modulePathRaw[MAX_PATH];
		DWORD pathLen = GetModuleFileNameW(hModule, modulePathRaw, MAX_PATH);
		if (pathLen == 0)
		{
			sprintf_s(error, "GetModuleFileNameW failed. Error: %d", GetLastError());
			ShowError(error);
			return PatchResult::Failed;
		}

		if (pathLen >= MAX_PATH)
		{
			ShowError("Module path exceeds MAX_PATH. Path may be truncated.");
			return PatchResult::Failed;
		}

		fs::path exePath = modulePathRaw;
		fs::path exeName = exePath.filename();

		std::error_code ec;
		if (!fs::exists(exePath, ec))
		{
			sprintf_s(error, "Executable path does not exist: %s", PathToString(exePath).c_str());
			ShowError(error);
			return PatchResult::Failed;
		}

		std::vector<uint8_t> buffer;
		if (!ReadFile(exePath, buffer))
		{
			sprintf_s(error, "Could not read executable file.\nPath: %s\nError: %d", PathToString(exePath).c_str(), GetLastError());
			ShowError(error);
			return PatchResult::Failed;
		}

		if (buffer.empty())
		{
			sprintf_s(error, "Executable file is empty.\nPath: %s", PathToString(exePath).c_str());
			ShowError(error);
			return PatchResult::Failed;
		}

		auto spaceInfo = fs::space(exePath.parent_path(), ec);
		if (!ec)
		{
			uintmax_t requiredSpace = buffer.size() * 2;
			if (spaceInfo.available < requiredSpace)
			{
				sprintf_s(error, "Not enough disk space to create backup and patched files.\n\nRequired: %llu MB\nAvailable: %llu MB", (requiredSpace / (1024 * 1024)) + 1, spaceInfo.available / (1024 * 1024));
				ShowError(error);
				return PatchResult::Failed;
			}
		}

		PEFile pe(buffer);
		if (!pe.Validate(error, sizeof(error)))
		{
			ShowError(error);
			return PatchResult::Failed;
		}

		bool isLAA = pe.IsLAAEnabled();
		auto bindSectionIndex = pe.FindSectionIndex(".bind");
		auto* textSection = pe.FindSection(".text");

		DWORD textSectionVA = textSection ? textSection->VirtualAddress : 0;
		DWORD textSectionFileOffset = textSection ? textSection->PointerToRawData : 0;
		DWORD textSectionFileSize = textSection ? textSection->SizeOfRawData : 0;

		if (isLAA && !bindSectionIndex)
			return PatchResult::Patched;

		// Check the decrypted code in memory before asking anything
		if (bindSectionIndex)
		{
			uint32_t textHash = MurmurHash3(reinterpret_cast<const uint8_t*>(hModule) + textSectionVA, textSectionFileSize);
			if (textHash != STEAM_TEXT_HASH)
			{
				sprintf_s(error, "LAA Patch failed: .text section hash mismatch.\n\nExpected: 0x%08X\nGot: 0x%08X\n\n"
					"This may indicate a different game version, or another mod modifying the game code.\n\n"
					"You can disable this check by setting CheckLAAPatch=0 in EchoPatch.ini", STEAM_TEXT_HASH, textHash);
				ShowError(error);
				return PatchResult::Failed;
			}
		}

		if (showConfirmation)
		{
			std::string msg = "Your game executable is missing the 4GB/LAA patch. "
				"This allows the game to use 4GB of memory instead of 2GB, which prevents crashes when loading levels.\n\n"
				"EchoPatch will patch " + PathToString(exeName) + (patchExpansions ? " (and the expansions, if installed)" : "") + " and create a backup.\n\n"
				"Apply patch and restart the game?\n\n"
				"(Selecting No disables this check, it can be enabled again by setting CheckLAAPatch=1 in the [Fixes] section of EchoPatch.ini)";

			if (MessageBoxA(NULL, msg.c_str(), "4GB/Large Address Aware patch missing!", MB_YESNO | MB_ICONEXCLAMATION) != IDYES)
				return PatchResult::Declined;
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
				return PatchResult::Failed;
			}

			DWORD bindOffset = sections[bindIdx].PointerToRawData;
			DWORD bindSize = sections[bindIdx].SizeOfRawData;

			if (!CheckBounds(bindOffset, bindSize, buffer.size(), ".bind section", error, sizeof(error)))
			{
				ShowError(error);
				return PatchResult::Failed;
			}

			size_t stubSize = storedElfanew - sizeof(IMAGE_DOS_HEADER);
			if (stubSize > 0)
			{
				memset(buffer.data() + sizeof(IMAGE_DOS_HEADER), 0, stubSize);
			}

			pe.NtHeaders()->OptionalHeader.AddressOfEntryPoint = STEAM_ENTRY_POINT;

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
				return PatchResult::Failed;
			}
			pe.NtHeaders()->OptionalHeader.SizeOfImage = (newSizeOfImage + align - 1) & ~(align - 1);

			buffer.erase(buffer.begin() + bindOffset, buffer.begin() + bindOffset + bindSize);
		}

		if (!ReplaceExecutable(exePath, buffer, bindSectionIndex.has_value(), textSectionFileOffset, textSectionFileSize))
			return PatchResult::Failed;

		if (patchExpansions)
		{
			PatchExpansion(exePath.parent_path() / "FEARXP" / "FEARXP.exe");
			PatchExpansion(exePath.parent_path() / "FEARXP2" / "FEARXP2.exe");
		}

		// Restart with the same command line, the new instance waits for this one to exit
		SetEnvironmentVariableW(RESTART_ENV_VAR, std::to_wstring(GetCurrentProcessId()).c_str());

		std::wstring commandLine = GetCommandLineW();
		STARTUPINFOW si = { sizeof(si) };
		PROCESS_INFORMATION pi = {};

		if (CreateProcessW(exePath.wstring().c_str(), commandLine.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		{
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
			ExitProcess(0);
		}
		else
		{
			DWORD err = GetLastError();
			SetEnvironmentVariableW(RESTART_ENV_VAR, NULL);
			sprintf_s(error, "Patching completed successfully, but failed to restart the application.\nError: %d\n\nPlease restart the game manually.", err);
			MessageBoxA(NULL, error, "EchoPatch - LAAPatcher Warning", MB_ICONWARNING);
			return PatchResult::Patched;
		}
	}
}