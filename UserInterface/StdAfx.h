#pragma once

#pragma warning(disable:4172)

#if _MSC_VER >= 1400
	#define _USE_32BIT_TIME_T
#endif

#include "../eterLib/StdAfx.h"
#include "../eterPythonLib/StdAfx.h"
#include "../gameLib/StdAfx.h"
#include "../scriptLib/StdAfx.h"
#include "../milesLib/StdAfx.h"
#include "../EffectLib/StdAfx.h"
#include "../PRTerrainLib/StdAfx.h"
#include "../SpeedTreeLib/StdAfx.h"

#ifndef __D3DRM_H__
	#define __D3DRM_H__
#endif

#include <array>
#include <dshow.h>
#include <qedit.h>

#include "Locale.h"
#include "../Controls.h"

#include "GameType.h"
extern DWORD __DEFAULT_CODE_PAGE__;

#define APP_NAME	"Metin 2"

enum
{
	POINT_MAX_NUM = 255,
	CHARACTER_NAME_MAX_LEN = 48,
	PLAYER_NAME_MAX_LEN = 12,
};

void initudp();
#ifdef ENABLE_PYTHON_DYNAMIC_MODULE_NAME
void initPythonApi();
#endif
void initapp();
void initime();
void initsystem();
void initchr();
void initchrmgr();
void initChat();
void initTextTail();
void initime();
void initItem();
void initNonPlayer();
void initnet();
void initPlayer();
void initSectionDisplayer();
void initServerStateChecker();
void initTrade();
void initMiniMap();
void initProfiler();
void initEvent();
void initeffect();
void initsnd();
void initeventmgr();
void initBackground();
void initwndMgr();
void initshop();
void initpack();
void initskill();
void initfly();
void initquest();
void initsafebox();
void initguild();
void initMessenger();
void initShining();
#ifdef ENABLE_GUILD_RANK_SYSTEM
void initguildranking();
#endif
#ifdef ENABLE_CONFIG_MODULE
void initcfg();
#endif
#ifdef ENABLE_RENEWAL_SWITCHBOT
void initSwitchbot();
#endif
#ifdef ENABLE_RENEWAL_CUBE
void intcuberenewal();
#endif
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
void initAcce();
#endif
#ifdef ENABLE_BIOLOG_SYSTEM
void initBiologManager();
#endif
#ifdef ENABLE_RENDER_TARGET
void initRenderTarget();
#endif
#ifdef ENABLE_INGAME_WIKI_SYSTEM
void initWiki();
#endif
#ifdef ENABLE_OFFLINESHOP_SEARCH_SYSTEM
void initprivateShopSearch();
#endif
#ifdef ENABLE_ACHIEVEMENT_SYSTEM
void InitAchievementModule();
#endif