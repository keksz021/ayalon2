#include "stdafx.h"
#include "MD5.h"
#include "ShieldHelper.h"
#include <cstdint>

PVOID* find(const char* szFunc, HMODULE hModule)
{
	if (!hModule)
		hModule = GetModuleHandle(0);

	PIMAGE_DOS_HEADER img_dos_headers = (PIMAGE_DOS_HEADER)hModule;

	PIMAGE_NT_HEADERS img_nt_headers = (PIMAGE_NT_HEADERS)((byte*)img_dos_headers + img_dos_headers->e_lfanew);

	PIMAGE_IMPORT_DESCRIPTOR img_import_desc = (PIMAGE_IMPORT_DESCRIPTOR)((byte*)img_dos_headers + img_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	if (img_dos_headers->e_magic != IMAGE_DOS_SIGNATURE)
		printf("e_magic dos sig\n");

	for (IMAGE_IMPORT_DESCRIPTOR* iid = img_import_desc; iid->Name != 0; iid++)
	{
		for (int func_idx = 0; *(func_idx + (void**)(iid->FirstThunk + (size_t)hModule)) != nullptr; func_idx++)
		{
			char* mod_func_name = (char*)(*(func_idx + (size_t*)(iid->OriginalFirstThunk + (size_t)hModule)) + (size_t)hModule + 2);

			const intptr_t nmod_func_name = (intptr_t)mod_func_name;

			if (nmod_func_name >= 0)
			{
				if (!::strcmp(szFunc, mod_func_name))
					return func_idx + (void**)(iid->FirstThunk + (size_t)hModule);
			}
		}
	}
	return 0;
}


std::uint32_t detour_ptr(const char* szFunc, PVOID newfunction, HMODULE module)
{
	void**&& func_ptr = find(szFunc, module);

	if (*func_ptr == newfunction || *func_ptr == nullptr)
		return 0;

	DWORD old_rights;
	DWORD new_rights = PAGE_READWRITE;

	VirtualProtect(func_ptr, sizeof(uintptr_t), new_rights, &old_rights);

	uintptr_t ret = (uintptr_t)*func_ptr;

	*func_ptr = newfunction;

	VirtualProtect(func_ptr, sizeof(uintptr_t), old_rights, &new_rights);

	return ret;
}

using WriteProcessMemoryFn = BOOL(__stdcall*)(HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T*);
WriteProcessMemoryFn oWriteProcessMemory;

BOOL __stdcall hkWriteProcessMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten)
{
	return oWriteProcessMemory(nullptr, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
}

void FreezeClient()
{
	while (1) {}
}

ShieldHelper::ShieldHelper()
{
	Checklist.clear();
}

ShieldHelper::~ShieldHelper()
{
	Checklist.clear();
}

bool ShieldHelper::ActivateProtection()
{
	ProtectReadProcessMemory();

	bool FileCheck = CheckFiles();

	if (!FileCheck)
		FreezeClient();

	return FileCheck;
}

void ShieldHelper::ProtectReadProcessMemory()
{
	oWriteProcessMemory = (WriteProcessMemoryFn)detour_ptr("WriteProcessMemory", (PVOID)hkWriteProcessMemory, GetModuleHandleA("Kernel32.dll"));
}

void ShieldHelper::AddFileEntry(std::string path, std::string hash)
{
	FileEntry entry;
	entry.PATH = path;
	entry.MD5 = hash;

	Checklist.push_back(entry);
}

bool ShieldHelper::CheckFiles()
{
	MD5 Checker;
	for (size_t i = 0; i < Checklist.size(); i++)
	{
		std::string resultMD5 = Checker.digestFile((char*)Checklist[i].PATH.c_str());

		if (Checklist[i].MD5 != resultMD5)
			return false;
	}

	return true;
}

