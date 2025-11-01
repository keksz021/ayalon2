#include "StdAfx.h"
#include "InstanceBase.h"
#include "PythonBackground.h"
#include "PythonNonPlayer.h"
#include "PythonPlayer.h"
#include "PythonCharacterManager.h"
#include "AbstractPlayer.h"
#include "AbstractApplication.h"
#include "packet.h"

#include "../eterlib/StateManager.h"
#include "../gamelib/ItemManager.h"
#include "../gamelib/RaceManager.h"
#define SHADOW_TEXTURE_MULTIPLIER 2

#ifdef ENABLE_GRAPHIC_ON_OFF
#include "PythonSystem.h"
#include "PythonTextTail.h"
#endif


#ifdef ENABLE_CONFIG_MODULE
#include "PythonConfig.h"
#endif

#ifdef ENABLE_HAIR_SPECULAR
#include <unordered_map>
#endif

#include "boost/algorithm/string.hpp"

std::map<int, char*> shiningdata;
std::map<int, char*>::iterator shiningit;

BOOL HAIR_COLOR_ENABLE = FALSE;
BOOL USE_ARMOR_SPECULAR = FALSE;
BOOL RIDE_HORSE_ENABLE = TRUE;
const float c_fDefaultRotationSpeed = 1200.0f;
const float c_fDefaultHorseRotationSpeed = 1500.0f;

bool IsWall(unsigned race)
{
	switch (race)
	{
	case 14201:
	case 14202:
	case 14203:
	case 14204:
		return true;
		break;
	}
	return false;
}

CInstanceBase::SHORSE::SHORSE()
{
	__Initialize();
}

CInstanceBase::SHORSE::~SHORSE()
{
	assert(m_pkActor == NULL);
}

void CInstanceBase::SHORSE::__Initialize()
{
	m_isMounting = false;
	m_pkActor = NULL;
}

void CInstanceBase::SHORSE::SetAttackSpeed(UINT uAtkSpd)
{
	if (!IsMounting())
		return;

	CActorInstance& rkActor = GetActorRef();
	rkActor.SetAttackSpeed(uAtkSpd / 100.0f);
}

void CInstanceBase::SHORSE::SetMoveSpeed(UINT uMovSpd)
{
	if (!IsMounting())
		return;

	CActorInstance& rkActor = GetActorRef();
	rkActor.SetMoveSpeed(uMovSpd / 100.0f);
}

void CInstanceBase::SHORSE::Create(const TPixelPosition& c_rkPPos, UINT eRace, UINT eHitEffect)
{
	assert(NULL == m_pkActor && "CInstanceBase::SHORSE::Create - ALREADY MOUNT");

	m_pkActor = new CActorInstance;

	CActorInstance& rkActor = GetActorRef();
	rkActor.SetEventHandler(CActorInstance::IEventHandler::GetEmptyPtr());
	if (!rkActor.SetRace(eRace))
	{
		delete m_pkActor;
		m_pkActor = NULL;
		return;
	}

	rkActor.SetShape(0,1.0f);
	rkActor.SetBattleHitEffect(eHitEffect);
	rkActor.SetAlphaValue(0.0f);
	rkActor.BlendAlphaValue(1.0f, 0.5f);
	rkActor.SetMoveSpeed(1.0f);
	rkActor.SetAttackSpeed(1.0f);
	rkActor.SetMotionMode(CRaceMotionData::MODE_GENERAL);
	rkActor.Stop();
	rkActor.RefreshActorInstance();

	rkActor.SetCurPixelPosition(c_rkPPos);

	m_isMounting = true;
}

void CInstanceBase::SHORSE::Destroy()
{
	if (m_pkActor)
	{
		m_pkActor->Destroy();
		delete m_pkActor;
	}

	__Initialize();
}

CActorInstance& CInstanceBase::SHORSE::GetActorRef()
{
	assert(NULL != m_pkActor && "CInstanceBase::SHORSE::GetActorRef");
	return *m_pkActor;
}

CActorInstance* CInstanceBase::SHORSE::GetActorPtr()
{
	return m_pkActor;
}

bool CInstanceBase::SHORSE::IsNewMount()
{
	if (!m_pkActor)
		return false;

	return false;
}

bool CInstanceBase::SHORSE::CanUseSkill()
{
	return true;
}

bool CInstanceBase::SHORSE::CanAttack()
{
	return true;
}

bool CInstanceBase::SHORSE::IsMounting()
{
	return m_isMounting;
}

void CInstanceBase::SHORSE::Deform()
{
	if (!IsMounting())
		return;

	CActorInstance& rkActor = GetActorRef();
	rkActor.INSTANCEBASE_Deform();
}

void CInstanceBase::SHORSE::Render()
{
	if (!IsMounting())
		return;

	CActorInstance& rkActor = GetActorRef();
	rkActor.Render();
}

void CInstanceBase::__AttachHorseSaddle()
{
	if (!IsMountingHorse())
		return;
	m_kHorse.m_pkActor->AttachModelInstance(CRaceData::PART_MAIN, "saddle", m_GraphicThingInstance, CRaceData::PART_MAIN);
}

void CInstanceBase::__DetachHorseSaddle()
{
	if (!IsMountingHorse())
		return;
	m_kHorse.m_pkActor->DetachModelInstance(CRaceData::PART_MAIN, m_GraphicThingInstance, CRaceData::PART_MAIN);
}

void CInstanceBase::BlockMovement()
{
	m_GraphicThingInstance.BlockMovement();
}

bool CInstanceBase::IsBlockObject(const CGraphicObjectInstance& c_rkBGObj)
{
	return m_GraphicThingInstance.IsBlockObject(c_rkBGObj);
}

bool CInstanceBase::AvoidObject(const CGraphicObjectInstance& c_rkBGObj)
{
	return m_GraphicThingInstance.AvoidObject(c_rkBGObj);
}

bool __ArmorVnumToShape(int iVnum, DWORD* pdwShape)
{
	*pdwShape = iVnum;

	if (0 == iVnum || 1 == iVnum)
		return false;

	if (!USE_ARMOR_SPECULAR)
		return false;

	CItemData* pItemData;
	if (!CItemManager::Instance().GetItemDataPointer(iVnum, &pItemData))
		return false;

	enum
	{
		SHAPE_VALUE_SLOT_INDEX = 3,
	};

	*pdwShape = pItemData->GetValue(SHAPE_VALUE_SLOT_INDEX);

	return true;
}

class CActorInstanceBackground : public IBackground
{
public:
	CActorInstanceBackground() {}
	virtual ~CActorInstanceBackground() {}
	bool IsBlock(int x, int y)
	{
		CPythonBackground& rkBG = CPythonBackground::Instance();
		return rkBG.isAttrOn(x, y, CTerrainImpl::ATTRIBUTE_BLOCK);
	}
};

static CActorInstanceBackground gs_kActorInstBG;

bool CInstanceBase::LessRenderOrder(CInstanceBase* pkInst)
{
	int nMainAlpha = (__GetAlphaValue() < 1.0f) ? 1 : 0;
	int nTestAlpha = (pkInst->__GetAlphaValue() < 1.0f) ? 1 : 0;
	if (nMainAlpha < nTestAlpha)
		return true;
	if (nMainAlpha > nTestAlpha)
		return false;

	if (GetRace() < pkInst->GetRace())
		return true;
	if (GetRace() > pkInst->GetRace())
		return false;

	if (GetShape() < pkInst->GetShape())
		return true;

	if (GetShape() > pkInst->GetShape())
		return false;

	UINT uLeftLODLevel = __LessRenderOrder_GetLODLevel();
	UINT uRightLODLevel = pkInst->__LessRenderOrder_GetLODLevel();
	if (uLeftLODLevel < uRightLODLevel)
		return true;
	if (uLeftLODLevel > uRightLODLevel)
		return false;

	if (m_awPart[CRaceData::PART_WEAPON] < pkInst->m_awPart[CRaceData::PART_WEAPON])
		return true;

	return false;
}

UINT CInstanceBase::__LessRenderOrder_GetLODLevel()
{
	CGrannyLODController* pLODCtrl = m_GraphicThingInstance.GetLODControllerPointer(0);
	if (!pLODCtrl)
		return 0;

	return pLODCtrl->GetLODLevel();
}

bool CInstanceBase::__Background_GetWaterHeight(const TPixelPosition& c_rkPPos, float* pfHeight)
{
	long lHeight;
	if (!CPythonBackground::Instance().GetWaterHeight(int(c_rkPPos.x), int(c_rkPPos.y), &lHeight))
		return false;

	*pfHeight = float(lHeight);

	return true;
}

bool CInstanceBase::__Background_IsWaterPixelPosition(const TPixelPosition& c_rkPPos)
{
	return CPythonBackground::Instance().isAttrOn(c_rkPPos.x, c_rkPPos.y, CTerrainImpl::ATTRIBUTE_WATER);
}

const float PC_DUST_RANGE = 2000.0f;
const float NPC_DUST_RANGE = 1000.0f;

DWORD CInstanceBase::ms_dwUpdateCounter = 0;
DWORD CInstanceBase::ms_dwRenderCounter = 0;
DWORD CInstanceBase::ms_dwDeformCounter = 0;

CDynamicPool<CInstanceBase> CInstanceBase::ms_kPool;

bool CInstanceBase::__IsInDustRange()
{
	if (!__IsExistMainInstance())
		return false;

	CInstanceBase* pkInstMain = __GetMainInstancePtr();

	float fDistance = NEW_GetDistanceFromDestInstance(*pkInstMain);

	if (IsPC())
	{
		if (fDistance <= PC_DUST_RANGE)
			return true;
	}

	if (fDistance <= NPC_DUST_RANGE)
		return true;

	return false;
}

void CInstanceBase::__EnableSkipCollision()
{
	if (__IsMainInstance())
	{
		TraceError("CInstanceBase::__EnableSkipCollision - ŔÚ˝ĹŔş Ăćµą°Ë»ç˝şĹµŔĚ µÇ¸é ľČµČ´Ů!!");
		return;
	}
	m_GraphicThingInstance.EnableSkipCollision();
}

void CInstanceBase::__DisableSkipCollision()
{
	m_GraphicThingInstance.DisableSkipCollision();
}

//DWORD CInstanceBase::__GetShadowMapColor(float x, float y)
//{
//	CPythonBackground& rkBG = CPythonBackground::Instance();
//	return rkBG.GetShadowMapColor(x, y);
//}

float CInstanceBase::__GetBackgroundHeight(float x, float y)
{
	CPythonBackground& rkBG = CPythonBackground::Instance();
	return rkBG.GetHeight(x, y);
}

BOOL CInstanceBase::IsInvisibility()
{
#ifdef ENABLE_CANSEEHIDDENTHING_FOR_GM
	if (IsAffect(AFFECT_INVISIBILITY) && !__MainCanSeeHiddenThing())
		return true;
#else
	if (IsAffect(AFFECT_INVISIBILITY))
		return true;
#endif

	return false;
}

BOOL CInstanceBase::IsParalysis()
{
	return m_GraphicThingInstance.IsParalysis();
}

BOOL CInstanceBase::IsGameMaster()
{
	if (m_kAffectFlagContainer.IsSet(AFFECT_YMIR))
		return true;

#ifdef ENABLE_RENEWAL_TEAM_AFFECT
	if (m_kAffectFlagContainer.IsSet(AFFECT_TEAM_SA))
		return true;

	if (m_kAffectFlagContainer.IsSet(AFFECT_TEAM_GA))
		return true;

	if (m_kAffectFlagContainer.IsSet(AFFECT_TEAM_GM))
		return true;

	if (m_kAffectFlagContainer.IsSet(AFFECT_TEAM_TGM))
		return true;
#endif
	return false;
}

#ifdef ENABLE_RENEWAL_PREMIUM_SYSTEM
BOOL CInstanceBase::IsPremium()
{
	if (m_kAffectFlagContainer.IsSet(AFFECT_PREMIUM))
		return true;
	return false;
}
#endif

BOOL CInstanceBase::IsSameEmpire(CInstanceBase& rkInstDst)
{
	if (0 == rkInstDst.m_dwEmpireID)
		return TRUE;

	if (IsGameMaster())
		return TRUE;

	if (rkInstDst.IsGameMaster())
		return TRUE;

	if (rkInstDst.m_dwEmpireID == m_dwEmpireID)
		return TRUE;

	return FALSE;
}

DWORD CInstanceBase::GetEmpireID()
{
	return m_dwEmpireID;
}

#ifdef ENABLE_SHOW_MOB_INFO
DWORD CInstanceBase::GetAIFlag()
{
	return m_dwAIFlag;
}
#endif

DWORD CInstanceBase::GetGuildID() const
{
	return m_dwGuildID;
}

#ifdef ENABLE_GUILD_LEADER_TEXTAIL
BYTE CInstanceBase::GetGuildLeader() const
{
	return m_dwGuildLeader;
}
#endif

#ifdef ENABLE_SKILL_COLOR_SYSTEM
DWORD* CInstanceBase::GetSkillColor(DWORD dwSkillIndex)
{
	DWORD dwSkillSlot = dwSkillIndex + 1;
	CPythonSkill::SSkillData* c_pSkillData;
	if (!CPythonSkill::Instance().GetSkillData(dwSkillSlot, &c_pSkillData))
		return 0;

	WORD dwEffectID = c_pSkillData->GradeData[CPythonSkill::SKILL_GRADE_COUNT].wMotionIndex - CRaceMotionData::NAME_SKILL - (1 * 25);
	return m_GraphicThingInstance.GetSkillColorByMotionID(dwEffectID);
}
#endif

int CInstanceBase::GetAlignment()
{
	return m_sAlignment;
}

UINT CInstanceBase::GetAlignmentGrade() const
{
#ifdef ENABLE_ALIGN_RENEWAL
	if (m_sAlignment >= 3000000)
		return 0;
	else if (m_sAlignment >= 2900000)
		return 1;
	else if (m_sAlignment >= 2800000)
		return 2;
	else if (m_sAlignment >= 2700000)
		return 3;
	else if (m_sAlignment >= 2600000)
		return 4;
	else if (m_sAlignment >= 2500000)
		return 5;
	else if (m_sAlignment >= 2400000)
		return 6;
	else if (m_sAlignment >= 2300000)
		return 7;
	else if (m_sAlignment >= 2200000)
		return 8;
	else if (m_sAlignment >= 2100000)
		return 9;
	else if (m_sAlignment >= 2000000)
		return 10;
	else if (m_sAlignment >= 1900000)
		return 11;
	else if (m_sAlignment >= 1800000)
		return 12;
	else if (m_sAlignment >= 1700000)
		return 13;
	else if (m_sAlignment >= 1600000)
		return 14;
	else if (m_sAlignment >= 1500000)
		return 15;
	else if (m_sAlignment >= 1400000)
		return 16;
	else if (m_sAlignment >= 1300000)
		return 17;
	else if (m_sAlignment >= 1200000)
		return 18;
	else if (m_sAlignment >= 1100000)
		return 19;
	else if (m_sAlignment >= 1000000)
		return 20;
	else if (m_sAlignment >= 900000)
		return 21;
	else if (m_sAlignment >= 800000)
		return 22;
	else if (m_sAlignment >= 700000)
		return 23;
	else if (m_sAlignment >= 600000)
		return 24;
	else if (m_sAlignment >= 500000)
		return 25;
	else if (m_sAlignment >= 400000)
		return 26;
	else if (m_sAlignment >= 300000)
		return 27;
	else if (m_sAlignment >= 200000)
		return 28;
	else if (m_sAlignment >= 100000)
		return 29;
	else if (m_sAlignment >= 90000)
		return 30;
	else if (m_sAlignment >= 80000)
		return 31;
	else if (m_sAlignment >= 70000)
		return 32;
	else if (m_sAlignment >= 60000)
		return 33;
	else if (m_sAlignment >= 50000)
		return 34;
	else if (m_sAlignment >= 40000)
		return 35;
	else if (m_sAlignment >= 32000)
		return 36;
	else if (m_sAlignment >= 24000)
		return 37;
	else if (m_sAlignment >= 16000)
		return 38;
	else if (m_sAlignment >= 8000)
		return 39;
	else if (m_sAlignment >= 1000)
		return 40;
	else if (m_sAlignment >= 0)
		return 41;
	else if (m_sAlignment > -4000)
		return 42;
	else if (m_sAlignment > -8000)
		return 43;
	else if (m_sAlignment > -12000)
		return 44;

	return 45;
#else
	if (m_sAlignment >= 12000)
		return 0;
	else if (m_sAlignment >= 8000)
		return 1;
	else if (m_sAlignment >= 4000)
		return 2;
	else if (m_sAlignment >= 1000)
		return 3;
	else if (m_sAlignment >= 0)
		return 4;
	else if (m_sAlignment > -4000)
		return 5;
	else if (m_sAlignment > -8000)
		return 6;
	else if (m_sAlignment > -12000)
		return 7;

	return 8;
#endif
}

int CInstanceBase::GetAlignmentType()
{
#ifdef ENABLE_ALIGN_RENEWAL
	switch (GetAlignmentGrade())
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
	case 26:
	case 27:
	case 28:
	case 29:
	case 30:
	case 31:
	case 32:
	case 33:
	case 34:
	case 35:
	case 36:
	case 37:
	case 38:
	case 39:
	case 40:
	{
		return ALIGNMENT_TYPE_WHITE;
		break;
	}

	case 42:
	case 43:
	case 44:
	case 45:
	{
		return ALIGNMENT_TYPE_DARK;
		break;
	}
	}

	return ALIGNMENT_TYPE_NORMAL;
#else
	switch (GetAlignmentGrade())
	{
	case 0:
	case 1:
	case 2:
	case 3:
	{
		return ALIGNMENT_TYPE_WHITE;
		break;
	}

	case 5:
	case 6:
	case 7:
	case 8:
	{
		return ALIGNMENT_TYPE_DARK;
		break;
	}
	}

	return ALIGNMENT_TYPE_NORMAL;
#endif
}

#ifdef ENABLE_TITLE_SYSTEM
int CInstanceBase::GetTitleID()
{
	return m_iTitleID;
}
#endif

BYTE CInstanceBase::GetPKMode()
{
	return m_byPKMode;
}

bool CInstanceBase::IsKiller()
{
	return m_isKiller;
}

bool CInstanceBase::IsPartyMember()
{
	return m_isPartyMember;
}

BOOL CInstanceBase::IsInSafe()
{
	const TPixelPosition& c_rkPPosCur = m_GraphicThingInstance.NEW_GetCurPixelPositionRef();
	if (CPythonBackground::Instance().isAttrOn(c_rkPPosCur.x, c_rkPPosCur.y, CTerrainImpl::ATTRIBUTE_BANPK))
		return TRUE;

	return FALSE;
}

float CInstanceBase::CalculateDistanceSq3d(const TPixelPosition& c_rkPPosDst)
{
	const TPixelPosition& c_rkPPosSrc = m_GraphicThingInstance.NEW_GetCurPixelPositionRef();
	return SPixelPosition_CalculateDistanceSq3d(c_rkPPosSrc, c_rkPPosDst);
}

void CInstanceBase::OnSelected()
{
	if (IsStoneDoor())
		return;

	if (IsDead())
		return;

#ifdef ENABLE_INTROSELECT_EFFECTS
	if (!m_GraphicThingInstance.IsLoginRender())
		__AttachSelectEffect();
#else
	__AttachSelectEffect();
#endif
}

void CInstanceBase::OnUnselected()
{
	__DetachSelectEffect();
}

void CInstanceBase::OnTargeted()
{
	if (IsStoneDoor())
		return;

	if (IsDead())
		return;

	__AttachTargetEffect();
}

void CInstanceBase::OnUntargeted()
{
	__DetachTargetEffect();
}

void CInstanceBase::DestroySystem()
{
	ms_kPool.Clear();
}

void CInstanceBase::CreateSystem(UINT uCapacity)
{
	ms_kPool.Create(uCapacity);

	memset(ms_adwCRCAffectEffect, 0, sizeof(ms_adwCRCAffectEffect));

	ms_fDustGap = 250.0f;
	ms_fHorseDustGap = 500.0f;
}

CInstanceBase* CInstanceBase::New()
{
	return ms_kPool.Alloc();
}

void CInstanceBase::Delete(CInstanceBase* pkInst)
{
	pkInst->Destroy();
	ms_kPool.Free(pkInst);
}

void CInstanceBase::SetMainInstance()
{
	CPythonCharacterManager& rkChrMgr = CPythonCharacterManager::Instance();

	DWORD dwVID = GetVirtualID();
	rkChrMgr.SetMainInstance(dwVID);

	m_GraphicThingInstance.SetMainInstance();
}

CInstanceBase* CInstanceBase::__GetMainInstancePtr()
{
	CPythonCharacterManager& rkChrMgr = CPythonCharacterManager::Instance();
	return rkChrMgr.GetMainInstancePtr();
}

void CInstanceBase::__ClearMainInstance()
{
	CPythonCharacterManager& rkChrMgr = CPythonCharacterManager::Instance();
	rkChrMgr.ClearMainInstance();
}

/* ˝ÇÁ¦ ÇĂ·ąŔĚľî Äł¸ŻĹÍŔÎÁö Á¶»ç.*/
bool CInstanceBase::__IsMainInstance()
{
	if (this == __GetMainInstancePtr())
		return true;

	return false;
}

bool CInstanceBase::__IsExistMainInstance()
{
	if (__GetMainInstancePtr())
		return true;
	else
		return false;
}

bool CInstanceBase::__MainCanSeeHiddenThing()
{
#ifdef ENABLE_CANSEEHIDDENTHING_FOR_GM
	CInstanceBase* pInstance = __GetMainInstancePtr();
	return (pInstance) ? TRUE == pInstance->IsGameMaster() : false;
#else
	return false;
#endif
}

float CInstanceBase::__GetBowRange()
{
	float fRange = 2500.0f - 100.0f;

	if (__IsMainInstance())
	{
		IAbstractPlayer& rPlayer = IAbstractPlayer::GetSingleton();
		fRange += float(rPlayer.GetStatus(POINT_BOW_DISTANCE));
	}

	return fRange;
}

CInstanceBase* CInstanceBase::__FindInstancePtr(DWORD dwVID)
{
	CPythonCharacterManager& rkChrMgr = CPythonCharacterManager::Instance();
	return rkChrMgr.GetInstancePtr(dwVID);
}

bool CInstanceBase::__FindRaceType(DWORD dwRace, BYTE* pbType)
{
	CPythonNonPlayer& rkNonPlayer = CPythonNonPlayer::Instance();
	return rkNonPlayer.GetInstanceType(dwRace, pbType);
}

bool CInstanceBase::Create(const SCreateData& c_rkCreateData)
{
	IAbstractApplication::GetSingleton().SkipRenderBuffering(300);

	SetInstanceType(c_rkCreateData.m_bType);


	if (!SetRace(c_rkCreateData.m_dwRace))
		return false;

	SetVirtualID(c_rkCreateData.m_dwVID);

	if (c_rkCreateData.m_isMain)
		SetMainInstance();

	if (IsGuildWall())
	{
		unsigned center_x;
		unsigned center_y;

		c_rkCreateData.m_kAffectFlags.ConvertToPosition(&center_x, &center_y);

		float center_z = __GetBackgroundHeight(center_x, center_y);
		NEW_SetPixelPosition(TPixelPosition(float(c_rkCreateData.m_lPosX), float(c_rkCreateData.m_lPosY), center_z));
	}
	else
	{
		SCRIPT_SetPixelPosition(float(c_rkCreateData.m_lPosX), float(c_rkCreateData.m_lPosY));
	}

	if (0 != c_rkCreateData.m_dwMountVnum)
		MountHorse(c_rkCreateData.m_dwMountVnum);

	SetArmor(c_rkCreateData.m_dwArmor);

	if (IsPC())
	{
		SetHair(c_rkCreateData.m_dwHair);
		SetWeapon(c_rkCreateData.m_dwWeapon);
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		SetAcce(c_rkCreateData.m_dwAcce);
#endif
#ifdef ENABLE_AURA_COSTUME_SYSTEM
		SetAura(c_rkCreateData.m_dwAura);
#endif
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
		SetLanguage(c_rkCreateData.m_bLanguage);
		SetLanguage2(c_rkCreateData.m_bLanguage2);
#endif
#ifdef ENABLE_SKILL_COLOR_SYSTEM
		ChangeSkillColor(*c_rkCreateData.m_dwSkillColor);
		memcpy(m_dwSkillColor, *c_rkCreateData.m_dwSkillColor, sizeof(m_dwSkillColor));
#endif
	}

	__Create_SetName(c_rkCreateData);

	m_dwLevel = c_rkCreateData.m_dwLevel;
#ifdef ENABLE_SHOW_MOB_INFO
	m_dwAIFlag = c_rkCreateData.m_dwAIFlag;
#endif
	m_dwGuildID = c_rkCreateData.m_dwGuildID;
	m_dwEmpireID = c_rkCreateData.m_dwEmpireID;
#ifdef ENABLE_GUILD_LEADER_TEXTAIL
	m_dwGuildLeader = c_rkCreateData.m_dwGuildLeader;
#endif
	SetVirtualNumber(c_rkCreateData.m_dwRace);
	SetRotation(c_rkCreateData.m_fRot);
#ifdef ENABLE_ACHIEVEMENT_SYSTEM
	SetAchievementTitle(c_rkCreateData.m_dwAchievementTitle);
#endif
	SetAlignment(c_rkCreateData.m_sAlignment);
#ifdef ENABLE_TITLE_SYSTEM
	SetTitleSystem(c_rkCreateData.m_iTitleID);
#endif
	SetPKMode(c_rkCreateData.m_byPKMode);

	SetMoveSpeed(c_rkCreateData.m_dwMovSpd);
	SetAttackSpeed(c_rkCreateData.m_dwAtkSpd);

#ifdef ENABLE_GROWTH_PET_SYSTEM
	m_bCharacterSize = c_rkCreateData.m_bCharacterSize;
	if (IsGrowthPet())
		SetPetLevel(c_rkCreateData.m_dwLevel);
#endif

#ifdef ENABLE_MOB_SCALE
	float fx, fy, fz = 1.0f;

	if (CPythonNonPlayer::Instance().GetScale(c_rkCreateData.m_dwRace, fx, fy, fz)
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		&& !c_rkCreateData.m_dwAcce
#endif
#ifdef ENABLE_AURA_COSTUME_SYSTEM
		&& !c_rkCreateData.m_dwAura
#endif
		)
		m_GraphicThingInstance.SetScale(fx, fy, fz, true);
#endif

	if (!IsWearingDress())
	{
		m_GraphicThingInstance.SetAlphaValue(0.0f);
		m_GraphicThingInstance.BlendAlphaValue(1.0f, 0.5f);
	}

	if (!IsGuildWall())
	{
		SetAffectFlagContainer(c_rkCreateData.m_kAffectFlags);
	}

	if (c_rkCreateData.m_dwStateFlags & ADD_CHARACTER_STATE_SPAWN)
	{
		if (IsAffect(AFFECT_SPAWN))
			__AttachEffect(EFFECT_SPAWN_APPEAR);

		if (IsPC())
		{
			Refresh(CRaceMotionData::NAME_WAIT, true);
		}
		else
		{
			Refresh(CRaceMotionData::NAME_SPAWN, false);
		}
	}
	else
	{
		Refresh(CRaceMotionData::NAME_WAIT, true);
	}

	__AttachEmpireEffect(c_rkCreateData.m_dwEmpireID);

	RegisterBoundingSphere();

	AttachTextTail();
	RefreshTextTail();

#ifdef ENABLE_STONE_SCALE_OPTION
	RefreshStoneScale();
#endif

#ifdef ENABLE_BOSS_EFFECT_OVER_HEAD
	DWORD dwRank = CPythonNonPlayer::Instance().GetMonsterRank(c_rkCreateData.m_dwRace);
	DWORD dwType = CPythonNonPlayer::Instance().GetMonsterType(c_rkCreateData.m_dwRace);

	if (dwRank >= 4 && dwType == 0)
		__AttachBossEffect();
#endif
#ifdef EFFECT_METIN_KOBRA
	DWORD dwRace = c_rkCreateData.m_dwRace;
	if (CPythonNonPlayer::Instance().IsMetinStoneWithEffect(dwRace))
		__AttachMetinEffect();
#endif
	if (c_rkCreateData.m_dwStateFlags & ADD_CHARACTER_STATE_DEAD)
		m_GraphicThingInstance.DieEnd();

	SetStateFlags(c_rkCreateData.m_dwStateFlags);

	m_GraphicThingInstance.SetBattleHitEffect(ms_adwCRCAffectEffect[EFFECT_HIT]);

	if (!IsPC())
	{
		DWORD dwBodyColor = CPythonNonPlayer::Instance().GetMonsterColor(c_rkCreateData.m_dwRace);
		if (0 != dwBodyColor)
		{
			SetModulateRenderMode();
			SetAddColor(dwBodyColor);
		}
	}

	__AttachHorseSaddle();

	const int c_iGuildSymbolRace = 14200;
	if (c_iGuildSymbolRace == GetRace())
	{
		std::string strFileName = GetGuildSymbolFileName(m_dwGuildID);
		if (IsFile(strFileName.c_str()))
			m_GraphicThingInstance.ChangeMaterial(strFileName.c_str());
	}

#ifdef ENABLE_CANSEEHIDDENTHING_FOR_GM
	if (IsAffect(AFFECT_INVISIBILITY) && __MainCanSeeHiddenThing())
		m_GraphicThingInstance.BlendAlphaValue(0.1f, 0.1f);
#endif
#ifdef WJ_ATTACH_SPECULAR_FROM_LIST
	struct SAttachSpecularData
	{
		DWORD race; float specularPower;
	};

	const int SPECULAR_MAX_NUM = 99999;
	static SAttachSpecularData structInfo[SPECULAR_MAX_NUM] =
	{
		{ 101, 0.4f },
		{ 491, 0.4f },
		{ 492, 0.4f },
		{ 493, 0.4f },
		{ 494, 0.4f },
		{ 591, 0.4f },
		{ 691, 0.4f },
		{ 692, 0.4f },
		{ 693, 0.4f },
		{ 791, 0.4f },
		{ 792, 0.4f },
		{ 793, 0.4f },
		{ 794, 0.4f },
		{ 795, 0.4f },
		{ 591, 0.4f },
		{ 592, 0.4f },
		{ 593, 0.4f },
		{ 594, 0.4f },
		{ 595, 0.4f },
		{ 1304, 0.4f },
		{ 1306, 0.4f },
		{ 1307, 0.4f },
		{ 1308, 0.4f },
		{ 1309, 0.4f },
		{ 1305, 0.4f },
		{ 1334, 0.4f },
		{ 1901, 0.4f },
		{ 1902, 0.4f },
		{ 1903, 0.4f },
		{ 1904, 0.4f },
		{ 1905, 0.4f },
		{ 1906, 0.4f },
		{ 2091, 0.4f },
		{ 2092, 0.4f },
		{ 2093, 0.4f },
		{ 2094, 0.4f },
		{ 2191, 0.4f },
		{ 2192, 0.4f },
		{ 2206, 0.4f },
		{ 2207, 0.4f },
		{ 2291, 0.4f },
		{ 2306, 0.4f },
		{ 2307, 0.4f },
		{ 2491, 0.4f },
		{ 2492, 0.4f },
		{ 2494, 0.4f },
		{ 2495, 0.4f },
		{ 2591, 0.4f },
		{ 2592, 0.4f },
		{ 2593, 0.4f },
		{ 2594, 0.4f },
		{ 2595, 0.4f },
		{ 2596, 0.4f },
		{ 2597, 0.4f },
		{ 2598, 0.4f },
		{ 3090, 0.4f },
		{ 3091, 0.4f },
		{ 3190, 0.4f },
		{ 3191, 0.4f },
		{ 3290, 0.4f },
		{ 3291, 0.4f },
		{ 3390, 0.4f },
		{ 3391, 0.4f },
		{ 3490, 0.4f },
		{ 3491, 0.4f },
		{ 3590, 0.4f },
		{ 3591, 0.4f },
		{ 3595, 0.4f },
		{ 3596, 0.4f },
		{ 3690, 0.4f },
		{ 3691, 0.4f },
		{ 3790, 0.4f },
		{ 3791, 0.4f },
		{ 3890, 0.4f },
		{ 3891, 0.4f },
		{ 3960, 0.4f },
		{ 3961, 0.4f },
		{ 3962, 0.4f },
		{ 3963, 0.4f },
		{ 3964, 0.4f },
		{ 5161, 0.4f },
		{ 5162, 0.4f },
		{ 5163, 0.4f },
		{ 6005, 0.4f },
		{ 6009, 0.4f },
		{ 6051, 0.4f },
		{ 6059, 0.4f },
		{ 6116, 0.4f },
		{ 6151, 0.4f },
		{ 6193, 0.4f },
		{ 6207, 0.4f },
		{ 6391, 0.4f },
		{ 6392, 0.4f },
		{ 6407, 0.4f },
		{ 6408, 0.4f },
		{ 6415, 0.4f },
		{ 6416, 0.4f },
		{ 6417, 0.4f },
		{ 6418, 0.4f },
		{ 6419, 0.4f },
		{ 6430, 0.4f },
		{ 6431, 0.4f },
		{ 6432, 0.4f },
		{ 6433, 0.4f },
		{ 6434, 0.4f },
		{ 6435, 0.4f },
		{ 6436, 0.4f },
		{ 6437, 0.4f },
		{ 6438, 0.4f },
		{ 6500, 0.4f },
		{ 6501, 0.4f },
		{ 6502, 0.4f },
		{ 6503, 0.4f },
		{ 6504, 0.4f },
		{ 9002, 0.4f },
		{ 9003, 0.4f },
		{ 9005, 0.4f },
		{ 9008, 0.4f },
		{ 9012, 0.4f },
		{ 9013, 0.4f },
		{ 9014, 0.4f },
		{ 20007, 0.4f },
		{ 20009, 0.4f },
		{ 20015, 0.4f },
		{ 20016, 0.4f },
		{ 20047, 0.4f },
		{ 20048, 0.4f },
		{ 20049, 0.4f },
		{ 20050, 0.4f },
		{ 20051, 0.4f },
		{ 20052, 0.4f },
		{ 20053, 0.4f },
		{ 20054, 0.4f },
		{ 20055, 0.4f },
		{ 20056, 0.4f },
		{ 20057, 0.4f },
		{ 20058, 0.4f },
		{ 20059, 0.4f },
		{ 20091, 0.4f },
		{ 20054, 0.4f },
		{ 20055, 0.4f },
		{ 20056, 0.4f },
		{ 20057, 0.4f },
		{ 20058, 0.4f },
		{ 20059, 0.4f },
		{ 20105, 0.4f },
		{ 20111, 0.4f },
		{ 20112, 0.4f },
		{ 20113, 0.4f },
		{ 20114, 0.4f },
		{ 20115, 0.4f },
		{ 20116, 0.4f },
		{ 20117, 0.4f },
		{ 20118, 0.4f },
		{ 20119, 0.4f },
		{ 20120, 0.4f },
		{ 20121, 0.4f },
		{ 20122, 0.4f },
		{ 20123, 0.4f },
		{ 20124, 0.4f },
		{ 20125, 0.4f },
		{ 20201, 0.4f },
		{ 20202, 0.4f },
		{ 20203, 0.4f },
		{ 20204, 0.4f },
		{ 20205, 0.4f },
		{ 20206, 0.4f },
		{ 20207, 0.4f },
		{ 20208, 0.4f },
		{ 20209, 0.4f },
		{ 20205, 0.4f },
		{ 20211, 0.4f },
		{ 20212, 0.4f },
		{ 20213, 0.4f },
		{ 20214, 0.4f },
		{ 20215, 0.4f },
		{ 20216, 0.4f },
		{ 20217, 0.4f },
		{ 20218, 0.4f },
		{ 20219, 0.4f },
		{ 20220, 0.4f },
		{ 20221, 0.4f },
		{ 20222, 0.4f },
		{ 20223, 0.4f },
		{ 20224, 0.4f },
		{ 20225, 0.4f },
		{ 20226, 0.4f },
		{ 20227, 0.4f },
		{ 20228, 0.4f },
		{ 20229, 0.4f },
		{ 20230, 0.4f },
		{ 20231, 0.4f },
		{ 20232, 0.4f },
		{ 20233, 0.4f },
		{ 20234, 0.4f },
		{ 20235, 0.4f },
		{ 20236, 0.4f },
		{ 20237, 0.4f },
		{ 20238, 0.4f },
		{ 20239, 0.4f },
		{ 20240, 0.4f },
		{ 20241, 0.4f },
		{ 20242, 0.4f },
		{ 20243, 0.4f },
		{ 20244, 0.4f },
		{ 20245, 0.4f },
		{ 20246, 0.4f },
		{ 20247, 0.4f },
		{ 20248, 0.4f },
		{ 20249, 0.4f },
		{ 20250, 0.4f },
		{ 20251, 0.4f },
		{ 20252, 0.4f },
		{ 20254, 0.4f },
		{ 20255, 0.4f },
		{ 20257, 0.4f },
		{ 20258, 0.4f },
		{ 20259, 0.4f },
		{ 20260, 0.4f },
		{ 20261, 0.4f },
		{ 20262, 0.4f },
		{ 20263, 0.4f },
		{ 20264, 0.4f },
		{ 20265, 0.4f },
		{ 20266, 0.4f },
		{ 20267, 0.4f },
		{ 20268, 0.4f },
		{ 20269, 0.4f },
		{ 20270, 0.4f },
		{ 20271, 0.4f },
		{ 20272, 0.4f },
		{ 20273, 0.4f },
		{ 20274, 0.4f },
		{ 20275, 0.4f },
		{ 20276, 0.4f },
		{ 20277, 0.4f },
		{ 20422, 0.4f },
		{ 30301, 0.4f },
		{ 30302, 0.4f },
		{ 30303, 0.4f },
		{ 30304, 0.4f },
		{ 30305, 0.4f },
		{ 30306, 0.4f },
		{ 30309, 0.4f },
		{ 30305, 0.4f },
		{ 30311, 0.4f },
		{ 30312, 0.4f },
		{ 30313, 0.4f },
		{ 34001, 0.4f },
		{ 34002, 0.4f },
		{ 34003, 0.4f },
		{ 34004, 0.4f },
		{ 34005, 0.4f },
		{ 34006, 0.4f },
		{ 34007, 0.4f },
		{ 34008, 0.4f },
		{ 34009, 0.4f },
		{ 34005, 0.4f },
		{ 34011, 0.4f },
		{ 34012, 0.4f },
		{ 34013, 0.4f },
		{ 34014, 0.4f },
		{ 34015, 0.4f },
		{ 34016, 0.4f },
		{ 34017, 0.4f },
		{ 34018, 0.4f },
		{ 34019, 0.4f },
		{ 34020, 0.4f },
		{ 34021, 0.4f },
		{ 34022, 0.4f },
		{ 34023, 0.4f },
		{ 34024, 0.4f },
		{ 34025, 0.4f },
		{ 34026, 0.4f },
		{ 34027, 0.4f },
		{ 34028, 0.4f },
		{ 34029, 0.4f },
		{ 34030, 0.4f },
		{ 34031, 0.4f },
		{ 34032, 0.4f },
		{ 34033, 0.4f },
		{ 34034, 0.4f },
		{ 34035, 0.4f },
		{ 34036, 0.4f },
		{ 34037, 0.4f },
		{ 34039, 0.4f },
		{ 34041, 0.4f },
		{ 34042, 0.4f },
		{ 34045, 0.4f },
		{ 34046, 0.4f },
		{ 34047, 0.4f },
		{ 34048, 0.4f },
		{ 34049, 0.4f },
		{ 34050, 0.4f },
		{ 34053, 0.4f },
		{ 34054, 0.4f },
		{ 34055, 0.4f },
		{ 34056, 0.4f },
		{ 34057, 0.4f },
		{ 34058, 0.4f },
		{ 34059, 0.4f },
		{ 34060, 0.4f },
		{ 34061, 0.4f },
		{ 34062, 0.4f },
		{ 34063, 0.4f },
		{ 34064, 0.4f },
		{ 34065, 0.4f },
		{ 34070, 0.4f },
		{ 34071, 0.4f },
		{ 34072, 0.4f },
		{ 34077, 0.4f },
		{ 34078, 0.4f },
		{ 34082, 0.4f },
		{ 34083, 0.4f },
		{ 34084, 0.4f },
		{ 34085, 0.4f },
		{ 34090, 0.4f },
		{ 34091, 0.4f },
		{ 34092, 0.4f },
		{ 34093, 0.4f },
		{ 34094, 0.4f },
		{ 34097, 0.4f },
		{ 34099, 0.4f },
		{ 34053, 0.4f },
		{ 34054, 0.4f },
		{ 34055, 0.4f },
		{ 34059, 0.4f },
		{ 34105, 0.4f },
		{ 34111, 0.4f },
		{ 34114, 0.4f },
		{ 34115, 0.4f },
		{ 34116, 0.4f },
		{ 34117, 0.4f },
		{ 34118, 0.4f },
		{ 34119, 0.4f },
		{ 34120, 0.4f },
		{ 34121, 0.4f },
		{ 34133, 0.4f },
		{ 34134, 0.4f },
		{ 34135, 0.4f },
		{ 34136, 0.4f },
		{ 34137, 0.4f },
		{ 34141, 0.4f },
		{ 34142, 0.4f },
		{ 34143, 0.4f },
		{ 34145, 0.4f },
		{ 34146, 0.4f },
		{ 34148, 0.4f },
		{ 34150, 0.4f },
		{ 34151, 0.4f },
		{ 34152, 0.4f },
		{ 34153, 0.4f },
		{ 34154, 0.4f },
		{ 34155, 0.4f },
	};


	for (DWORD i = 0; i < SPECULAR_MAX_NUM; i++)
	{
		if (GetRace() == structInfo[i].race)
		{
			SMaterialData data;
			data.pImage = NULL;
			data.isSpecularEnable = TRUE;
			data.fSpecularPower = structInfo[i].specularPower;
			data.bSphereMapIndex = 1;
			m_GraphicThingInstance.SetMaterialData(0, NULL, data);
		}
	}
#endif
	return true;
}

#ifdef ENABLE_SKILL_COLOR_SYSTEM
void CInstanceBase::ChangeSkillColor(const DWORD* c_dwSkillColor)
{
	unsigned long ulSkillVnumColor[CRaceMotionData::SKILL_NUM][ESkillColorLength::MAX_EFFECT_COUNT];
	memset(ulSkillVnumColor, 0, sizeof(ulSkillVnumColor));

	unsigned long ulSkillColorSlot[ESkillColorLength::MAX_SKILL_COUNT + MAX_BUFF_COUNT][ESkillColorLength::MAX_EFFECT_COUNT];
	memcpy(ulSkillColorSlot, c_dwSkillColor, sizeof(ulSkillColorSlot));

	for (uint16_t byRace = 0; byRace < NPlayerData::MAIN_RACE_MAX_NUM; byRace++)
	{
		uint16_t bySkillVnum = 0;
		switch (byRace)
		{
		case NPlayerData::MAIN_RACE_WARRIOR_M:
			bySkillVnum = 1;
			break;

		case NPlayerData::MAIN_RACE_ASSASSIN_W:
			bySkillVnum = 31;
			break;

		case NPlayerData::MAIN_RACE_SURA_M:
			bySkillVnum = 76;
			break;

		case NPlayerData::MAIN_RACE_SHAMAN_W:
			bySkillVnum = 111;
			break;

		case NPlayerData::MAIN_RACE_WARRIOR_W:
			bySkillVnum = 16;
			break;

		case NPlayerData::MAIN_RACE_ASSASSIN_M:
			bySkillVnum = 46;
			break;

		case NPlayerData::MAIN_RACE_SURA_W:
			bySkillVnum = 61;
			break;

		case NPlayerData::MAIN_RACE_SHAMAN_M:
			bySkillVnum = 91;
			break;
		}
		uint16_t byNextSkillVnum = bySkillVnum;

		for (uint16_t bySkillSlot = 0; bySkillSlot < ESkillColorLength::MAX_SKILL_COUNT; bySkillSlot++)
		{
			if (bySkillSlot > 6)
				continue;

			for (uint16_t byLayer = 0; byLayer < ESkillColorLength::MAX_EFFECT_COUNT; ++byLayer)
			{
				ulSkillVnumColor[byNextSkillVnum][byLayer] = ulSkillColorSlot[byNextSkillVnum - bySkillVnum][byLayer];
			}
			++byNextSkillVnum;
		}
	}

	for (uint16_t byBuffSlot = ESkillColorLength::BUFF_BEGIN; byBuffSlot < ESkillColorLength::MAX_COLOR_SLOTS; byBuffSlot++)
	{
		BYTE byBuffSkillVnum = 0;
		switch (byBuffSlot)
		{
		case BUFF_BEGIN + 0:
			byBuffSkillVnum = 94;
			break;

		case BUFF_BEGIN + 1:
			byBuffSkillVnum = 95;
			break;

		case BUFF_BEGIN + 2:
			byBuffSkillVnum = 96;
			break;

		case BUFF_BEGIN + 3:
			byBuffSkillVnum = 110;
			break;

		case BUFF_BEGIN + 4:
			byBuffSkillVnum = 111;
			break;

		case BUFF_BEGIN + 5:
			byBuffSkillVnum = 175;
			break;
		}

		if (byBuffSkillVnum == 0)
			continue;

		for (uint16_t byLayer = 0; byLayer < ESkillColorLength::MAX_EFFECT_COUNT; ++byLayer)
			ulSkillVnumColor[byBuffSkillVnum][byLayer] = ulSkillColorSlot[byBuffSlot][byLayer];
	}

	m_GraphicThingInstance.ChangeSkillColor(*ulSkillVnumColor);
}
#endif

void CInstanceBase::__Create_SetName(const SCreateData& c_rkCreateData)
{
	if (IsGoto())
	{
		SetNameString("", 0);
		return;
	}
	if (IsWarp())
	{
		__Create_SetWarpName(c_rkCreateData);
		return;
	}

	SetNameString(c_rkCreateData.m_stName.c_str(), c_rkCreateData.m_stName.length());
}

void CInstanceBase::__Create_SetWarpName(const SCreateData& c_rkCreateData)
{
	const char* c_szName;
	if (CPythonNonPlayer::Instance().GetName(c_rkCreateData.m_dwRace, &c_szName))
	{
		std::string strName = c_szName;
		int iFindingPos = strName.find_first_of(" ", 0);
		if (iFindingPos > 0)
		{
			strName.resize(iFindingPos);
		}
		SetNameString(strName.c_str(), strName.length());
	}
	else
	{
		SetNameString(c_rkCreateData.m_stName.c_str(), c_rkCreateData.m_stName.length());
	}
}

void CInstanceBase::SetNameString(const char* c_szName, int len)
{
	m_stName.assign(c_szName, len);
}


bool CInstanceBase::SetRace(DWORD eRace)
{
	m_dwRace = eRace;

	if (!m_GraphicThingInstance.SetRace(eRace))
		return false;

	if (!__FindRaceType(m_dwRace, &m_eRaceType))
		m_eRaceType = CActorInstance::TYPE_PC;

	return true;
}

BOOL CInstanceBase::__IsChangableWeapon(int iWeaponID)
{
	if (IsWearingDress())
	{
		const int c_iBouquets[] =
		{
			50201, // Bouquet for Assassin
			50202, // Bouquet for Shaman
			50203,
			50204,
			0,
		};

		for (int i = 0; c_iBouquets[i] != 0; ++i)
			if (iWeaponID == c_iBouquets[i])
				return true;

		return false;
	}
	else
		return true;
}

BOOL CInstanceBase::IsWearingDress()
{
	const int c_iWeddingDressShape = 201;
	return c_iWeddingDressShape == m_eShape;
}

BOOL CInstanceBase::IsHoldingPickAxe()
{
	const int c_iPickAxeStart = 29101;
	const int c_iPickAxeEnd = 29110;
	return m_awPart[CRaceData::PART_WEAPON] >= c_iPickAxeStart && m_awPart[CRaceData::PART_WEAPON] <= c_iPickAxeEnd;
}

BOOL CInstanceBase::IsNewMount()
{
	return m_kHorse.IsNewMount();
}

BOOL CInstanceBase::IsMountingHorse()
{
	return m_kHorse.IsMounting();
}

void CInstanceBase::MountHorse(UINT eRace)
{
	m_kHorse.Destroy();
	m_kHorse.Create(m_GraphicThingInstance.NEW_GetCurPixelPositionRef(), eRace, ms_adwCRCAffectEffect[EFFECT_HIT]);

	SetMotionMode(CRaceMotionData::MODE_HORSE);
	SetRotationSpeed(c_fDefaultHorseRotationSpeed);

	m_GraphicThingInstance.MountHorse(m_kHorse.GetActorPtr());
	m_GraphicThingInstance.Stop();
	m_GraphicThingInstance.RefreshActorInstance();
}

void CInstanceBase::DismountHorse()
{
	m_kHorse.Destroy();
}

void CInstanceBase::GetInfo(std::string* pstInfo)
{
	char szInfo[256];
	sprintf(szInfo, "Inst - UC %d, RC %d Pool - %d ",
		ms_dwUpdateCounter,
		ms_dwRenderCounter,
		ms_kPool.GetCapacity()
	);

	pstInfo->append(szInfo);
}

void CInstanceBase::ResetPerformanceCounter()
{
	ms_dwUpdateCounter = 0;
	ms_dwRenderCounter = 0;
	ms_dwDeformCounter = 0;
}

bool CInstanceBase::NEW_IsLastPixelPosition()
{
	return m_GraphicThingInstance.IsPushing();
}

const TPixelPosition& CInstanceBase::NEW_GetLastPixelPositionRef()
{
	return m_GraphicThingInstance.NEW_GetLastPixelPositionRef();
}

void CInstanceBase::NEW_SetDstPixelPositionZ(FLOAT z)
{
	m_GraphicThingInstance.NEW_SetDstPixelPositionZ(z);
}

void CInstanceBase::NEW_SetDstPixelPosition(const TPixelPosition& c_rkPPosDst)
{
	m_GraphicThingInstance.NEW_SetDstPixelPosition(c_rkPPosDst);
}

void CInstanceBase::NEW_SetSrcPixelPosition(const TPixelPosition& c_rkPPosSrc)
{
	m_GraphicThingInstance.NEW_SetSrcPixelPosition(c_rkPPosSrc);
}

const TPixelPosition& CInstanceBase::NEW_GetCurPixelPositionRef()
{
	return m_GraphicThingInstance.NEW_GetCurPixelPositionRef();
}

const TPixelPosition& CInstanceBase::NEW_GetDstPixelPositionRef()
{
	return m_GraphicThingInstance.NEW_GetDstPixelPositionRef();
}

const TPixelPosition& CInstanceBase::NEW_GetSrcPixelPositionRef()
{
	return m_GraphicThingInstance.NEW_GetSrcPixelPositionRef();
}

void CInstanceBase::OnSyncing()
{
	m_GraphicThingInstance.__OnSyncing();
}

void CInstanceBase::OnWaiting()
{
	m_GraphicThingInstance.__OnWaiting();
}

void CInstanceBase::OnMoving()
{
	m_GraphicThingInstance.__OnMoving();
}

#ifdef ENABLE_GUILD_LEADER_TEXTAIL
void CInstanceBase::ChangeGuild(DWORD dwGuildID, DWORD dwGuildLeader)
#else
void CInstanceBase::ChangeGuild(DWORD dwGuildID)
#endif
{
	m_dwGuildID = dwGuildID;
#ifdef ENABLE_GUILD_LEADER_TEXTAIL
	m_dwGuildLeader = dwGuildLeader;
#endif

	DetachTextTail();
	AttachTextTail();
	RefreshTextTail();
}

DWORD CInstanceBase::GetPart(CRaceData::EParts part)
{
	assert(part >= 0 && part < CRaceData::PART_MAX_NUM);
	return m_awPart[part];
}

DWORD CInstanceBase::GetShape()
{
	return m_eShape;
}

#ifdef ENABLE_HIDE_COSTUME_SYSTEM
bool CInstanceBase::CanInteract()
{
	if (!CanAct())
		return false;

	if (IsStun())
		return false;

	if (IsDead())
		return false;

	if (!CanMove())
		return false;

	if (IsWalking())
		return false;

	if (m_GraphicThingInstance.isAttacking())
		return false;

	if (m_GraphicThingInstance.IsAttacked())
		return false;

	return true;
}
#endif

bool CInstanceBase::CanAct()
{
	return m_GraphicThingInstance.CanAct();
}

bool CInstanceBase::CanMove()
{
	return m_GraphicThingInstance.CanMove();
}

bool CInstanceBase::CanUseSkill()
{
	if (IsPoly())
		return false;

	if (IsWearingDress())
		return false;

	if (IsHoldingPickAxe())
		return false;

	if (!m_kHorse.CanUseSkill())
		return false;

	if (!m_GraphicThingInstance.CanUseSkill())
		return false;

	return true;
}

bool CInstanceBase::CanAttack()
{
	if (!m_kHorse.CanAttack())
		return false;

	if (IsWearingDress())
		return false;

	if (IsHoldingPickAxe())
		return false;

	return m_GraphicThingInstance.CanAttack();
}



bool CInstanceBase::CanFishing()
{
	return m_GraphicThingInstance.CanFishing();
}


BOOL CInstanceBase::IsBowMode()
{
	return m_GraphicThingInstance.IsBowMode();
}

BOOL CInstanceBase::IsHandMode()
{
	return m_GraphicThingInstance.IsHandMode();
}

BOOL CInstanceBase::IsFishingMode()
{
	if (CRaceMotionData::MODE_FISHING == m_GraphicThingInstance.GetMotionMode())
		return true;

	return false;
}

BOOL CInstanceBase::IsFishing()
{
	return m_GraphicThingInstance.IsFishing();
}

BOOL CInstanceBase::IsDead()
{
	return m_GraphicThingInstance.IsDead();
}

BOOL CInstanceBase::IsStun()
{
	return m_GraphicThingInstance.IsStun();
}

BOOL CInstanceBase::IsSleep()
{
	return m_GraphicThingInstance.IsSleep();
}


BOOL CInstanceBase::__IsSyncing()
{
	return m_GraphicThingInstance.__IsSyncing();
}



void CInstanceBase::NEW_SetOwner(DWORD dwVIDOwner)
{
	m_GraphicThingInstance.SetOwner(dwVIDOwner);
}

float CInstanceBase::GetLocalTime()
{
	return m_GraphicThingInstance.GetLocalTime();
}


void CInstanceBase::PushUDPState(DWORD dwCmdTime, const TPixelPosition& c_rkPPosDst, float fDstRot, UINT eFunc, UINT uArg)
{
}

DWORD	ELTimer_GetServerFrameMSec();

void CInstanceBase::PushTCPStateExpanded(DWORD dwCmdTime, const TPixelPosition& c_rkPPosDst, float fDstRot, UINT eFunc, UINT uArg, UINT uTargetVID)
{
	SCommand kCmdNew;
	kCmdNew.m_kPPosDst = c_rkPPosDst;
	kCmdNew.m_dwChkTime = dwCmdTime + 100;
	kCmdNew.m_dwCmdTime = dwCmdTime;
	kCmdNew.m_fDstRot = fDstRot;
	kCmdNew.m_eFunc = eFunc;
	kCmdNew.m_uArg = uArg;
	kCmdNew.m_uTargetVID = uTargetVID;
	m_kQue_kCmdNew.push_back(kCmdNew);
}

void CInstanceBase::PushTCPState(DWORD dwCmdTime, const TPixelPosition& c_rkPPosDst, float fDstRot, UINT eFunc, UINT uArg)
{
	if (__IsMainInstance())
	{
		TraceError("CInstanceBase::PushTCPState ÇĂ·ąŔĚľî ŔÚ˝Ĺżˇ°Ô ŔĚµżĆĐĹ¶Ŕş żŔ¸é ľČµČ´Ů!");
		return;
	}

	int nNetworkGap = ELTimer_GetServerFrameMSec() - dwCmdTime;

	m_nAverageNetworkGap = (m_nAverageNetworkGap * 70 + nNetworkGap * 30) / 100;

	SCommand kCmdNew;
	kCmdNew.m_kPPosDst = c_rkPPosDst;
	kCmdNew.m_dwChkTime = dwCmdTime + m_nAverageNetworkGap;
	kCmdNew.m_dwCmdTime = dwCmdTime;
	kCmdNew.m_fDstRot = fDstRot;
	kCmdNew.m_eFunc = eFunc;
	kCmdNew.m_uArg = uArg;
	m_kQue_kCmdNew.push_back(kCmdNew);
}

BOOL CInstanceBase::__CanProcessNetworkStatePacket()
{
	if (m_GraphicThingInstance.IsDead())
		return FALSE;
	if (m_GraphicThingInstance.IsKnockDown())
		return FALSE;
	if (m_GraphicThingInstance.IsUsingSkill())
		if (!m_GraphicThingInstance.CanCancelSkill())
			return FALSE;

	return TRUE;
}

BOOL CInstanceBase::__IsEnableTCPProcess(UINT eCurFunc)
{
	if (m_GraphicThingInstance.IsActEmotion())
	{
		return FALSE;
	}

	if (!m_bEnableTCPState)
	{
		if (FUNC_EMOTION != eCurFunc)
		{
			return FALSE;
		}
	}

	return TRUE;
}

void CInstanceBase::StateProcess()
{
	while (1)
	{
		if (m_kQue_kCmdNew.empty())
			return;

		DWORD dwDstChkTime = m_kQue_kCmdNew.front().m_dwChkTime;
		DWORD dwCurChkTime = ELTimer_GetServerFrameMSec();

		if (dwCurChkTime < dwDstChkTime)
			return;

		SCommand kCmdTop = m_kQue_kCmdNew.front();
		m_kQue_kCmdNew.pop_front();

		TPixelPosition kPPosDst = kCmdTop.m_kPPosDst;

		FLOAT fRotDst = kCmdTop.m_fDstRot;
		UINT eFunc = kCmdTop.m_eFunc;
		UINT uArg = kCmdTop.m_uArg;
		UINT uVID = GetVirtualID();
		UINT uTargetVID = kCmdTop.m_uTargetVID;

		TPixelPosition kPPosCur;
		NEW_GetPixelPosition(&kPPosCur);

		TPixelPosition kPPosDir = kPPosDst - kPPosCur;
		float fDirLen = (float)sqrt(kPPosDir.x * kPPosDir.x + kPPosDir.y * kPPosDir.y);

		if (!__CanProcessNetworkStatePacket())
		{
			Lognf(0, "vid=%d żňÁ÷ŔĎ Ľö ľř´Â »óĹÂ¶ó ˝şĹµ IsDead=%d, IsKnockDown=%d", uVID, m_GraphicThingInstance.IsDead(), m_GraphicThingInstance.IsKnockDown());
			return;
		}

		if (!__IsEnableTCPProcess(eFunc))
		{
			return;
		}

		switch (eFunc)
		{
		case FUNC_WAIT:
		{
			if (fDirLen > 1.0f)
			{
				NEW_SetSrcPixelPosition(kPPosCur);
				NEW_SetDstPixelPosition(kPPosDst);

				__EnableSkipCollision();

				m_fDstRot = fRotDst;
				m_isGoing = TRUE;

				m_kMovAfterFunc.eFunc = FUNC_WAIT;

				if (!IsWalking())
					StartWalking();
			}
			else
			{
				m_isGoing = FALSE;

				if (!IsWaiting())
					EndWalking();

				SCRIPT_SetPixelPosition(kPPosDst.x, kPPosDst.y);
				SetAdvancingRotation(fRotDst);
				SetRotation(fRotDst);
			}
			break;
		}

		case FUNC_MOVE:
		{
			NEW_SetSrcPixelPosition(kPPosCur);
			NEW_SetDstPixelPosition(kPPosDst);
			m_fDstRot = fRotDst;
			m_isGoing = TRUE;
			__EnableSkipCollision();

			m_kMovAfterFunc.eFunc = FUNC_MOVE;

			if (!IsWalking())
			{
				StartWalking();
			}
			else
			{
			}
			break;
		}

		case FUNC_COMBO:
		{
			if (fDirLen >= 50.0f)
			{
				NEW_SetSrcPixelPosition(kPPosCur);
				NEW_SetDstPixelPosition(kPPosDst);
				m_fDstRot = fRotDst;
				m_isGoing = TRUE;
				__EnableSkipCollision();

				m_kMovAfterFunc.eFunc = FUNC_COMBO;
				m_kMovAfterFunc.uArg = uArg;

				if (!IsWalking())
					StartWalking();
			}
			else
			{
				m_isGoing = FALSE;

				if (IsWalking())
					EndWalking();

				SCRIPT_SetPixelPosition(kPPosDst.x, kPPosDst.y);
				RunComboAttack(fRotDst, uArg);
			}
			break;
		}

		case FUNC_ATTACK:
		{
			if (fDirLen >= 50.0f)
			{
				NEW_SetSrcPixelPosition(kPPosCur);
				NEW_SetDstPixelPosition(kPPosDst);
				m_fDstRot = fRotDst;
				m_isGoing = TRUE;
				__EnableSkipCollision();

				m_kMovAfterFunc.eFunc = FUNC_ATTACK;

				if (!IsWalking())
					StartWalking();
			}
			else
			{
				m_isGoing = FALSE;

				if (IsWalking())
					EndWalking();

				SCRIPT_SetPixelPosition(kPPosDst.x, kPPosDst.y);
				BlendRotation(fRotDst);

				RunNormalAttack(fRotDst);
			}
			break;
		}

		case FUNC_MOB_SKILL:
		{
			if (fDirLen >= 50.0f)
			{
				NEW_SetSrcPixelPosition(kPPosCur);
				NEW_SetDstPixelPosition(kPPosDst);
				m_fDstRot = fRotDst;
				m_isGoing = TRUE;
				__EnableSkipCollision();

				m_kMovAfterFunc.eFunc = FUNC_MOB_SKILL;
				m_kMovAfterFunc.uArg = uArg;

				if (!IsWalking())
					StartWalking();
			}
			else
			{
				m_isGoing = FALSE;

				if (IsWalking())
					EndWalking();

				SCRIPT_SetPixelPosition(kPPosDst.x, kPPosDst.y);
				BlendRotation(fRotDst);

				m_GraphicThingInstance.InterceptOnceMotion(CRaceMotionData::NAME_SPECIAL_1 + uArg);
			}
			break;
		}

		case FUNC_EMOTION:
		{
			if (fDirLen > 100.0f)
			{
				NEW_SetSrcPixelPosition(kPPosCur);
				NEW_SetDstPixelPosition(kPPosDst);
				m_fDstRot = fRotDst;
				m_isGoing = TRUE;

				if (__IsMainInstance())
					__EnableSkipCollision();

				m_kMovAfterFunc.eFunc = FUNC_EMOTION;
				m_kMovAfterFunc.uArg = uArg;
				m_kMovAfterFunc.uArgExpanded = uTargetVID;
				m_kMovAfterFunc.kPosDst = kPPosDst;

				if (!IsWalking())
					StartWalking();
			}
			else
			{
				__ProcessFunctionEmotion(uArg, uTargetVID, kPPosDst);
			}
			break;
		}

		default:
		{
			if (eFunc & FUNC_SKILL)
			{
				if (fDirLen >= 50.0f)
				{
					NEW_SetSrcPixelPosition(kPPosCur);
					NEW_SetDstPixelPosition(kPPosDst);
					m_fDstRot = fRotDst;
					m_isGoing = TRUE;

					__EnableSkipCollision();

					m_kMovAfterFunc.eFunc = eFunc;
					m_kMovAfterFunc.uArg = uArg;

					if (!IsWalking())
						StartWalking();
				}
				else
				{
					m_isGoing = FALSE;

					if (IsWalking())
						EndWalking();

					SCRIPT_SetPixelPosition(kPPosDst.x, kPPosDst.y);
					SetAdvancingRotation(fRotDst);
					SetRotation(fRotDst);

					NEW_UseSkill(0, eFunc & 0x7f, uArg & 0x0f, (uArg >> 4) ? true : false);
				}
			}
			break;
		}
		}
	}
}


void CInstanceBase::MovementProcess()
{
	TPixelPosition kPPosCur;
	NEW_GetPixelPosition(&kPPosCur);

	TPixelPosition kPPosNext;
	{
		const D3DXVECTOR3& c_rkV3Mov = m_GraphicThingInstance.GetMovementVectorRef();

		kPPosNext.x = kPPosCur.x + (+c_rkV3Mov.x);
		kPPosNext.y = kPPosCur.y + (-c_rkV3Mov.y);
		kPPosNext.z = kPPosCur.z + (+c_rkV3Mov.z);
	}

	TPixelPosition kPPosDeltaSC = kPPosCur - NEW_GetSrcPixelPositionRef();
	TPixelPosition kPPosDeltaSN = kPPosNext - NEW_GetSrcPixelPositionRef();
	TPixelPosition kPPosDeltaSD = NEW_GetDstPixelPositionRef() - NEW_GetSrcPixelPositionRef();

	float fCurLen = sqrtf(kPPosDeltaSC.x * kPPosDeltaSC.x + kPPosDeltaSC.y * kPPosDeltaSC.y);
	float fNextLen = sqrtf(kPPosDeltaSN.x * kPPosDeltaSN.x + kPPosDeltaSN.y * kPPosDeltaSN.y);
	float fTotalLen = sqrtf(kPPosDeltaSD.x * kPPosDeltaSD.x + kPPosDeltaSD.y * kPPosDeltaSD.y);
	float fRestLen = fTotalLen - fCurLen;

	if (__IsMainInstance())
	{
		if (m_isGoing && IsWalking())
		{
			float fDstRot = NEW_GetAdvancingRotationFromPixelPosition(NEW_GetSrcPixelPositionRef(), NEW_GetDstPixelPositionRef());

			SetAdvancingRotation(fDstRot);

			if (fRestLen <= 0.0)
			{
				if (IsWalking())
					EndWalking();

				m_isGoing = FALSE;

				BlockMovement();

				if (FUNC_EMOTION == m_kMovAfterFunc.eFunc)
				{
					DWORD dwMotionNumber = m_kMovAfterFunc.uArg;
					DWORD dwTargetVID = m_kMovAfterFunc.uArgExpanded;
					__ProcessFunctionEmotion(dwMotionNumber, dwTargetVID, m_kMovAfterFunc.kPosDst);
					m_kMovAfterFunc.eFunc = FUNC_WAIT;
					return;
				}
			}
		}
	}
	else
	{
		if (m_isGoing && IsWalking())
		{
			float fDstRot = NEW_GetAdvancingRotationFromPixelPosition(NEW_GetSrcPixelPositionRef(), NEW_GetDstPixelPositionRef());

			SetAdvancingRotation(fDstRot);

			if (fRestLen < -100.0f)
			{
				NEW_SetSrcPixelPosition(kPPosCur);

				float fDstRot = NEW_GetAdvancingRotationFromPixelPosition(kPPosCur, NEW_GetDstPixelPositionRef());
				SetAdvancingRotation(fDstRot);

				if (FUNC_MOVE == m_kMovAfterFunc.eFunc)
				{
					m_kMovAfterFunc.eFunc = FUNC_WAIT;
				}
			}
			else if (fCurLen <= fTotalLen && fTotalLen <= fNextLen)
			{
				if (m_GraphicThingInstance.IsDead() || m_GraphicThingInstance.IsKnockDown())
				{
					__DisableSkipCollision();

					m_isGoing = FALSE;
				}
				else
				{
					switch (m_kMovAfterFunc.eFunc)
					{
					case FUNC_ATTACK:
					{
						if (IsWalking())
							EndWalking();

						__DisableSkipCollision();
						m_isGoing = FALSE;

						BlockMovement();
						SCRIPT_SetPixelPosition(NEW_GetDstPixelPositionRef().x, NEW_GetDstPixelPositionRef().y);
						SetAdvancingRotation(m_fDstRot);
						SetRotation(m_fDstRot);

						RunNormalAttack(m_fDstRot);
						break;
					}

					case FUNC_COMBO:
					{
						if (IsWalking())
							EndWalking();

						__DisableSkipCollision();
						m_isGoing = FALSE;

						BlockMovement();
						SCRIPT_SetPixelPosition(NEW_GetDstPixelPositionRef().x, NEW_GetDstPixelPositionRef().y);
						RunComboAttack(m_fDstRot, m_kMovAfterFunc.uArg);
						break;
					}

					case FUNC_EMOTION:
					{
						m_isGoing = FALSE;
						m_kMovAfterFunc.eFunc = FUNC_WAIT;
						__DisableSkipCollision();
						BlockMovement();

						DWORD dwMotionNumber = m_kMovAfterFunc.uArg;
						DWORD dwTargetVID = m_kMovAfterFunc.uArgExpanded;
						__ProcessFunctionEmotion(dwMotionNumber, dwTargetVID, m_kMovAfterFunc.kPosDst);
						break;
					}

					case FUNC_MOVE:
					{
						if (!IsWaiting())
							EndWalkingWithoutBlending();
						break;
					}

					case FUNC_MOB_SKILL:
					{
						if (IsWalking())
							EndWalking();

						__DisableSkipCollision();
						m_isGoing = FALSE;

						BlockMovement();
						SCRIPT_SetPixelPosition(NEW_GetDstPixelPositionRef().x, NEW_GetDstPixelPositionRef().y);
						SetAdvancingRotation(m_fDstRot);
						SetRotation(m_fDstRot);

						m_GraphicThingInstance.InterceptOnceMotion(CRaceMotionData::NAME_SPECIAL_1 + m_kMovAfterFunc.uArg);
						break;
					}

					default:
					{
						if (m_kMovAfterFunc.eFunc & FUNC_SKILL)
						{
							SetAdvancingRotation(m_fDstRot);
							BlendRotation(m_fDstRot);
							NEW_UseSkill(0, m_kMovAfterFunc.eFunc & 0x7f, m_kMovAfterFunc.uArg & 0x0f, (m_kMovAfterFunc.uArg >> 4) ? true : false);
						}
						else
						{
							__DisableSkipCollision();
							m_isGoing = FALSE;

							BlockMovement();
							SCRIPT_SetPixelPosition(NEW_GetDstPixelPositionRef().x, NEW_GetDstPixelPositionRef().y);
							SetAdvancingRotation(m_fDstRot);
							BlendRotation(m_fDstRot);
							if (!IsWaiting())
							{
								EndWalking();
							}
						}
						break;
					}
					}

				}
			}

		}
	}

	if (IsWalking() || m_GraphicThingInstance.IsUsingMovingSkill())
	{
		float fRotation = m_GraphicThingInstance.GetRotation();
		float fAdvancingRotation = m_GraphicThingInstance.GetAdvancingRotation();
		int iDirection = GetRotatingDirection(fRotation, fAdvancingRotation);

		if (DEGREE_DIRECTION_SAME != m_iRotatingDirection)
		{
			if (DEGREE_DIRECTION_LEFT == iDirection)
			{
				fRotation = fmodf(fRotation + m_fRotSpd * m_GraphicThingInstance.GetSecondElapsed(), 360.0f);
			}
			else if (DEGREE_DIRECTION_RIGHT == iDirection)
			{
				fRotation = fmodf(fRotation - m_fRotSpd * m_GraphicThingInstance.GetSecondElapsed() + 360.0f, 360.0f);
			}

			if (m_iRotatingDirection != GetRotatingDirection(fRotation, fAdvancingRotation))
			{
				m_iRotatingDirection = DEGREE_DIRECTION_SAME;
				fRotation = fAdvancingRotation;
			}

			m_GraphicThingInstance.SetRotation(fRotation);
		}

		if (__IsInDustRange())
		{
			float fDustDistance = NEW_GetDistanceFromDestPixelPosition(m_kPPosDust);
			if (IsMountingHorse())
			{
				if (fDustDistance > ms_fHorseDustGap)
				{
					NEW_GetPixelPosition(&m_kPPosDust);
					__AttachEffect(EFFECT_HORSE_DUST);
				}
			}
			else
			{
				if (fDustDistance > ms_fDustGap)
				{
					NEW_GetPixelPosition(&m_kPPosDust);
					__AttachEffect(EFFECT_DUST);
				}
			}
		}
	}
}

void CInstanceBase::__ProcessFunctionEmotion(DWORD dwMotionNumber, DWORD dwTargetVID, const TPixelPosition& c_rkPosDst)
{
	if (IsWalking())
		EndWalkingWithoutBlending();

	__EnableChangingTCPState();
	SCRIPT_SetPixelPosition(c_rkPosDst.x, c_rkPosDst.y);

	CInstanceBase* pTargetInstance = CPythonCharacterManager::Instance().GetInstancePtr(dwTargetVID);
	if (pTargetInstance)
	{
		pTargetInstance->__EnableChangingTCPState();

		if (pTargetInstance->IsWalking())
			pTargetInstance->EndWalkingWithoutBlending();

		WORD wMotionNumber1 = HIWORD(dwMotionNumber);
		WORD wMotionNumber2 = LOWORD(dwMotionNumber);

		int src_job = RaceToJob(GetRace());
		int dst_job = RaceToJob(pTargetInstance->GetRace());

		NEW_LookAtDestInstance(*pTargetInstance);
		m_GraphicThingInstance.InterceptOnceMotion(wMotionNumber1 + dst_job);
		m_GraphicThingInstance.SetRotation(m_GraphicThingInstance.GetTargetRotation());
		m_GraphicThingInstance.SetAdvancingRotation(m_GraphicThingInstance.GetTargetRotation());

		pTargetInstance->NEW_LookAtDestInstance(*this);
		pTargetInstance->m_GraphicThingInstance.InterceptOnceMotion(wMotionNumber2 + src_job);
		pTargetInstance->m_GraphicThingInstance.SetRotation(pTargetInstance->m_GraphicThingInstance.GetTargetRotation());
		pTargetInstance->m_GraphicThingInstance.SetAdvancingRotation(pTargetInstance->m_GraphicThingInstance.GetTargetRotation());

		if (pTargetInstance->__IsMainInstance())
		{
			IAbstractPlayer& rPlayer = IAbstractPlayer::GetSingleton();
			rPlayer.EndEmotionProcess();
		}
	}

	if (__IsMainInstance())
	{
		IAbstractPlayer& rPlayer = IAbstractPlayer::GetSingleton();
		rPlayer.EndEmotionProcess();
	}
}

int g_iAccumulationTime = 0;

void CInstanceBase::Update()
{
	++ms_dwUpdateCounter;

	StateProcess();
	m_GraphicThingInstance.PhysicsProcess();
	m_GraphicThingInstance.RotationProcess();
	m_GraphicThingInstance.ComboProcess();
	m_GraphicThingInstance.AccumulationMovement();

	if (m_GraphicThingInstance.IsMovement())
	{
		TPixelPosition kPPosCur;
		NEW_GetPixelPosition(&kPPosCur);

		DWORD dwCurTime = ELTimer_GetFrameMSec();

		{
			m_dwNextUpdateHeightTime = dwCurTime;
			kPPosCur.z = __GetBackgroundHeight(kPPosCur.x, kPPosCur.y);
			NEW_SetPixelPosition(kPPosCur);
		}

		//{
		//	DWORD dwMtrlColor = __GetShadowMapColor(kPPosCur.x, kPPosCur.y);
		//	m_GraphicThingInstance.SetMaterialColor(dwMtrlColor);
		//}
	}

	m_GraphicThingInstance.UpdateAdvancingPointInstance();

	AttackProcess();
	MovementProcess();

	m_GraphicThingInstance.MotionProcess(IsPC());

	if (IsPoly())
		__ClearArmorRefineEffect(); // Fix

#ifdef ENABLE_GRAPHIC_ON_OFF
	if (CPythonSystem::instance().GetEffectLevel() == 4)
	{
		m_GraphicThingInstance.SetDeactiveAllAttachingEffect();
	}
	else if (CPythonSystem::instance().GetEffectLevel() == 3)
	{
		if (!__IsMainInstance())
			m_GraphicThingInstance.SetDeactiveAllAttachingEffect();
		else
			if (!IsInvisibility())
				m_GraphicThingInstance.SetActiveAllAttachingEffect();
	}
	else if (CPythonSystem::instance().GetEffectLevel() == 2)
	{
		if (!IsPC())
			m_GraphicThingInstance.SetDeactiveAllAttachingEffect();
		else
			if (!IsInvisibility())
				m_GraphicThingInstance.SetActiveAllAttachingEffect();
	}
	else if (CPythonSystem::instance().GetEffectLevel() == 1)
	{
		if (!__IsMainInstance() || !IsEnemy() && !IsNPC())
			m_GraphicThingInstance.SetDeactiveAllAttachingEffect();
		else
			if (!IsInvisibility())
				m_GraphicThingInstance.SetActiveAllAttachingEffect();
	}
	else
	{
		if (!IsInvisibility())
			m_GraphicThingInstance.SetActiveAllAttachingEffect();
	}

	if (IsPet() || IsMount()
#ifdef ENABLE_GROWTH_PET_SYSTEM
		|| IsGrowthPet()
#endif
		)
	{
		if (CPythonSystem::instance().IsPetStatus() == 1)
		{
			if (!IsAffect(AFFECT_INVISIBILITY))
			{
				__SetAffect(AFFECT_INVISIBILITY, true);
				m_kAffectFlagContainer.Set(AFFECT_INVISIBILITY, true);
			}
		}
		else
		{
			if (IsAffect(AFFECT_INVISIBILITY))
			{
				__SetAffect(AFFECT_INVISIBILITY, false);
				m_kAffectFlagContainer.Set(AFFECT_INVISIBILITY, false);
			}
		}
	}

	if (IsShop())
	{
		if (CPythonSystem::instance().GetPrivateShopLevel() >= 3)
		{
			if (!IsAffect(AFFECT_INVISIBILITY))
			{
				__SetAffect(AFFECT_INVISIBILITY, true);
				m_kAffectFlagContainer.Set(AFFECT_INVISIBILITY, true);
			}
		}
		else
		{
			if (IsAffect(AFFECT_INVISIBILITY))
			{
				__SetAffect(AFFECT_INVISIBILITY, false);
				m_kAffectFlagContainer.Set(AFFECT_INVISIBILITY, false);
			}
		}
	}
#endif

	if (IsMountingHorse())
	{
		m_kHorse.m_pkActor->HORSE_MotionProcess(FALSE);
	}

	__ComboProcess();

	ProcessRemoveOldDamage();
	ProcessDamage();

#ifdef ENABLE_STONE_SCALE_OPTION
	RefreshStoneScale();
#endif
}

void CInstanceBase::Transform()
{
	if (__IsSyncing())
	{
		//OnSyncing();
	}
	else
	{
		if (IsWalking() || m_GraphicThingInstance.IsUsingMovingSkill())
		{
			const D3DXVECTOR3& c_rv3Movment = m_GraphicThingInstance.GetMovementVectorRef();

			float len = (c_rv3Movment.x * c_rv3Movment.x) + (c_rv3Movment.y * c_rv3Movment.y);
			if (len > 1.0f)
				OnMoving();
			else
				OnWaiting();
		}
	}

	m_GraphicThingInstance.INSTANCEBASE_Transform();
}

void CInstanceBase::Deform()
{
	if (!__CanRender())
		return;

	++ms_dwDeformCounter;

	m_GraphicThingInstance.INSTANCEBASE_Deform();

	m_kHorse.Deform();
}

void CInstanceBase::RenderTrace()
{
	if (!__CanRender())
		return;

	m_GraphicThingInstance.RenderTrace();
}

void CInstanceBase::Render()
{
	if (!__CanRender())
		return;

	++ms_dwRenderCounter;

	//Shadow napfényre reagál stb..
	//D3DMATERIAL8 mat;
	//ZeroMemory(&mat, sizeof(D3DMATERIAL8));
	//
	//mat.Specular.r = mat.Specular.g = mat.Specular.b = 1.0f;
	//mat.Specular.a = 1.0f;
	//mat.Power = 50.0f;
	//
	//mat.Diffuse.r = mat.Diffuse.g = mat.Diffuse.b = 1.0f;
	//mat.Diffuse.a = 1.0f;
	//mat.Ambient = mat.Diffuse;
	//
	//STATEMANAGER.SetRenderState(D3DRS_SPECULARENABLE, TRUE);
	//STATEMANAGER.SetMaterial(&mat);
	//vége
	m_kHorse.Render();
	m_GraphicThingInstance.Render();

	if (CActorInstance::IsDirLine())
	{
		if (NEW_GetDstPixelPositionRef().x != 0.0f)
		{
			static CScreen s_kScreen;

			STATEMANAGER.SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
			STATEMANAGER.SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
			STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
			STATEMANAGER.SaveRenderState(D3DRS_ZENABLE, FALSE);
			STATEMANAGER.SetRenderState(D3DRS_FOGENABLE, FALSE);
			STATEMANAGER.SetRenderState(D3DRS_LIGHTING, FALSE);

			TPixelPosition px;
			m_GraphicThingInstance.GetPixelPosition(&px);
			D3DXVECTOR3 kD3DVt3Cur(px.x, px.y, px.z);
			D3DXVECTOR3 kD3DVt3Dest(NEW_GetDstPixelPositionRef().x, -NEW_GetDstPixelPositionRef().y, NEW_GetDstPixelPositionRef().z);

			s_kScreen.SetDiffuseColor(0.0f, 0.0f, 1.0f);
			s_kScreen.RenderLine3d(kD3DVt3Cur.x, kD3DVt3Cur.y, px.z, kD3DVt3Dest.x, kD3DVt3Dest.y, px.z);
			STATEMANAGER.RestoreRenderState(D3DRS_ZENABLE);
			STATEMANAGER.SetRenderState(D3DRS_FOGENABLE, TRUE);
			STATEMANAGER.SetRenderState(D3DRS_LIGHTING, TRUE);
		}
	}
}

void CInstanceBase::RenderToShadowMap()
{
	if (IsDoor())
		return;

	if (IsBuilding())
		return;

	if (!__CanRender())
		return;

	if (!__IsExistMainInstance())
		return;

	CInstanceBase* pkInstMain = __GetMainInstancePtr();

	//const float SHADOW_APPLY_DISTANCE = 2500.0f;
	const float SHADOW_APPLY_DISTANCE = 2500.0f * SHADOW_DISTANCE_MULTIPLIER;

	float fDistance = NEW_GetDistanceFromDestInstance(*pkInstMain);
	if (fDistance >= SHADOW_APPLY_DISTANCE)
		return;

	m_GraphicThingInstance.RenderToShadowMap();
}

void CInstanceBase::RenderCollision()
{
	m_GraphicThingInstance.RenderCollisionData();
}

void CInstanceBase::SetVirtualID(DWORD dwVirtualID)
{
	m_GraphicThingInstance.SetVirtualID(dwVirtualID);
}

void CInstanceBase::SetVirtualNumber(DWORD dwVirtualNumber)
{
	m_dwVirtualNumber = dwVirtualNumber;
}

void CInstanceBase::SetInstanceType(int iInstanceType)
{
	m_GraphicThingInstance.SetActorType(iInstanceType);
}



#ifdef ENABLE_ALIGN_RENEWAL
void CInstanceBase::SetAlignment(int sAlignment)
#else
void CInstanceBase::SetAlignment(int sAlignment)
{
}
void CInstanceBase::SetAlignment(short sAlignment)
#endif

{
	m_sAlignment = sAlignment;
	RefreshTextTailTitle();
}

#ifdef ENABLE_TITLE_SYSTEM
void CInstanceBase::SetTitleSystem(int iTitle)
{
	m_iTitleID = iTitle;
	RefreshTextTailTitle();
}
#endif

void CInstanceBase::SetPKMode(BYTE byPKMode)
{
	if (m_byPKMode == byPKMode)
		return;

	m_byPKMode = byPKMode;

	if (__IsMainInstance())
	{
		IAbstractPlayer& rPlayer = IAbstractPlayer::GetSingleton();
		rPlayer.NotifyChangePKMode();
	}
}

void CInstanceBase::SetKiller(bool bFlag)
{
	if (m_isKiller == bFlag)
		return;

	m_isKiller = bFlag;
	RefreshTextTail();
}

void CInstanceBase::SetPartyMemberFlag(bool bFlag)
{
	m_isPartyMember = bFlag;
}

void CInstanceBase::SetStateFlags(DWORD dwStateFlags)
{
	if (dwStateFlags & ADD_CHARACTER_STATE_KILLER)
		SetKiller(TRUE);
	else
		SetKiller(FALSE);

	if (dwStateFlags & ADD_CHARACTER_STATE_PARTY)
		SetPartyMemberFlag(TRUE);
	else
		SetPartyMemberFlag(FALSE);
}

void CInstanceBase::SetComboType(UINT uComboType)
{
	m_GraphicThingInstance.SetComboType(uComboType);
}

const char* CInstanceBase::GetNameString()
{
	return m_stName.c_str();
}

void CInstanceBase::SetLevel(DWORD level)
{
	m_dwLevel = level;
	UpdateTextTailLevel(m_dwLevel);
}

#ifdef ENABLE_GROWTH_PET_SYSTEM
void CInstanceBase::SetPetLevel(DWORD dwLevel)
{
	m_dwLevel = dwLevel;
	UpdateTextTailLevel(dwLevel);

	float fScale = m_dwLevel * 0.4f + (0.10f + 0.10f * m_bCharacterSize);
	m_GraphicThingInstance.SetScale(fScale, fScale, fScale, true);
}
#endif

DWORD CInstanceBase::GetRace()
{
	return m_dwRace;
}

DWORD CInstanceBase::GetLevel()
{
	return m_dwLevel;
}

bool CInstanceBase::IsConflictAlignmentInstance(CInstanceBase& rkInstVictim)
{
	if (PK_MODE_PROTECT == rkInstVictim.GetPKMode())
		return false;

	switch (GetAlignmentType())
	{
	case ALIGNMENT_TYPE_NORMAL:
	case ALIGNMENT_TYPE_WHITE:
		if (ALIGNMENT_TYPE_DARK == rkInstVictim.GetAlignmentType())
			return true;
		break;
	case ALIGNMENT_TYPE_DARK:
		if (GetAlignmentType() != rkInstVictim.GetAlignmentType())
			return true;
		break;
	}

	return false;
}

void CInstanceBase::SetDuelMode(DWORD type)
{
	m_dwDuelMode = type;
}

DWORD CInstanceBase::GetDuelMode()
{
	return m_dwDuelMode;
}

bool CInstanceBase::IsAttackableInstance(CInstanceBase& rkInstVictim)
{
	if (__IsMainInstance())
	{
		CPythonPlayer& rkPlayer = CPythonPlayer::Instance();
		if (rkPlayer.IsObserverMode())
			return false;
	}

	if (GetVirtualID() == rkInstVictim.GetVirtualID())
		return false;

	if (IsStone())
	{
		if (rkInstVictim.IsPC())
			return true;
	}
	else if (IsPC())
	{
		if (rkInstVictim.IsStone())
			return true;

		if (rkInstVictim.IsPC())
		{
#ifdef ENABLE_RENEWAL_REGEN
			CPythonPlayer& rkPlayer = CPythonPlayer::Instance();
			if (rkPlayer.CheckBossSafeRange())
				return false;
#endif
			if (GetDuelMode())
			{
				switch (GetDuelMode())
				{
				case DUEL_CANNOTATTACK:
					return false;
				case DUEL_START:
					if (__FindDUELKey(GetVirtualID(), rkInstVictim.GetVirtualID()))
						return true;
					else
						return false;
				}
			}
			if (PK_MODE_GUILD == GetPKMode())
				if (GetGuildID() == rkInstVictim.GetGuildID())
					return false;

			if (rkInstVictim.IsKiller())
				if (!IAbstractPlayer::GetSingleton().IsSamePartyMember(GetVirtualID(), rkInstVictim.GetVirtualID()))
					return true;

			if (PK_MODE_PROTECT != GetPKMode())
			{
				if (PK_MODE_FREE == GetPKMode())
				{
					if (PK_MODE_PROTECT != rkInstVictim.GetPKMode())
						if (!IAbstractPlayer::GetSingleton().IsSamePartyMember(GetVirtualID(), rkInstVictim.GetVirtualID()))
							return true;
				}
				if (PK_MODE_GUILD == GetPKMode())
				{
					if (PK_MODE_PROTECT != rkInstVictim.GetPKMode())
						if (!IAbstractPlayer::GetSingleton().IsSamePartyMember(GetVirtualID(), rkInstVictim.GetVirtualID()))
							if (GetGuildID() != rkInstVictim.GetGuildID())
								return true;
				}
			}

			if (IsSameEmpire(rkInstVictim))
			{
				if (IsPVPInstance(rkInstVictim))
					return true;

				if (PK_MODE_REVENGE == GetPKMode())
					if (!IAbstractPlayer::GetSingleton().IsSamePartyMember(GetVirtualID(), rkInstVictim.GetVirtualID()))
						if (IsConflictAlignmentInstance(rkInstVictim))
							return true;
			}
			else
			{
				return true;
			}
		}

		if (rkInstVictim.IsEnemy())
			return true;

		if (rkInstVictim.IsWoodenDoor())
			return true;
	}
	else if (IsEnemy())
	{
		if (rkInstVictim.IsPC())
			return true;

		if (rkInstVictim.IsBuilding())
			return true;

	}
	else if (IsPoly())
	{
		if (rkInstVictim.IsPC())
			return true;

		if (rkInstVictim.IsEnemy())
			return true;
	}
	return false;
}

bool CInstanceBase::IsTargetableInstance(CInstanceBase& rkInstVictim)
{
	return rkInstVictim.CanPickInstance();
}

bool CInstanceBase::CanChangeTarget()
{
	return m_GraphicThingInstance.CanChangeTarget();
}

bool CInstanceBase::CanPickInstance()
{
	if (!__IsInViewFrustum())
		return false;

	if (IsDoor())
	{
		if (IsDead())
			return false;
	}

	if (IsPC())
	{
		if (IsAffect(AFFECT_EUNHYEONG))
		{
			if (!__MainCanSeeHiddenThing())
				return false;
		}

#ifdef ENABLE_CANSEEHIDDENTHING_FOR_GM
		if (IsAffect(AFFECT_REVIVE_INVISIBILITY) && !__MainCanSeeHiddenThing())
			return false;
#else
		if (IsAffect(AFFECT_REVIVE_INVISIBILITY))
			return false;
#endif

#ifdef ENABLE_CANSEEHIDDENTHING_FOR_GM
		if (IsAffect(AFFECT_INVISIBILITY) && !__MainCanSeeHiddenThing())
			return false;
#else
		if (IsAffect(AFFECT_INVISIBILITY))
			return false;
#endif
	}

	if (IsDead())
		return false;

	return true;
}

bool CInstanceBase::CanViewTargetHP(CInstanceBase& rkInstVictim)
{
	if (rkInstVictim.IsStone())
		return true;
	if (rkInstVictim.IsWoodenDoor())
		return true;
	if (rkInstVictim.IsEnemy())
		return true;
#ifdef ENABLE_VIEW_TARGET_PLAYER_HP
	if (rkInstVictim.IsPC())
		return true;
#endif
#ifdef ENABLE_SHIP_DEFENCE_DUNGEON
	if (rkInstVictim.IsHydraNPC())
		return true;
#endif

	return false;
}

BOOL CInstanceBase::IsPoly()
{
	return m_GraphicThingInstance.IsPoly();
}

#ifdef ENABLE_RENEWAL_OFFLINESHOP
BOOL CInstanceBase::IsShop()
{
	return m_GraphicThingInstance.IsShop();
}
#endif

BOOL CInstanceBase::IsPC()
{
	return m_GraphicThingInstance.IsPC();
}

BOOL CInstanceBase::IsNPC()
{
	return m_GraphicThingInstance.IsNPC();
}

BOOL CInstanceBase::IsEnemy()
{
	return m_GraphicThingInstance.IsEnemy();
}

#ifdef BOSS_MARK_SYSTEM
BOOL CInstanceBase::IsBoss()
{
	const CPythonNonPlayer::TMobTable* pkTab = CPythonNonPlayer::Instance().GetTable(GetRace());

	if (!pkTab)
		return false;

	if (pkTab->bRank >= 4 && !IsStone() && pkTab->bType == 0)
		return TRUE;

	return FALSE;
}
#endif

BOOL CInstanceBase::IsStone()
{
	return m_GraphicThingInstance.IsStone();
}

BOOL CInstanceBase::IsHorse()
{
	return m_GraphicThingInstance.IsHorse();
}

BOOL CInstanceBase::IsPet()
{
	return m_GraphicThingInstance.IsPet();
}

#ifdef ENABLE_GROWTH_PET_SYSTEM
BOOL CInstanceBase::IsGrowthPet()
{
	return m_GraphicThingInstance.IsGrowthPet();
}
#endif

BOOL CInstanceBase::IsMount()
{
	return m_GraphicThingInstance.IsMount();
}

BOOL CInstanceBase::IsGuildWall()
{
	return IsWall(m_dwRace);
}

BOOL CInstanceBase::IsResource()
{
	switch (m_dwVirtualNumber)
	{
	case 20047:
	case 20048:
	case 20049:
	case 20050:
	case 20051:
	case 20052:
	case 20053:
	case 20054:
	case 20055:
	case 20056:
	case 20057:
	case 20058:
	case 20059:
	case 30301:
	case 30302:
	case 30303:
	case 30304:
	case 30305:
		return TRUE;
	}

	return FALSE;
}

BOOL CInstanceBase::IsWarp()
{
	return m_GraphicThingInstance.IsWarp();
}

BOOL CInstanceBase::IsGoto()
{
	return m_GraphicThingInstance.IsGoto();
}

BOOL CInstanceBase::IsObject()
{
	return m_GraphicThingInstance.IsObject();
}

BOOL CInstanceBase::IsBuilding()
{
	return m_GraphicThingInstance.IsBuilding();
}

BOOL CInstanceBase::IsDoor()
{
	return m_GraphicThingInstance.IsDoor();
}

BOOL CInstanceBase::IsWoodenDoor()
{
	if (m_GraphicThingInstance.IsDoor())
	{
		int vnum = GetVirtualNumber();
		if (vnum == 13000)
			return true;
		else if (vnum >= 30111 && vnum <= 30119)
			return true;
		else
			return false;
	}
	else
	{
		return false;
	}
}

BOOL CInstanceBase::IsStoneDoor()
{
	return m_GraphicThingInstance.IsDoor() && 13001 == GetVirtualNumber();
}

BOOL CInstanceBase::IsFlag()
{
	if (GetRace() == 20035)
		return TRUE;
	if (GetRace() == 20036)
		return TRUE;
	if (GetRace() == 20037)
		return TRUE;

	return FALSE;
}

BOOL CInstanceBase::IsForceVisible()
{
	if (IsAffect(AFFECT_SHOW_ALWAYS))
		return TRUE;

	if (IsObject() || IsBuilding() || IsDoor())
		return TRUE;

	return FALSE;
}

int	CInstanceBase::GetInstanceType()
{
	return m_GraphicThingInstance.GetActorType();
}

DWORD CInstanceBase::GetVirtualID()
{
	return m_GraphicThingInstance.GetVirtualID();
}

DWORD CInstanceBase::GetVirtualNumber()
{
	return m_dwVirtualNumber;
}

bool CInstanceBase::__IsInViewFrustum()
{
	return m_GraphicThingInstance.isShow();
}

bool CInstanceBase::__CanRender()
{
	if (!__IsInViewFrustum())
		return false;

#ifdef ENABLE_RENDER_TARGET
	if (IsAlwaysRender())
		return true;
#endif

#ifdef ENABLE_CANSEEHIDDENTHING_FOR_GM
	if (IsAffect(AFFECT_INVISIBILITY) && !__MainCanSeeHiddenThing())
		return false;
#else
	if (IsAffect(AFFECT_INVISIBILITY))
		return false;
#endif

	return true;
}

bool CInstanceBase::IntersectBoundingBox()
{
	float u, v, t;
	return m_GraphicThingInstance.Intersect(&u, &v, &t);
}

bool CInstanceBase::IntersectDefendingSphere()
{
	return m_GraphicThingInstance.IntersectDefendingSphere();
}

float CInstanceBase::GetDistance(CInstanceBase* pkTargetInst)
{
	TPixelPosition TargetPixelPosition;
	pkTargetInst->m_GraphicThingInstance.GetPixelPosition(&TargetPixelPosition);
	return GetDistance(TargetPixelPosition);
}

float CInstanceBase::GetDistance(const TPixelPosition& c_rPixelPosition)
{
	TPixelPosition PixelPosition;
	m_GraphicThingInstance.GetPixelPosition(&PixelPosition);

	float fdx = PixelPosition.x - c_rPixelPosition.x;
	float fdy = PixelPosition.y - c_rPixelPosition.y;

	return sqrtf((fdx * fdx) + (fdy * fdy));
}

CActorInstance& CInstanceBase::GetGraphicThingInstanceRef()
{
	return m_GraphicThingInstance;
}

CActorInstance* CInstanceBase::GetGraphicThingInstancePtr()
{
	return &m_GraphicThingInstance;
}

void CInstanceBase::RefreshActorInstance()
{
	m_GraphicThingInstance.RefreshActorInstance();
}

void CInstanceBase::Refresh(DWORD dwMotIndex, bool isLoop)
{
	RefreshState(dwMotIndex, isLoop);
}

void CInstanceBase::RestoreRenderMode()
{
	m_GraphicThingInstance.RestoreRenderMode();
}

void CInstanceBase::SetAddRenderMode()
{
	m_GraphicThingInstance.SetAddRenderMode();
}

void CInstanceBase::SetModulateRenderMode()
{
	m_GraphicThingInstance.SetModulateRenderMode();
}

void CInstanceBase::SetRenderMode(int iRenderMode)
{
	m_GraphicThingInstance.SetRenderMode(iRenderMode);
}

void CInstanceBase::SetAddColor(const D3DXCOLOR& c_rColor)
{
	m_GraphicThingInstance.SetAddColor(c_rColor);
}

void CInstanceBase::__SetBlendRenderingMode()
{
	m_GraphicThingInstance.SetBlendRenderMode();
}

void CInstanceBase::__SetAlphaValue(float fAlpha)
{
	m_GraphicThingInstance.SetAlphaValue(fAlpha);
}

float CInstanceBase::__GetAlphaValue()
{
	return m_GraphicThingInstance.GetAlphaValue();
}

void CInstanceBase::SetHair(DWORD eHair)
{
	if (!HAIR_COLOR_ENABLE)
		return;

	if (IsPC() == false)
		return;

	m_awPart[CRaceData::PART_HAIR] = eHair;

#ifdef ENABLE_HAIR_SPECULAR
	float fSpecularPower = 0.0f;

	static std::unordered_map<DWORD, float> setAllowedHair = {
		{5057, 50.0f}, // Azrael's Helmet
	};

	auto it = setAllowedHair.find(eHair);
	if (it != setAllowedHair.end())
		fSpecularPower = it->second;
#endif

	m_GraphicThingInstance.SetHair(eHair
#ifdef ENABLE_HAIR_SPECULAR
		, fSpecularPower
#endif
	);
}

void CInstanceBase::ChangeHair(DWORD eHair)
{
	if (!HAIR_COLOR_ENABLE)
		return;

	if (IsPC() == false)
		return;

	if (GetPart(CRaceData::PART_HAIR) == eHair)
		return;

	SetHair(eHair);

	RefreshState(CRaceMotionData::NAME_WAIT, true);
}

void CInstanceBase::SetArmor(DWORD dwArmor)
{
	DWORD dwShape;
	if (__ArmorVnumToShape(dwArmor, &dwShape))
	{
		CItemData* pItemData;
		if (CItemManager::Instance().GetItemDataPointer(dwArmor, &pItemData))
		{
			float fSpecularPower = pItemData->GetSpecularPowerf();
			SetShape(dwShape, fSpecularPower);
			__GetRefinedEffect(pItemData);
#ifdef ENABLE_SHINING_SYSTEM
			__GetShiningEffect(pItemData);
#endif
			return;
		}
		else
			__ClearArmorRefineEffect();
#ifdef ENABLE_SHINING_SYSTEM
		__ClearArmorShiningEffect();
#endif
	}

	SetShape(dwArmor);
}

#ifdef ENABLE_RENDER_TARGET
DWORD CInstanceBase::GetArmor()
{
	return GetPart(CRaceData::PART_MAIN);
}

DWORD CInstanceBase::GetHair()
{
	return GetPart(CRaceData::PART_HAIR);
}

DWORD CInstanceBase::GetWeapon()
{
	return GetPart(CRaceData::PART_WEAPON);
}
void CInstanceBase::SetEffect()
{
	GetGraphicThingInstanceRef().RenderAllAttachingEffect();
	Refresh(CRaceMotionData::NAME_WAIT, true);
}
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
DWORD CInstanceBase::GetAcce()
{
	return GetPart(CRaceData::PART_ACCE);
}
#endif
#endif

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
static double GetMountAccePosPlushZ(int mount_vnum, int race)
{
	if (mount_vnum >= 20246 && mount_vnum <= 20249)
	{
		switch (race)
		{
		case NPlayerData::MAIN_RACE_WARRIOR_M:
		case NPlayerData::MAIN_RACE_SURA_M:
		case NPlayerData::MAIN_RACE_SHAMAN_M:
		case NPlayerData::MAIN_RACE_WARRIOR_W:
			return 25.0;
		default:
			return 35.0;
		}
	}
	else if (mount_vnum >= 20250 && mount_vnum <= 20251)
	{
		switch (race)
		{
		case NPlayerData::MAIN_RACE_WARRIOR_M:
		case NPlayerData::MAIN_RACE_SURA_M:
			return 5;
		case NPlayerData::MAIN_RACE_SHAMAN_M:
		case NPlayerData::MAIN_RACE_WARRIOR_W:
			return 10.0;
		default:
			return 15.0;
		}
	}
	return 0;
}

void CInstanceBase::SetAcce(DWORD dwAcce)
{
	if (!IsPC() || IsPoly() || IsWearingDress() || __IsShapeAnimalWear())
		return;

	dwAcce += 85000;
	ClearAcceEffect();
#ifdef ENABLE_SHINING_SYSTEM
	__ClearAcceShiningEffect();
#endif

	if (dwAcce == 0)
	{
		m_GraphicThingInstance.AttachAcce(nullptr, 0);
		return;
	}

	float fSpecular = 65.0f;
	//if (dwAcce > 87000)
	//{
	//	dwAcce -= 2000;
	//	fSpecular += 35;
	//
	//	m_dwAcceEffect = EFFECT_REFINED + EFFECT_ACCE;
	//	__EffectContainer_AttachEffect(m_dwAcceEffect);
	//}
	
	
	if (86076 <= dwAcce && dwAcce <= 86086)
	{
		fSpecular += 35;
	
		m_dwAcceEffect = EFFECT_REFINED + EFFECT_WINGS_NEWS2;
		__EffectContainer_AttachEffect(m_dwAcceEffect);
	}

	fSpecular /= 100.0f;

	m_awPart[CRaceData::PART_ACCE] = dwAcce;

	CItemData* pItemData = nullptr;
	if (!CItemManager::Instance().GetItemDataPointer(dwAcce, &pItemData))

		return;

#ifdef ENABLE_SHINING_SYSTEM

	else

		__GetShiningEffect(pItemData);

#endif

	m_GraphicThingInstance.AttachAcce(pItemData, fSpecular);

#ifdef ENABLE_OBJ_SCALLING
	DWORD dwRace = GetRace(), dwPos = RaceToJob(dwRace), dwSex = RaceToSex(dwRace);
	dwPos += 1;
	if (dwSex == 0)
		dwPos += 5;

	float fScaleX, fScaleY, fScaleZ, fPositionX, fPositionY, fPositionZ;
	if (pItemData && pItemData->GetItemScale(dwPos, fScaleX, fScaleY, fScaleZ, fPositionX, fPositionY, fPositionZ))
	{
		m_GraphicThingInstance.SetScale(fScaleX, fScaleY, fScaleZ, true);
		if (m_kHorse.IsMounting())
		{
			fPositionZ += 10.0f;
			auto pHorse = m_kHorse.GetActorPtr();
			if (pHorse && pHorse->GetRace())
				fPositionZ += GetMountAccePosPlushZ(pHorse->GetRace(), dwRace);
		}

		m_GraphicThingInstance.SetScalePosition(fPositionX, fPositionY, fPositionZ);
	}
#endif
}

void CInstanceBase::ChangeAcce(DWORD dwAcce)
{
	if (!IsPC())
		return;

	SetAcce(dwAcce);
}

void CInstanceBase::ClearAcceEffect()
{ 
	if (!m_dwAcceEffect)
		return;

	__EffectContainer_DetachEffect(m_dwAcceEffect);
	m_dwAcceEffect = 0;
}
#endif

void CInstanceBase::SetShape(DWORD eShape, float fSpecular)
{
	if (IsPoly())
	{
		m_GraphicThingInstance.SetShape(0);
	}
	else
	{
#ifdef ENABLE_MONSTER_SPECULAR
		if (!IsPC() && !fSpecular)
			fSpecular = CPythonNonPlayer::Instance().GetMonsterSpecular(m_dwRace);
#endif
		m_GraphicThingInstance.SetShape(eShape, fSpecular);
	}

	m_eShape = eShape;
}

DWORD CInstanceBase::GetWeaponType()
{
	DWORD dwWeapon = GetPart(CRaceData::PART_WEAPON);
	CItemData* pItemData;

	if (!CItemManager::Instance().GetItemDataPointer(dwWeapon, &pItemData))
		return CItemData::WEAPON_NONE;

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	if (pItemData->GetType() == CItemData::ITEM_TYPE_COSTUME)
		return pItemData->GetValue(3);
#endif

	return pItemData->GetWeaponType();
}

void CInstanceBase::__ClearWeaponRefineEffect()
{
	if (m_swordRefineEffectRight)
	{
		__DetachEffect(m_swordRefineEffectRight);
		m_swordRefineEffectRight = 0;
	}
	if (m_swordRefineEffectLeft)
	{
		__DetachEffect(m_swordRefineEffectLeft);
		m_swordRefineEffectLeft = 0;
	}

#ifdef ENABLE_MDE_EFFECT
	if (m_dwSpecialEffect)
	{
		__DetachEffect(m_dwSpecialEffect);
		__DetachEffect(m_dwSpecialEffectLeft);

		m_dwSpecialEffect = 0;
		m_dwSpecialEffectLeft = 0;
	}
#endif
}

void CInstanceBase::__ClearArmorRefineEffect()
{
	if (m_armorRefineEffect)
	{
		__DetachEffect(m_armorRefineEffect);
		m_armorRefineEffect = 0;
	}
}

UINT CInstanceBase::__GetRefinedEffect(CItemData* pItem)
{
	DWORD refine = max(pItem->GetRefine() + pItem->GetSocketCount(), CItemData::ITEM_SOCKET_MAX_NUM) - CItemData::ITEM_SOCKET_MAX_NUM;
	DWORD vnum = pItem->GetIndex();

	switch (pItem->GetType())
	{
	case CItemData::ITEM_TYPE_WEAPON:
		__ClearWeaponRefineEffect();

		if (refine < 7)
			return 0;

		switch (pItem->GetSubType())
		{
		case CItemData::WEAPON_DAGGER:
			m_swordRefineEffectRight = EFFECT_REFINED + EFFECT_SMALLSWORD_REFINED7 + refine - 7;
			m_swordRefineEffectLeft = EFFECT_REFINED + EFFECT_SMALLSWORD_REFINED7_LEFT + refine - 7;
			break;

		case CItemData::WEAPON_FAN:
			m_swordRefineEffectRight = EFFECT_REFINED + EFFECT_FANBELL_REFINED7 + refine - 7;
			break;

		case CItemData::WEAPON_ARROW:
		case CItemData::WEAPON_BELL:
			m_swordRefineEffectRight = EFFECT_REFINED + EFFECT_SMALLSWORD_REFINED7 + refine - 7;
			break;

		case CItemData::WEAPON_BOW:
			m_swordRefineEffectRight = EFFECT_REFINED + EFFECT_BOW_REFINED7 + refine - 7;
			break;

		default:
			m_swordRefineEffectRight = EFFECT_REFINED + EFFECT_SWORD_REFINED7 + refine - 7;
		}

		if (m_swordRefineEffectRight)
			m_swordRefineEffectRight = __AttachEffect(m_swordRefineEffectRight);

		if (m_swordRefineEffectLeft)
			m_swordRefineEffectLeft = __AttachEffect(m_swordRefineEffectLeft);

		break;

	case CItemData::ITEM_TYPE_COSTUME:
		__ClearArmorRefineEffect();

		if (pItem->GetSubType() == CItemData::COSTUME_BODY)
		{
			DWORD vnum = pItem->GetIndex();

			if (!shiningdata.empty())
			{
				for (shiningit = shiningdata.begin(); shiningit != shiningdata.end(); shiningit++)
					if (shiningit->first == vnum)
					{
						std::string substr(shiningit->second);
						std::vector<std::string> chars;
						boost::split(chars, substr, boost::is_any_of("#"));
						for (std::vector<std::string>::size_type i = 0; i != chars.size(); i++)
						{
							__AttachEffectToArmours(chars[i]);
						}
					}
			}
		}

	case CItemData::ITEM_TYPE_ARMOR:
		__ClearArmorRefineEffect();

		if (12010 <= vnum && vnum <= 12049)
		{
			__AttachEffect(EFFECT_REFINED + EFFECT_BODYARMOR_SPECIAL);
			__AttachEffect(EFFECT_REFINED + EFFECT_BODYARMOR_SPECIAL2);
		}
		//if (86065 == vnum)
		//{
		//	__AttachEffect(EFFECT_REFINED + EFFECT_WINGS_NEWS1);
		//}
		if (!shiningdata.empty())
		{
			for (shiningit = shiningdata.begin(); shiningit != shiningdata.end(); shiningit++)
				if (shiningit->first == vnum)
				{
					std::string substr(shiningit->second);
					std::vector<std::string> chars;
					boost::split(chars, substr, boost::is_any_of("#"));
					for (std::vector<std::string>::size_type i = 0; i != chars.size(); i++)
					{
						__AttachEffectToArmours(chars[i]);
					}
				}
		}

		if (refine < 7)
			return 0;

		if (pItem->GetSubType() == CItemData::ARMOR_BODY)
		{
			m_armorRefineEffect = EFFECT_REFINED + EFFECT_BODYARMOR_REFINED7 + refine - 7;
			__AttachEffect(m_armorRefineEffect);
		}
		break;
	}
	return 0;
}

bool CInstanceBase::SetWeapon(DWORD eWeapon)
{
	if (IsPoly())
	{
		return false;
	}

	if (__IsShapeAnimalWear())
	{
		return false;
	}

	if (__IsChangableWeapon(eWeapon) == false)
	{
		eWeapon = 0;
	}



	m_GraphicThingInstance.AttachWeapon(eWeapon);
	m_awPart[CRaceData::PART_WEAPON] = eWeapon;

	CItemData* pItemData;
	if (CItemManager::Instance().GetItemDataPointer(eWeapon, &pItemData))
	{
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
		if (pItemData->GetType() == CItemData::ITEM_TYPE_COSTUME)
			__ClearWeaponShiningEffect();
		//			__ClearWeaponRefineEffect();
#endif
		__GetRefinedEffect(pItemData);
#ifdef ENABLE_SHINING_SYSTEM
		__GetShiningEffect(pItemData);
#endif
	}
	else
	{
		__ClearWeaponRefineEffect();
#ifdef ENABLE_SHINING_SYSTEM
		__ClearWeaponShiningEffect();
#endif
	}
	return true;
}

void CInstanceBase::ChangeWeapon(DWORD eWeapon)
{
	if (eWeapon == m_GraphicThingInstance.GetPartItemID(CRaceData::PART_WEAPON))
		return;

	if (SetWeapon(eWeapon))
		RefreshState(CRaceMotionData::NAME_WAIT, true);
}

bool CInstanceBase::ChangeArmor(DWORD dwArmor)
{
	DWORD eShape;
	__ArmorVnumToShape(dwArmor, &eShape);

	if (GetShape() == eShape)
		return false;

	CAffectFlagContainer kAffectFlagContainer;
	kAffectFlagContainer.CopyInstance(m_kAffectFlagContainer);

	DWORD dwVID = GetVirtualID();
	DWORD dwRace = GetRace();
	DWORD eHair = GetPart(CRaceData::PART_HAIR);
	DWORD eWeapon = GetPart(CRaceData::PART_WEAPON);
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	DWORD dwAcce = GetPart(CRaceData::PART_ACCE);
#endif
#ifdef ENABLE_AURA_COSTUME_SYSTEM
	DWORD eAura = GetPart(CRaceData::PART_AURA);
#endif

	float fRot = GetRotation();
	float fAdvRot = GetAdvancingRotation();

	if (IsWalking())
		EndWalking();

	__ClearAffects();

	if (!SetRace(dwRace))
	{
		TraceError("CPythonCharacterManager::ChangeArmor - SetRace VID[%d] Race[%d] ERROR", dwVID, dwRace);
		return false;
	}

	SetArmor(dwArmor);
	SetHair(eHair);
	SetWeapon(eWeapon);
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	SetAcce(dwAcce);
#endif
#ifdef ENABLE_AURA_COSTUME_SYSTEM
	SetAura(eAura);
#endif

	SetRotation(fRot);
	SetAdvancingRotation(fAdvRot);

	__AttachHorseSaddle();

	RefreshState(CRaceMotionData::NAME_WAIT, TRUE);
	SetAffectFlagContainer(kAffectFlagContainer);

	CActorInstance::IEventHandler& rkEventHandler = GetEventHandlerRef();
	rkEventHandler.OnChangeShape();

	return true;
}

bool CInstanceBase::__IsShapeAnimalWear()
{
	if (100 == GetShape() ||
		101 == GetShape() ||
		102 == GetShape() ||
		103 == GetShape())
		return true;

	return false;
}

DWORD CInstanceBase::__GetRaceType()
{
	return m_eRaceType;
}


void CInstanceBase::RefreshState(DWORD dwMotIndex, bool isLoop)
{
	DWORD dwPartItemID = m_GraphicThingInstance.GetPartItemID(CRaceData::PART_WEAPON);

	BYTE byItemType = 0xff;
	BYTE bySubType = 0xff;

	CItemManager& rkItemMgr = CItemManager::Instance();
	CItemData* pItemData;

	if (rkItemMgr.GetItemDataPointer(dwPartItemID, &pItemData))
	{
		byItemType = pItemData->GetType();
		bySubType = pItemData->GetWeaponType();
	}

	if (IsPoly())
	{
		SetMotionMode(CRaceMotionData::MODE_GENERAL);
	}
	else if (IsWearingDress())
	{
		SetMotionMode(CRaceMotionData::MODE_WEDDING_DRESS);
	}
	else if (IsHoldingPickAxe())
	{
		if (m_kHorse.IsMounting())
		{
			SetMotionMode(CRaceMotionData::MODE_HORSE);
		}
		else
		{
			SetMotionMode(CRaceMotionData::MODE_GENERAL);
		}
	}
	else if (CItemData::ITEM_TYPE_ROD == byItemType)
	{
		if (m_kHorse.IsMounting())
		{
			SetMotionMode(CRaceMotionData::MODE_HORSE);
		}
		else
		{
			SetMotionMode(CRaceMotionData::MODE_FISHING);
		}
	}
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	else if (byItemType == CItemData::ITEM_TYPE_COSTUME)
	{
		switch (pItemData->GetValue(3))
		{
		case CItemData::WEAPON_SWORD:
			if (m_kHorse.IsMounting())
				SetMotionMode(CRaceMotionData::MODE_HORSE_ONEHAND_SWORD);
			else
				SetMotionMode(CRaceMotionData::MODE_ONEHAND_SWORD);
			break;
		case CItemData::WEAPON_DAGGER:
			if (m_kHorse.IsMounting())
				SetMotionMode(CRaceMotionData::MODE_HORSE_DUALHAND_SWORD);
			else
				SetMotionMode(CRaceMotionData::MODE_DUALHAND_SWORD);
			break;
		case CItemData::WEAPON_BOW:
			if (m_kHorse.IsMounting())
				SetMotionMode(CRaceMotionData::MODE_HORSE_BOW);
			else
#ifdef ENABLE_NEW_ARROW_SYSTEM
			{
				if (m_awPart[CRaceData::PART_ARROW_TYPE] == CItemData::WEAPON_UNLIMITED_ARROW)
					SetMotionMode(CRaceMotionData::MODE_BOW_SPECIAL);
				else
					SetMotionMode(CRaceMotionData::MODE_BOW);
			}
#else
				SetMotionMode(CRaceMotionData::MODE_BOW);
#endif
			break;
		case CItemData::WEAPON_TWO_HANDED:
			if (m_kHorse.IsMounting())
				SetMotionMode(CRaceMotionData::MODE_HORSE_TWOHAND_SWORD);
			else
				SetMotionMode(CRaceMotionData::MODE_TWOHAND_SWORD);
			break;
		case CItemData::WEAPON_BELL:
			if (m_kHorse.IsMounting())
				SetMotionMode(CRaceMotionData::MODE_HORSE_BELL);
			else
				SetMotionMode(CRaceMotionData::MODE_BELL);
			break;
		case CItemData::WEAPON_FAN:
			if (m_kHorse.IsMounting())
				SetMotionMode(CRaceMotionData::MODE_HORSE_FAN);
			else
				SetMotionMode(CRaceMotionData::MODE_FAN);
			break;
		default:
			if (m_kHorse.IsMounting())
				SetMotionMode(CRaceMotionData::MODE_HORSE);
			else
				SetMotionMode(CRaceMotionData::MODE_GENERAL);
			break;
		}
	}
#endif
	else if (m_kHorse.IsMounting())
	{
		switch (bySubType)
		{
		case CItemData::WEAPON_SWORD:
			SetMotionMode(CRaceMotionData::MODE_HORSE_ONEHAND_SWORD);
			break;

		case CItemData::WEAPON_TWO_HANDED:
			SetMotionMode(CRaceMotionData::MODE_HORSE_TWOHAND_SWORD); // Only Warrior
			break;

		case CItemData::WEAPON_DAGGER:
			SetMotionMode(CRaceMotionData::MODE_HORSE_DUALHAND_SWORD); // Only Assassin
			break;

		case CItemData::WEAPON_FAN:
			SetMotionMode(CRaceMotionData::MODE_HORSE_FAN); // Only Shaman
			break;

		case CItemData::WEAPON_BELL:
			SetMotionMode(CRaceMotionData::MODE_HORSE_BELL); // Only Shaman
			break;

		case CItemData::WEAPON_BOW:
			SetMotionMode(CRaceMotionData::MODE_HORSE_BOW); // Only Shaman
			break;

		default:
			SetMotionMode(CRaceMotionData::MODE_HORSE);
			break;
		}
	}
	else
	{
		switch (bySubType)
		{
		case CItemData::WEAPON_SWORD:
			SetMotionMode(CRaceMotionData::MODE_ONEHAND_SWORD);
			break;

		case CItemData::WEAPON_TWO_HANDED:
			SetMotionMode(CRaceMotionData::MODE_TWOHAND_SWORD); // Only Warrior
			break;

		case CItemData::WEAPON_DAGGER:
			SetMotionMode(CRaceMotionData::MODE_DUALHAND_SWORD); // Only Assassin
			break;

		case CItemData::WEAPON_BOW:
			SetMotionMode(CRaceMotionData::MODE_BOW); // Only Assassin
			break;

		case CItemData::WEAPON_FAN:
			SetMotionMode(CRaceMotionData::MODE_FAN); // Only Shaman
			break;

		case CItemData::WEAPON_BELL:
			SetMotionMode(CRaceMotionData::MODE_BELL); // Only Shaman
			break;

		case CItemData::WEAPON_ARROW:
		default:
			SetMotionMode(CRaceMotionData::MODE_GENERAL);
			break;
		}
	}

	if (isLoop)
		m_GraphicThingInstance.InterceptLoopMotion(dwMotIndex);
	else
		m_GraphicThingInstance.InterceptOnceMotion(dwMotIndex);

	RefreshActorInstance();
}

void CInstanceBase::RegisterBoundingSphere()
{
	if (!IsStone())
	{
		m_GraphicThingInstance.DeformNoSkin();
	}

	m_GraphicThingInstance.RegisterBoundingSphere();
}

bool CInstanceBase::CreateDeviceObjects()
{
	return m_GraphicThingInstance.CreateDeviceObjects();
}

void CInstanceBase::DestroyDeviceObjects()
{
	m_GraphicThingInstance.DestroyDeviceObjects();
}

void CInstanceBase::Destroy()
{
	DetachTextTail();

	DismountHorse();

	m_kQue_kCmdNew.clear();

	__EffectContainer_Destroy();
	__StoneSmoke_Destroy();

	if (__IsMainInstance())
		__ClearMainInstance();

	m_GraphicThingInstance.Destroy();

	__Initialize();
}

void CInstanceBase::__InitializeRotationSpeed()
{
	SetRotationSpeed(c_fDefaultRotationSpeed);
}

void CInstanceBase::__Warrior_Initialize()
{
	m_kWarrior.m_dwGeomgyeongEffect = 0;
}

void CInstanceBase::__Initialize()
{
	__Warrior_Initialize();
	__StoneSmoke_Inialize();
	__EffectContainer_Initialize();
	__InitializeRotationSpeed();

	SetEventHandler(CActorInstance::IEventHandler::GetEmptyPtr());

	m_kAffectFlagContainer.Clear();

	m_dwLevel = 0;
#ifdef ENABLE_SHOW_MOB_INFO
	m_dwAIFlag = 0;
#endif
	m_dwGuildID = 0;
	m_dwEmpireID = 0;
#ifdef ENABLE_GUILD_LEADER_TEXTAIL
	m_dwGuildLeader = 0;
#endif

	m_eType = 0;
	m_eRaceType = 0;
	m_eShape = 0;
	m_dwRace = 0;
	m_dwVirtualNumber = 0;

	m_dwBaseCmdTime = 0;
	m_dwBaseChkTime = 0;
	m_dwSkipTime = 0;

	m_GraphicThingInstance.Initialize();

	m_dwAdvActorVID = 0;
	m_dwLastDmgActorVID = 0;

	m_nAverageNetworkGap = 0;
	m_dwNextUpdateHeightTime = 0;

	m_iRotatingDirection = DEGREE_DIRECTION_SAME;

	m_isTextTail = FALSE;
	m_isGoing = FALSE;
	NEW_SetSrcPixelPosition(TPixelPosition(0, 0, 0));
	NEW_SetDstPixelPosition(TPixelPosition(0, 0, 0));

	m_kPPosDust = TPixelPosition(0, 0, 0);


	m_kQue_kCmdNew.clear();

	m_dwLastComboIndex = 0;

	m_swordRefineEffectRight = 0;
	m_swordRefineEffectLeft = 0;
	m_armorRefineEffect = 0;
#ifdef ENABLE_SHINING_SYSTEM
	__ClearWeaponShiningEffect(false);
	__ClearArmorShiningEffect(false);
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	__ClearAcceShiningEffect(false);
#endif
#endif
#ifdef ENABLE_MDE_EFFECT
	m_dwSpecialEffect = 0;
	m_dwSpecialEffectLeft = 0;
#endif

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	m_dwAcceEffect = 0;
#endif

#ifdef ENABLE_AURA_COSTUME_SYSTEM
	m_auraRefineEffect = 0;
#endif

	m_sAlignment = 0;
#ifdef ENABLE_TITLE_SYSTEM
	m_iTitleID = 0;
#endif
	m_byPKMode = 0;
	m_isKiller = false;
	m_isPartyMember = false;

	m_bEnableTCPState = TRUE;

	m_stName = "";

	memset(m_awPart, 0, sizeof(m_awPart));
	memset(m_adwCRCAffectEffect, 0, sizeof(m_adwCRCAffectEffect));
	memset(&m_kMovAfterFunc, 0, sizeof(m_kMovAfterFunc));

	m_bDamageEffectType = false;
	m_dwDuelMode = DUEL_NONE;
	m_dwEmoticonTime = 0;
#ifdef ENABLE_ACHIEVEMENT_SYSTEM
	m_dwAchievementTitle = 0;
#endif
#ifdef ENABLE_RENDER_TARGET
	m_IsAlwaysRender = false;
#endif

#ifdef ENABLE_GROWTH_PET_SYSTEM
	m_bCharacterSize = 0;
#endif
}

CInstanceBase::CInstanceBase()
{
	__Initialize();
}

CInstanceBase::~CInstanceBase()
{
	Destroy();
}

DWORD CInstanceBase::GetPartData(BYTE partIndex)
{
	if (partIndex >= _countof(m_awPart))
		return 0;

	return m_awPart[partIndex];
}

void CInstanceBase::GetBoundBox(D3DXVECTOR3* vtMin, D3DXVECTOR3* vtMax)
{
	m_GraphicThingInstance.GetBoundBox(vtMin, vtMax);
}

float CInstanceBase::GetBaseHeight()
{
	CActorInstance* pkHorse = m_kHorse.GetActorPtr();
	if (!m_kHorse.IsMounting() || !pkHorse)
		return 0.0f;

	DWORD dwHorseVnum = m_kHorse.m_pkActor->GetRace();
	if ((dwHorseVnum >= 20101 && dwHorseVnum <= 20109) || (dwHorseVnum == 20029 || dwHorseVnum == 20030))
		return 100.0f;

	if ((dwHorseVnum >= 20110 && dwHorseVnum <= 20125) || (dwHorseVnum >= 20201 && dwHorseVnum <= 20281))
		return 50.0f;

	float fRaceHeight = CRaceManager::instance().GetRaceHeight(dwHorseVnum);
	if (fRaceHeight == 0.0f)
		return 100.0f;
	else
		return fRaceHeight;
}

#ifdef ENABLE_RENDER_TARGET
bool CInstanceBase::IsAlwaysRender()
{
	return m_IsAlwaysRender;
}

void CInstanceBase::SetAlwaysRender(bool val)
{
	m_IsAlwaysRender = val;
}

void CInstanceBase::AttachWikiAffect(DWORD effectIndex)
{
	const DWORD newAffect = __AttachEffect(effectIndex);
	m_vecWikiEffects.push_back(newAffect);
}

void CInstanceBase::RemoveWikiAffect(DWORD effectIndex)
{
	const auto it = std::find(m_vecWikiEffects.begin(), m_vecWikiEffects.end(), effectIndex);
	if (it != m_vecWikiEffects.end())
	{
		__DetachEffect(effectIndex);
		m_vecWikiEffects.erase(it);
	}
}

void CInstanceBase::ClearWikiAffect()
{
	for (const auto& effectIndex : m_vecWikiEffects)
		__DetachEffect(effectIndex);
	m_vecWikiEffects.clear();
}
#endif

#ifdef ENABLE_STONE_SCALE_OPTION
void CInstanceBase::RefreshStoneScale()
{
	if (!IsStone())
		return;

	const float StoneScale = 1 + CPythonSystem::Instance().GetStoneScale();
	m_GraphicThingInstance.SetScale(StoneScale, StoneScale, StoneScale, true);
	DetachTextTail();
	AttachTextTail();
	RefreshTextTail();
}
#endif

#ifdef ENABLE_AURA_COSTUME_SYSTEM
void CInstanceBase::ChangeAura(DWORD eAura)
{
	if (m_GraphicThingInstance.GetPartItemID(CRaceData::PART_AURA) != eAura)
		SetAura(eAura);
}

bool CInstanceBase::SetAura(DWORD eAura)
{
	if (!IsPC() || IsPoly() || __IsShapeAnimalWear())
		return false;

	m_GraphicThingInstance.ChangePart(CRaceData::PART_AURA, eAura);
	if (!eAura)
	{
		if (m_auraRefineEffect)
		{
			__DetachEffect(m_auraRefineEffect);
			m_auraRefineEffect = 0;
		}
		m_awPart[CRaceData::PART_AURA] = 0;
		return true;
	}

	CItemData* pItemData;
	if (!CItemManager::Instance().GetItemDataPointer(eAura, &pItemData))
	{
		if (m_auraRefineEffect)
		{
			__DetachEffect(m_auraRefineEffect);
			m_auraRefineEffect = 0;
		}
		m_awPart[CRaceData::PART_AURA] = 0;
		return true;
	}

	BYTE byRace = (BYTE)GetRace();
	BYTE byJob = (BYTE)RaceToJob(byRace);
	BYTE bySex = (BYTE)RaceToSex(byRace);

	D3DXVECTOR3 v3MeshScale = pItemData->GetAuraMeshScaleVector(byJob, bySex);
	float fParticleScale = pItemData->GetAuraParticleScale(byJob, bySex);
	m_auraRefineEffect = m_GraphicThingInstance.AttachEffectByID(0, "Bip01 Spine2", pItemData->GetAuraEffectID(), NULL
#ifdef ENABLE_SKILL_COLOR_SYSTEM
		, NULL
#endif
		, fParticleScale, &v3MeshScale);
	m_awPart[CRaceData::PART_AURA] = eAura;
	return true;
}
#endif

#ifdef ENABLE_SHIP_DEFENCE_DUNGEON
BOOL CInstanceBase::IsHydraNPC()
{
	switch (m_dwVirtualNumber)
	{
	case 6801:
		return TRUE;
	}
	return FALSE;
}
#endif
#ifdef ENABLE_SHINING_SYSTEM

// --- Kis helper az üres szöveg ellenőrzésére
static inline bool HasText(const char* s)
{
    return s && s[0] != '\0';
}

void CInstanceBase::__GetShiningEffect(CItemData* pItem)
{
    if (!pItem)
        return;

    // Ha igaz: ha van külön shining, a refine effektet eltüntetjük alóla
    const bool removeRefineEffect = true;

    const CItemData::TItemShiningTable shiningTable = pItem->GetItemShiningTable();
    const bool hasAnyShining = shiningTable.Any();

    // -------- WEAPON / FEKETE-FEGYVER KATEGÓRIA --------
    if (pItem->GetType() == CItemData::ITEM_TYPE_WEAPON)
    {
        // Mindig pucoljuk a korábbi shiningot
        __ClearWeaponShiningEffect();

        // Csak akkor töröljük a refine effektet, ha tényleg lesz shining
        if (hasAnyShining && removeRefineEffect)
            __ClearWeaponRefineEffect();

        for (int i = 0; i < CItemData::ITEM_SHINING_MAX_COUNT; ++i)
        {
            const char* eff = shiningTable.szShinings[i];
            if (!HasText(eff))
                continue;

#ifdef ENABLE_WOLFMAN_CHARACTER
            const bool twoSided =
                pItem->GetSubType() == CItemData::WEAPON_DAGGER ||
                pItem->GetSubType() == CItemData::WEAPON_CLAW  ||
                (IsMountingHorse() && pItem->GetSubType() == CItemData::WEAPON_FAN);
#else
            const bool twoSided =
                pItem->GetSubType() == CItemData::WEAPON_DAGGER ||
                (IsMountingHorse() && pItem->GetSubType() == CItemData::WEAPON_FAN);
#endif

            // Íj: mindig bal kéz csont
            if (pItem->GetSubType() == CItemData::WEAPON_BOW)
            {
                __AttachWeaponShiningEffect(i, eff, "PART_WEAPON_LEFT");
                continue;
            }

            // Kétoldalas eset: bal+jobb
            if (twoSided)
                __AttachWeaponShiningEffect(i, eff, "PART_WEAPON_LEFT");

            // Alapeset: jobb kéz
            __AttachWeaponShiningEffect(i, eff, "PART_WEAPON");
        }
    }

    // -------- ARMOR / BODY és COSTUME_BODY --------
    if ( (pItem->GetType() == CItemData::ITEM_TYPE_ARMOR  && pItem->GetSubType() == CItemData::ARMOR_BODY) ||
         (pItem->GetType() == CItemData::ITEM_TYPE_COSTUME && pItem->GetSubType() == CItemData::COSTUME_BODY) )
    {
        __ClearArmorShiningEffect();

        if (hasAnyShining && removeRefineEffect)
            __ClearArmorRefineEffect();

        for (int i = 0; i < CItemData::ITEM_SHINING_MAX_COUNT; ++i)
        {
            const char* eff = shiningTable.szShinings[i];
            if (!HasText(eff))
                continue;

            // Headerben legyen default param "Bip01" – ha nincs, add meg itt harmadik paramként.
            __AttachArmorShiningEffect(i, eff /*, "Bip01"*/);
        }
    }

#ifdef ENABLE_ACCE_SYSTEM
    // -------- ACCE COSTUME --------
    if (pItem->GetType() == CItemData::ITEM_TYPE_COSTUME && pItem->GetSubType() == CItemData::COSTUME_ACCE)
    {
        // Shining törlés egyszer!
        __ClearAcceShiningEffect();

        // Ha van külön "refine" effekt az ACCE-hez és el akarod rejteni shining esetén,
        // itt hívd meg a refine clear-t (ha létezik):
        // if (hasAnyShining && removeRefineEffect)
        //     __ClearAcceRefineEffect();

        for (int i = 0; i < CItemData::ITEM_SHINING_MAX_COUNT; ++i)
        {
            const char* eff = shiningTable.szShinings[i];
            if (!HasText(eff))
                continue;

            __AttachAcceShiningEffect(i, eff /*, "Bip01"*/);
        }
    }
#endif // ENABLE_ACCE_SYSTEM

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
    // -------- WEAPON COSTUME --------
    if (pItem->GetType() == CItemData::ITEM_TYPE_COSTUME && pItem->GetSubType() == CItemData::COSTUME_WEAPON)
    {
        __ClearWeaponShiningEffect();

        if (hasAnyShining && removeRefineEffect)
            __ClearWeaponRefineEffect();

        // A fegyver-kosztümnél a Value[3] szokott a fegyver-subtype lenni
        const int weaponSub = pItem->GetValue(3);

        for (int i = 0; i < CItemData::ITEM_SHINING_MAX_COUNT; ++i)
        {
            const char* eff = shiningTable.szShinings[i];
            if (!HasText(eff))
                continue;

#ifdef ENABLE_WOLFMAN_CHARACTER
            const bool twoSided =
                weaponSub == CItemData::WEAPON_DAGGER ||
                weaponSub == CItemData::WEAPON_CLAW  ||
                (IsMountingHorse() && weaponSub == CItemData::WEAPON_FAN);
#else
            const bool twoSided =
                weaponSub == CItemData::WEAPON_DAGGER ||
                (IsMountingHorse() && weaponSub == CItemData::WEAPON_FAN);
#endif

            if (weaponSub == CItemData::WEAPON_BOW)
            {
                __AttachWeaponShiningEffect(i, eff, "PART_WEAPON_LEFT");
                continue;
            }

            if (twoSided)
                __AttachWeaponShiningEffect(i, eff, "PART_WEAPON_LEFT");

            __AttachWeaponShiningEffect(i, eff, "PART_WEAPON");
        }
    }
#endif // ENABLE_WEAPON_COSTUME_SYSTEM
}

void CInstanceBase::__AttachWeaponShiningEffect(int effectIndex, const char* effectFileName, const char* boneName)
{
    if (IsAffect(AFFECT_INVISIBILITY))
        return;

    if (effectIndex < 0 || effectIndex >= CItemData::ITEM_SHINING_MAX_COUNT)
        return;

    if (!HasText(effectFileName) || !HasText(boneName))
        return;

    CEffectManager::Instance().RegisterEffect(effectFileName, false, false);

    if (!strcmp(boneName, "PART_WEAPON"))
    {
        const char* rightBone = nullptr;
        m_GraphicThingInstance.GetAttachingBoneName(CRaceData::PART_WEAPON, &rightBone);
        if (HasText(rightBone))
            m_weaponShiningEffects[0][effectIndex] = m_GraphicThingInstance.AttachEffectByName(0, rightBone, effectFileName);
    }
    else if (!strcmp(boneName, "PART_WEAPON_LEFT"))
    {
        const char* leftBone = nullptr;
        m_GraphicThingInstance.GetAttachingBoneName(CRaceData::PART_WEAPON_LEFT, &leftBone);
        if (HasText(leftBone))
            m_weaponShiningEffects[1][effectIndex] = m_GraphicThingInstance.AttachEffectByName(0, leftBone, effectFileName);
    }
    else
    {
        Tracef("Invalid partname for getting attaching bone name. %s - %s", effectFileName, boneName);
    }
}

void CInstanceBase::__AttachArmorShiningEffect(int effectIndex, const char* effectFileName, const char* boneName /*= "Bip01"*/)
{
    if (IsAffect(AFFECT_INVISIBILITY))
        return;

    if (effectIndex < 0 || effectIndex >= CItemData::ITEM_SHINING_MAX_COUNT)
        return;

    if (!HasText(effectFileName) || !HasText(boneName))
    {
        Tracef("Empty data for armor shining. idx=%d, eff=%s, bone=%s",
               effectIndex, effectFileName ? effectFileName : "(null)", boneName ? boneName : "(null)");
        return;
    }

    CEffectManager::Instance().RegisterEffect(effectFileName, false, false);
    m_armorShiningEffects[effectIndex] = m_GraphicThingInstance.AttachEffectByName(0, boneName, effectFileName);
}

void CInstanceBase::__ClearWeaponShiningEffect(bool detaching /*=true*/)
{
    if (detaching)
    {
        for (int i = 0; i < CItemData::ITEM_SHINING_MAX_COUNT; ++i)
        {
            if (m_weaponShiningEffects[0][i])
                __DetachEffect(m_weaponShiningEffects[0][i]);
            if (m_weaponShiningEffects[1][i])
                __DetachEffect(m_weaponShiningEffects[1][i]);
        }
    }
    memset(&m_weaponShiningEffects, 0, sizeof(m_weaponShiningEffects));
}

void CInstanceBase::__ClearArmorShiningEffect(bool detaching /*=true*/)
{
    if (detaching)
    {
        for (int i = 0; i < CItemData::ITEM_SHINING_MAX_COUNT; ++i)
            __DetachEffect(m_armorShiningEffects[i]);
    }
    memset(&m_armorShiningEffects, 0, sizeof(m_armorShiningEffects));
}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
void CInstanceBase::__ClearAcceShiningEffect(bool detaching /*=true*/)
{
    if (detaching)
    {
        for (int i = 0; i < CItemData::ITEM_SHINING_MAX_COUNT; ++i)
            __DetachEffect(m_acceShiningEffects[i]);
    }
    memset(&m_acceShiningEffects, 0, sizeof(m_acceShiningEffects));
}
#endif // ENABLE_ACCE_COSTUME_SYSTEM

#endif // ENABLE_SHINING_SYSTEM


#ifdef ENABLE_METIN_QUEUE
void CInstanceBase::SetMetinQueueEffect(bool clear)
{
	if (!IsStone())
		return;

	if (clear)
	{
		__EffectContainer_DetachEffect(EFFECT_METIN_QUEUE);
	}
	else
	{
		__EffectContainer_AttachEffect(EFFECT_METIN_QUEUE);
	}
}
#endif

#ifdef ENABLE_METIN_QUEUE
BOOL CInstanceBase::IsMetinQueue()
{
	if (m_kAffectFlagContainer.IsSet(AFFECT_METIN_QUEUE))
		return 1;

	return 0;
}
#endif