#pragma once


void InstallCrashHandler();
LONG __stdcall LynixExceptionFilter(_EXCEPTION_POINTERS* pExceptionInfo);