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
	constexpr uintptr_t STEAM_IMAGE_BASE = 0x400000;
	constexpr size_t STEAM_FILE_SIZE = 0x1E3000;
	constexpr size_t STEAM_TEXT_OFFSET = 0x1000;
	constexpr size_t STEAM_TEXT_ENCRYPTED_SIZE = 0x14AFF0;
	constexpr size_t STEAM_KEY_OFFSET = 0x1E20A8;
	constexpr size_t STEAM_IV_OFFSET = 0x1E1BB4;
	constexpr DWORD STEAM_ENTRY_POINT = 0x13E428;
	constexpr uint32_t STEAM_PATCHED_HASH = 0x8786B8E8;

	constexpr wchar_t RESTART_ENV_VAR[] = L"ECHOPATCH_LAA_RESTART";
	constexpr wchar_t PATCH_ENV_VAR[] = L"ECHOPATCH_LAA_PATCH";

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

	class AES256
	{
	public:
		explicit AES256(const uint8_t* key)
		{
			uint8_t p = 1, q = 1;
			do
			{
				p ^= (p << 1) ^ ((p & 0x80) ? 0x1B : 0);
				q ^= q << 1;
				q ^= q << 2;
				q ^= q << 4;
				q ^= (q & 0x80) ? 0x09 : 0;

				uint8_t value = q ^ RotateLeft(q, 1) ^ RotateLeft(q, 2) ^ RotateLeft(q, 3) ^ RotateLeft(q, 4) ^ 0x63;
				m_sbox[p] = value;
				m_inverseSbox[value] = p;
			} while (p != 1);

			m_sbox[0] = 0x63;
			m_inverseSbox[0x63] = 0;

			for (int i = 0; i < 256; i++)
			{
				m_multiply[0][i] = Multiply(static_cast<uint8_t>(i), 14);
				m_multiply[1][i] = Multiply(static_cast<uint8_t>(i), 11);
				m_multiply[2][i] = Multiply(static_cast<uint8_t>(i), 13);
				m_multiply[3][i] = Multiply(static_cast<uint8_t>(i), 9);
			}

			memcpy(m_roundKeys, key, 32);

			uint8_t roundConstant = 1;
			for (int i = 32; i < 15 * 16; i += 4)
			{
				uint8_t word[4];
				memcpy(word, m_roundKeys + i - 4, 4);

				if (i % 32 == 0)
				{
					uint8_t first = word[0];
					word[0] = m_sbox[word[1]] ^ roundConstant;
					word[1] = m_sbox[word[2]];
					word[2] = m_sbox[word[3]];
					word[3] = m_sbox[first];
					roundConstant = Multiply(roundConstant, 2);
				}
				else if (i % 32 == 16)
				{
					for (uint8_t& value : word)
					{
						value = m_sbox[value];
					}
				}

				for (int j = 0; j < 4; j++)
				{
					m_roundKeys[i + j] = m_roundKeys[i - 32 + j] ^ word[j];
				}
			}
		}

		void DecryptBlock(const uint8_t* in, uint8_t* out) const
		{
			uint8_t state[16], shifted[16];

			for (int i = 0; i < 16; i++)
			{
				state[i] = in[i] ^ m_roundKeys[14 * 16 + i];
			}

			for (int round = 13; ; round--)
			{
				// InvShiftRows, InvSubBytes and AddRoundKey
				for (int i = 0; i < 16; i++)
				{
					shifted[i] = m_inverseSbox[state[(i - 4 * (i % 4)) & 15]] ^ m_roundKeys[round * 16 + i];
				}

				if (round == 0)
					break;

				// InvMixColumns
				for (int i = 0; i < 16; i++)
				{
					int column = i & ~3;
					state[i] = m_multiply[0][shifted[i]] ^ m_multiply[1][shifted[column + ((i + 1) & 3)]] ^ m_multiply[2][shifted[column + ((i + 2) & 3)]] ^ m_multiply[3][shifted[column + ((i + 3) & 3)]];
				}
			}

			memcpy(out, shifted, 16);
		}

	private:
		static uint8_t RotateLeft(uint8_t value, int count) { return static_cast<uint8_t>((value << count) | (value >> (8 - count))); }

		// Multiplication in GF(2^8)
		static uint8_t Multiply(uint8_t a, uint8_t b)
		{
			uint8_t result = 0;

			for (; b; b >>= 1)
			{
				if (b & 1)
					result ^= a;

				a = static_cast<uint8_t>((a << 1) ^ ((a & 0x80) ? 0x1B : 0));
			}

			return result;
		}

		uint8_t m_sbox[256];
		uint8_t m_inverseSbox[256];
		uint8_t m_multiply[4][256];
		uint8_t m_roundKeys[15 * 16];
	};

	inline void ReadSteamStubData(const std::vector<uint8_t>& buffer, size_t offset, uint8_t* outData, size_t size)
	{
		for (size_t i = 0; i < size; i += 4)
		{
			uint32_t value, previous;
			memcpy(&value, buffer.data() + offset + i, sizeof(uint32_t));
			memcpy(&previous, buffer.data() + offset + i - 4, sizeof(uint32_t));

			value ^= previous;
			memcpy(outData + i, &value, sizeof(uint32_t));
		}
	}

	inline bool HasSteamStub(HMODULE hModule)
	{
		auto* dos = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
		auto* nt = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uint8_t*>(hModule) + dos->e_lfanew);

		return SectionNameEquals(IMAGE_FIRST_SECTION(nt)[nt->FileHeader.NumberOfSections - 1], ".bind");
	}

	inline bool IsSteamStubBroken(HMODULE hModule)
	{
		return HasSteamStub(hModule) && reinterpret_cast<uintptr_t>(hModule) != STEAM_IMAGE_BASE;
	}

	inline bool RemoveSteamStub(PEFile& pe)
	{
		std::vector<uint8_t>& buffer = pe.Buffer();
		IMAGE_SECTION_HEADER* bindSection = pe.FindSection(".bind");

		if (buffer.size() != STEAM_FILE_SIZE || !bindSection || bindSection->PointerToRawData > buffer.size())
			return false;

		uint8_t key[32], iv[32];
		ReadSteamStubData(buffer, STEAM_KEY_OFFSET, key, sizeof(key));
		ReadSteamStubData(buffer, STEAM_IV_OFFSET, iv, sizeof(iv));

		AES256 aes(key);
		uint8_t* text = buffer.data() + STEAM_TEXT_OFFSET;

		std::vector<uint8_t> encrypted(16 + STEAM_TEXT_ENCRYPTED_SIZE);
		memcpy(encrypted.data(), iv + 16, 16);
		memcpy(encrypted.data() + 16, text, STEAM_TEXT_ENCRYPTED_SIZE);
		aes.DecryptBlock(iv, iv);

		const uint8_t* previous = iv;
		for (size_t i = 0; i < STEAM_TEXT_ENCRYPTED_SIZE; i += 16)
		{
			aes.DecryptBlock(encrypted.data() + i, text + i);

			for (int j = 0; j < 16; j++)
			{
				text[i + j] ^= previous[j];
			}

			previous = encrypted.data() + i;
		}

		memset(buffer.data() + sizeof(IMAGE_DOS_HEADER), 0, pe.DosHeader()->e_lfanew - sizeof(IMAGE_DOS_HEADER));

		pe.NtHeaders()->OptionalHeader.AddressOfEntryPoint = STEAM_ENTRY_POINT;
		pe.NtHeaders()->OptionalHeader.SizeOfImage = bindSection->VirtualAddress;
		pe.NtHeaders()->FileHeader.NumberOfSections--;

		buffer.resize(bindSection->PointerToRawData);
		memset(bindSection, 0, sizeof(IMAGE_SECTION_HEADER));

		return MurmurHash3(buffer.data(), buffer.size()) == STEAM_PATCHED_HASH;
	}

	// ========================
	// File Replacement
	// ========================

	// Writes the patched file next to the executable, validates it, then swaps them (the original is kept as a backup)
	inline bool ReplaceExecutable(const fs::path& exePath, const std::vector<uint8_t>& buffer)
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

		std::vector<uint8_t> writtenData;
		if (!ReadFile(newPath, writtenData) || writtenData != buffer)
		{
			sprintf_s(error, "Validation failed: Patched file was not written correctly.\nPath: %s", PathToString(newPath).c_str());
			ShowError(error);
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

	inline bool StartNewInstance(const fs::path& exePath, bool applyPatch = false)
	{
		SetEnvironmentVariableW(RESTART_ENV_VAR, std::to_wstring(GetCurrentProcessId()).c_str());
		SetEnvironmentVariableW(PATCH_ENV_VAR, applyPatch ? L"1" : NULL);

		std::wstring commandLine = GetCommandLineW();
		STARTUPINFOW si = { sizeof(si) };
		PROCESS_INFORMATION pi = {};

		if (!CreateProcessW(exePath.wstring().c_str(), commandLine.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		{
			DWORD err = GetLastError();
			SetEnvironmentVariableW(RESTART_ENV_VAR, NULL);
			SetEnvironmentVariableW(PATCH_ENV_VAR, NULL);
			SetLastError(err);
			return false;
		}

		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		return true;
	}

	inline bool IsPatchRequested()
	{
		if (GetEnvironmentVariableW(PATCH_ENV_VAR, NULL, 0) == 0)
			return false;

		SetEnvironmentVariableW(PATCH_ENV_VAR, NULL);
		return true;
	}

	// ========================
	// Main Patching Logic
	// ========================

	// Also asked by the EchoPatch menu
	inline std::string GetPatchQuestion(const fs::path& exeName, bool patchExpansions)
	{
		return "Your game executable is missing the 4GB/LAA patch. "
			"This allows the game to use 4GB of memory instead of 2GB, which prevents crashes when loading levels.\n\n"
			"EchoPatch will patch " + PathToString(exeName) + (patchExpansions ? " (and the expansions, if installed)" : "") + " and create a backup.\n\n"
			"Apply patch and restart the game?";
	}

	// Also the case without the LAA flag when it is forced for every game (Proton)
	inline bool HasLargeAddressSpace()
	{
		MEMORYSTATUSEX status = { sizeof(status) };
		return GlobalMemoryStatusEx(&status) && status.ullTotalVirtual > 0x80000000;
	}

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

		ReplaceExecutable(exePath, buffer);
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

		bool hasSteamStub = HasSteamStub(hModule);
		bool isSteamStubBroken = IsSteamStubBroken(hModule);

		// Nothing to gain from the patch when the game can already use 4GB of memory
		if (HasLargeAddressSpace() && !isSteamStubBroken)
			return PatchResult::Patched;

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

		if (pe.IsLAAEnabled() && !hasSteamStub)
			return PatchResult::Patched;

		pe.EnableLAA();
		pe.ClearChecksum();

		// Check that the Steam DRM can be removed before asking anything
		if (hasSteamStub && !RemoveSteamStub(pe))
		{
			ShowError("LAA Patch failed: The Steam DRM could not be removed.\n\n"
				"This may indicate a different game version or a modified executable.\n\n"
				"You can disable this check by setting CheckLAAPatch=0 in EchoPatch.ini");
			return PatchResult::Failed;
		}

		if (showConfirmation)
		{
			const char* title = "4GB/Large Address Aware patch missing!";
			std::string msg = GetPatchQuestion(exeName, patchExpansions) + "\n\n"
				"(Selecting No disables this check, it can be enabled again by setting CheckLAAPatch=1 in the [Fixes] section of EchoPatch.ini)";

			if (isSteamStubBroken)
			{
				title = "The game cannot start!";
				msg = "The game cannot start because the Steam DRM does not support its executable being relocated by Windows (Mandatory ASLR).\n\n"
					"EchoPatch will remove the Steam DRM from " + PathToString(exeName) + ", apply the 4GB/LAA patch" + (patchExpansions ? " (also to the expansions, if installed)" : "") + " and create a backup.\n\n"
					"Apply patch and restart the game?\n\n"
					"(Selecting No closes the game, the other solution is to turn off \"Force randomization for images (Mandatory ASLR)\" for " + PathToString(exeName) + " in the Windows exploit protection settings)";
			}

			if (MessageBoxA(NULL, msg.c_str(), title, MB_YESNO | MB_ICONEXCLAMATION) != IDYES)
				return PatchResult::Declined;
		}

		if (!ReplaceExecutable(exePath, buffer))
			return PatchResult::Failed;

		if (patchExpansions)
		{
			PatchExpansion(exePath.parent_path() / "FEARXP" / "FEARXP.exe");
			PatchExpansion(exePath.parent_path() / "FEARXP2" / "FEARXP2.exe");
		}

		if (StartNewInstance(exePath))
		{
			ExitProcess(0);
		}

		sprintf_s(error, "Patching completed successfully, but failed to restart the application.\nError: %d\n\nPlease restart the game manually.", GetLastError());
		MessageBoxA(NULL, error, "EchoPatch - LAAPatcher Warning", MB_ICONWARNING);
		return PatchResult::Patched;
	}
}