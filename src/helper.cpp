#pragma once

#include "ini.hpp"
#include <dsound.h>
#include <stacktrace>
#include "Core/DInputProxy.hpp"
#include "MinHook.hpp"

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

	static const uint32_t* GetModuleByteCounts(BYTE* base, DWORD imageSize)
	{
		static BYTE* countedBase = nullptr;
		static DWORD countedSize = 0;
		static uint32_t counts[256];

		if (countedBase != base || countedSize != imageSize)
		{
			memset(counts, 0, sizeof(counts));

			for (DWORD i = 0; i < imageSize; i += 7)
				counts[base[i]]++;

			countedBase = base;
			countedSize = imageSize;
		}

		return counts;
	}

	inline DWORD PatternScan(HMODULE hModule, std::string_view signature)
	{
		auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
		if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return 0;

		auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<BYTE*>(hModule) + dosHeader->e_lfanew);
		if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return 0;

		BYTE* base = reinterpret_cast<BYTE*>(hModule);
		DWORD imageSize = ntHeaders->OptionalHeader.SizeOfImage;

		uint8_t pat[256] = {};
		uint8_t msk[256] = {};
		size_t patSize = 0;

		for (size_t i = 0; i < signature.length() && patSize < 256; i++)
		{
			if (signature[i] == ' ') continue;
			if (signature[i] == '?')
			{
				patSize++;
				if (i + 1 < signature.length() && signature[i + 1] == '?') i++;
			}
			else
			{
				auto h = [](char c) -> uint8_t
				{
					if (c >= '0' && c <= '9') return c - '0';
					if (c >= 'A' && c <= 'F') return c - 'A' + 10;
					if (c >= 'a' && c <= 'f') return c - 'a' + 10;
					return 0;
				};

				pat[patSize] = (h(signature[i]) << 4) | h(signature[i + 1]);
				msk[patSize] = 0xFF;
				patSize++;
				i++;
			}
		}

		if (patSize == 0 || imageSize < patSize) return 0;

		const uint32_t* counts = GetModuleByteCounts(base, imageSize);

		size_t a1 = SIZE_MAX;
		for (size_t i = 0; i < patSize; i++)
		{
			if (msk[i] && (a1 == SIZE_MAX || counts[pat[i]] < counts[pat[a1]])) a1 = i;
		}

		if (a1 == SIZE_MAX) return reinterpret_cast<DWORD>(base);

		size_t a2 = a1;
		for (size_t i = 0; i < patSize; i++)
		{
			if (msk[i] && i != a1 && (a2 == a1 || counts[pat[i]] < counts[pat[a2]])) a2 = i;
		}

		const bool useDual = (a2 != a1);

		__m128i simPat = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pat));
		__m128i simMask = _mm_loadu_si128(reinterpret_cast<const __m128i*>(msk));
		__m128i needle1 = _mm_set1_epi8(static_cast<char>(pat[a1]));
		__m128i needle2 = useDual ? _mm_set1_epi8(static_cast<char>(pat[a2])) : _mm_setzero_si128();

		BYTE* scanEnd = base + imageSize - patSize;

		size_t maxAnchor = useDual ? (a1 > a2 ? a1 : a2) : a1;
		size_t margin = maxAnchor + 16;
		if (margin < patSize + 15) margin = patSize + 15;
		if (margin < 31) margin = 31;

		BYTE* simdEnd = (imageSize > margin) ? base + imageSize - margin : base;
		BYTE* pos = base;

		while (pos <= simdEnd)
		{
			__m128i blk1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pos + a1));
			int hits = _mm_movemask_epi8(_mm_cmpeq_epi8(blk1, needle1));

			if (hits && useDual)
			{
				__m128i blk2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pos + a2));
				hits &= _mm_movemask_epi8(_mm_cmpeq_epi8(blk2, needle2));
			}

			while (hits)
			{
				unsigned long bit;
				_BitScanForward(&bit, hits);
				hits &= hits - 1;

				BYTE* cand = pos + bit;

				__m128i mem = _mm_loadu_si128(reinterpret_cast<const __m128i*>(cand));
				__m128i masked = _mm_and_si128(mem, simMask);

				if (_mm_movemask_epi8(_mm_cmpeq_epi8(masked, simPat)) == 0xFFFF)
				{
					if (patSize <= 16)
						return reinterpret_cast<DWORD>(cand);

					bool ok = true;
					for (size_t k = 16; k < patSize; k++)
					{
						if (msk[k] && cand[k] != pat[k]) { ok = false; break; }
					}

					if (ok) return reinterpret_cast<DWORD>(cand);
				}
			}

			pos += 16;
		}

		while (pos <= scanEnd)
		{
			if (pos[a1] == pat[a1] && (!useDual || pos[a2] == pat[a2]))
			{
				bool ok = true;
				for (size_t k = 0; k < patSize; k++)
				{
					if (msk[k] && pos[k] != pat[k]) { ok = false; break; }
				}

				if (ok) return reinterpret_cast<DWORD>(pos);
			}

			pos++;
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

	inline DWORD ResolveRelativeAddress(uintptr_t BaseAddress, size_t InstructionOffset)
	{
		if (BaseAddress == 0) return 0;

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

	static std::string GetModulePath()
	{
		HMODULE hModule = nullptr;

		// Get the module handle for the DLL containing this code
		GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&GetModulePath, &hModule);

		wchar_t path[FILENAME_MAX] = { 0 };
		GetModuleFileNameW(hModule, path, FILENAME_MAX);
		return std::filesystem::path(path).parent_path().string();
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

	static bool IsUALPresent()
	{
		for (const auto& entry : std::stacktrace::current())
		{
			HMODULE hModule = NULL;
			if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)entry.native_handle(), &hModule))
			{
				if (GetProcAddress(hModule, "IsUltimateASILoader") != NULL)
					return true;
			}
		}

		return false;
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

	inline std::string ReadString(const char* sectionName, const char* valueName, const std::string& defaultValue)
	{
		if (iniReader.has(sectionName) && iniReader.get(sectionName).has(valueName))
		{
			const std::string& s = iniReader.get(sectionName).get(valueName);
			if (!s.empty())
				return s;
		}
		return defaultValue;
	}
};

namespace HookHelper
{
	inline bool isMHInitialized = false;
	inline int batchDepth = 0;

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

	static void BeginBatch()
	{
		batchDepth++;
	}

	static void EndBatch()
	{
		if (--batchDepth > 0 || !isMHInitialized)
			return;

		MH_STATUS status = MH_ApplyQueued();
		if (status != MH_OK)
		{
			char errorMsg[0x100];
			sprintf_s(errorMsg, "Failed to enable the hooks\nError: %s", MH_StatusToString(status));
			MessageBoxA(NULL, errorMsg, "Error", MB_ICONERROR | MB_OK);
		}
	}

	static MH_STATUS EnableHook(LPVOID target)
	{
		return batchDepth > 0 ? MH_QueueEnableHook(target) : MH_EnableHook(target);
	}

	static bool ApplyHook(void* addr, LPVOID hookFunc, LPVOID* originalFunc, bool checkRegion = false)
	{
		if (!InitializeMinHook()) return false;

		if (checkRegion && !EnsureExecutable(addr))
		{
			char msg[0x100];
			sprintf_s(msg, "Could not make address %p executable.", addr);
			MessageBoxA(NULL, msg, "Error", MB_ICONERROR);
			return false;
		}

		MH_STATUS status = MH_CreateHook(addr, hookFunc, originalFunc);
		if (status != MH_OK)
		{
			char errorMsg[0x100];
			sprintf_s(errorMsg, "Failed to create hook at address: %p\nError: %s", addr, MH_StatusToString(status));
			MessageBoxA(NULL, errorMsg, "Error", MB_ICONERROR | MB_OK);
			return false;
		}

		status = EnableHook(addr);
		if (status != MH_OK)
		{
			char errorMsg[0x100];
			sprintf_s(errorMsg, "Failed to enable hook at address: %p\nError: %s", addr, MH_StatusToString(status));
			MessageBoxA(NULL, errorMsg, "Error", MB_ICONERROR | MB_OK);
			return false;
		}

		return true;
	}

	static bool ApplyHookReplaceable(void* addr, LPVOID hookFunc, LPVOID* originalFunc)
	{
		if (!InitializeMinHook()) return false;

		MH_STATUS status = MH_CreateHook(addr, hookFunc, originalFunc);
		if (status == MH_ERROR_ALREADY_CREATED)
		{
			MH_RemoveHook(addr);
			status = MH_CreateHook(addr, hookFunc, originalFunc);
		}

		if (status != MH_OK)
		{
			char errorMsg[0x100];
			sprintf_s(errorMsg, "Failed to create hook at address: %p\nError: %s", addr, MH_StatusToString(status));
			MessageBoxA(NULL, errorMsg, "Error", MB_ICONERROR | MB_OK);
			return false;
		}

		status = EnableHook(addr);
		if (status != MH_OK)
		{
			char errorMsg[0x100];
			sprintf_s(errorMsg, "Failed to enable hook at address: %p\nError: %s", addr, MH_StatusToString(status));
			MessageBoxA(NULL, errorMsg, "Error", MB_ICONERROR | MB_OK);
			return false;
		}

		return true;
	}

	static bool ApplyHookAPI(LPCWSTR moduleName, LPCSTR apiName, LPVOID hookFunc, LPVOID* originalFunc)
	{
		if (!InitializeMinHook()) return false;

		LPVOID target = nullptr;
		MH_STATUS status = MH_CreateHookApiEx(moduleName, apiName, hookFunc, originalFunc, &target);
		if (status != MH_OK)
		{
			char errorMsg[0x100];
			sprintf_s(errorMsg, "Failed to create hook for API: %s\nError: %s", apiName, MH_StatusToString(status));
			MessageBoxA(NULL, errorMsg, "Error", MB_ICONERROR | MB_OK);
			return false;
		}

		status = EnableHook(target);
		if (status != MH_OK)
		{
			char errorMsg[0x100];
			sprintf_s(errorMsg, "Failed to enable hook for API: %s\nError: %s", apiName, MH_StatusToString(status));
			MessageBoxA(NULL, errorMsg, "Error", MB_ICONERROR | MB_OK);
			return false;
		}

		return true;
	}
};

namespace DirectSoundHelper
{
	const GUID CLSID_DirectSound8 = { 0x3901CC3F, 0x84B5, 0x4FA4, {0xBA, 0x35, 0xAA, 0x81, 0x72, 0xB8, 0xA0, 0x9B} };

	inline HRESULT(WINAPI* ori_DirectSoundCreate8)(LPCGUID, LPDIRECTSOUND8*, LPUNKNOWN) = nullptr;
	inline HRESULT(WINAPI* ori_CoCreateInstance)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID*) = nullptr;

	inline HMODULE hLocalDSound = nullptr;
	inline decltype(ori_DirectSoundCreate8) cachedDSCreate8 = nullptr;

	inline thread_local bool isCreatingLocalDSound = false;

	inline HMODULE SafeLoadLibrary(const char* path)
	{
		__try 
		{
			return LoadLibraryA(path);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) 
		{
			return nullptr;
		}
	}

	inline bool IsDownmixableStereo(const WAVEFORMATEX* pFormat)
	{
		return pFormat && pFormat->wFormatTag == WAVE_FORMAT_PCM && pFormat->nChannels == 2 && (pFormat->wBitsPerSample == 8 || pFormat->wBitsPerSample == 16) && pFormat->nBlockAlign == pFormat->wBitsPerSample / 4;
	}

	class DownmixBufferProxy : public IDirectSoundBuffer8
	{
	private:
		struct PendingLock
		{
			std::unique_ptr<BYTE[]> data;
			void* pReal1;
			DWORD realBytes1;
			void* pReal2;
			DWORD realBytes2;
		};

		inline static void* s_vtable = nullptr;

		IDirectSoundBuffer8* m_pReal;
		volatile LONG m_refCount;
		WAVEFORMATEX m_format;
		SRWLOCK m_lock = SRWLOCK_INIT;
		std::vector<PendingLock> m_locks;

		DWORD ToMono(DWORD bytes) const { return bytes / m_format.nBlockAlign * (m_format.nBlockAlign / 2); }
		DWORD ToStereo(DWORD bytes) const { return bytes / (m_format.nBlockAlign / 2) * m_format.nBlockAlign; }

		void Downmix(const BYTE* pStereo, void* pMono, DWORD frames) const
		{
			if (m_format.wBitsPerSample == 16)
			{
				auto src = reinterpret_cast<const int16_t*>(pStereo);
				auto dst = static_cast<int16_t*>(pMono);
				for (DWORD i = 0; i < frames; i++)
				{
					dst[i] = static_cast<int16_t>((src[i * 2] + src[i * 2 + 1]) / 2);
				}
			}
			else
			{
				auto dst = static_cast<BYTE*>(pMono);
				for (DWORD i = 0; i < frames; i++)
				{
					dst[i] = static_cast<BYTE>((pStereo[i * 2] + pStereo[i * 2 + 1]) / 2);
				}
			}
		}

	public:
		DownmixBufferProxy(IDirectSoundBuffer8* pReal, const WAVEFORMATEX* pFormat) : m_pReal(pReal), m_refCount(1), m_format(*pFormat)
		{
			m_format.cbSize = 0;
			s_vtable = *reinterpret_cast<void**>(this);
		}

		~DownmixBufferProxy()
		{
			if (m_pReal)
				m_pReal->Release();
		}

		static DownmixBufferProxy* FromBuffer(IDirectSoundBuffer* pBuffer)
		{
			if (!pBuffer || !s_vtable || *reinterpret_cast<void**>(pBuffer) != s_vtable)
				return nullptr;

			return static_cast<DownmixBufferProxy*>(reinterpret_cast<IDirectSoundBuffer8*>(pBuffer));
		}

		static HRESULT Create(IDirectSoundBuffer* pMono, const WAVEFORMATEX* pFormat, LPDIRECTSOUNDBUFFER* ppDSBuffer)
		{
			IDirectSoundBuffer8* pMono8 = nullptr;
			HRESULT hr = pMono->QueryInterface(IID_IDirectSoundBuffer8, reinterpret_cast<LPVOID*>(&pMono8));
			pMono->Release();
			if (FAILED(hr))
				return hr;

			auto proxy = new (std::nothrow) DownmixBufferProxy(pMono8, pFormat);
			if (!proxy)
			{
				pMono8->Release();
				return DSERR_OUTOFMEMORY;
			}

			*ppDSBuffer = reinterpret_cast<LPDIRECTSOUNDBUFFER>(static_cast<IDirectSoundBuffer8*>(proxy));
			return DS_OK;
		}

		HRESULT Duplicate(IDirectSound8* pDS, LPDIRECTSOUNDBUFFER* ppDSBufferDuplicate)
		{
			IDirectSoundBuffer* pMono = nullptr;
			HRESULT hr = pDS->DuplicateSoundBuffer(reinterpret_cast<LPDIRECTSOUNDBUFFER>(m_pReal), &pMono);
			if (FAILED(hr))
				return hr;

			return Create(pMono, &m_format, ppDSBufferDuplicate);
		}

		// IUnknown
		STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppv)
		{
			if (!ppv) return E_POINTER;
			*ppv = nullptr;

			if (riid == IID_IUnknown || riid == IID_IDirectSoundBuffer || riid == IID_IDirectSoundBuffer8)
			{
				*ppv = static_cast<IDirectSoundBuffer8*>(this);
				AddRef();
				return S_OK;
			}

			return m_pReal->QueryInterface(riid, ppv);
		}

		STDMETHOD_(ULONG, AddRef)()
		{
			return InterlockedIncrement(&m_refCount);
		}

		STDMETHOD_(ULONG, Release)()
		{
			ULONG ref = InterlockedDecrement(&m_refCount);
			if (ref == 0) delete this;
			return ref;
		}

		// IDirectSoundBuffer
		STDMETHOD(GetCaps)(LPDSBCAPS pDSBufferCaps)
		{
			HRESULT hr = m_pReal->GetCaps(pDSBufferCaps);
			if (SUCCEEDED(hr))
				pDSBufferCaps->dwBufferBytes = ToStereo(pDSBufferCaps->dwBufferBytes);
			return hr;
		}

		STDMETHOD(GetCurrentPosition)(LPDWORD pdwCurrentPlayCursor, LPDWORD pdwCurrentWriteCursor)
		{
			HRESULT hr = m_pReal->GetCurrentPosition(pdwCurrentPlayCursor, pdwCurrentWriteCursor);
			if (SUCCEEDED(hr))
			{
				if (pdwCurrentPlayCursor) *pdwCurrentPlayCursor = ToStereo(*pdwCurrentPlayCursor);
				if (pdwCurrentWriteCursor) *pdwCurrentWriteCursor = ToStereo(*pdwCurrentWriteCursor);
			}
			return hr;
		}

		STDMETHOD(GetFormat)(LPWAVEFORMATEX pwfxFormat, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten)
		{
			if (!pwfxFormat && !pdwSizeWritten)
				return DSERR_INVALIDPARAM;

			if (pwfxFormat)
				memcpy(pwfxFormat, &m_format, (std::min)(dwSizeAllocated, static_cast<DWORD>(sizeof(m_format))));
			if (pdwSizeWritten)
				*pdwSizeWritten = sizeof(m_format);
			return DS_OK;
		}

		STDMETHOD(GetVolume)(LPLONG plVolume) { return m_pReal->GetVolume(plVolume); }
		STDMETHOD(GetPan)(LPLONG plPan) { return m_pReal->GetPan(plPan); }
		STDMETHOD(GetFrequency)(LPDWORD pdwFrequency) { return m_pReal->GetFrequency(pdwFrequency); }
		STDMETHOD(GetStatus)(LPDWORD pdwStatus) { return m_pReal->GetStatus(pdwStatus); }
		STDMETHOD(Initialize)(LPDIRECTSOUND pDirectSound, LPCDSBUFFERDESC pcDSBufferDesc) { return m_pReal->Initialize(pDirectSound, pcDSBufferDesc); }

		STDMETHOD(Lock)(DWORD dwOffset, DWORD dwBytes, LPVOID* ppvAudioPtr1, LPDWORD pdwAudioBytes1, LPVOID* ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags)
		{
			if (!ppvAudioPtr1 || !pdwAudioBytes1)
				return DSERR_INVALIDPARAM;

			PendingLock pending = {};
			HRESULT hr = m_pReal->Lock(ToMono(dwOffset), ToMono(dwBytes), &pending.pReal1, &pending.realBytes1, ppvAudioPtr2 ? &pending.pReal2 : nullptr, ppvAudioPtr2 ? &pending.realBytes2 : nullptr, dwFlags);
			if (FAILED(hr))
				return hr;

			DWORD bytes1 = ToStereo(pending.realBytes1);
			DWORD bytes2 = ToStereo(pending.realBytes2);
			pending.data.reset(new (std::nothrow) BYTE[bytes1 + bytes2]);
			if (!pending.data)
			{
				m_pReal->Unlock(pending.pReal1, 0, pending.pReal2, 0);
				return DSERR_OUTOFMEMORY;
			}

			*ppvAudioPtr1 = pending.data.get();
			*pdwAudioBytes1 = bytes1;
			if (ppvAudioPtr2) *ppvAudioPtr2 = bytes2 ? pending.data.get() + bytes1 : nullptr;
			if (pdwAudioBytes2) *pdwAudioBytes2 = bytes2;

			AcquireSRWLockExclusive(&m_lock);
			m_locks.push_back(std::move(pending));
			ReleaseSRWLockExclusive(&m_lock);
			return DS_OK;
		}

		STDMETHOD(Play)(DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags) { return m_pReal->Play(dwReserved1, dwPriority, dwFlags); }
		STDMETHOD(SetCurrentPosition)(DWORD dwNewPosition) { return m_pReal->SetCurrentPosition(ToMono(dwNewPosition)); }
		STDMETHOD(SetFormat)(LPCWAVEFORMATEX pcfxFormat) { return m_pReal->SetFormat(pcfxFormat); }
		STDMETHOD(SetVolume)(LONG lVolume) { return m_pReal->SetVolume(lVolume); }
		STDMETHOD(SetPan)(LONG lPan) { return m_pReal->SetPan(lPan); }
		STDMETHOD(SetFrequency)(DWORD dwFrequency) { return m_pReal->SetFrequency(dwFrequency); }
		STDMETHOD(Stop)() { return m_pReal->Stop(); }

		STDMETHOD(Unlock)(LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2)
		{
			PendingLock pending = {};
			AcquireSRWLockExclusive(&m_lock);
			for (auto it = m_locks.begin(); it != m_locks.end(); ++it)
			{
				if (it->data.get() == pvAudioPtr1)
				{
					pending = std::move(*it);
					m_locks.erase(it);
					break;
				}
			}
			ReleaseSRWLockExclusive(&m_lock);

			if (!pending.data)
				return DSERR_INVALIDPARAM;

			DWORD sampleBytes = m_format.nBlockAlign / 2;
			DWORD bytes1 = (std::min)(ToMono(dwAudioBytes1), pending.realBytes1);
			DWORD bytes2 = pvAudioPtr2 ? (std::min)(ToMono(dwAudioBytes2), pending.realBytes2) : 0;
			Downmix(pending.data.get(), pending.pReal1, bytes1 / sampleBytes);
			if (bytes2)
				Downmix(pending.data.get() + ToStereo(pending.realBytes1), pending.pReal2, bytes2 / sampleBytes);

			return m_pReal->Unlock(pending.pReal1, bytes1, pending.pReal2, bytes2);
		}

		STDMETHOD(Restore)() { return m_pReal->Restore(); }

		// IDirectSoundBuffer8
		STDMETHOD(SetFX)(DWORD dwEffectsCount, LPDSEFFECTDESC pDSFXDesc, LPDWORD pdwResultCodes) { return m_pReal->SetFX(dwEffectsCount, pDSFXDesc, pdwResultCodes); }
		STDMETHOD(AcquireResources)(DWORD dwFlags, DWORD dwEffectsCount, LPDWORD pdwResultCodes) { return m_pReal->AcquireResources(dwFlags, dwEffectsCount, pdwResultCodes); }
		STDMETHOD(GetObjectInPath)(REFGUID rguidObject, DWORD dwIndex, REFGUID rguidInterface, LPVOID* ppObject) { return m_pReal->GetObjectInPath(rguidObject, dwIndex, rguidInterface, ppObject); }
	};

	class DirectSound8Proxy : public IDirectSound8
	{
	private:
		IDirectSound8* m_pReal;
		volatile LONG m_refCount;

		HRESULT CreateBuffer(DSBUFFERDESC desc, LPDIRECTSOUNDBUFFER* ppDSBuffer, LPUNKNOWN pUnkOuter)
		{
			HRESULT hr = m_pReal->CreateSoundBuffer(&desc, ppDSBuffer, pUnkOuter);
			if (hr == DSERR_ALLOCATED && (desc.dwFlags & DSBCAPS_LOCHARDWARE))
			{
				desc.dwFlags &= ~DSBCAPS_LOCHARDWARE;
				hr = m_pReal->CreateSoundBuffer(&desc, ppDSBuffer, pUnkOuter);
			}
			return hr;
		}

	public:
		DirectSound8Proxy(IDirectSound8* pReal) : m_pReal(pReal), m_refCount(1) {}

		~DirectSound8Proxy()
		{
			if (m_pReal)
				m_pReal->Release();
		}

		// IUnknown
		STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppv)
		{
			if (!ppv) return E_POINTER;
			*ppv = nullptr;

			if (riid == IID_IUnknown || riid == IID_IDirectSound || riid == IID_IDirectSound8)
			{
				*ppv = static_cast<IDirectSound8*>(this);
				AddRef();
				return S_OK;
			}

			return m_pReal->QueryInterface(riid, ppv);
		}

		STDMETHOD_(ULONG, AddRef)()
		{
			return InterlockedIncrement(&m_refCount);
		}

		STDMETHOD_(ULONG, Release)()
		{
			ULONG ref = InterlockedDecrement(&m_refCount);
			if (ref == 0) delete this;
			return ref;
		}

		STDMETHOD(Initialize)(LPCGUID pcGuidDevice)
		{
			return DS_OK;
		}

		STDMETHOD(CreateSoundBuffer)(LPCDSBUFFERDESC pcDSBufferDesc, LPDIRECTSOUNDBUFFER* ppDSBuffer, LPUNKNOWN pUnkOuter)
		{
			if (!FixHWMixing || !pcDSBufferDesc || !ppDSBuffer)
				return m_pReal->CreateSoundBuffer(pcDSBufferDesc, ppDSBuffer, pUnkOuter);

			DSBUFFERDESC desc = {};
			memcpy(&desc, pcDSBufferDesc, (std::min)(static_cast<size_t>(pcDSBufferDesc->dwSize), sizeof(desc)));

			HRESULT hr = CreateBuffer(desc, ppDSBuffer, pUnkOuter);
			if (hr != DSERR_INVALIDPARAM || !(desc.dwFlags & DSBCAPS_CTRL3D) || !IsDownmixableStereo(desc.lpwfxFormat))
				return hr;

			const WAVEFORMATEX* pStereoFormat = desc.lpwfxFormat;
			WAVEFORMATEX monoFormat = *pStereoFormat;
			monoFormat.nChannels = 1;
			monoFormat.nBlockAlign = pStereoFormat->nBlockAlign / 2;
			monoFormat.nAvgBytesPerSec = monoFormat.nSamplesPerSec * monoFormat.nBlockAlign;
			monoFormat.cbSize = 0;

			desc.lpwfxFormat = &monoFormat;
			desc.dwBufferBytes = desc.dwBufferBytes / pStereoFormat->nBlockAlign * monoFormat.nBlockAlign;

			IDirectSoundBuffer* pMono = nullptr;
			hr = CreateBuffer(desc, &pMono, pUnkOuter);
			if (FAILED(hr))
				return hr;

			return DownmixBufferProxy::Create(pMono, pStereoFormat, ppDSBuffer);
		}

		STDMETHOD(GetCaps)(LPDSCAPS pDSCaps)
		{
			return m_pReal->GetCaps(pDSCaps);
		}

		STDMETHOD(DuplicateSoundBuffer)(LPDIRECTSOUNDBUFFER pDSBufferOriginal, LPDIRECTSOUNDBUFFER* ppDSBufferDuplicate)
		{
			DownmixBufferProxy* pDownmix = DownmixBufferProxy::FromBuffer(pDSBufferOriginal);
			if (pDownmix && ppDSBufferDuplicate)
				return pDownmix->Duplicate(m_pReal, ppDSBufferDuplicate);

			return m_pReal->DuplicateSoundBuffer(pDSBufferOriginal, ppDSBufferDuplicate);
		}

		STDMETHOD(SetCooperativeLevel)(HWND hwnd, DWORD dwLevel)
		{
			return m_pReal->SetCooperativeLevel(hwnd, dwLevel);
		}

		STDMETHOD(Compact)()
		{
			return m_pReal->Compact();
		}

		STDMETHOD(GetSpeakerConfig)(LPDWORD pdwSpeakerConfig)
		{
			return m_pReal->GetSpeakerConfig(pdwSpeakerConfig);
		}

		STDMETHOD(SetSpeakerConfig)(DWORD dwSpeakerConfig)
		{
			return m_pReal->SetSpeakerConfig(dwSpeakerConfig);
		}

		STDMETHOD(VerifyCertification)(LPDWORD pdwCertified)
		{
			return m_pReal->VerifyCertification(pdwCertified);
		}
	};

	inline HRESULT SafeCallDSCreate(LPUNKNOWN pUnkOuter, IDirectSound8** ppRealDS)
	{
		__try
		{
			return cachedDSCreate8(nullptr, ppRealDS, pUnkOuter);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return E_FAIL;
		}
	}

	inline void AllowStereo3DSounds()
	{
		HMODULE hSndDrv = GetModuleHandleA("SndDrv.dll");
		if (!hSndDrv)
			return;

		DWORD addr = ScanModuleSignature(hSndDrv, "66 83 78 02 ?? ?? 57 8D 78 02 8B", "FixHWMixing");
		if (addr != 0)
		{
			MemoryHelper::WriteMemory<uint8_t>(addr + 0x4, 2);
		}
	}

	inline HRESULT WINAPI CoCreateInstance_Hook(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv)
	{
		if (!ppv) return E_POINTER;
		*ppv = nullptr;

		// Only intercept DirectSound8 requests
		if (!isCreatingLocalDSound && IsEqualGUID(rclsid, CLSID_DirectSound8) && cachedDSCreate8 && pUnkOuter == nullptr && (riid == IID_IDirectSound8 || riid == IID_IDirectSound || riid == IID_IUnknown))
		{
			if (FixHWMixing)
				AllowStereo3DSounds();

			IDirectSound8* pRealDS = nullptr;
			isCreatingLocalDSound = true;
			HRESULT hr = SafeCallDSCreate(nullptr, &pRealDS);
			isCreatingLocalDSound = false;

			if (SUCCEEDED(hr) && pRealDS)
			{
				auto proxy = new (std::nothrow) DirectSound8Proxy(pRealDS);
				if (proxy)
				{
					hr = proxy->QueryInterface(riid, ppv);
					proxy->Release();
					if (SUCCEEDED(hr))
					{
						return S_OK;
					}
				}
				else
				{
					pRealDS->Release();
				}
			}
		}

		return ori_CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
	}

	inline bool Init()
	{
		// Build path to local dsound.dll next to the executable
		char exePath[MAX_PATH];
		if (!GetModuleFileNameA(NULL, exePath, MAX_PATH))
			return false;

		char* lastSlash = strrchr(exePath, '\\');
		if (lastSlash)
			*(lastSlash + 1) = '\0';
		else
			exePath[0] = '\0';

		std::string dsoundPath = std::string(exePath) + "dsound.dll";

		// Check if local dsound.dll exists
		if (!SystemHelper::FileExists(dsoundPath))
			return false;

		// Load the local DSOAL dsound.dll
		hLocalDSound = SafeLoadLibrary(dsoundPath.c_str());
		if (!hLocalDSound)
			return false;

		// Verify it exports DirectSoundCreate8
		cachedDSCreate8 = reinterpret_cast<decltype(ori_DirectSoundCreate8)>(GetProcAddress(hLocalDSound, "DirectSoundCreate8"));

		if (!cachedDSCreate8)
		{
			FreeLibrary(hLocalDSound);
			hLocalDSound = nullptr;
			return false;
		}

		if (!HookHelper::ApplyHookAPI(L"ole32.dll", "CoCreateInstance", &CoCreateInstance_Hook, (LPVOID*)&ori_CoCreateInstance))
		{
			FreeLibrary(hLocalDSound);
			hLocalDSound = nullptr;
			cachedDSCreate8 = nullptr;
			return false;
		}

		return true;
	}
};

namespace HashHelper
{
	constexpr uint32_t FNV_PRIME = 0x01000193;
	constexpr uint32_t FNV_OFFSET_BASIS = 0x811C9DC5;

	constexpr uint32_t FNV1a(const char* str, uint32_t hash = FNV_OFFSET_BASIS)
	{
		return (*str == '\0') ? hash : FNV1a(str + 1, (hash ^ static_cast<uint32_t>(*str)) * FNV_PRIME);
	}

	inline uint32_t FNV1aRuntime(const char* str)
	{
		uint32_t hash = FNV_OFFSET_BASIS;
		while (*str)
		{
			hash ^= static_cast<uint32_t>(*str++);
			hash *= FNV_PRIME;
		}
		return hash;
	}

	inline uint32_t FNV1aRuntime(const char* str, size_t length)
	{
		uint32_t hash = FNV_OFFSET_BASIS;
		for (size_t i = 0; i < length; ++i)
		{
			hash ^= static_cast<uint32_t>(str[i]);
			hash *= FNV_PRIME;
		}
		return hash;
	}

	constexpr uint32_t operator""_hash(const char* str, size_t)
	{
		return FNV1a(str);
	}

	// Precomputed HUD element hashes
	struct HUDHashes
	{
		static constexpr uint32_t HUDSlowMo2 = FNV1a("HUDSlowMo2");
		static constexpr uint32_t HUDFlashlight = FNV1a("HUDFlashlight");
		static constexpr uint32_t HUDHealth = FNV1a("HUDHealth");
		static constexpr uint32_t HUDScores = FNV1a("HUDScores");
		static constexpr uint32_t HUDRespawn = FNV1a("HUDRespawn");
		static constexpr uint32_t HUDControlPointBar = FNV1a("HUDControlPointBar");
		static constexpr uint32_t HUDDialogue = FNV1a("HUDDialogue");
		static constexpr uint32_t AdditionalRect = FNV1a("AdditionalRect");
		static constexpr uint32_t AdditionalFloat = FNV1a("AdditionalFloat");
		static constexpr uint32_t AdditionalInt = FNV1a("AdditionalInt");
		static constexpr uint32_t TextSize = FNV1a("TextSize");
		static constexpr uint32_t NavMarkerTextSize = FNV1a("Text Size");
		static constexpr uint32_t FrameRect = FNV1a("FrameRect");
	};

	// Precomputed string hashes
	struct StringHashes
	{
		static constexpr uint32_t IDS_QUICKSAVE = FNV1a("IDS_QUICKSAVE");
		static constexpr uint32_t ScreenFailure_PressAnyKey = FNV1a("ScreenFailure_PressAnyKey");
		static constexpr uint32_t IDS_SELECT = FNV1a("IDS_SELECT");
		static constexpr uint32_t IDS_CATEGORY = FNV1a("IDS_CATEGORY");
		static constexpr uint32_t ScreenCrosshair_Size_Help = FNV1a("ScreenCrosshair_Size_Help");
		static constexpr uint32_t IDS_HELP_PICKUP_MSG_DUR = FNV1a("IDS_HELP_PICKUP_MSG_DUR");

		static constexpr uint32_t IDS_CONTROLLER_SENSITIVITY = FNV1a("IDS_CONTROLLER_SENSITIVITY");
		static constexpr uint32_t IDS_EDGE_ACCELERATION = FNV1a("IDS_EDGE_ACCELERATION");
		static constexpr uint32_t IDS_GYRO_SENSITIVITY = FNV1a("IDS_GYRO_SENSITIVITY");
		static constexpr uint32_t IDS_GYRO_SMOOTHING = FNV1a("IDS_GYRO_SMOOTHING");

		static constexpr uint32_t IDS_HELP_CONTROLLER_SENSITIVITY = FNV1a("IDS_HELP_CONTROLLER_SENSITIVITY");
		static constexpr uint32_t IDS_HELP_EDGE_ACCELERATION = FNV1a("IDS_HELP_EDGE_ACCELERATION");
		static constexpr uint32_t IDS_HELP_RUMBLE = FNV1a("IDS_HELP_RUMBLE");
		static constexpr uint32_t IDS_HELP_GYRO_ENABLED = FNV1a("IDS_HELP_GYRO_ENABLED");
		static constexpr uint32_t IDS_HELP_GYRO_TYPE = FNV1a("IDS_HELP_GYRO_TYPE");
		static constexpr uint32_t IDS_HELP_GYRO_PERSISTENCE_ENABLED = FNV1a("IDS_HELP_GYRO_PERSISTENCE_ENABLED");
		static constexpr uint32_t IDS_HELP_GYRO_SENSITIVITY = FNV1a("IDS_HELP_GYRO_SENSITIVITY");
		static constexpr uint32_t IDS_HELP_GYRO_SMOOTHING = FNV1a("IDS_HELP_GYRO_SMOOTHING");
		static constexpr uint32_t IDS_HELP_TOUCHPAD = FNV1a("IDS_HELP_TOUCHPAD");

		static constexpr uint32_t IDS_HOLD_DURATION = FNV1a("IDS_HOLD_DURATION");
		static constexpr uint32_t IDS_HELP_BUTTON_MAPPING = FNV1a("IDS_HELP_BUTTON_MAPPING");
		static constexpr uint32_t IDS_HELP_BINDINGS_FACE = FNV1a("IDS_HELP_BINDINGS_FACE");
		static constexpr uint32_t IDS_HELP_BINDINGS_SHOULDERS = FNV1a("IDS_HELP_BINDINGS_SHOULDERS");
		static constexpr uint32_t IDS_HELP_BINDINGS_DPAD = FNV1a("IDS_HELP_BINDINGS_DPAD");
		static constexpr uint32_t IDS_HELP_BINDINGS_STICKS = FNV1a("IDS_HELP_BINDINGS_STICKS");
		static constexpr uint32_t IDS_HELP_BINDINGS_EXTRA = FNV1a("IDS_HELP_BINDINGS_EXTRA");
		static constexpr uint32_t IDS_HELP_BINDING_PRESS = FNV1a("IDS_HELP_BINDING_PRESS");
		static constexpr uint32_t IDS_HELP_BINDING_HOLD = FNV1a("IDS_HELP_BINDING_HOLD");
		static constexpr uint32_t IDS_HELP_HOLD_DURATION = FNV1a("IDS_HELP_HOLD_DURATION");
		static constexpr uint32_t IDS_HELP_RESTORE_BINDINGS = FNV1a("IDS_HELP_RESTORE_BINDINGS");

		static constexpr uint32_t IDS_HELP_VSYNC = FNV1a("IDS_HELP_VSYNC");
		static constexpr uint32_t IDS_HELP_FPS_LIMIT = FNV1a("IDS_HELP_FPS_LIMIT");

		static constexpr uint32_t IDS_SUPERSAMPLING = FNV1a("IDS_SUPERSAMPLING");
		static constexpr uint32_t IDS_FIELD_OF_VIEW = FNV1a("IDS_FIELD_OF_VIEW");
		static constexpr uint32_t IDS_HUD_SIZE = FNV1a("IDS_HUD_SIZE");
		static constexpr uint32_t IDS_SMALL_TEXT_SIZE = FNV1a("IDS_SMALL_TEXT_SIZE");
		static constexpr uint32_t IDS_SENSITIVITY_MULTIPLIER = FNV1a("IDS_SENSITIVITY_MULTIPLIER");
		static constexpr uint32_t IDS_WEAPON_CAPACITY = FNV1a("IDS_WEAPON_CAPACITY");

		static constexpr uint32_t IDS_HELP_ECHOPATCH = FNV1a("IDS_HELP_ECHOPATCH");
		static constexpr uint32_t IDS_HELP_ECHOPATCH_GRAPHICS = FNV1a("IDS_HELP_ECHOPATCH_GRAPHICS");
		static constexpr uint32_t IDS_HELP_ECHOPATCH_DISPLAY = FNV1a("IDS_HELP_ECHOPATCH_DISPLAY");
		static constexpr uint32_t IDS_HELP_ECHOPATCH_MOUSE = FNV1a("IDS_HELP_ECHOPATCH_MOUSE");
		static constexpr uint32_t IDS_HELP_ECHOPATCH_MEMORY = FNV1a("IDS_HELP_ECHOPATCH_MEMORY");
		static constexpr uint32_t IDS_HELP_ECHOPATCH_EXTRA = FNV1a("IDS_HELP_ECHOPATCH_EXTRA");
		static constexpr uint32_t IDS_HELP_HIGH_RESOLUTION_REFLECTIONS = FNV1a("IDS_HELP_HIGH_RESOLUTION_REFLECTIONS");
		static constexpr uint32_t IDS_HELP_SUPERSAMPLING = FNV1a("IDS_HELP_SUPERSAMPLING");
		static constexpr uint32_t IDS_HELP_MAXIMUM_MODEL_DETAIL = FNV1a("IDS_HELP_MAXIMUM_MODEL_DETAIL");
		static constexpr uint32_t IDS_HELP_PERSISTENT_WORLD = FNV1a("IDS_HELP_PERSISTENT_WORLD");
		static constexpr uint32_t IDS_HELP_SAVE_PERSISTENT_WORLD = FNV1a("IDS_HELP_SAVE_PERSISTENT_WORLD");
		static constexpr uint32_t IDS_HELP_FIELD_OF_VIEW = FNV1a("IDS_HELP_FIELD_OF_VIEW");
		static constexpr uint32_t IDS_HELP_HUD_SCALING = FNV1a("IDS_HELP_HUD_SCALING");
		static constexpr uint32_t IDS_HELP_HUD_SIZE = FNV1a("IDS_HELP_HUD_SIZE");
		static constexpr uint32_t IDS_HELP_SMALL_TEXT_SIZE = FNV1a("IDS_HELP_SMALL_TEXT_SIZE");
		static constexpr uint32_t IDS_HELP_LETTERBOX = FNV1a("IDS_HELP_LETTERBOX");
		static constexpr uint32_t IDS_HELP_SENSITIVITY_MULTIPLIER = FNV1a("IDS_HELP_SENSITIVITY_MULTIPLIER");
		static constexpr uint32_t IDS_HELP_REDUCE_TEXTURE_MEMORY = FNV1a("IDS_HELP_REDUCE_TEXTURE_MEMORY");
		static constexpr uint32_t IDS_HELP_4GB_PATCH = FNV1a("IDS_HELP_4GB_PATCH");
		static constexpr uint32_t IDS_HELP_INFINITE_FLASHLIGHT = FNV1a("IDS_HELP_INFINITE_FLASHLIGHT");
		static constexpr uint32_t IDS_HELP_CUSTOM_WEAPON_CAPACITY = FNV1a("IDS_HELP_CUSTOM_WEAPON_CAPACITY");
		static constexpr uint32_t IDS_HELP_WEAPON_CAPACITY = FNV1a("IDS_HELP_WEAPON_CAPACITY");
		static constexpr uint32_t IDS_HELP_DOOR_BASHING = FNV1a("IDS_HELP_DOOR_BASHING");
		static constexpr uint32_t IDS_HELP_ACCURACY_PENALTY = FNV1a("IDS_HELP_ACCURACY_PENALTY");
	};

	// Console variable name hashes
	struct CVarHashes
	{
		// Display/Graphics
		static constexpr uint32_t StreamResources = FNV1a("StreamResources");
		static constexpr uint32_t Windowed = FNV1a("Windowed");
		static constexpr uint32_t ModelLODDistanceScale = FNV1a("ModelLODDistanceScale");
		static constexpr uint32_t CameraFirstPersonLODBias = FNV1a("CameraFirstPersonLODBias");
		static constexpr uint32_t AntiAliasFSOverSample = FNV1a("AntiAliasFSOverSample");

		// Gamepad
		static constexpr uint32_t GPadAimSensitivity = FNV1a("GPadAimSensitivity");
		static constexpr uint32_t GPadAimEdgeThreshold = FNV1a("GPadAimEdgeThreshold");
		static constexpr uint32_t GPadAimEdgeAccelTime = FNV1a("GPadAimEdgeAccelTime");
		static constexpr uint32_t GPadAimEdgeDelayTime = FNV1a("GPadAimEdgeDelayTime");
		static constexpr uint32_t GPadAimEdgeMultiplier = FNV1a("GPadAimEdgeMultiplier");
		static constexpr uint32_t GPadAimAspectRatio = FNV1a("GPadAimAspectRatio");
		static constexpr uint32_t MouseInvertY = FNV1a("MouseInvertY");

		// Resolution
		static constexpr uint32_t Performance_ScreenHeight = FNV1a("Performance_ScreenHeight");
		static constexpr uint32_t Performance_ScreenWidth = FNV1a("Performance_ScreenWidth");

		// HUD
		static constexpr uint32_t CrosshairSize = FNV1a("CrosshairSize");
		static constexpr uint32_t PerturbScale = FNV1a("PerturbScale");
		static constexpr uint32_t UseTextScaling = FNV1a("UseTextScaling");

		// Save
		static constexpr uint32_t CheckPointOptimizeVideoMemory = FNV1a("CheckPointOptimizeVideoMemory");

		// FOV
		static constexpr uint32_t FovY = FNV1a("FovY");
		static constexpr uint32_t FovYWidescreen = FNV1a("FovYWidescreen");
		static constexpr uint32_t CameraAttachedOffsetZ = FNV1a("CameraAttachedOffsetZ");
	};

	struct WeaponHashes
	{
		static constexpr uint32_t Pistol = FNV1a("Pistol");
		static constexpr uint32_t SMG = FNV1a("SMG");
		static constexpr uint32_t Minigun = FNV1a("Minigun");
		static constexpr uint32_t Laser = FNV1a("Laser");
		static constexpr uint32_t AssaultRifle = FNV1a("AssaultRifle");
		static constexpr uint32_t Rifle = FNV1a("Rifle");
		static constexpr uint32_t NailGun = FNV1a("NailGun");
		static constexpr uint32_t AdvancedRifle = FNV1a("AdvancedRifle");
		static constexpr uint32_t Turret_Ceiling = FNV1a("Turret_Ceiling");
		static constexpr uint32_t Shotgun = FNV1a("Shotgun");
		static constexpr uint32_t GrenadeLauncher = FNV1a("GrenadeLauncher");
		static constexpr uint32_t Cannon = FNV1a("Cannon");
		static constexpr uint32_t Missile = FNV1a("Missile");
		static constexpr uint32_t Plasma = FNV1a("Plasma");
		static constexpr uint32_t ChainLightningGun = FNV1a("ChainLightningGun");
		static constexpr uint32_t Frag = FNV1a("Frag");
		static constexpr uint32_t Proximity = FNV1a("Proximity");
		static constexpr uint32_t RemoteCharge = FNV1a("Remote Charge");
		static constexpr uint32_t RemoteDetonator = FNV1a("Remote Detonator");
		static constexpr uint32_t DeployableTurretGrenade = FNV1a("DeployableTurretGrenade");
		static constexpr uint32_t Melee_RifleButt = FNV1a("Melee_RifleButt");
		static constexpr uint32_t Melee_JumpKick = FNV1a("Melee_JumpKick");
		static constexpr uint32_t Melee_SlideKick = FNV1a("Melee_SlideKick");
		static constexpr uint32_t Melee_JabRight = FNV1a("Melee_JabRight");
		static constexpr uint32_t Melee_JabLeft = FNV1a("Melee_JabLeft");
		static constexpr uint32_t Melee_RunKickRight = FNV1a("Melee_RunKickRight");
		static constexpr uint32_t Melee_RunKickLeft = FNV1a("Melee_RunKickLeft");
	};

	struct GearHashes
	{
		static constexpr uint32_t ArmorLightSP = FNV1a("Armor Light SP");
		static constexpr uint32_t Medkit = FNV1a("MedKit");
		static constexpr uint32_t HealthMax = FNV1a("HealthMax");
		static constexpr uint32_t SlowMoMax = FNV1a("SlowMoMax");
	};
};
