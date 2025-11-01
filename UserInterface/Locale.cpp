#include "StdAfx.h"
#include "Locale.h"
#include "PythonApplication.h"
#include "resource.h"

#include "../eterBase/CRC32.h"
#include "../eterpack/EterPackManager.h"
#include "../eterLocale/Japanese.h"
#include <windowsx.h>

char MULTI_LOCALE_PATH[256] = "locale/ymir";
char MULTI_LOCALE_NAME[256] = "ymir";

int LOCALE_CODEPAGE = 949;
void LoadConfig(const char *fileName)
{
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	bool bMultiLocale = false;
	const char *szMultiLocale = "lang.cfg";
	if (_access(szMultiLocale, 0) == 0)
	{
		bMultiLocale = true;
		fileName = szMultiLocale;
	}
#endif

	FILE* fp = fopen(fileName, "rt");

	if (fp)
	{
		char line[256];
		char name[256];
		int code;
		int id;

		if (fgets(line, sizeof(line)-1, fp))
		{
			line[sizeof(line)-1] = '\0';

#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
			if (bMultiLocale)
			{
				sscanf(line, "%d %s", &code, name);
			}
			else
			{
				sscanf(line, "%d %d %s", &id, &code, name);
			}
#else
			sscanf(line, "%d %d %s", &id, &code, name);
#endif

			LOCALE_CODEPAGE = code;
			strcpy(MULTI_LOCALE_NAME, name);
			sprintf(MULTI_LOCALE_PATH, "locale/%s", MULTI_LOCALE_NAME);
		}
		fclose(fp);
	}
}

unsigned GetGuildLastExp(int level)
{
	static const int GUILD_LEVEL_MAX = 20;
	static DWORD GUILD_EXP_LIST[GUILD_LEVEL_MAX + 1] =
	{
	0,
	15000UL,
	45000UL,
	90000UL,
	160000UL,
	235000UL,
	325000UL,
	430000UL,
	550000UL,
	685000UL,
	835000UL,
	1000000UL,
	1500000UL,
	2100000UL,
	2800000UL,
	3600000UL,
	4500000UL,
	6500000UL,
	8000000UL,
	10000000UL,
	42000000UL
	};

	if (level < 0 && level >= GUILD_LEVEL_MAX)
	{
		return 0;
	}

	return GUILD_EXP_LIST[level];
}

int GetSkillPower(unsigned level)
{
	static const unsigned SKILL_POWER_NUM = 50;
	if (level >= SKILL_POWER_NUM)
	{
		return 0;
	}

	static unsigned SKILL_POWERS[SKILL_POWER_NUM] =
	{
		0,
		5,  6,  8, 10, 12,
		14, 16, 18, 20, 22,
		24, 26, 28, 30, 32,
		34, 36, 38, 40, 50,
		52, 54, 56, 58, 60,
		63, 66, 69, 72, 82,
		85, 88, 91, 94, 98,
		102, 106, 110, 115, 125,
		125,
	};

	return SKILL_POWERS[level];
}

unsigned int GetCodePage()
{
	return LOCALE_CODEPAGE;
}

int StringCompareCI (LPCSTR szStringLeft, LPCSTR szStringRight, size_t sizeLength)
{
	return strnicmp (szStringLeft, szStringRight, sizeLength);
}

const char *LocaleService_GetLocaleName()
{
	return MULTI_LOCALE_NAME;
}

const char *	LocaleService_GetLocalePath()
{
	return MULTI_LOCALE_PATH;
}

#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
BYTE LocaleService_GetLocaID()
{
	const char *c_szLocale = LocaleService_GetLoca();
	if (strcmp(c_szLocale, "en") == 0)
		return CPythonApplication::LOCALE_EN;
	else if (strcmp(c_szLocale, "ro") == 0)
		return CPythonApplication::LOCALE_RO;
#ifdef EXTENDED_LANGUAGE
	else if (strcmp(c_szLocale, "pt") == 0)
		return CPythonApplication::LOCALE_PT;
	else if (strcmp(c_szLocale, "es") == 0)
		return CPythonApplication::LOCALE_ES;
	else if (strcmp(c_szLocale, "fr") == 0)
		return CPythonApplication::LOCALE_FR;
	else if (strcmp(c_szLocale, "de") == 0)
		return CPythonApplication::LOCALE_DE;
	else if (strcmp(c_szLocale, "pl") == 0)
		return CPythonApplication::LOCALE_PL;
	else if (strcmp(c_szLocale, "it") == 0)
		return CPythonApplication::LOCALE_IT;
	else if (strcmp(c_szLocale, "cz") == 0)
		return CPythonApplication::LOCALE_CZ;
	else if (strcmp(c_szLocale, "hu") == 0)
		return CPythonApplication::LOCALE_HU;
	else if (strcmp(c_szLocale, "tr") == 0)
		return CPythonApplication::LOCALE_TR;
#endif
	else
		return CPythonApplication::LOCALE_DEFAULT;
}

const char *LocaleService_GetLoca() { return MULTI_LOCALE_NAME; }
bool LocaleService_SaveLoca(int iCodePage, const char *szLocale)
{
	LOCALE_CODEPAGE = iCodePage;
	strcpy(MULTI_LOCALE_NAME, szLocale);
	sprintf(MULTI_LOCALE_PATH, "locale/%s", MULTI_LOCALE_NAME);

	SetDefaultCodePage(iCodePage);

	FILE* File;

	if (NULL == (File = fopen("lang.cfg", "wt")))
		return false;

	fprintf(File, "%d %s", iCodePage, szLocale);
	fclose(File);
	return true;
}
#endif
