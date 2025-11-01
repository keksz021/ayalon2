#ifndef __INC_CRC32_H__
#define __INC_CRC32_H__

#include "StdAfx.h"
#include <windows.h>
#include <cstddef>

DWORD GetCRC32(const char* buffer, size_t count);

#ifdef ENABLE_SKILL_COLOR_SYSTEM
// A 3. paraméter opcionális, így a régi objectek is linkelnek
DWORD GetCaseCRC32(const char* buf, size_t len, const char* name = NULL);
#else
DWORD GetCaseCRC32(const char* buf, size_t len);
#endif

DWORD GetHFILECRC32(HANDLE hFile);
DWORD GetFileCRC32(const char* c_szFileName);

// FIGYELEM: Ez a mi, név szerinti fájlméret-olvasónk.
// Nem ütközik a WinAPI ::GetFileSize(HANDLE, LPDWORD)-dal.
DWORD GetFileSize(const char* c_szFileName);

#endif
