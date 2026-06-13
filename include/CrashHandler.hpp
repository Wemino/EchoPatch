#pragma once

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cwchar>
#include <cstdlib>
#include <csignal>
#include <exception>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>

#pragma comment(lib, "Dbghelp.lib")

namespace CrashHandler
{
	struct ModuleEntry
	{
		DWORD_PTR base;
		DWORD size;
		char name[64];
	};

	static const DWORD CODE_CRT_INVALID_PARAMETER = 0xE0000001;
	static const DWORD CODE_CRT_PURECALL = 0xE0000002;
	static const DWORD CODE_CRT_TERMINATE = 0xE0000003;
	static const DWORD CODE_CRT_ABORT = 0xE0000004;

	static LPTOP_LEVEL_EXCEPTION_FILTER g_previousFilter = nullptr;
	static PVOID g_vectoredHandler = nullptr;
	static volatile LONG g_reportWritten = 0;
	static bool g_terminateOnCrash = false;

	static EXCEPTION_POINTERS* g_crashEp = nullptr;
	static const char* g_crashTag = nullptr;
	static DWORD g_crashThreadId = 0;
	static volatile DWORD g_writerThreadId = 0;
	static DWORD_PTR g_crashStackBase = 0;

	static ModuleEntry g_modules[512];
	static int g_moduleCount = 0;

	static void WriteStr(HANDLE file, const char* text)
	{
		DWORD bw;
		WriteFile(file, text, (DWORD)strlen(text), &bw, nullptr);
	}

	static void WriteFmt(HANDLE file, const char* fmt, ...)
	{
		static char buf[2048];
		va_list args;
		va_start(args, fmt);
		_vsnprintf_s(buf, _countof(buf), _TRUNCATE, fmt, args);
		va_end(args);
		WriteStr(file, buf);
	}

	static const char* FindFileNameA(const char* path)
	{
		const char* bs = strrchr(path, '\\');
		const char* fs = strrchr(path, '/');
		const char* sep = bs > fs ? bs : fs;

		return sep ? sep + 1 : path;
	}

	static void RemoveFileSpecW(wchar_t* path)
	{
		wchar_t* bs = wcsrchr(path, L'\\');
		wchar_t* fs = wcsrchr(path, L'/');
		wchar_t* sep = bs > fs ? bs : fs;

		if (sep)
		{
			*sep = L'\0';
		}
	}

	static bool IsAddressInModule(DWORD_PTR addr)
	{
		for (int i = 0; i < g_moduleCount; i++)
		{
			if (addr >= g_modules[i].base && addr < g_modules[i].base + g_modules[i].size)
			{
				return true;
			}
		}

		return false;
	}

	static bool IsPrecededByCall(DWORD_PTR addr)
	{
		const BYTE* p = (const BYTE*)addr;

		if (p[-5] == 0xE8)
		{
			return true;
		}

		if (p[-2] == 0xFF)
		{
			BYTE modrm = p[-1];
			if ((modrm & 0xF8) == 0xD0)
			{
				return true;
			}
			if ((modrm & 0xF8) == 0x10 && modrm != 0x14 && modrm != 0x15)
			{
				return true;
			}
		}

		if (p[-3] == 0xFF)
		{
			BYTE modrm = p[-2];
			if (modrm == 0x14)
			{
				return true;
			}
			if ((modrm & 0xF8) == 0x50 && modrm != 0x54)
			{
				return true;
			}
		}

		if (p[-4] == 0xFF && p[-3] == 0x54)
		{
			return true;
		}

		if (p[-6] == 0xFF)
		{
			BYTE modrm = p[-5];
			if (modrm == 0x15)
			{
				return true;
			}
			if ((modrm & 0xF8) == 0x90 && modrm != 0x94)
			{
				return true;
			}
		}

		if (p[-7] == 0xFF && p[-6] == 0x94)
		{
			return true;
		}

		return false;
	}

	static const char* ExceptionCodeName(DWORD code)
	{
		switch (code)
		{
			case EXCEPTION_ACCESS_VIOLATION:         return "ACCESS_VIOLATION";
			case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "ARRAY_BOUNDS_EXCEEDED";
			case EXCEPTION_BREAKPOINT:               return "BREAKPOINT";
			case EXCEPTION_DATATYPE_MISALIGNMENT:    return "DATATYPE_MISALIGNMENT";
			case EXCEPTION_FLT_DENORMAL_OPERAND:     return "FLT_DENORMAL_OPERAND";
			case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "FLT_DIVIDE_BY_ZERO";
			case EXCEPTION_FLT_INEXACT_RESULT:       return "FLT_INEXACT_RESULT";
			case EXCEPTION_FLT_INVALID_OPERATION:    return "FLT_INVALID_OPERATION";
			case EXCEPTION_FLT_OVERFLOW:             return "FLT_OVERFLOW";
			case EXCEPTION_FLT_STACK_CHECK:          return "FLT_STACK_CHECK";
			case EXCEPTION_FLT_UNDERFLOW:            return "FLT_UNDERFLOW";
			case EXCEPTION_ILLEGAL_INSTRUCTION:      return "ILLEGAL_INSTRUCTION";
			case EXCEPTION_IN_PAGE_ERROR:            return "IN_PAGE_ERROR";
			case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "INT_DIVIDE_BY_ZERO";
			case EXCEPTION_INT_OVERFLOW:             return "INT_OVERFLOW";
			case EXCEPTION_INVALID_DISPOSITION:      return "INVALID_DISPOSITION";
			case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "NONCONTINUABLE_EXCEPTION";
			case EXCEPTION_PRIV_INSTRUCTION:         return "PRIV_INSTRUCTION";
			case EXCEPTION_SINGLE_STEP:              return "SINGLE_STEP";
			case EXCEPTION_STACK_OVERFLOW:           return "STACK_OVERFLOW";
			case 0xE06D7363:                         return "CPP_EXCEPTION";
			case CODE_CRT_INVALID_PARAMETER:         return "CRT_INVALID_PARAMETER";
			case CODE_CRT_PURECALL:                  return "PURE_VIRTUAL_CALL";
			case CODE_CRT_TERMINATE:                 return "CPP_TERMINATE";
			case CODE_CRT_ABORT:                     return "ABORT";
			default:                                 return "UNKNOWN";
		}
	}

	static bool IsFatalCode(DWORD code)
	{
		switch (code)
		{
			case EXCEPTION_ACCESS_VIOLATION:
			case EXCEPTION_STACK_OVERFLOW:
			case EXCEPTION_ILLEGAL_INSTRUCTION:
			case EXCEPTION_PRIV_INSTRUCTION:
			case EXCEPTION_INT_DIVIDE_BY_ZERO:
			case EXCEPTION_IN_PAGE_ERROR:
			case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			case EXCEPTION_DATATYPE_MISALIGNMENT:
			case EXCEPTION_NONCONTINUABLE_EXCEPTION:
			case EXCEPTION_INVALID_DISPOSITION:
			case CODE_CRT_INVALID_PARAMETER:
			case CODE_CRT_PURECALL:
			case CODE_CRT_TERMINATE:
			case CODE_CRT_ABORT:
			return true;
		default:
			return false;
		}
	}

	static void FormatAddress(DWORD_PTR addr, char* out, size_t outSize)
	{
		for (int i = 0; i < g_moduleCount; i++)
		{
			if (addr >= g_modules[i].base && addr < g_modules[i].base + g_modules[i].size)
			{
				DWORD_PTR offset = addr - g_modules[i].base;
				_snprintf_s(out, outSize, _TRUNCATE, "%s+0x%X (0x%08X)", g_modules[i].name, (unsigned)offset, (unsigned)addr);
				return;
			}
		}

		_snprintf_s(out, outSize, _TRUNCATE, "0x%08X", (unsigned)addr);
	}

	static void WriteRegisters(HANDLE file, CONTEXT* ctx)
	{
		WriteStr(file, "\r\n=== Registers ===\r\n");

		WriteFmt(file,
			"  EAX=0x%08X  EBX=0x%08X  ECX=0x%08X  EDX=0x%08X\r\n"
			"  ESI=0x%08X  EDI=0x%08X  EBP=0x%08X  ESP=0x%08X\r\n"
			"  EIP=0x%08X  EFLAGS=0x%08X\r\n"
			"  CS=0x%04X  DS=0x%04X  ES=0x%04X  FS=0x%04X  GS=0x%04X  SS=0x%04X\r\n",
			ctx->Eax, ctx->Ebx, ctx->Ecx, ctx->Edx,
			ctx->Esi, ctx->Edi, ctx->Ebp, ctx->Esp,
			ctx->Eip, ctx->EFlags,
			ctx->SegCs, ctx->SegDs, ctx->SegEs, ctx->SegFs, ctx->SegGs, ctx->SegSs);

		if (ctx->ContextFlags & CONTEXT_DEBUG_REGISTERS)
		{
			WriteStr(file, "\r\n=== Debug Registers ===\r\n");
			WriteFmt(file,
				"  DR0=0x%08X  DR1=0x%08X  DR2=0x%08X  DR3=0x%08X\r\n"
				"  DR6=0x%08X  DR7=0x%08X\r\n",
				ctx->Dr0, ctx->Dr1, ctx->Dr2, ctx->Dr3,
				ctx->Dr6, ctx->Dr7);
		}

		__try
		{
			if (ctx->ContextFlags & CONTEXT_FLOATING_POINT)
			{
				WriteStr(file, "\r\n=== x87 FPU ===\r\n");
				WriteFmt(file,
					"  ControlWord=0x%04X  StatusWord=0x%04X  TagWord=0x%04X\r\n"
					"  ErrorOffset=0x%08X  ErrorSelector=0x%08X\r\n"
					"  DataOffset=0x%08X   DataSelector=0x%08X\r\n",
					ctx->FloatSave.ControlWord & 0xFFFF, ctx->FloatSave.StatusWord & 0xFFFF, ctx->FloatSave.TagWord & 0xFFFF,
					ctx->FloatSave.ErrorOffset, ctx->FloatSave.ErrorSelector,
					ctx->FloatSave.DataOffset, ctx->FloatSave.DataSelector);

				for (int i = 0; i < 8; i++)
				{
					const BYTE* st = &ctx->FloatSave.RegisterArea[i * 10];
					WriteFmt(file,
						"  ST%d = %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
						i, st[0], st[1], st[2], st[3], st[4], st[5], st[6], st[7], st[8], st[9]);
				}
			}

			if (ctx->ContextFlags & CONTEXT_EXTENDED_REGISTERS)
			{
				WriteStr(file, "\r\n=== SSE / XMM ===\r\n");

				DWORD mxcsr = *(DWORD*)&ctx->ExtendedRegisters[24];
				WriteFmt(file, "  MXCSR=0x%08X\r\n", mxcsr);

				for (int i = 0; i < 8; i++)
				{
					const BYTE* xmm = &ctx->ExtendedRegisters[160 + i * 16];
					DWORD d0 = *(DWORD*)&xmm[0];
					DWORD d1 = *(DWORD*)&xmm[4];
					DWORD d2 = *(DWORD*)&xmm[8];
					DWORD d3 = *(DWORD*)&xmm[12];
					float f0 = *(float*)&d0;
					float f1 = *(float*)&d1;
					float f2 = *(float*)&d2;
					float f3 = *(float*)&d3;
					WriteFmt(file,
						"  XMM%d = %08X %08X %08X %08X   [ %g %g %g %g ]\r\n",
						i, d0, d1, d2, d3, f0, f1, f2, f3);
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}

	static void WriteModuleList(HANDLE file)
	{
		WriteStr(file, "\r\n=== Loaded Modules ===\r\n");

		g_moduleCount = 0;

		__try
		{
			BYTE* peb = (BYTE*)__readfsdword(0x30);
			BYTE* ldr = *(BYTE**)(peb + 0x0C);
			LIST_ENTRY* head = (LIST_ENTRY*)(ldr + 0x14);
			LIST_ENTRY* node = head->Flink;

			while (node != head && g_moduleCount < 512)
			{
				BYTE* entry = (BYTE*)node - 0x08;
				BYTE* base = *(BYTE**)(entry + 0x18);
				DWORD size = *(DWORD*)(entry + 0x20);
				USHORT nameLen = *(USHORT*)(entry + 0x24);
				wchar_t* nameBuf = *(wchar_t**)(entry + 0x28);

				wchar_t fullName[MAX_PATH];
				USHORT chars = (USHORT)(nameLen / sizeof(wchar_t));
				if (chars >= MAX_PATH)
				{
					chars = MAX_PATH - 1;
				}
				for (USHORT j = 0; j < chars; j++)
				{
					fullName[j] = nameBuf[j];
				}
				fullName[chars] = 0;

				WriteFmt(file, "  0x%08X - 0x%08X  %ls\r\n",
					(unsigned)(DWORD_PTR)base,
					(unsigned)((DWORD_PTR)base + size),
					fullName);

				const wchar_t* leaf = fullName;
				const wchar_t* slash = wcsrchr(fullName, L'\\');
				if (slash)
				{
					leaf = slash + 1;
				}

				ModuleEntry* m = &g_modules[g_moduleCount];
				m->base = (DWORD_PTR)base;
				m->size = size;
				int k = 0;
				while (leaf[k] && k < 63)
				{
					m->name[k] = (char)leaf[k];
					k++;
				}
				m->name[k] = 0;

				g_moduleCount++;
				node = node->Flink;
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}

	static void WriteRawStack(HANDLE file, CONTEXT* ctx)
	{
		WriteStr(file, "\r\n=== Raw Stack Scan ===\r\n");

		static char addrStr[256];

		__try
		{
			DWORD_PTR* sp = (DWORD_PTR*)ctx->Esp;
			int printed = 0;

			for (int i = 0; i < 8192 && printed < 64; i++)
			{
				if (g_crashStackBase && (DWORD_PTR)&sp[i] >= g_crashStackBase)
				{
					break;
				}

				DWORD_PTR value = sp[i];

				if (IsAddressInModule(value) && IsAddressInModule(value - 8) && IsPrecededByCall(value))
				{
					FormatAddress(value, addrStr, sizeof(addrStr));
					WriteFmt(file, "  [esp+0x%04X]  %s\r\n", (unsigned)(i * sizeof(DWORD_PTR)), addrStr);
					printed++;
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}

	static void WriteStackTrace(HANDLE file, CONTEXT* ctx)
	{
		HANDLE process = GetCurrentProcess();
		HANDLE thread = GetCurrentThread();

		static char addrStr[256];
		static char line[1024];

		__try
		{
			SymRefreshModuleList(process);

			STACKFRAME64 frame{};
			frame.AddrPC.Offset = ctx->Eip;
			frame.AddrPC.Mode = AddrModeFlat;
			frame.AddrFrame.Offset = ctx->Ebp;
			frame.AddrFrame.Mode = AddrModeFlat;
			frame.AddrStack.Offset = ctx->Esp;
			frame.AddrStack.Mode = AddrModeFlat;

			int frameNum = 0;

			while (StackWalk64(IMAGE_FILE_MACHINE_I386, process, thread, &frame, ctx, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
			{
				if (frame.AddrPC.Offset == 0)
				{
					break;
				}

				FormatAddress((DWORD_PTR)frame.AddrPC.Offset, addrStr, sizeof(addrStr));

				char symBuf[sizeof(SYMBOL_INFO) + 256]{};
				SYMBOL_INFO* sym = (SYMBOL_INFO*)symBuf;
				sym->SizeOfStruct = sizeof(SYMBOL_INFO);
				sym->MaxNameLen = 255;
				DWORD64 displacement = 0;

				if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, sym) && displacement < 0x10000)
				{
					_snprintf_s(line, _countof(line), _TRUNCATE, "  #%02d  %s   %s+0x%llx\r\n", frameNum, addrStr, sym->Name, displacement);
				}
				else
				{
					_snprintf_s(line, _countof(line), _TRUNCATE, "  #%02d  %s\r\n", frameNum, addrStr);
				}

				WriteStr(file, line);
				frameNum++;

				if (frameNum > 64)
				{
					break;
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}

	static HANDLE OpenReportFile(wchar_t* outPath, size_t outCount)
	{
		wchar_t exePath[MAX_PATH];
		if (!GetModuleFileNameW(nullptr, exePath, MAX_PATH))
		{
			exePath[0] = 0;
		}

		wchar_t exeName[MAX_PATH];
		const wchar_t* leaf = exePath;
		const wchar_t* slash = wcsrchr(exePath, L'\\');
		if (slash)
		{
			leaf = slash + 1;
		}
		_snwprintf_s(exeName, _countof(exeName), _TRUNCATE, L"%s", leaf);
		wchar_t* dot = wcsrchr(exeName, L'.');
		if (dot)
		{
			*dot = 0;
		}
		if (exeName[0] == 0)
		{
			_snwprintf_s(exeName, _countof(exeName), _TRUNCATE, L"Game");
		}

		wchar_t exeDir[MAX_PATH];
		_snwprintf_s(exeDir, _countof(exeDir), _TRUNCATE, L"%s", exePath);
		RemoveFileSpecW(exeDir);

		SYSTEMTIME st;
		GetLocalTime(&st);

		wchar_t bases[3][MAX_PATH];
		int baseCount = 0;

		if (exeDir[0])
		{
			_snwprintf_s(bases[baseCount++], MAX_PATH, _TRUNCATE, L"%s", exeDir);
		}

		wchar_t localApp[MAX_PATH];
		if (GetEnvironmentVariableW(L"LOCALAPPDATA", localApp, MAX_PATH))
		{
			_snwprintf_s(bases[baseCount++], MAX_PATH, _TRUNCATE, L"%s\\%s", localApp, exeName);
		}

		wchar_t tempDir[MAX_PATH];
		if (GetTempPathW(MAX_PATH, tempDir))
		{
			_snwprintf_s(bases[baseCount++], MAX_PATH, _TRUNCATE, L"%s%s", tempDir, exeName);
		}

		for (int i = 0; i < baseCount; i++)
		{
			wchar_t dumpDir[MAX_PATH];
			_snwprintf_s(dumpDir, _countof(dumpDir), _TRUNCATE, L"%s\\CrashDump", bases[i]);

			CreateDirectoryW(bases[i], nullptr);
			CreateDirectoryW(dumpDir, nullptr);

			wchar_t path[MAX_PATH];
			_snwprintf_s(path, _countof(path), _TRUNCATE,
				L"%s\\crash_%04d%02d%02d_%02d%02d%02d_%03d_%lu.txt",
				dumpDir, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, GetCurrentProcessId());

			HANDLE file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (file != INVALID_HANDLE_VALUE)
			{
				_snwprintf_s(outPath, outCount, _TRUNCATE, L"%s", path);
				return file;
			}
		}

		if (outCount)
		{
			outPath[0] = 0;
		}

		return INVALID_HANDLE_VALUE;
	}

	static bool WriteMiniDump(EXCEPTION_POINTERS* ep, const wchar_t* txtPath)
	{
		wchar_t dmpPath[MAX_PATH];
		_snwprintf_s(dmpPath, _countof(dmpPath), _TRUNCATE, L"%s", txtPath);

		wchar_t* dot = wcsrchr(dmpPath, L'.');
		if (!dot || wcslen(dot) != 4)
		{
			return false;
		}

		dot[1] = L'd';
		dot[2] = L'm';
		dot[3] = L'p';

		HANDLE file = CreateFileW(dmpPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		MINIDUMP_EXCEPTION_INFORMATION mei;
		mei.ThreadId = g_crashThreadId;
		mei.ExceptionPointers = ep;
		mei.ClientPointers = FALSE;

		MINIDUMP_TYPE dumpType = (MINIDUMP_TYPE)(
			MiniDumpWithDataSegs |
			MiniDumpWithHandleData |
			MiniDumpWithIndirectlyReferencedMemory |
			MiniDumpWithUnloadedModules |
			MiniDumpWithThreadInfo);

		BOOL ok = FALSE;

		__try
		{
			ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, dumpType, &mei, nullptr, nullptr);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}

		CloseHandle(file);

		return ok != FALSE;
	}

	static void WriteCrashReport(EXCEPTION_POINTERS* ep, const char* sourceTag)
	{
		wchar_t txtPath[MAX_PATH];
		HANDLE file = OpenReportFile(txtPath, _countof(txtPath));
		if (file == INVALID_HANDLE_VALUE)
		{
			return;
		}

		EXCEPTION_RECORD* er = ep->ExceptionRecord;
		CONTEXT* ctx = ep->ContextRecord;

		SYSTEMTIME st;
		GetLocalTime(&st);

		WriteFmt(file,
			"=== Crash Report ===\r\n"
			"Source:     %s\r\n"
			"Time:       %04d-%02d-%02d %02d:%02d:%02d\r\n"
			"Exception:  0x%08X (%s)\r\n"
			"Address:    0x%08X\r\n"
			"ProcessId:  %lu\r\n"
			"ThreadId:   %lu\r\n",
			sourceTag,
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
			(unsigned)er->ExceptionCode,
			ExceptionCodeName(er->ExceptionCode),
			(unsigned)(DWORD_PTR)er->ExceptionAddress,
			GetCurrentProcessId(),
			GetCurrentThreadId());

		if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && er->NumberParameters >= 2)
		{
			const char* op = er->ExceptionInformation[0] == 0 ? "read" : er->ExceptionInformation[0] == 1 ? "write" : er->ExceptionInformation[0] == 8 ? "DEP" : "execute";
			WriteFmt(file, "AV Detail:  %s at 0x%08X\r\n", op, (unsigned)er->ExceptionInformation[1]);
		}

		if (er->ExceptionCode == EXCEPTION_IN_PAGE_ERROR && er->NumberParameters >= 3)
		{
			WriteFmt(file, "NTSTATUS:   0x%08X\r\n", (unsigned)er->ExceptionInformation[2]);
		}

		WriteRegisters(file, ctx);
		FlushFileBuffers(file);

		WriteModuleList(file);
		FlushFileBuffers(file);

		static char faultStr[256];
		FormatAddress((DWORD_PTR)er->ExceptionAddress, faultStr, sizeof(faultStr));
		WriteFmt(file, "\r\nFaulting:   %s\r\n", faultStr);

		WriteRawStack(file, ctx);
		FlushFileBuffers(file);

		WriteStr(file, "\r\n=== Stack Trace ===\r\n");
		CONTEXT ctxCopy = *ctx;
		WriteStackTrace(file, &ctxCopy);
		FlushFileBuffers(file);

		CloseHandle(file);

		bool dumpOk = WriteMiniDump(ep, txtPath);

		static wchar_t msg[1024];
		_snwprintf_s(msg, _countof(msg), _TRUNCATE,
			L"The game has crashed.\n\n"
			L"Exception: 0x%08X (%hs)\n"
			L"At: %hs\n\n"
			L"%s\n"
			L"Crash report saved to:\n%s%s",
			(unsigned)er->ExceptionCode,
			ExceptionCodeName(er->ExceptionCode),
			faultStr,
			g_terminateOnCrash ? L"The game will close when you press OK." : L"The process has been paused for debugging.",
			txtPath,
			dumpOk ? L"\n\nA minidump (.dmp) was saved next to it." : L"");
		MessageBoxW(nullptr, msg, L"Game Crash", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL | MB_SETFOREGROUND | MB_TOPMOST);

		if (g_terminateOnCrash)
		{
			TerminateProcess(GetCurrentProcess(), er->ExceptionCode);
		}
	}

	static DWORD WINAPI WriterThread(LPVOID)
	{
		g_writerThreadId = GetCurrentThreadId();
		WriteCrashReport(g_crashEp, g_crashTag);
		g_writerThreadId = 0;
		return 0;
	}

	static void HandleCrash(EXCEPTION_POINTERS* ep, const char* sourceTag)
	{
		if (InterlockedExchange(&g_reportWritten, 1) != 0)
		{
			Sleep(INFINITE);
			return;
		}

		g_crashEp = ep;
		g_crashTag = sourceTag;
		g_crashThreadId = GetCurrentThreadId();
		g_crashStackBase = __readfsdword(0x04);

		HANDLE thread = CreateThread(nullptr, 0, WriterThread, nullptr, 0, nullptr);
		if (thread)
		{
			CloseHandle(thread);
		}
		else
		{
			g_writerThreadId = GetCurrentThreadId();
			WriteCrashReport(ep, sourceTag);
			g_writerThreadId = 0;
		}

		Sleep(INFINITE);
	}

	static LONG WINAPI VectoredCrashFilter(EXCEPTION_POINTERS* ep)
	{
		if (g_reportWritten && GetCurrentThreadId() == g_writerThreadId)
		{
			return EXCEPTION_CONTINUE_SEARCH;
		}

		if (!IsFatalCode(ep->ExceptionRecord->ExceptionCode))
		{
			return EXCEPTION_CONTINUE_SEARCH;
		}

		if (IsDebuggerPresent())
		{
			return EXCEPTION_CONTINUE_SEARCH;
		}

		HandleCrash(ep, "VEH");

		return EXCEPTION_CONTINUE_SEARCH;
	}

	static LONG WINAPI UnhandledCrashFilter(EXCEPTION_POINTERS* ep)
	{
		if (IsDebuggerPresent())
		{
			return g_previousFilter ? g_previousFilter(ep) : EXCEPTION_CONTINUE_SEARCH;
		}

		HandleCrash(ep, "Unhandled");

		return EXCEPTION_EXECUTE_HANDLER;
	}

	static LPTOP_LEVEL_EXCEPTION_FILTER WINAPI SetUnhandledExceptionFilterStub(LPTOP_LEVEL_EXCEPTION_FILTER)
	{
		return nullptr;
	}

	static void LockUnhandledExceptionFilter()
	{
		HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
		if (!k32)
		{
			return;
		}

		void* target = (void*)GetProcAddress(k32, "SetUnhandledExceptionFilter");
		if (!target)
		{
			return;
		}

		unsigned char patch[5] = { 0xE9, 0, 0, 0, 0 };
		intptr_t rel = (intptr_t)&SetUnhandledExceptionFilterStub - (intptr_t)target - 5;
		memcpy(patch + 1, &rel, 4);

		DWORD oldProtect;
		if (VirtualProtect(target, sizeof(patch), PAGE_EXECUTE_READWRITE, &oldProtect))
		{
			memcpy(target, patch, sizeof(patch));
			VirtualProtect(target, sizeof(patch), oldProtect, &oldProtect);
			FlushInstructionCache(GetCurrentProcess(), target, sizeof(patch));
		}
	}

	static void __cdecl OnInvalidParameter(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t)
	{
		RaiseException(CODE_CRT_INVALID_PARAMETER, EXCEPTION_NONCONTINUABLE, 0, nullptr);
	}

	static void __cdecl OnPurecall()
	{
		RaiseException(CODE_CRT_PURECALL, EXCEPTION_NONCONTINUABLE, 0, nullptr);
	}

	static void __cdecl OnTerminate()
	{
		RaiseException(CODE_CRT_TERMINATE, EXCEPTION_NONCONTINUABLE, 0, nullptr);
	}

	static void __cdecl OnAbort(int)
	{
		RaiseException(CODE_CRT_ABORT, EXCEPTION_NONCONTINUABLE, 0, nullptr);
	}

	static void GuardThreadStack()
	{
		ULONG stackReserve = 64 * 1024;
		SetThreadStackGuarantee(&stackReserve);
	}

	static void Install(bool useVEH = true, bool terminateOnCrash = false)
	{
		g_terminateOnCrash = terminateOnCrash;

		SetErrorMode(GetErrorMode() | SEM_NOGPFAULTERRORBOX);

		_set_invalid_parameter_handler(OnInvalidParameter);
		_set_purecall_handler(OnPurecall);
		std::set_terminate(OnTerminate);
		_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
		signal(SIGABRT, OnAbort);

		GuardThreadStack();

		SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_NO_PROMPTS | SYMOPT_FAIL_CRITICAL_ERRORS);
		SymInitialize(GetCurrentProcess(), nullptr, TRUE);

		g_previousFilter = SetUnhandledExceptionFilter(UnhandledCrashFilter);

		if (useVEH)
		{
			g_vectoredHandler = AddVectoredExceptionHandler(1, VectoredCrashFilter);
		}

		LockUnhandledExceptionFilter();
	}

	static void Uninstall()
	{
		if (g_vectoredHandler)
		{
			RemoveVectoredExceptionHandler(g_vectoredHandler);
			g_vectoredHandler = nullptr;
		}

		SetUnhandledExceptionFilter(g_previousFilter);
		g_previousFilter = nullptr;

		SymCleanup(GetCurrentProcess());
	}
}
