#include "StdAfx.h"

#include "../eterPack/EterPackManager.h"

#include "pythonnonplayer.h"
#include "InstanceBase.h"
#include "PythonCharacterManager.h"

#ifdef ENABLE_INGAME_WIKI_SYSTEM
	#include "PythonWiki.h"
#endif

bool CPythonNonPlayer::LoadNonPlayerData(const char *c_szFileName)
{
	static DWORD s_adwMobProtoKey[4] =
	{
		4813894,
		18955,
		552631,
		6822045
	};

	CMappedFile file;
	LPCVOID pvData;

	Tracef("CPythonNonPlayer::LoadNonPlayerData: %s, sizeof(TMobTable)=%u\n", c_szFileName, sizeof(TMobTable));

	if (!CEterPackManager::Instance().Get(file, c_szFileName, &pvData))
		return false;

	DWORD dwFourCC, dwElements, dwDataSize;

	file.Read(&dwFourCC, sizeof(DWORD));

	if (dwFourCC != MAKEFOURCC('M', 'M', 'P', 'T'))
	{
		TraceError("CPythonNonPlayer::LoadNonPlayerData: invalid Mob proto type %s", c_szFileName);
		return false;
	}

	file.Read(&dwElements, sizeof(DWORD));
	file.Read(&dwDataSize, sizeof(DWORD));

	BYTE * pbData = new BYTE[dwDataSize];
	file.Read(pbData, dwDataSize);
	/////

	CLZObject zObj;

	if (!CLZO::Instance().Decompress(zObj, pbData, s_adwMobProtoKey))
	{
		delete [] pbData;
		return false;
	}

	if ((zObj.GetSize() % sizeof(TMobTable)) != 0)
	{
		TraceError("CPythonNonPlayer::LoadNonPlayerData: invalid size %u check data format.", zObj.GetSize());
		return false;
	}

#ifdef ENABLE_INGAME_WIKI_SYSTEM
	CPythonWiki::Instance().ClearData();
#endif

	TMobTable * pTable = (TMobTable *) zObj.GetBuffer();
	for (DWORD i = 0; i < dwElements; ++i, ++pTable)
	{
		TMobTable * pNonPlayerData = new TMobTable;
		memcpy(pNonPlayerData, pTable, sizeof(TMobTable));

#ifdef ENABLE_INGAME_WIKI_SYSTEM
		CPythonWiki::Instance().LoadMonster(pNonPlayerData);
#endif

		m_NonPlayerDataMap.insert(TNonPlayerDataMap::value_type(pNonPlayerData->dwVnum, pNonPlayerData));
	}

#ifdef ENABLE_INGAME_WIKI_SYSTEM
	CPythonWiki::Instance().ListReverse();
#endif

	delete [] pbData;
	return true;
}

bool CPythonNonPlayer::GetName(DWORD dwVnum, const char ** c_pszName)
{
	const TMobTable * p = GetTable(dwVnum);

	if (!p)
		return false;

	*c_pszName = p->szLocaleName;

	return true;
}

bool CPythonNonPlayer::GetInstanceType(DWORD dwVnum, BYTE* pbType)
{
	const TMobTable * p = GetTable(dwVnum);

	if (!p)
		return false;

	*pbType=p->bType;

	return true;
}

const CPythonNonPlayer::TMobTable * CPythonNonPlayer::GetTable(DWORD dwVnum)
{
	TNonPlayerDataMap::iterator itor = m_NonPlayerDataMap.find(dwVnum);

	if (itor == m_NonPlayerDataMap.end())
		return NULL;

	return itor->second;
}

BYTE CPythonNonPlayer::GetEventType(DWORD dwVnum)
{
	const TMobTable * p = GetTable(dwVnum);

	if (!p)
	{
		return ON_CLICK_EVENT_NONE;
	}

	return p->bOnClickType;
}

BYTE CPythonNonPlayer::GetEventTypeByVID(DWORD dwVID)
{
	CInstanceBase * pInstance = CPythonCharacterManager::Instance().GetInstancePtr(dwVID);

	if (NULL == pInstance)
	{
		return ON_CLICK_EVENT_NONE;
	}

	WORD dwVnum = pInstance->GetVirtualNumber();
	return GetEventType(dwVnum);
}

const char *	CPythonNonPlayer::GetMonsterName(DWORD dwVnum)
{	
	const CPythonNonPlayer::TMobTable * c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		static const char *sc_szEmpty="";
		return sc_szEmpty;
	}

	return c_pTable->szLocaleName;
}

#ifdef ENABLE_MONSTER_TARGET_ELEMENT
bool CPythonNonPlayer::MonsterHasRaceFlag(DWORD dwVnum, const char *szSearchString)
{
	std::string raceFlagList[] = {
		"ANIMAL","UNDEAD","DEVIL","HUMAN","ORC","MILGYO","INSECT","FIRE","ICE","DESERT","TREE",
		"ATT_ELEC","ATT_FIRE","ATT_ICE","ATT_WIND","ATT_EARTH","ATT_DARK"
	};

	const CPythonNonPlayer::TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
		return false;

	DWORD dwRaceFlag = c_pTable->dwRaceFlag;

	if (dwRaceFlag == 0)
		return false;

	std::string toSearchString("");
	std::string searchString(szSearchString);
	int tmpFlag;

	for (int i = 0; i < sizeof(raceFlagList) / sizeof(raceFlagList[0]); i++)
	{
		tmpFlag = static_cast<int>(pow(2.0, static_cast<double>(i)));
		if (dwRaceFlag & tmpFlag)
		{
			if (!!toSearchString.compare(""))
				toSearchString += ",";
			toSearchString += raceFlagList[i];
		}
	}

	size_t found = toSearchString.find(searchString);
	if (found != std::string::npos)
	{
		return true;
	}

	return false;
}
#endif

DWORD CPythonNonPlayer::GetMonsterColor(DWORD dwVnum)

{

	const CPythonNonPlayer::TMobTable * c_pTable = GetTable(dwVnum);
	if (!c_pTable)
		return 0;

	return c_pTable->dwMonsterColor;

}

#ifdef ENABLE_MONSTER_SPECULAR
float CPythonNonPlayer::GetMonsterSpecular(DWORD dwVnum)
{
	const TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
		return 0.0f;

	return c_pTable->fSpecular;
}
#endif

#ifdef ENABLE_BOSS_EFFECT_OVER_HEAD
DWORD CPythonNonPlayer::GetMonsterType(DWORD dwVnum)
{
	const CPythonNonPlayer::TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
		return 0;

	return c_pTable->bType;
}

DWORD CPythonNonPlayer::GetMonsterRank(DWORD dwVnum)
{
	const CPythonNonPlayer::TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
		return 0;

	return c_pTable->bRank;
}
#endif
#ifdef EFFECT_METIN_KOBRA
bool CPythonNonPlayer::IsMetinStoneWithEffect(DWORD dwVnum)
{
	static const std::set<DWORD> s_MetinEffectVnums = {
		8501, 8502, 8503, 8504, 8505,
		8506, 8507, 8508, 8509, 8510, 8511,
	};

	if (s_MetinEffectVnums.count(dwVnum) == 0)
		return false;

	return GetMonsterType(dwVnum) == CActorInstance::TYPE_STONE;
}
#endif
void CPythonNonPlayer::GetMatchableMobList(int iLevel, int iInterval, TMobTableList * pMobTableList)
{
}

#ifdef ENABLE_MOB_SCALE
bool CPythonNonPlayer::LoadMobScale(const char *c_szFileName)
{
	const VOID* pvData;
	CMappedFile kFile;

	if (!CEterPackManager::Instance().Get(kFile, c_szFileName, &pvData))
		return false;

	CMemoryTextFileLoader kTextFileLoader;
	kTextFileLoader.Bind(kFile.Size(), pvData);
	CTokenVector kTokenVector;

	for (DWORD i = 0; i < kTextFileLoader.GetLineCount(); ++i)
	{
		if (!kTextFileLoader.SplitLineByTab(i, &kTokenVector))
			continue;

		if (kTokenVector.size() < 6)
		{
			TraceError("LoadMobScale: invalid line %d (%s).", i, c_szFileName);
			continue;
		}

		DWORD dwMobNum = atoi(kTokenVector[MOBSCALETABLE_MOB_NUM].c_str());
		float fx = atof(kTokenVector[MOBSCALETABLE_X].c_str());
		float fy = atof(kTokenVector[MOBSCALETABLE_Y].c_str());
		float fz = atof(kTokenVector[MOBSCALETABLE_Z].c_str());
		float ra = atof(kTokenVector[MOBSCALETABLE_RANDOM_A].c_str());
		float rb = atof(kTokenVector[MOBSCALETABLE_RANDOM_B].c_str());
		TMobScaleTable* pMobScale = new TMobScaleTable;
		pMobScale->dwMobNum = dwMobNum;
		pMobScale->fx = fx;
		pMobScale->fy = fy;
		pMobScale->fz = fz;
		pMobScale->ra = ra;
		pMobScale->rb = rb;
		m_NonMobScaleDataMap.insert(TNonMobScaleDataMap::value_type(dwMobNum, pMobScale));
	}

	return true;
}

const CPythonNonPlayer::TMobScaleTable* CPythonNonPlayer::GetScaleTable(DWORD dwVnum)
{
	TNonMobScaleDataMap::iterator itor = m_NonMobScaleDataMap.find(dwVnum);

	if (itor == m_NonMobScaleDataMap.end())
		return NULL;

	return itor->second;
}

bool CPythonNonPlayer::GetScale(DWORD dwVnum, float& fx, float& fy, float& fz)
{
	const CPythonNonPlayer::TMobScaleTable* c_pTable = GetScaleTable(dwVnum);

	if (!c_pTable)
	{
		fx = 1.0f;
		fy = 1.0f;
		fz = 1.0f;
	}
	else
	{
		if (c_pTable->ra > 0 && c_pTable->rb > 0)
		{
			float scale = frandom(c_pTable->ra, c_pTable->rb);
			fx = scale;
			fy = scale;
			fz = scale;
		}
		else
		{
			fx = c_pTable->fx;
			fy = c_pTable->fy;
			fz = c_pTable->fz;
		}
	}

	return true;
}
#endif

#ifdef ENABLE_INGAME_WIKI_SYSTEM
DWORD CPythonNonPlayer::GetMonsterPrice1(DWORD dwVnum)
{
	const CPythonNonPlayer::TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
		return 0;

	return c_pTable->dwGoldMin;
}

DWORD CPythonNonPlayer::GetMonsterPrice2(DWORD dwVnum)
{
	const CPythonNonPlayer::TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
		return 0;

	return c_pTable->dwGoldMax;
}

char CPythonNonPlayer::GetMonsterResist(DWORD dwVnum, BYTE byResist)
{
	if (byResist >= MOB_RESISTS_MAX_NUM)
		return 0;

	const CPythonNonPlayer::TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
		return 0;

	return c_pTable->cResists[byResist];
}

DWORD CPythonNonPlayer::GetMonsterMaxHP(DWORD dwVnum)
{
	const CPythonNonPlayer::TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		DWORD dwMaxHP = 0;
		return dwMaxHP;
	}

	return c_pTable->dwMaxHP;
}

DWORD CPythonNonPlayer::GetMonsterRaceFlag(DWORD dwVnum)
{
	const CPythonNonPlayer::TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		DWORD dwRaceFlag = 0;
		return dwRaceFlag;
	}

	return c_pTable->dwRaceFlag;
}

DWORD CPythonNonPlayer::GetMonsterLevel(DWORD dwVnum)
{
	const CPythonNonPlayer::TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		DWORD level = 0;
		return level;
	}

	return c_pTable->bLevel;
}

DWORD CPythonNonPlayer::GetMonsterDamage1(DWORD dwVnum)
{
	const CPythonNonPlayer::TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		DWORD range = 0;
		return range;
	}

	return c_pTable->dwDamageRange[0];
}

DWORD CPythonNonPlayer::GetMonsterDamage2(DWORD dwVnum)
{
	const CPythonNonPlayer::TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		DWORD range = 0;
		return range;
	}

	return c_pTable->dwDamageRange[1];
}

DWORD CPythonNonPlayer::GetMonsterExp(DWORD dwVnum)
{
	const CPythonNonPlayer::TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		DWORD dwExp = 0;
		return dwExp;
	}

	return c_pTable->dwExp;
}

float CPythonNonPlayer::GetMonsterDamageMultiply(DWORD dwVnum)
{
	const CPythonNonPlayer::TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		DWORD fDamMultiply = 0;
		return fDamMultiply;
	}

	return c_pTable->fDamMultiply;
}

DWORD CPythonNonPlayer::GetMonsterST(DWORD dwVnum)
{
	const CPythonNonPlayer::TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		DWORD bStr = 0;
		return bStr;
	}

	return c_pTable->bStr;
}

DWORD CPythonNonPlayer::GetMonsterDX(DWORD dwVnum)
{
	const CPythonNonPlayer::TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		DWORD bDex = 0;
		return bDex;
	}

	return c_pTable->bDex;
}

bool CPythonNonPlayer::IsMonsterStone(DWORD dwVnum)
{
	const CPythonNonPlayer::TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		return 0;
	}

	return c_pTable->bType == 2;
}
#endif


void CPythonNonPlayer::Clear()
{
}

void CPythonNonPlayer::Destroy()
{
	for (TNonPlayerDataMap::iterator itor=m_NonPlayerDataMap.begin(); itor!=m_NonPlayerDataMap.end(); ++itor)
	{
		delete itor->second;
	}
	m_NonPlayerDataMap.clear();
}

CPythonNonPlayer::CPythonNonPlayer()
{
	Clear();
}

CPythonNonPlayer::~CPythonNonPlayer(void)
{
	Destroy();
}