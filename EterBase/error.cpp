#include "StdAfx.h"

#include <stdio.h>
#include <time.h>

#include <winsock2.h>   // winsock2 elõbb jöjjön, mint windows.h (StdAfx általában már húzza)
#include <windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")

static FILE* fException = nullptr;

// --- PE headerbõl timestamp olvasás (kiváltja a GetTimestampForLoadedLibrary-t) ---
static DWORD GetModuleTimestamp(HMODULE hMod)
{
    if (!hMod) return 0;
    auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(hMod);
    if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
    auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(
        reinterpret_cast<const BYTE*>(hMod) + dos->e_lfanew);
    if (!nt || nt->Signature != IMAGE_NT_SIGNATURE) return 0;
    return nt->FileHeader.TimeDateStamp;
}

#if _MSC_VER >= 1400
BOOL CALLBACK EnumerateLoadedModulesProc(PCSTR ModuleName, ULONG ModuleBase, ULONG ModuleSize, PVOID UserContext)
#else
BOOL CALLBACK EnumerateLoadedModulesProc(PSTR ModuleName, ULONG ModuleBase, ULONG ModuleSize, PVOID UserContext)
#endif
{
    DWORD offset = *reinterpret_cast<DWORD*>(UserContext);

    if (offset >= ModuleBase && offset <= ModuleBase + ModuleSize)
    {
        fprintf(fException, "%s", ModuleName);
        return FALSE; // megvan a modul
    }
    return TRUE; // folytasd a keresést
}

LONG __stdcall EterExceptionFilter(_EXCEPTION_POINTERS* pExceptionInfo)
{
    HANDLE hProcess = GetCurrentProcess();
    HANDLE hThread = GetCurrentThread();

    fException = fopen("ErrorLog.txt", "wt");
    if (fException)
    {
        char   module_name[MAX_PATH] = { 0 };
        time_t module_time_t = 0;

        HMODULE hModule = GetModuleHandleA(nullptr);
        GetModuleFileNameA(hModule, module_name, sizeof(module_name));

        DWORD ts = GetModuleTimestamp(hModule);
        module_time_t = static_cast<time_t>(ts);

        // fejléc
        fprintf(fException, "Module Name: %s\n", module_name);
        fprintf(fException, "Time Stamp: 0x%08X - %s\n", ts,
            (module_time_t ? ctime(&module_time_t) : "n/a\n"));
        fprintf(fException, "\n");
        fprintf(fException, "Exception Type: 0x%08X\n", pExceptionInfo->ExceptionRecord->ExceptionCode);
        fprintf(fException, "\n");

        CONTEXT& context = *pExceptionInfo->ContextRecord;

        // x86 regiszter dump
        fprintf(fException, "eax: 0x%08X\tebx: 0x%08X\n", context.Eax, context.Ebx);
        fprintf(fException, "ecx: 0x%08X\tedx: 0x%08X\n", context.Ecx, context.Edx);
        fprintf(fException, "esi: 0x%08X\tedi: 0x%08X\n", context.Esi, context.Edi);
        fprintf(fException, "ebp: 0x%08X\tesp: 0x%08X\n", context.Ebp, context.Esp);
        fprintf(fException, "eip: 0x%08X\teflags: 0x%08X\n", context.Eip, context.EFlags);
        fprintf(fException, "\n");

        // verem beállítás
        STACKFRAME stackFrame = {};
        stackFrame.AddrPC.Offset = context.Eip;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = context.Esp;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = context.Ebp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;

        // StackWalk (DbgHelp)
        for (int i = 0; i < 512 && stackFrame.AddrPC.Offset; ++i)
        {
            BOOL ok = StackWalk(
                IMAGE_FILE_MACHINE_I386,
                hProcess,
                hThread,
                &stackFrame,
                &context,
                NULL,    // ReadProcessMemory -> NULL jó saját folyamathoz
                NULL,    // SymFunctionTableAccess -> NULL elég, ha nincs szimbólum
                NULL,    // SymGetModuleBase -> NULL elég
                NULL);

            if (!ok)
                break;

            fprintf(fException, "0x%08X\t", (DWORD)stackFrame.AddrPC.Offset);
            EnumerateLoadedModules(
                hProcess,
                (PENUMLOADED_MODULES_CALLBACK)EnumerateLoadedModulesProc,
                &stackFrame.AddrPC.Offset);
            fprintf(fException, "\n");
        }

        fprintf(fException, "\n");
        fflush(fException);
        fclose(fException);
        fException = nullptr;

        // opcionális: saját hibamegjelenítõ
        WinExec("errorlog.exe", SW_SHOW);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

void SetEterExceptionHandler()
{
    // fontos: ne dobjon fel rendszer dialógust
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    SetUnhandledExceptionFilter(EterExceptionFilter);
}
