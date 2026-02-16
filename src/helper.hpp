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

		size_t a1 = SIZE_MAX, a2 = SIZE_MAX;
		for (size_t i = 0; i < patSize; i++)
		{
			if (msk[i])
			{
				if (a1 == SIZE_MAX) a1 = i;
				a2 = i;
			}
		}

		if (a1 == SIZE_MAX) return reinterpret_cast<DWORD>(base);
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

		status = MH_EnableHook(addr);
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

		status = MH_EnableHook(addr);
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

		MH_STATUS status = MH_CreateHookApi(moduleName, apiName, hookFunc, originalFunc);
		if (status != MH_OK)
		{
			char errorMsg[0x100];
			sprintf_s(errorMsg, "Failed to create hook for API: %s\nError: %s", apiName, MH_StatusToString(status));
			MessageBoxA(NULL, errorMsg, "Error", MB_ICONERROR | MB_OK);
			return false;
		}

		status = MH_EnableHook(MH_ALL_HOOKS);
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

	class DirectSound8Proxy : public IDirectSound8
	{
	private:
		IDirectSound8* m_pReal;
		volatile LONG m_refCount;

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
			return m_pReal->CreateSoundBuffer(pcDSBufferDesc, ppDSBuffer, pUnkOuter);
		}

		STDMETHOD(GetCaps)(LPDSCAPS pDSCaps)
		{
			return m_pReal->GetCaps(pDSCaps);
		}

		STDMETHOD(DuplicateSoundBuffer)(LPDIRECTSOUNDBUFFER pDSBufferOriginal, LPDIRECTSOUNDBUFFER* ppDSBufferDuplicate)
		{
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

	inline HRESULT WINAPI CoCreateInstance_Hook(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv)
	{
		if (!ppv) return E_POINTER;
		*ppv = nullptr;

		// Only intercept DirectSound8 requests
		if (IsEqualGUID(rclsid, CLSID_DirectSound8) && cachedDSCreate8 && pUnkOuter == nullptr && (riid == IID_IDirectSound8 || riid == IID_IDirectSound || riid == IID_IUnknown))
		{
			IDirectSound8* pRealDS = nullptr;
			HRESULT hr = SafeCallDSCreate(nullptr, &pRealDS);

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
		static constexpr uint32_t AdditionalRect = FNV1a("AdditionalRect");
		static constexpr uint32_t AdditionalFloat = FNV1a("AdditionalFloat");
		static constexpr uint32_t AdditionalInt = FNV1a("AdditionalInt");
		static constexpr uint32_t TextSize = FNV1a("TextSize");
	};

	// Precomputed string hashes
	struct StringHashes
	{
		static constexpr uint32_t IDS_QUICKSAVE = FNV1a("IDS_QUICKSAVE");
		static constexpr uint32_t ScreenFailure_PressAnyKey = FNV1a("ScreenFailure_PressAnyKey");
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
	};

	// Console variable name hashes
	struct CVarHashes
	{
		// Display/Graphics
		static constexpr uint32_t StreamResources = FNV1a("StreamResources");
		static constexpr uint32_t Windowed = FNV1a("Windowed");
		static constexpr uint32_t ModelLODDistanceScale = FNV1a("ModelLODDistanceScale");
		static constexpr uint32_t CameraFirstPersonLODBias = FNV1a("CameraFirstPersonLODBias");

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
		static constexpr uint32_t Medkit = FNV1a("Medkit");
		static constexpr uint32_t HealthMax = FNV1a("HealthMax");
		static constexpr uint32_t SlowMoMax = FNV1a("SlowMoMax");
	};
};

#pragma once

namespace TextureHelper
{
	inline bool StartsWithI(const char* path, const char* prefix)
	{
		while (*prefix)
		{
			char p = *path++;
			char x = *prefix++;

			if (p >= 'A' && p <= 'Z') p += 32;
			if (x >= 'A' && x <= 'Z') x += 32;

			if (p != x)
				return false;
		}
		return true;
	}

	inline bool ContainsI(const char* path, const char* substr)
	{
		if (!path || !substr)
			return false;

		size_t subLen = strlen(substr);
		size_t pathLen = strlen(path);

		if (subLen > pathLen)
			return false;

		for (size_t i = 0; i <= pathLen - subLen; i++)
		{
			bool match = true;
			for (size_t j = 0; j < subLen; j++)
			{
				char p = path[i + j];
				char s = substr[j];

				if (p >= 'A' && p <= 'Z') p += 32;
				if (s >= 'A' && s <= 'Z') s += 32;

				if (p != s)
				{
					match = false;
					break;
				}
			}
			if (match)
				return true;
		}
		return false;
	}

	inline bool ShouldKeepSharp(const char* path)
	{
		if (!path)
			return false;

		// Prefix (base game)
		if (StartsWithI(path, "attachments\\"))
			return true;
		if (StartsWithI(path, "chars\\skins\\"))
			return true;
		if (StartsWithI(path, "FX\\"))
			return true;
		if (StartsWithI(path, "guns\\"))
			return true;
		if (StartsWithI(path, "Interface\\"))
			return true;

		// Prefix (expansions)
		if (StartsWithI(path, "charsxp\\"))
			return true;
		if (StartsWithI(path, "fx-xp\\"))
			return true;
		if (StartsWithI(path, "guns-xp\\"))
			return true;
		if (StartsWithI(path, "Attachments-XP\\"))
			return true;
		if (StartsWithI(path, "CharsXP\\"))
			return true;
		if (StartsWithI(path, "FX-XP\\"))
			return true;
		if (StartsWithI(path, "Guns-XP\\"))
			return true;

		// Prefabs - full folder paths
		if (StartsWithI(path, "Prefabs\\Systemic\\Elevators\\"))
			return true;
		if (StartsWithI(path, "Prefabs\\Systemic\\Security\\"))
			return true;
		if (StartsWithI(path, "Prefabs\\Systemic\\Vehicles\\"))
			return true;
		if (StartsWithI(path, "Prefabs\\Industrial\\Spillway\\"))
			return true;
		if (StartsWithI(path, "Prefabs\\Industrial\\Interactive\\"))
			return true;

		// Materials - Industrial
		if (StartsWithI(path, "Materials\\Industrial\\"))
		{
			if (ContainsI(path, "Cabinet") ||
				ContainsI(path, "Elevator"))
				return true;
		}

		// Prefabs - Industrial
		if (StartsWithI(path, "Prefabs\\Industrial\\"))
		{
			if (ContainsI(path, "Barrel") ||
				ContainsI(path, "box") ||
				ContainsI(path, "Bucket") ||
				ContainsI(path, "Cargo") ||
				ContainsI(path, "control_unit") ||
				ContainsI(path, "Door") ||
				ContainsI(path, "hammer") ||
				ContainsI(path, "generator") ||
				ContainsI(path, "Garage_Gate") ||
				ContainsI(path, "Dumpster") ||
				ContainsI(path, "Fan") ||
				ContainsI(path, "Machine") ||
				ContainsI(path, "Phone") ||
				ContainsI(path, "rack_pallet") ||
				ContainsI(path, "power_") ||
				ContainsI(path, "shelf_metal") ||
				ContainsI(path, "spray_") ||
				ContainsI(path, "tool_chest") ||
				ContainsI(path, "Wallbox") ||
				ContainsI(path, "workbench") ||
				ContainsI(path, "wrench"))
				return true;
		}

		// Prefabs - Office
		if (StartsWithI(path, "Prefabs\\Office\\"))
		{
			if (ContainsI(path, "Book") ||
				ContainsI(path, "Cactus") ||
				ContainsI(path, "Copier") ||
				ContainsI(path, "Kiosk") ||
				ContainsI(path, "mouse_"))
				return true;
		}

		// Prefabs - Tech
		if (StartsWithI(path, "Prefabs\\Tech\\"))
		{
			if (ContainsI(path, "alma") ||
				ContainsI(path, "barrel_tech_"))
				return true;
		}

		// Prefabs - Test
		if (StartsWithI(path, "Prefabs\\Test\\"))
		{
			if (ContainsI(path, "chair02") ||
				ContainsI(path, "desk_") ||
				ContainsI(path, "DeskMonitor"))
				return true;
		}

		// Tex - Office
		if (StartsWithI(path, "Tex\\Office\\"))
		{
			if (ContainsI(path, "CorporateART01") ||
				ContainsI(path, "Painting01") ||
				ContainsI(path, "Painting03"))
				return true;
		}

		// Tex - Industrial
		if (StartsWithI(path, "Tex\\Industrial\\"))
		{
			if (ContainsI(path, "Fence"))
				return true;
		}

		return false;
	}
}