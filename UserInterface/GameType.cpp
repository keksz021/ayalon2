#include "StdAfx.h"
#include "Packet.h"
#include "GameType.h"

std::string g_strResourcePath = "d:/ymir work/";
std::string g_strImagePath = "d:/ymir work/ui/";

std::string g_strGuildSymbolPathName = "mark/10/";

static std::string gs_strDefaultFontName = "±¼¸²Ã¼:12.fnt";
static std::string gs_strDefaultItalicFontName = "±¼¸²Ã¼:12i.fnt";
static CResource* gs_pkDefaultFont = NULL;
static CResource* gs_pkDefaultItalicFont = NULL;

static bool gs_isReloadDefaultFont = false;

void DefaultFont_Startup()
{
	gs_pkDefaultFont = NULL;
}

void DefaultFont_Cleanup()
{
	if (gs_pkDefaultFont)
		gs_pkDefaultFont->Release();
}

void DefaultFont_SetName(const char *c_szFontName)
{
	gs_strDefaultFontName = c_szFontName;
	gs_strDefaultFontName += ".fnt";

	gs_strDefaultItalicFontName = c_szFontName;
	if(strchr(c_szFontName, ':'))
		gs_strDefaultItalicFontName += "i";
	gs_strDefaultItalicFontName += ".fnt";

	gs_isReloadDefaultFont = true;
}

bool ReloadDefaultFonts()
{
	CResourceManager& rkResMgr = CResourceManager::Instance();

	gs_isReloadDefaultFont = false;

	CResource* pkNewFont = rkResMgr.GetResourcePointer(gs_strDefaultFontName.c_str());
	pkNewFont->AddReference();
	if (gs_pkDefaultFont)
		gs_pkDefaultFont->Release();
	gs_pkDefaultFont = pkNewFont;

	CResource* pkNewItalicFont = rkResMgr.GetResourcePointer(gs_strDefaultItalicFontName.c_str());
	pkNewItalicFont->AddReference();
	if (gs_pkDefaultItalicFont)
		gs_pkDefaultItalicFont->Release();
	gs_pkDefaultItalicFont = pkNewItalicFont;

	return true;
}

CResource* DefaultFont_GetResource()
{
	if (!gs_pkDefaultFont || gs_isReloadDefaultFont)
		ReloadDefaultFonts();
	return gs_pkDefaultFont;
}

CResource* DefaultItalicFont_GetResource()
{	
	if (!gs_pkDefaultItalicFont || gs_isReloadDefaultFont)
		ReloadDefaultFonts();
	return gs_pkDefaultItalicFont;
}

void SetGuildSymbolPath(const char *c_szPathName)
{
	g_strGuildSymbolPathName = "mark/";
	g_strGuildSymbolPathName += c_szPathName;
	g_strGuildSymbolPathName += "/";
}

const char *GetGuildSymbolFileName(DWORD dwGuildID)
{
	return _getf("%s%03d.jpg", g_strGuildSymbolPathName.c_str(), dwGuildID);
}

BYTE c_aSlotTypeToInvenType[SLOT_TYPE_MAX] =
{
	RESERVED_WINDOW,		// SLOT_TYPE_NONE
	INVENTORY,				// SLOT_TYPE_INVENTORY
	RESERVED_WINDOW,		// SLOT_TYPE_SKILL
	RESERVED_WINDOW,		// SLOT_TYPE_EMOTION
	RESERVED_WINDOW,		// SLOT_TYPE_SHOP
	RESERVED_WINDOW,		// SLOT_TYPE_EXCHANGE_OWNER
	RESERVED_WINDOW,		// SLOT_TYPE_EXCHANGE_TARGET
	RESERVED_WINDOW,		// SLOT_TYPE_QUICK_SLOT
	RESERVED_WINDOW,		// SLOT_TYPE_SAFEBOX
	RESERVED_WINDOW,		// SLOT_TYPE_PRIVATE_SHOP
	RESERVED_WINDOW,		// SLOT_TYPE_MALL
	DRAGON_SOUL_INVENTORY,	// SLOT_TYPE_DRAGON_SOUL_INVENTORY
	BELT_INVENTORY,			// SLOT_TYPE_BELT_INVENTORY
#ifdef ENABLE_RENEWAL_SWITCHBOT
	SWITCHBOT,				// SLOT_TYPE_SWITCHBOT
#endif
#ifdef ENABLE_SPECIAL_INVENTORY
	INVENTORY,				// SLOT_TYPE_SKILL_BOOK_INVENTORY
	INVENTORY,				// SLOT_TYPE_MATERIAL_INVENTORY
	INVENTORY,				// SLOT_TYPE_STONE_INVENTORY
	INVENTORY,				// SLOT_TYPE_GIFT_BOX_INVENTORY
	INVENTORY,				// SLOT_TYPE_CHANGERS_INVENTORY
#endif
#ifdef ENABLE_AURA_COSTUME_SYSTEM
	AURA_REFINE,			// SLOT_TYPE_AURA
#endif
};

BYTE SlotTypeToInvenType(BYTE bSlotType)
{
	if (bSlotType >= SLOT_TYPE_MAX)
		return RESERVED_WINDOW;
	else
		return c_aSlotTypeToInvenType[bSlotType];
}

#ifdef ENABLE_AURA_COSTUME_SYSTEM
static int s_aiAuraRefineInfo[CItemData::AURA_GRADE_MAX_NUM][AURA_REFINE_INFO_MAX] = {
	{1,   1,  49,  1000, 30617, 10,  5000000, 100},
	{2,  50,  99,  2000, 31136, 10,  5000000, 100},
	{3, 100, 149,  4000, 31137, 10,  5000000, 100},
	{4, 150, 199,  8000, 31138, 10,  8000000, 100},
	{5, 200, 249, 16000, 31138, 20, 10000000, 100},
	{6, 250, 250,     0,     0,  0,        0,   0},
	{0,   0,   0,     0,     0,  0,        0,   0}
};

int* GetAuraRefineInfo(BYTE bLevel)
{
	if (bLevel > 250)
		return NULL;

	for (int i = 0; i < CItemData::AURA_GRADE_MAX_NUM + 1; ++i)
	{
		if (bLevel >= s_aiAuraRefineInfo[i][AURA_REFINE_INFO_LEVEL_MIN] && bLevel <= s_aiAuraRefineInfo[i][AURA_REFINE_INFO_LEVEL_MAX])
			return s_aiAuraRefineInfo[i];
	}

	return NULL;
}
#endif



#ifdef ENABLE_RENEWAL_BONUS_BOARD

typedef struct SApplyInfo
{
	BYTE bPointType;
} TApplyInfo;

const TApplyInfo aApplyInfo[CItemData::MAX_APPLY_NUM] =
{
	{ POINT_NONE,						},	// APPLY_NONE,
	{ POINT_MAX_HP,						},	// APPLY_MAX_HP,
	{ POINT_MAX_SP,						},	// APPLY_MAX_SP,
	{ POINT_HT,							},	// APPLY_CON,
	{ POINT_IQ,							},	// APPLY_INT,
	{ POINT_ST,							},	// APPLY_STR,
	{ POINT_DX,							},	// APPLY_DEX,
	{ POINT_ATT_SPEED,					},	// APPLY_ATT_SPEED,
	{ POINT_MOV_SPEED,					},	// APPLY_MOV_SPEED,
	{ POINT_CASTING_SPEED,				},	// APPLY_CAST_SPEED,
	{ POINT_HP_REGEN,					},	// APPLY_HP_REGEN,
	{ POINT_SP_REGEN,					},	// APPLY_SP_REGEN,
	{ POINT_POISON_PCT,					},	// APPLY_POISON_PCT,
	{ POINT_STUN_PCT,					},	// APPLY_STUN_PCT,
	{ POINT_SLOW_PCT,					},	// APPLY_SLOW_PCT,
	{ POINT_CRITICAL_PCT,				},	// APPLY_CRITICAL_PCT,
	{ POINT_PENETRATE_PCT,				},	// APPLY_PENETRATE_PCT,
	{ POINT_ATTBONUS_HUMAN,				},	// APPLY_ATTBONUS_HUMAN,
	{ POINT_ATTBONUS_ANIMAL,			},	// APPLY_ATTBONUS_ANIMAL,
	{ POINT_ATTBONUS_ORC,				},	// APPLY_ATTBONUS_ORC,
	{ POINT_ATTBONUS_MILGYO,			},	// APPLY_ATTBONUS_MILGYO,
	{ POINT_ATTBONUS_UNDEAD,			},	// APPLY_ATTBONUS_UNDEAD,
	{ POINT_ATTBONUS_DEVIL,				},	// APPLY_ATTBONUS_DEVIL,
	{ POINT_STEAL_HP,					},	// APPLY_STEAL_HP,
	{ POINT_STEAL_SP,					},	// APPLY_STEAL_SP,
	{ POINT_MANA_BURN_PCT,				},	// APPLY_MANA_BURN_PCT,
	{ POINT_DAMAGE_SP_RECOVER,			},	// APPLY_DAMAGE_SP_RECOVER,
	{ POINT_BLOCK,						},	// APPLY_BLOCK,
	{ POINT_DODGE,						},	// APPLY_DODGE,
	{ POINT_RESIST_SWORD,				},	// APPLY_RESIST_SWORD,
	{ POINT_RESIST_TWOHAND,				},	// APPLY_RESIST_TWOHAND,
	{ POINT_RESIST_DAGGER,				},	// APPLY_RESIST_DAGGER,
	{ POINT_RESIST_BELL,				},	// APPLY_RESIST_BELL,
	{ POINT_RESIST_FAN,					},	// APPLY_RESIST_FAN,
	{ POINT_RESIST_BOW,					},	// APPLY_RESIST_BOW,
	{ POINT_RESIST_FIRE,				},	// APPLY_RESIST_FIRE,
	{ POINT_RESIST_ELEC,				},	// APPLY_RESIST_ELEC,
	{ POINT_RESIST_MAGIC,				},	// APPLY_RESIST_MAGIC,
	{ POINT_RESIST_WIND,				},	// APPLY_RESIST_WIND,
	{ POINT_REFLECT_MELEE,				},	// APPLY_REFLECT_MELEE,
	{ POINT_REFLECT_CURSE,				},	// APPLY_REFLECT_CURSE,
	{ POINT_POISON_REDUCE,				},	// APPLY_POISON_REDUCE,
	{ POINT_KILL_SP_RECOVER,			},	// APPLY_KILL_SP_RECOVER,
	{ POINT_EXP_DOUBLE_BONUS,			},	// APPLY_EXP_DOUBLE_BONUS,
	{ POINT_GOLD_DOUBLE_BONUS,			},	// APPLY_GOLD_DOUBLE_BONUS,
	{ POINT_ITEM_DROP_BONUS,			},	// APPLY_ITEM_DROP_BONUS,
	{ POINT_POTION_BONUS,				},	// APPLY_POTION_BONUS,
	{ POINT_KILL_HP_RECOVER,			},	// APPLY_KILL_HP_RECOVER,
	{ POINT_IMMUNE_STUN,				},	// APPLY_IMMUNE_STUN,
	{ POINT_IMMUNE_SLOW,				},	// APPLY_IMMUNE_SLOW,
	{ POINT_IMMUNE_FALL,				},	// APPLY_IMMUNE_FALL,
	{ POINT_NONE,						},	// APPLY_SKILL,
	{ POINT_BOW_DISTANCE,				},	// APPLY_BOW_DISTANCE,
	{ POINT_ATT_GRADE_BONUS,			},	// APPLY_ATT_GRADE,
	{ POINT_DEF_GRADE_BONUS,			},	// APPLY_DEF_GRADE,
	{ POINT_MAGIC_ATT_GRADE_BONUS,		},	// APPLY_MAGIC_ATT_GRADE,
	{ POINT_MAGIC_DEF_GRADE_BONUS,		},	// APPLY_MAGIC_DEF_GRADE,
	{ POINT_CURSE_PCT,					},	// APPLY_CURSE_PCT,
	{ POINT_MAX_STAMINA					},	// APPLY_MAX_STAMINA
	{ POINT_ATTBONUS_WARRIOR			},	// APPLY_ATTBONUS_WARRIOR
	{ POINT_ATTBONUS_ASSASSIN			},	// APPLY_ATTBONUS_ASSASSIN
	{ POINT_ATTBONUS_SURA				},	// APPLY_ATTBONUS_SURA
	{ POINT_ATTBONUS_SHAMAN				},	// APPLY_ATTBONUS_SHAMAN
	{ POINT_ATTBONUS_MONSTER			},	// APPLY_ATTBONUS_MONSTER
	{ POINT_ATT_BONUS					},	// APPLY_MALL_ATTBONUS
	{ POINT_MALL_DEFBONUS				},	// APPLY_MALL_DEFBONUS
	{ POINT_MALL_EXPBONUS				},	// APPLY_MALL_EXPBONUS
	{ POINT_MALL_ITEMBONUS				},	// APPLY_MALL_ITEMBONUS
	{ POINT_MALL_GOLDBONUS				},	// APPLY_MALL_GOLDBONUS
	{ POINT_MAX_HP_PCT					},	// APPLY_MAX_HP_PCT
	{ POINT_MAX_SP_PCT					},	// APPLY_MAX_SP_PCT
	{ POINT_SKILL_DAMAGE_BONUS			},	// APPLY_SKILL_DAMAGE_BONUS
	{ POINT_NORMAL_HIT_DAMAGE_BONUS		},	// APPLY_NORMAL_HIT_DAMAGE_BONUS
	{ POINT_SKILL_DEFEND_BONUS			},	// APPLY_SKILL_DEFEND_BONUS
	{ POINT_NORMAL_HIT_DEFEND_BONUS		},	// APPLY_NORMAL_HIT_DEFEND_BONUS
	{ POINT_NONE,						},	// APPLY_EXTRACT_HP_PCT
	{ POINT_RESIST_WARRIOR,				},	// APPLY_RESIST_WARRIOR
	{ POINT_RESIST_ASSASSIN,			},	// APPLY_RESIST_ASSASSIN
	{ POINT_RESIST_SURA,				},	// APPLY_RESIST_SURA
	{ POINT_RESIST_SHAMAN,				},	// APPLY_RESIST_SHAMAN
	{ POINT_ENERGY						},	// APPLY_ENERGY
	{ POINT_DEF_GRADE					},	// APPLY_DEF_GRADE
	{ POINT_COSTUME_ATTR_BONUS			},	// APPLY_COSTUME_ATTR_BONUS
	{ POINT_MAGIC_ATT_BONUS_PER 		},	// APPLY_MAGIC_ATTBONUS_PER
	{ POINT_MELEE_MAGIC_ATT_BONUS_PER	},	// APPLY_MELEE_MAGIC_ATTBONUS_PER
	{ POINT_RESIST_ICE,					},	// APPLY_RESIST_ICE
	{ POINT_RESIST_EARTH,				},	// APPLY_RESIST_EARTH
	{ POINT_RESIST_DARK,				},	// APPLY_RESIST_DARK
	{ POINT_RESIST_CRITICAL,			},	// APPLY_ANTI_CRITICAL_PCT
	{ POINT_RESIST_PENETRATE,			},	// APPLY_ANTI_PENETRATE_PCT
#ifdef ENABLE_PENDANT_SYSTEM
	{ POINT_ATTBONUS_ELEC,				},	// APPLY_ATTBONUS_ELEC
	{ POINT_ATTBONUS_FIRE,				},	// APPLY_ATTBONUS_FIRE
	{ POINT_ATTBONUS_ICE,				},	// APPLY_ATTBONUS_ICE
	{ POINT_ATTBONUS_WIND,				},	// APPLY_ATTBONUS_WIND
	{ POINT_ATTBONUS_EARTH,				},	// APPLY_ATTBONUS_EARTH
	{ POINT_ATTBONUS_DARK,				},	// APPLY_ATTBONUS_DARK
#endif
#ifdef ENABLE_ATTBONUS_METIN
	{ POINT_ATTBONUS_METIN,				},	// APPLY_ATTBONUS_METIN
#endif
#ifdef ENABLE_ATTBONUS_BOSS
	{ POINT_ATTBONUS_BOSS,				},	// APPLY_ATTBONUS_BOSS
#endif
};

BYTE ApplyTypeToPointType(BYTE bApplyType)
{
	if (bApplyType >= CItemData::MAX_APPLY_NUM)
		return POINT_NONE;
	else
		return aApplyInfo[bApplyType].bPointType;
}

BYTE PointTypeToApplyType(BYTE bPointType)
{
	auto i = 0;
	for (const auto& apply : aApplyInfo)
	{
		if (apply.bPointType == bPointType)
			return i;

		i++;
	}

	return 0;
}
#endif



