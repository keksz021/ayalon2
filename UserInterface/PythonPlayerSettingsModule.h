#pragma once

#include <thread>

#include "PythonItem.h"
#include "PythonSkill.h"
#include "PythonPlayer.h"
#include "InstanceBase.h"
#include "GameType.h"

#include "../GameLib/RaceData.h"
#include "../GameLib/RaceMotionData.h"
#include "../GameLib/RaceManager.h"
#include "../GameLib/FlyingObjectManager.h"

#define ENABLE_RENDER_TARGET

typedef std::array<uint8_t, 3> RGB;

static const char* NPC_LIST_FILE_NAME = "npclist.txt";
static bool loaded = false;

class CPythonPlayerSettingsModule : public CSingleton<CPythonPlayerSettingsModule>
{
	public:
		CPythonPlayerSettingsModule() = default;
		virtual ~CPythonPlayerSettingsModule() = default;
		void Load();

	public:
		void RegisterSkills();
		void RegisterSkills(uint32_t race, uint8_t skill_group, uint8_t empire);

		void __RegisterCacheMotionData(CRaceData* pRaceData, const uint16_t wMotionMode, const uint16_t wMotionIndex, const char* c_szFileName, const uint8_t byPercentage = 100);
		void __SetIntroMotions(CRaceData* pRaceData, const std::string& c_rstrFolderName);
		void __SetGeneralMotions(CRaceData* pRaceData, const uint16_t wMotionMode, const std::string& c_rstrFolderName);

		void __LoadComboRace(CRaceData* pRaceData, const uint16_t wMotionMode, const std::string& c_rstrFolderName);

		void __LoadCombo(CRaceData* pRaceData, const uint16_t wMotionMode);
		void __LoadGameWarriorEx(CRaceData* pRaceData, const std::string& c_rstrFolderName);
		void __LoadGameAssassinEx(CRaceData* pRaceData, const std::string& c_rstrFolderName);
		void __LoadGameSuraEx(CRaceData* pRaceData, const std::string& c_rstrFolderName);
		void __LoadGameShamanEx(CRaceData* pRaceData, const std::string& c_rstrFolderName);
		void __LoadGameRace(CRaceData* pRaceData, const std::string& c_rstrFolderName);
		void __LoadFishing(CRaceData* pRaceData, const std::string& c_rstrFolderName);
		void __LoadWedding(CRaceData* pRaceData, const std::string& c_rstrFolderName);
		void __LoadActions(CRaceData* pRaceData, const std::string& c_rstrFolderName);
		void __LoadGuildSkill(CRaceData* pRaceData, const std::string& c_rstrFolderName);
		void __LoadWeaponMotion(CRaceData* pRaceData, WORD wMotionMode, const std::string& c_rstrFolderName);

		void __RegisterDungeonMapNames();
		void __RegisterTitleNames();
		void __RegisterNameColors();
		void __RegisterTitleColors();
		void __RegisterEmotionIcons();

#ifdef ENABLE_TITLE_SYSTEM
		void __RegisterTitleSystemNames();
		void __RegisterTitleSystemColors();
#endif

	public:
		bool RegisterGuildBuildingList(const char* filepath);
		const char* GetLocaleString(const char* key);
		int32_t GetCharacterTitleColor(uint8_t index);
#ifdef ENABLE_TITLE_SYSTEM
		int32_t GetCharacterTitleSystemColor(uint8_t index);
#endif

	private:
		enum EGuildBuildingListToken
		{
			GUIILD_BUILDING_TOKEN_VNUM,
			GUIILD_BUILDING_TOKEN_TYPE,
			GUIILD_BUILDING_TOKEN_NAME,
			GUIILD_BUILDING_TOKEN_LOCAL_NAME,
			GUIILD_BUILDING_NO_USE_TOKEN_SIZE_1,
			GUIILD_BUILDING_NO_USE_TOKEN_SIZE_2,
			GUIILD_BUILDING_NO_USE_TOKEN_SIZE_3,
			GUIILD_BUILDING_NO_USE_TOKEN_SIZE_4,
			GUIILD_BUILDING_TOKEN_X_ROT_LIMIT,
			GUIILD_BUILDING_TOKEN_Y_ROT_LIMIT,
			GUIILD_BUILDING_TOKEN_Z_ROT_LIMIT,
			GUIILD_BUILDING_TOKEN_PRICE,
			GUIILD_BUILDING_TOKEN_MATERIAL,
			GUIILD_BUILDING_TOKEN_NPC,
			GUIILD_BUILDING_TOKEN_GROUP,
			GUIILD_BUILDING_TOKEN_DEPEND_GROUP,
			GUIILD_BUILDING_TOKEN_ENABLE_FLAG,
			GUIILD_BUILDING_LIMIT_TOKEN_COUNT,
		} EGuildBuildingListToken;

		std::map<std::string_view, std::string> m_buildingTypeToFolder{
			{"HEADQUARTER", "headquarter"},
			{"FACILITY", "facility"},
			{"OBJECT", "object"},
			{"WALL", "fence"},
		};

	public:
		void __LoadGameSound();
		void __LoadGameEffects();
		void __LoadEmoticons();
		void __LoadGameInit();
		void __LoadGameNPC();

	public:
		std::thread m_loadThread;
		bool m_loadedThread = false;

		void startLoadThread()
		{
			if (!m_loadedThread)
			{
				m_loadThread = std::thread(&CPythonPlayerSettingsModule::Load, this);
				m_loadedThread = true;
			}
		}

		void joinLoadThread()
		{
			if (m_loadThread.joinable())
			{
				m_loadThread.join();
			}
		}
};

typedef std::pair<DWORD, std::string> TSoundFileName;
static const std::array<TSoundFileName, CPythonItem::USESOUND_NUM - 1> vecUseSoundFileName =
{ {
	{CPythonItem::USESOUND_DEFAULT, "sound/ui/drop.wav"},
	{CPythonItem::USESOUND_ACCESSORY, "sound/ui/equip_ring_amulet.wav"},
	{CPythonItem::USESOUND_ARMOR, "sound/ui/equip_metal_armor.wav"},
	{CPythonItem::USESOUND_BOW, "sound/ui/equip_bow.wav"},
	{CPythonItem::USESOUND_WEAPON, "sound/ui/equip_metal_weapon.wav"},
	{CPythonItem::USESOUND_POTION, "sound/ui/eat_potion.wav"},
	{CPythonItem::USESOUND_PORTAL, "sound/ui/potal_scroll.wav"},
} };

static const std::array<TSoundFileName, CPythonItem::DROPSOUND_NUM> vecDropSoundFileName =
{ {
		{CPythonItem::DROPSOUND_DEFAULT, "sound/ui/drop.wav"},
		{CPythonItem::DROPSOUND_ACCESSORY, "sound/ui/equip_ring_amulet.wav"},
		{CPythonItem::DROPSOUND_ARMOR, "sound/ui/equip_metal_armor.wav"},
		{CPythonItem::DROPSOUND_BOW, "sound/ui/equip_bow.wav"},
		{CPythonItem::DROPSOUND_WEAPON, "sound/ui/equip_metal_weapon.wav"},
} };

const constexpr uint8_t GENERAL_MOTION_NUM = 13;
typedef std::tuple<DWORD, const char*, const uint8_t> TGMotionList;
constexpr std::array<TGMotionList, GENERAL_MOTION_NUM> vecGeneralMotions =
{ {
		{CRaceMotionData::NAME_WAIT, "wait.msa", 100},
		{CRaceMotionData::NAME_WALK, "walk.msa", 100},
		{CRaceMotionData::NAME_RUN, "run.msa", 100},
		{CRaceMotionData::NAME_DAMAGE, "damage.msa", 50},
		{CRaceMotionData::NAME_DAMAGE, "damage_1.msa", 50},
		{CRaceMotionData::NAME_DAMAGE_BACK, "damage_2.msa", 50},
		{CRaceMotionData::NAME_DAMAGE_BACK, "damage_3.msa", 50},
		{CRaceMotionData::NAME_DAMAGE_FLYING, "damage_flying.msa", 100},
		{CRaceMotionData::NAME_STAND_UP, "falling_stand.msa", 100},
		{CRaceMotionData::NAME_DAMAGE_FLYING_BACK, "back_damage_flying.msa", 100},
		{CRaceMotionData::NAME_STAND_UP_BACK, "back_falling_stand.msa", 100},
		{CRaceMotionData::NAME_DEAD, "dead.msa", 100},
		{CRaceMotionData::NAME_DIG, "dig.msa", 100},
} };

const constexpr uint8_t WEDDING_MOTION_NUM = 3;
typedef std::pair<DWORD, const char*> TWeddingList;
constexpr std::array<TWeddingList, WEDDING_MOTION_NUM> vecWeddingList =
{ {
		{CRaceMotionData::NAME_WAIT, "wait.msa"},
		{CRaceMotionData::NAME_WALK, "walk.msa"},
		{CRaceMotionData::NAME_RUN, "walk.msa"},
} };

const constexpr uint8_t ACTION_MOTION_NUM = 60;
typedef std::pair<DWORD, const char*> TActionsList;
constexpr std::array<TActionsList, ACTION_MOTION_NUM> vecActionsList =
{ {
		{CRaceMotionData::NAME_CLAP, "clap.msa"},
		{CRaceMotionData::NAME_CHEERS_1, "cheers_1.msa"},
		{CRaceMotionData::NAME_CHEERS_2, "cheers_2.msa"},
		{CRaceMotionData::NAME_DANCE_1, "dance_1.msa"},
		{CRaceMotionData::NAME_DANCE_2, "dance_2.msa"},
		{CRaceMotionData::NAME_DANCE_3, "dance_3.msa"},
		{CRaceMotionData::NAME_DANCE_4, "dance_4.msa"},
		{CRaceMotionData::NAME_DANCE_5, "dance_5.msa"},
		{CRaceMotionData::NAME_DANCE_6, "dance_6.msa"},
		{CRaceMotionData::NAME_CONGRATULATION, "congratulation.msa"},
		{CRaceMotionData::NAME_DANCE_7, "dance_7.msa" },
		{CRaceMotionData::NAME_SELFIE, "selfie.msa" },
		{CRaceMotionData::NAME_DOZE, "doze.msa" },
		{CRaceMotionData::NAME_EXERCISE, "exercise.msa" },
		{CRaceMotionData::NAME_PUSHUP, "pushup.msa" },
		{CRaceMotionData::NAME_EMOTION_1, "amin.msa" },
		{CRaceMotionData::NAME_EMOTION_2, "backflip.msa" },
		{CRaceMotionData::NAME_EMOTION_3, "break.msa" },
		{CRaceMotionData::NAME_EMOTION_4, "cocalar.msa" },
		{CRaceMotionData::NAME_EMOTION_5, "dab.msa" },
		{CRaceMotionData::NAME_EMOTION_6, "facepalm.msa" },
		{CRaceMotionData::NAME_EMOTION_7, "fuckyou.msa" },
		{CRaceMotionData::NAME_EMOTION_8, "laba.msa" },
		{CRaceMotionData::NAME_EMOTION_9, "oriental.msa" },
		{CRaceMotionData::NAME_EMOTION_10, "ridack_animoney.msa" },
		{CRaceMotionData::NAME_EMOTION_11, "pingu.msa" },
		{CRaceMotionData::NAME_EMOTION_12, "twerk.msa" },
		{CRaceMotionData::NAME_EMOTION_13, "samba.msa" },
		{CRaceMotionData::NAME_EMOTION_14, "shimmy.msa" },
		{CRaceMotionData::NAME_EMOTION_15, "slow.msa" },
		{CRaceMotionData::NAME_EMOTION_16, "bestmates.msa" },
		{CRaceMotionData::NAME_EMOTION_17, "flair.msa" },
		{CRaceMotionData::NAME_EMOTION_18, "horon.msa" },
		{CRaceMotionData::NAME_EMOTION_19, "robotdance.msa" },
		{CRaceMotionData::NAME_EMOTION_20, "snowball.msa" },
		{CRaceMotionData::NAME_EMOTION_21, "surprised2.msa" },
		{CRaceMotionData::NAME_FORGIVE, "forgive.msa"},
		{CRaceMotionData::NAME_ANGRY, "angry.msa"},
		{CRaceMotionData::NAME_ATTRACTIVE, "attractive.msa"},
		{CRaceMotionData::NAME_SAD, "sad.msa"},
		{CRaceMotionData::NAME_SHY, "shy.msa"},
		{CRaceMotionData::NAME_CHEERUP, "cheerup.msa"},
		{CRaceMotionData::NAME_BANTER, "banter.msa"},
		{CRaceMotionData::NAME_JOY, "joy.msa"},
		{CRaceMotionData::NAME_FRENCH_KISS_WITH_WARRIOR, "french_kiss_with_warrior.msa"},
		{CRaceMotionData::NAME_FRENCH_KISS_WITH_ASSASSIN, "french_kiss_with_assassin.msa"},
		{CRaceMotionData::NAME_FRENCH_KISS_WITH_SURA, "french_kiss_with_sura.msa"},
		{CRaceMotionData::NAME_FRENCH_KISS_WITH_SHAMAN, "french_kiss_with_shaman.msa"},
		{CRaceMotionData::NAME_KISS_WITH_WARRIOR, "kiss_with_warrior.msa"},
		{CRaceMotionData::NAME_KISS_WITH_ASSASSIN, "kiss_with_assassin.msa"},
		{CRaceMotionData::NAME_KISS_WITH_SURA, "kiss_with_sura.msa"},
		{CRaceMotionData::NAME_KISS_WITH_SHAMAN, "kiss_with_shaman.msa"},
		{CRaceMotionData::NAME_SLAP_HIT_WITH_WARRIOR, "slap_hit.msa"},
		{CRaceMotionData::NAME_SLAP_HIT_WITH_ASSASSIN, "slap_hit.msa"},
		{CRaceMotionData::NAME_SLAP_HIT_WITH_SURA, "slap_hit.msa"},
		{CRaceMotionData::NAME_SLAP_HIT_WITH_SHAMAN, "slap_hit.msa"},
		{CRaceMotionData::NAME_SLAP_HURT_WITH_WARRIOR, "slap_hurt.msa"},
		{CRaceMotionData::NAME_SLAP_HURT_WITH_ASSASSIN, "slap_hurt.msa"},
		{CRaceMotionData::NAME_SLAP_HURT_WITH_SURA, "slap_hurt.msa"},
		{CRaceMotionData::NAME_SLAP_HURT_WITH_SHAMAN, "slap_hurt.msa"},
} };

typedef std::tuple<uint32_t, const char*, const char*, bool> TEffectList;
static const std::vector<TEffectList> vecEffectList =
{
	{ CInstanceBase::EFFECT_SPAWN_APPEAR, "Bip01", "d:/ymir work/effect/etc/appear_die/monster_appear.mse", true},
	{ CInstanceBase::EFFECT_SPAWN_DISAPPEAR, "Bip01", "d:/ymir work/effect/etc/appear_die/monster_die.mse", true},
	{ CInstanceBase::EFFECT_FLAME_ATTACK, "equip_right_hand", "d:/ymir work/effect/hit/blow_flame/flame_3_weapon.mse", true},
	{ CInstanceBase::EFFECT_FLAME_HIT, "", "d:/ymir work/effect/hit/blow_flame/flame_3_blow.mse", true},
	{ CInstanceBase::EFFECT_FLAME_ATTACH, "", "d:/ymir work/effect/hit/blow_flame/flame_3_body.mse", true},
	{ CInstanceBase::EFFECT_ELECTRIC_ATTACK, "equip_right", "d:/ymir work/effect/hit/blow_electric/light_1_weapon.mse", true},
	{ CInstanceBase::EFFECT_ELECTRIC_HIT, "", "d:/ymir work/effect/hit/blow_electric/light_1_blow.mse", true},
	{ CInstanceBase::EFFECT_ELECTRIC_ATTACH, "", "d:/ymir work/effect/hit/blow_electric/light_1_body.mse", true},
#ifdef ENABLE_BOSS_EFFECT_OVER_HEAD
	{ CInstanceBase::EFFECT_BOSS, "", "d:/ymir work/effect/etc/boss_effect/boss.mse", true},
#endif
#ifdef EFFECT_METIN_KOBRA
	{ CInstanceBase::EFFECT_METIN, "", "d:/ymir work/effect/etc/metin_effect/boss.mse", true},
#endif
	{ CInstanceBase::EFFECT_LEVELUP, "", "d:/ymir work/effect/etc/levelup_1/level_up.mse", true},
	{ CInstanceBase::EFFECT_SKILLUP, "", "d:/ymir work/effect/etc/skillup/skillup_1.mse", true},

	{ CInstanceBase::EFFECT_EMPIRE + 1, "Bip01", "d:/ymir work/effect/etc/empire/empire_A.mse", true},
	{ CInstanceBase::EFFECT_EMPIRE + 2, "Bip01", "d:/ymir work/effect/etc/empire/empire_B.mse", true},
	{ CInstanceBase::EFFECT_EMPIRE + 3, "Bip01", "d:/ymir work/effect/etc/empire/empire_C.mse", true},

	{ CInstanceBase::EFFECT_WEAPON + 1, "equip_right_hand", "d:/ymir work/pc/warrior/effect/geom_sword_loop.mse", true},
	{ CInstanceBase::EFFECT_WEAPON + 2, "equip_right_hand", "d:/ymir work/pc/warrior/effect/geom_spear_loop.mse", true},

#ifndef ENABLE_RENEWAL_TEAM_AFFECT
	{ CInstanceBase::EFFECT_AFFECT + 0, "Bip01", "d:/ymir work/ui/game/staff_effect/gm/gm.mse", true},
#endif

	{ CInstanceBase::EFFECT_AFFECT + 3, "Bip01", "d:/ymir work/effect/hit/blow_poison/poison_loop.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 4, "", "d:/ymir work/effect/affect/slow.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 5, "Bip01 Head", "d:/ymir work/effect/etc/stun/stun_loop.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 6, "", "d:/ymir work/effect/etc/ready/ready.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 16, "", "d:/ymir work/pc/warrior/effect/gyeokgongjang_loop.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 17, "", "d:/ymir work/pc/assassin/effect/gyeonggong_loop.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 19, "Bip01 R Finger2", "d:/ymir work/pc/sura/effect/gwigeom_loop.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 20, "", "d:/ymir work/pc/sura/effect/fear_loop.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 21, "", "d:/ymir work/pc/sura/effect/jumagap_loop.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 22, "", "d:/ymir work/pc/shaman/effect/3hosin_loop.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 23, "", "d:/ymir work/pc/shaman/effect/boho_loop.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 24, "", "d:/ymir work/pc/shaman/effect/10kwaesok_loop.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 25, "", "d:/ymir work/pc/sura/effect/heuksin_loop.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 26, "", "d:/ymir work/pc/sura/effect/muyeong_loop.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 28, "Bip01", "d:/ymir work/effect/hit/blow_flame/flame_loop.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 29, "Bip01 R Hand", "d:/ymir work/pc/shaman/effect/6gicheon_hand.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 30, "Bip01 L Hand", "d:/ymir work/pc/shaman/effect/jeungryeok_hand.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 32, "Bip01 Head", "d:/ymir work/pc/sura/effect/pabeop_loop.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 33, "", "d:/ymir work/pc/warrior/effect/gyeokgongjang_loop.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 35, "", "d:/ymir work/effect/etc/guild_war_flag/flag_red.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 36, "", "d:/ymir work/effect/etc/guild_war_flag/flag_blue.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 37, "", "d:/ymir work/effect/etc/guild_war_flag/flag_yellow.mse", true},

#ifdef ENABLE_RENEWAL_TEAM_AFFECT
	{ CInstanceBase::EFFECT_AFFECT + 42, "Bip01", "d:/ymir work/ui/game/staff_effect/sa/sa.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 43, "Bip01", "d:/ymir work/ui/game/staff_effect/ga/ga.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 44, "Bip01", "d:/ymir work/ui/game/staff_effect/gm/gm.mse", true},
	{ CInstanceBase::EFFECT_AFFECT + 45, "Bip01", "d:/ymir work/ui/game/staff_effect/tgm/tgm.mse", true},
#endif

	{ CInstanceBase::EFFECT_REFINED + 1, "PART_WEAPON", "D:/ymir work/pc/common/effect/sword/sword_7.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 2, "PART_WEAPON", "D:/ymir work/pc/common/effect/sword/sword_8.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 3, "PART_WEAPON", "D:/ymir work/pc/common/effect/sword/sword_9.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 4, "PART_WEAPON_LEFT", "D:/ymir work/pc/common/effect/sword/sword_7_b.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 5, "PART_WEAPON_LEFT", "D:/ymir work/pc/common/effect/sword/sword_8_b.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 6, "PART_WEAPON_LEFT", "D:/ymir work/pc/common/effect/sword/sword_9_b.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 7, "PART_WEAPON", "D:/ymir work/pc/common/effect/sword/sword_7_f.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 8, "PART_WEAPON", "D:/ymir work/pc/common/effect/sword/sword_8_f.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 9, "PART_WEAPON", "D:/ymir work/pc/common/effect/sword/sword_9_f.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 10, "PART_WEAPON", "D:/ymir work/pc/common/effect/sword/sword_7_s.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 11, "PART_WEAPON", "D:/ymir work/pc/common/effect/sword/sword_8_s.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 12, "PART_WEAPON", "D:/ymir work/pc/common/effect/sword/sword_9_s.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 13, "PART_WEAPON_LEFT", "D:/ymir work/pc/common/effect/sword/sword_7_s.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 14, "PART_WEAPON_LEFT", "D:/ymir work/pc/common/effect/sword/sword_8_s.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 15, "PART_WEAPON_LEFT", "D:/ymir work/pc/common/effect/sword/sword_9_s.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 16, "Bip01", "D:/ymir work/pc/common/effect/armor/armor_7.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 17, "Bip01", "D:/ymir work/pc/common/effect/armor/armor_8.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 18, "Bip01", "D:/ymir work/pc/common/effect/armor/armor_9.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 19, "Bip01", "D:/ymir work/pc/common/effect/armor/armor-4-2-1.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 20, "Bip01", "D:/ymir work/pc/common/effect/armor/armor-4-2-2.mse", true},
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	{ CInstanceBase::EFFECT_REFINED + 21, "Bip01", "d:/ymir work/pc/common/effect/armor/acc_01.mse", true},
	//{ CInstanceBase::EFFECT_REFINED + 23, "Bip01 Spine2", "D:/ymir work/asenis/wing/1/wing.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 23, "Bip01", "D:/ymir work/item/wing/plechito/astral_wings/astral_wings.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 24, "Bip01", "D:/ymir work/item/wing/plechito/android_wing/dark_wing.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 25, "Bip01", "D:/ymir work/item/wing/plechito/android_wing/gold_wing.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 26, "Bip01", "D:/ymir work/item/wing/plechito/astral_wings/astral_wings.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 27, "Bip01 Spine2", "D:/ymir work/asenis/wing/5/wing.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 28, "Bip01 Spine2", "D:/ymir work/asenis/wing/6/wing.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 29, "Bip01 Spine2", "D:/ymir work/asenis/wing/7/wing.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 30, "Bip01 Spine2", "D:/ymir work/asenis/wing/8/wing.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 31, "Bip01 Spine2", "D:/ymir work/asenis/wing/9/wing.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 32, "Bip01 Spine2", "D:/ymir work/asenis/wing/10/wing.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 33, "Bip01 Spine2", "D:/ymir work/asenis/wing/11/wing.mse", true},

#endif
	

#ifdef ENABLE_MDE_EFFECT
/*{CInstanceBase::EFFECT_REFINED + 22, "PART_WEAPON", "D:/ymir work/effect/sword/fms.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 23, "PART_WEAPON", "D:/ymir work/effect/sword/tryton.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 24, "PART_WEAPON", "D:/ymir work/effect/sword/zatruty2.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 25, "PART_WEAPON", "D:/ymir work/effect/sword/lwi.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 26, "PART_WEAPON", "D:/ymir work/effect/sword/brzegowe.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 27, "PART_WEAPON", "D:/ymir work/effect/sword/swiety.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 28, "PART_WEAPON", "D:/ymir work/effect/sword/rib.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 29, "PART_WEAPON", "D:/ymir work/effect/sword/zal.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 30, "PART_WEAPON_LEFT", "D:/ymir work/effect/sword/jelonek.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 31, "PART_WEAPON_LEFT", "D:/ymir work/effect/sword/kruk.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 32, "PART_WEAPON_LEFT", "D:/ymir work/effect/sword/diabla_l.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 33, "PART_WEAPON", "D:/ymir work/effect/sword/antyk.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 34, "PART_WEAPON", "D:/ymir work/effect/sword/bambus.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 35, "PART_WEAPON", "D:/ymir work/effect/sword/szczeki.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 36, "PART_WEAPON", "D:/ymir work/effect/sword/kozik.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 37, "PART_WEAPON_LEFT", "D:/ymir work/effect/sword/kozik.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 38, "PART_WEAPON", "D:/ymir work/effect/sword/skrzydla.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 39, "PART_WEAPON_LEFT", "D:/ymir work/effect/sword/skrzydla.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 40, "PART_WEAPON", "D:/ymir work/effect/sword/bezduszne.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 41, "PART_WEAPON_LEFT", "D:/ymir work/effect/sword/bezduszne.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 42, "PART_WEAPON", "D:/ymir work/effect/sword/jesion.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 43, "PART_WEAPON_LEFT", "D:/ymir work/effect/sword/jesion.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 44, "PART_WEAPON", "D:/ymir work/effect/sword/diabla_w.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 45, "PART_WEAPON_LEFT", "D:/ymir work/effect/sword/diabla_w.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 46, "PART_WEAPON", "D:/ymir work/effect/sword/piekielne.mse", true},
	{ CInstanceBase::EFFECT_REFINED + 101, "Bip01", "D:/ymir work/effektek/asenis/wing/11/wing.mse", true },*/
#endif

	{ CInstanceBase::EFFECT_DUST, "", "d:/ymir work/effect/etc/dust/dust.mse", true},
	{ CInstanceBase::EFFECT_HORSE_DUST, "", "d:/ymir work/effect/etc/dust/running_dust.mse", true},
	{ CInstanceBase::EFFECT_HIT, "", "d:/ymir work/effect/hit/blow_1/blow_1_low.mse", true},
	{ CInstanceBase::EFFECT_HPUP_RED, "", "d:/ymir work/effect/etc/recuperation/drugup_red.mse", true},
	{ CInstanceBase::EFFECT_SPUP_BLUE, "", "d:/ymir work/effect/etc/recuperation/drugup_blue.mse", true},
	{ CInstanceBase::EFFECT_SPEEDUP_GREEN, "", "d:/ymir work/effect/etc/recuperation/drugup_green.mse", true},
	{ CInstanceBase::EFFECT_DXUP_PURPLE, "", "d:/ymir work/effect/etc/recuperation/drugup_purple.mse", true},
	{ CInstanceBase::EFFECT_AUTO_HPUP, "", "d:/ymir work/effect/etc/recuperation/autodrugup_red.mse", true},
	{ CInstanceBase::EFFECT_AUTO_SPUP, "", "d:/ymir work/effect/etc/recuperation/autodrugup_blue.mse", true},
	{ CInstanceBase::EFFECT_RAMADAN_RING_EQUIP, "", "d:/ymir work/effect/etc/buff/buff_item1.mse", true},
	{ CInstanceBase::EFFECT_HALLOWEEN_CANDY_EQUIP, "", "d:/ymir work/effect/etc/buff/buff_item2.mse", true},
	{ CInstanceBase::EFFECT_HAPPINESS_RING_EQUIP, "", "d:/ymir work/effect/etc/buff/buff_item3.mse", true},
	{ CInstanceBase::EFFECT_LOVE_PENDANT_EQUIP, "", "d:/ymir work/effect/etc/buff/buff_item4.mse", true},
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	{ CInstanceBase::EFFECT_ACCE_SUCCEDED, "", "d:/ymir work/effect/etc/buff/buff_item6.mse", true},
	{ CInstanceBase::EFFECT_ACCE_EQUIP, "", "d:/ymir work/effect/etc/buff/buff_item7.mse", true},
#endif
	{ CInstanceBase::EFFECT_PENETRATE, "Bip01", "d:/ymir work/effect/hit/gwantong.mse", true},
	{ CInstanceBase::EFFECT_FIRECRACKER, "", "d:/ymir work/effect/etc/firecracker/newyear_firecracker.mse", true},
	{ CInstanceBase::EFFECT_SPIN_TOP, "", "d:/ymir work/effect/etc/firecracker/paing_i.mse", true},
	{ CInstanceBase::EFFECT_SELECT, "", "d:/ymir work/effect/etc/click/click_select.mse", true},
	{ CInstanceBase::EFFECT_TARGET, "", "d:/ymir work/effect/etc/click/click_glow_select.mse", true},
	{ CInstanceBase::EFFECT_STUN, "Bip01 Head", "d:/ymir work/effect/etc/stun/stun.mse", true},
	{ CInstanceBase::EFFECT_CRITICAL, "Bip01 R Hand", "d:/ymir work/effect/hit/critical.mse", true},
	{ CInstanceBase::EFFECT_DAMAGE_TARGET, "", "d:/ymir work/effect/affect/damagevalue/target.mse", true},
	{ CInstanceBase::EFFECT_DAMAGE_NOT_TARGET, "", "d:/ymir work/effect/affect/damagevalue/nontarget.mse", true},
	{ CInstanceBase::EFFECT_DAMAGE_SELFDAMAGE, "", "d:/ymir work/effect/affect/damagevalue/damage.mse", true},
	{ CInstanceBase::EFFECT_DAMAGE_SELFDAMAGE2, "", "d:/ymir work/effect/affect/damagevalue/damage_1.mse", true},
	{ CInstanceBase::EFFECT_DAMAGE_POISON, "", "d:/ymir work/effect/affect/damagevalue/poison.mse", true},
	{ CInstanceBase::EFFECT_DAMAGE_MISS, "", "d:/ymir work/effect/affect/damagevalue/miss.mse", true},
	{ CInstanceBase::EFFECT_DAMAGE_TARGETMISS, "", "d:/ymir work/effect/affect/damagevalue/target_miss.mse", true},
	{ CInstanceBase::EFFECT_PERCENT_DAMAGE1, "", "d:/ymir work/effect/hit/percent_damage1.mse", true},
	{ CInstanceBase::EFFECT_PERCENT_DAMAGE2, "", "d:/ymir work/effect/hit/percent_damage2.mse", true},
	{ CInstanceBase::EFFECT_PERCENT_DAMAGE3, "", "d:/ymir work/effect/hit/percent_damage3.mse", true},
#ifdef ENABLE_RENEWAL_BATTLE_PASS
	{ CInstanceBase::EFFECT_BP_NORMAL_MISSION_COMPLETED, "", "d:/ymir work/effect/battlepass/normal_mission_complete.mse", true},
	{ CInstanceBase::EFFECT_BP_PREMIUM_MISSION_COMPLETED, "", "d:/ymir work/effect/battlepass/premium_mission_complete.mse", true},
	{ CInstanceBase::EFFECT_BP_NORMAL_BATTLEPASS_COMPLETED, "", "d:/ymir work/effect/battlepass/normal_battlepass_complete.mse", true},
	{ CInstanceBase::EFFECT_BP_PREMIUM_BATTLEPASS_COMPLETED, "", "d:/ymir work/effect/battlepass/premium_battlepass_complete.mse", true},
#endif
#ifdef ENABLE_GROWTH_PET_SYSTEM
	{ CInstanceBase::EFFECT_GYEONGGONG_BOOM, "", "d:/ymir work/effect/hit/gyeonggong_boom.mse", true},
#endif
#ifdef ENABLE_METIN_QUEUE
	{ CInstanceBase::EFFECT_METIN_QUEUE, "", "d:/ymir work/effect/ivan/metin_loop_effect/queue_effect_purple.mse", true },
#endif
};

const constexpr uint8_t INTRO_MOTION_NUM = 3;
typedef std::pair<DWORD, const char*> TIMotionList;
constexpr std::array<TIMotionList, INTRO_MOTION_NUM> vecIntroMotions =
{ {
		{CRaceMotionData::NAME_INTRO_WAIT, "wait.msa"},
		{CRaceMotionData::NAME_INTRO_SELECTED, "selected.msa"},
		{CRaceMotionData::NAME_INTRO_NOT_SELECTED, "not_selected.msa"},
} };

const constexpr uint8_t FISHING_MOTION_NUM = 8;
typedef std::pair<DWORD, const char*> TFishing;
constexpr std::array<TFishing, FISHING_MOTION_NUM> vecFishing =
{ {
		{CRaceMotionData::NAME_WAIT, "wait.msa"},
		{CRaceMotionData::NAME_WALK, "walk.msa"},
		{CRaceMotionData::NAME_FISHING_THROW, "throw.msa"},
		{CRaceMotionData::NAME_FISHING_WAIT, "fishing_wait.msa"},
		{CRaceMotionData::NAME_FISHING_STOP, "fishing_cancel.msa"},
		{CRaceMotionData::NAME_FISHING_REACT, "fishing_react.msa"},
		{CRaceMotionData::NAME_FISHING_CATCH, "fishing_catch.msa"},
		{CRaceMotionData::NAME_FISHING_FAIL, "fishing_fail.msa"},
} };

const constexpr uint8_t GUILD_SKILL_MOTION_NUM = 6;
typedef std::pair<DWORD, const char*> TGuildSkill;
constexpr std::array<TGuildSkill, GUILD_SKILL_MOTION_NUM> vecGuildSkill =
{ {
		{CRaceMotionData::NAME_SKILL + 107, "guild_yongsinuipi.msa"},
		{CRaceMotionData::NAME_SKILL + 108, "guild_yongsinuichukbok.msa"},
		{CRaceMotionData::NAME_SKILL + 109, "guild_seonghwigap.msa"},
		{CRaceMotionData::NAME_SKILL + 110, "guild_gasokhwa.msa"},
		{CRaceMotionData::NAME_SKILL + 111, "guild_yongsinuibunno.msa"},
		{CRaceMotionData::NAME_SKILL + 112, "guild_jumunsul.msa"},
} };

typedef std::pair<DWORD, const char*> TRaceData;
constexpr std::array<TRaceData, NPlayerData::MAIN_RACE_MAX_NUM> vecRaceData =
{ {
		{NPlayerData::MAIN_RACE_WARRIOR_M, "d:/ymir work/pc/warrior/"},
		{NPlayerData::MAIN_RACE_WARRIOR_W, "d:/ymir work/pc2/warrior/"},
		{NPlayerData::MAIN_RACE_ASSASSIN_W, "d:/ymir work/pc/assassin/"},
		{NPlayerData::MAIN_RACE_ASSASSIN_M, "d:/ymir work/pc2/assassin/"},
		{NPlayerData::MAIN_RACE_SURA_M, "d:/ymir work/pc/sura/"},
		{NPlayerData::MAIN_RACE_SURA_W, "d:/ymir work/pc2/sura/"},
		{NPlayerData::MAIN_RACE_SHAMAN_W, "d:/ymir work/pc/shaman/"},
		{NPlayerData::MAIN_RACE_SHAMAN_M, "d:/ymir work/pc2/shaman/"},
} };

enum ComboType
{
	COMBO_TYPE_1,
	COMBO_TYPE_2,
	COMBO_TYPE_3,
};

enum ComboIndex
{
	COMBO_INDEX_1,
	COMBO_INDEX_2,
	COMBO_INDEX_3,
	COMBO_INDEX_4,
	COMBO_INDEX_5,
	COMBO_INDEX_6,
};

constexpr uint8_t GUILD_MATERIAL_NUM = 3;

enum EGuildMaterialItems
{
	MATERIAL_STONE_ID = 90010,
	MATERIAL_LOG_ID,
	MATERIAL_PLYWOOD_ID,
};

static const uint8_t getGuildMaterialIndex(const int32_t& vnum)
{
	switch (vnum)
	{
		case MATERIAL_STONE_ID:
		case MATERIAL_LOG_ID:
		case MATERIAL_PLYWOOD_ID:
			return vnum - 90010;
	}

	return -1;
}

static const std::array<const char*, 46> titleNameList{
	"PVP_LEVEL0","PVP_LEVEL1","PVP_LEVEL2","PVP_LEVEL3","PVP_LEVEL4","PVP_LEVEL5","PVP_LEVEL6","PVP_LEVEL7","PVP_LEVEL8","PVP_LEVEL9","PVP_LEVEL10",
	"PVP_LEVEL11","PVP_LEVEL12","PVP_LEVEL13","PVP_LEVEL14","PVP_LEVEL15","PVP_LEVEL16","PVP_LEVEL17","PVP_LEVEL18","PVP_LEVEL19","PVP_LEVEL20","PVP_LEVEL21","PVP_LEVEL22",
	"PVP_LEVEL23","PVP_LEVEL24","PVP_LEVEL25","PVP_LEVEL26","PVP_LEVEL27","PVP_LEVEL28","PVP_LEVEL29","PVP_LEVEL30","PVP_LEVEL31","PVP_LEVEL32","PVP_LEVEL33","PVP_LEVEL34","PVP_LEVEL35",
	"PVP_LEVEL36","PVP_LEVEL37","PVP_LEVEL38","PVP_LEVEL39","PVP_LEVEL40","PVP_LEVEL41","PVP_LEVEL42","PVP_LEVEL43","PVP_LEVEL44","PVP_LEVEL45"
};

#ifdef ENABLE_TITLE_SYSTEM
static const std::array<const char*, 21> titleSystemNameList{ 
	"TITLE_SYSTEM_NAME_0", "TITLE_SYSTEM_NAME_1","TITLE_SYSTEM_NAME_2", "TITLE_SYSTEM_NAME_3", "TITLE_SYSTEM_NAME_4", "TITLE_SYSTEM_NAME_5", "TITLE_SYSTEM_NAME_6",
	"TITLE_SYSTEM_NAME_7",  "TITLE_SYSTEM_NAME_8", "TITLE_SYSTEM_NAME_9", "TITLE_SYSTEM_NAME_10", "TITLE_SYSTEM_NAME_11", "TITLE_SYSTEM_NAME_12", "TITLE_SYSTEM_NAME_13",
	"TITLE_SYSTEM_NAME_14", "TITLE_SYSTEM_NAME_15","TITLE_SYSTEM_NAME_16", "TITLE_SYSTEM_NAME_17", "TITLE_SYSTEM_NAME_18", "TITLE_SYSTEM_NAME_19", "TITLE_SYSTEM_NAME_20" };
#endif

typedef std::map<uint8_t, RGB> TCharacterColorMap;
static const TCharacterColorMap characterNameColors
{
	{ CInstanceBase::NAMECOLOR_NORMAL_PC, { 255, 215, 76 }},
	{ CInstanceBase::NAMECOLOR_NORMAL_NPC, { 122, 231, 93 }},
	{ CInstanceBase::NAMECOLOR_NORMAL_MOB, { 235, 22, 9 }},
	{ CInstanceBase::NAMECOLOR_PVP, { 238, 54, 223 }},
	{ CInstanceBase::NAMECOLOR_PK, { 180, 100, 0 }},
	{ CInstanceBase::NAMECOLOR_PARTY, { 128, 192, 255 }},
	{ CInstanceBase::NAMECOLOR_WARP, { 136, 218, 241 }},
	{ CInstanceBase::NAMECOLOR_WAYPOINT, { 255, 255, 255 }},
	{ CInstanceBase::NAMECOLOR_EMPIRE_MOB, { 235, 22, 9 }},
	{ CInstanceBase::NAMECOLOR_EMPIRE_NPC, { 122, 231, 93 }},
	{ CInstanceBase::NAMECOLOR_EMPIRE_PC + 1, { 157, 0, 0 }},
	{ CInstanceBase::NAMECOLOR_EMPIRE_PC + 2, { 222, 160, 47 }},
	{ CInstanceBase::NAMECOLOR_EMPIRE_PC + 3, { 23, 30, 138 }},
#ifdef ENABLE_METIN_STONES_MINIMAP
	{ CInstanceBase::NAMECOLOR_METIN, { 240, 255, 255 }},
#endif
#ifdef ENABLE_RENEWAL_OFFLINESHOP
	{ CInstanceBase::NAMECOLOR_SHOP, { 255, 215, 76 }},
#endif
};

static const TCharacterColorMap characterTitleColors
{
	{ 0, { 218,165,85 }},
	{ 1, { 0,255, 153 }},
	{ 2, { 128, 0, 128 }},
	{ 3, { 255, 20, 147 }},
	{ 4, { 193, 255, 193}},
	{ 5, { 218,165,85 }},
	{ 6, { 6,153,51 }},
	{ 7, {  0, 204, 255}},
	{ 8, {  0, 144, 255}},
	{ 9, { 92, 110, 255}},
	{ 10, { 155, 155, 255}},
	{ 11, { 0, 255, 153 }},
	{ 12, { 218,165,85 }},
	{ 13, { 193, 255, 193 }},
	{ 14, { 193, 255, 193}},
	{ 15, { 218,165,85 }},
	{ 16, { 6,153,51 }},
	{ 17, {  0, 204, 255}},
	{ 18, {  0, 144, 255}},
	{ 19, { 92, 110, 255}},
	{ 20, {155, 155, 255}},
	{ 21, { 0, 255, 153 }},
	{ 22, { 218,165,85 }},
	{ 23, { 193, 255, 193 }},
	{ 24, { 193, 255, 193}},
	{ 25, { 218,165,85 }},
	{ 26, { 6,153,51 }},
	{ 27, {  0, 204, 255}},
	{ 28, {  0, 144, 255}},
	{ 29, { 92, 110, 255}},
	{ 30, {155, 155, 255}},
	{ 31, { 0, 255, 153 }},
	{ 32, { 218,165,85 }},
	{ 33, { 193, 255, 193 }},
	{ 34, { 193, 255, 193}},
	{ 35, { 218,165,85 }},
	{ 36, { 6,153,51 }},
	{ 37, {  0, 204, 255 }},
	{ 38, {  0, 144, 255 }},
	{ 39, { 92, 110, 255 }},
	{ 40, {155, 155, 255 }},
	{ 41, {255, 255, 255 }},
	{ 42, {207, 117,   0 }},
	{ 43, {235,  83,   0 }},
	{ 44, {227,   0,   0 }},
	{ 45, {255,   0,   0}},

};

#ifdef ENABLE_TITLE_SYSTEM
static const TCharacterColorMap characterTitleSystemColors
{
	{ 0, { 255, 230, 186 }},
	{ 1, { 255, 230, 186 }},
	{ 2, { 255, 230, 186 }},
	{ 3, { 255, 230, 186 }},
	{ 4, { 255, 230, 186 }},
	{ 5, { 255, 230, 186 }},
	{ 6, { 255, 230, 186 }},
	{ 7, { 255, 230, 186 }},
	{ 8, { 255, 230, 186 }},
	{ 9, { 255, 230, 186 }},
	{ 10, { 255, 230, 186 }},
	{ 11, { 255, 230, 186 }},
	{ 12, { 255, 230, 186 }},
	{ 13, { 255, 230, 186 }},
	{ 14, { 255, 230, 186 }},
	{ 15, { 255, 230, 186 }},
	{ 16, { 255, 230, 186 }},
	{ 17, { 255, 230, 186 }},
	{ 18, { 255, 230, 186 }},
	{ 19, { 255, 230, 186 }},
	{ 20, { 255, 230, 186 }},
};
#endif

static const std::map<uint8_t, const char*> emotionIcons =
{
	{ NPlayerData::EMOTION_CLAP, "d:/ymir work/ui/game/windows/emotion_clap.sub" },
	{ NPlayerData::EMOTION_CHEERS_1, "d:/ymir work/ui/game/windows/emotion_cheers_1.sub" },
	{ NPlayerData::EMOTION_CHEERS_2, "d:/ymir work/ui/game/windows/emotion_cheers_2.sub" },
	{ NPlayerData::EMOTION_DANCE_1, "icon/action/dance1.tga" },
	{ NPlayerData::EMOTION_DANCE_2, "icon/action/dance2.tga" },
	{ NPlayerData::EMOTION_CONGRATULATION, "icon/action/congratulation.tga" },
	{ NPlayerData::EMOTION_FORGIVE, "icon/action/forgive.tga" },
	{ NPlayerData::EMOTION_ANGRY, "icon/action/angry.tga" },
	{ NPlayerData::EMOTION_ATTRACTIVE, "icon/action/attractive.tga" },
	{ NPlayerData::EMOTION_SAD, "icon/action/sad.tga" },
	{ NPlayerData::EMOTION_SHY, "icon/action/shy.tga" },
	{ NPlayerData::EMOTION_CHEERUP, "icon/action/cheerup.tga" },
	{ NPlayerData::EMOTION_BANTER, "icon/action/banter.tga" },
	{ NPlayerData::EMOTION_JOY, "icon/action/joy.tga" },
	{ NPlayerData::EMOTION_DANCE_1, "icon/action/dance1.tga" },
	{ NPlayerData::EMOTION_DANCE_2, "icon/action/dance2.tga" },
	{ NPlayerData::EMOTION_DANCE_3, "icon/action/dance3.tga" },
	{ NPlayerData::EMOTION_DANCE_4, "icon/action/dance4.tga" },
	{ NPlayerData::EMOTION_DANCE_5, "icon/action/dance5.tga" },
	{ NPlayerData::EMOTION_DANCE_6, "icon/action/dance6.tga" },
	{ NPlayerData::EMOTION_DANCE_7, "icon/action/dance7.tga" },
	{ NPlayerData::EMOTION_SELFIE, "icon/action/selfie.tga" },
	{ NPlayerData::EMOTION_DOZE, "icon/action/doze.tga" },
	{ NPlayerData::EMOTION_EXERCISE, "icon/action/exercise.tga" },
	{ NPlayerData::EMOTION_PUSHUP, "icon/action/pushup.tga" },
	{ NPlayerData::EMOTION_EMOTION_1, "icon/action/emotion_1.tga" },
	{ NPlayerData::EMOTION_EMOTION_2, "icon/action/emotion_2.tga" },
	{ NPlayerData::EMOTION_EMOTION_3, "icon/action/emotion_3.tga" },
	{ NPlayerData::EMOTION_EMOTION_4, "icon/action/emotion_4.tga" },
	{ NPlayerData::EMOTION_EMOTION_5, "icon/action/emotion_5.tga" },
	{ NPlayerData::EMOTION_EMOTION_6, "icon/action/emotion_6.tga" },
	{ NPlayerData::EMOTION_EMOTION_7, "icon/action/emotion_7.tga" },
	{ NPlayerData::EMOTION_EMOTION_8, "icon/action/emotion_8.tga" },
	{ NPlayerData::EMOTION_EMOTION_9, "icon/action/emotion_9.tga" },
	{ NPlayerData::EMOTION_EMOTION_10, "icon/action/emotion_10.tga" },
	{ NPlayerData::EMOTION_EMOTION_11, "icon/action/emotion_11.tga" },
	{ NPlayerData::EMOTION_EMOTION_12, "icon/action/emotion_12.tga" },
	{ NPlayerData::EMOTION_EMOTION_13, "icon/action/bani.tga" },
	{ NPlayerData::EMOTION_EMOTION_14, "icon/action/bani.tga" },
	{ NPlayerData::EMOTION_EMOTION_15, "icon/action/bani.tga" },
	{ NPlayerData::EMOTION_EMOTION_16, "icon/action/bestmates.tga" },
	{ NPlayerData::EMOTION_EMOTION_17, "icon/action/flair.tga" },
	{ NPlayerData::EMOTION_EMOTION_18, "icon/action/horon.tga" },
	{ NPlayerData::EMOTION_EMOTION_19, "icon/action/robotdance.tga" },
	{ NPlayerData::EMOTION_EMOTION_20, "icon/action/snowball.tga" },
	{ NPlayerData::EMOTION_EMOTION_21, "icon/action/surprised2.tga" },
	{ NPlayerData::EMOTION_KISS, "d:/ymir work/ui/game/windows/emotion_kiss.sub" },
	{ NPlayerData::EMOTION_FRENCH_KISS, "d:/ymir work/ui/game/windows/emotion_french_kiss.sub" },
	{ NPlayerData::EMOTION_SLAP, "d:/ymir work/ui/game/windows/emotion_slap.sub" },
};

static const std::map<uint8_t, std::map<uint8_t, std::vector<uint8_t>>> skillIndexes{
	{ NRaceData::JOB_WARRIOR, {
		{1, {1, 2, 3, 4, 5, 0, 0, 0, 137, 0, 138, 0, 139, 0}},
		{2, {16, 17, 18, 19, 20, 0, 0, 0, 137, 0, 138, 0, 139, 0}},
	}},
	{ NRaceData::JOB_ASSASSIN, {
		{1, {31, 32, 33, 34, 35, 0, 0, 0, 137, 0, 138, 0, 139, 0, 140}},
		{2, {46, 47, 48, 49, 50, 0, 0, 0, 137, 0, 138, 0, 139, 0, 140}},
	}},
	{ NRaceData::JOB_SURA, {
		{1, {61, 62, 63, 64, 65, 66, 0, 0, 137, 0, 138, 0, 139, 0}},
		{2, {76, 77, 78, 79, 80, 81, 0, 0, 137, 0, 138, 0, 139, 0}},
	}},
	{ NRaceData::JOB_SHAMAN, {
		{1, {91, 92, 93, 94, 95, 96, 0, 0, 137, 0, 138, 0, 139, 0}},
		{2, {106, 107, 108, 109, 110, 111, 0, 0, 137, 0, 138, 0, 139, 0}},
	}},
};


constexpr std::array<uint8_t, 12> supportSkillIndexes =
{
	{122, 123, 121, 124, 125, 129, 130, 131, 137, 138, 139, 140},
};

static const std::map < std::string, std::vector<uint8_t>> guildSkills
{
	{ "PASSIVE", { 151 }},
	{ "ACTIVE", { 152, 153, 154, 155, 156, 157 }},
};
