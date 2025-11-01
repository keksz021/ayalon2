#include "StdAfx.h"
#include <windows.h>
#include <dbghelp.h>
#include <cstdio>
#include <cstdarg>
#pragma comment(lib, "Dbghelp.lib")

static wchar_t g_appName[128] = L"App";
static wchar_t g_logExe[260] = L"";
static wchar_t g_logTmp[260] = L"";

static void BuildPaths()
{
    wchar_t mod[MAX_PATH] = { 0 }; GetModuleFileNameW(NULL, mod, MAX_PATH);
    wchar_t dir[MAX_PATH] = { 0 }; lstrcpyW(dir, mod);
    for (int i = (int)lstrlenW(dir) - 1; i >= 0; --i) { if (dir[i] == L'\\' || dir[i] == L'/') { dir[i] = 0; break; } }
    wsprintfW(g_logExe, L"%s\\syserr.txt", dir);

    wchar_t tmp[MAX_PATH] = { 0 }; GetTempPathW(MAX_PATH, tmp);
    wsprintfW(g_logTmp, L"%s\\%s_syserr.txt", tmp, g_appName[0] ? g_appName : L"App");
}

static void WriteLogOne(const wchar_t* path, const char* fmt, va_list ap)
{
    FILE* f = nullptr; _wfopen_s(&f, path, L"a");
    if (!f) return;
    vfprintf(f, fmt, ap);
    fputc('\n', f);
    fclose(f);
}

static void Log(const char* fmt, ...)
{
    va_list ap; va_start(ap, fmt); WriteLogOne(g_logExe, fmt, ap); va_end(ap);
    va_list ap2; va_start(ap2, fmt); WriteLogOne(g_logTmp, fmt, ap2); va_end(ap2);
}

void LogBoot(const char* tag) { Log("[BOOT] %s", tag); }

static void WriteDump(EXCEPTION_POINTERS* pExp)
{
    SYSTEMTIME st; GetLocalTime(&st);
    wchar_t folder[MAX_PATH] = { 0 }; lstrcpyW(folder, g_logExe);
    for (int i = (int)lstrlenW(folder) - 1; i >= 0; --i) { if (folder[i] == L'\\' || folder[i] == L'/') { folder[i] = 0; break; } }
    wchar_t dirCrash[MAX_PATH] = { 0 }; wsprintfW(dirCrash, L"%s\\crashdumps", folder);
    CreateDirectoryW(dirCrash, NULL);

    wchar_t dmp[MAX_PATH] = { 0 };
    wsprintfW(dmp, L"%s\\%s_%04d%02d%02d_%02d%02d%02d.dmp",
        dirCrash, g_appName, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    HANDLE h = CreateFileW(dmp, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) { Log("Minidump open FAILED"); return; }

    MINIDUMP_EXCEPTION_INFORMATION mei{};
    mei.ThreadId = GetCurrentThreadId();
    mei.ExceptionPointers = pExp;
    mei.ClientPointers = FALSE;

    BOOL ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), h,
        MiniDumpWithThreadInfo, &mei, nullptr, nullptr);
    CloseHandle(h);
    Log("Minidump: %ls (%s)", dmp, ok ? "OK" : "FAILED");
}

static LONG WINAPI Unhandled(EXCEPTION_POINTERS* pExp)
{
    SYSTEMTIME st; GetLocalTime(&st);
    DWORD code = pExp->ExceptionRecord->ExceptionCode;
    PVOID addr = pExp->ExceptionRecord->ExceptionAddress;
    Log("===== CRASH %04d-%02d-%02d %02d:%02d:%02d =====", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    Log("Unhandled Exception code=0x%08X at %p", code, addr);
    WriteDump(pExp);
    return EXCEPTION_EXECUTE_HANDLER;
}

void InstallCrashHandlers(const wchar_t* appName)
{
    wcsncpy_s(g_appName, appName, _TRUNCATE);
    BuildPaths();
    Log("=== %ls crash handler SAFE STARTUP ===", g_appName);
    SetUnhandledExceptionFilter(Unhandled);
}

extern "C" void InstallCrashHandler() {
    InstallCrashHandlers(L"Ayalon2Rework");
}