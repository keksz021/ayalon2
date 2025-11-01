#include "StdAfx.h"
#include "EffectInstance.h"
#include "ParticleSystemInstance.h"
#include "SimpleLightInstance.h"

#include "../eterBase/Stl.h"
#include "../eterLib/StateManager.h"
#include "../MilesLib/SoundManager.h"

CDynamicPool<CEffectInstance>	CEffectInstance::ms_kPool;
int CEffectInstance::ms_iRenderingEffectCount = 0;

bool CEffectInstance::LessRenderOrder(CEffectInstance* pkEftInst)
{
	return (m_pkEftData<pkEftInst->m_pkEftData);	
}

void CEffectInstance::ResetRenderingEffectCount()
{
	ms_iRenderingEffectCount = 0;
}

int CEffectInstance::GetRenderingEffectCount()
{
	return ms_iRenderingEffectCount;
}

CEffectInstance* CEffectInstance::New()
{
	CEffectInstance* pkEftInst=ms_kPool.Alloc();
	return pkEftInst;
}

void CEffectInstance::Delete(CEffectInstance* pkEftInst)
{
	pkEftInst->Clear();
	ms_kPool.Free(pkEftInst);
}

void CEffectInstance::DestroySystem()
{
	ms_kPool.Destroy();

	CParticleSystemInstance::DestroySystem();
	CEffectMeshInstance::DestroySystem();
	CLightInstance::DestroySystem();
}

void CEffectInstance::UpdateSound()
{
	if (m_pSoundInstanceVector)
	{
		CSoundManager& rkSndMgr=CSoundManager::Instance();
		rkSndMgr.UpdateSoundInstance(m_matGlobal._41, m_matGlobal._42, m_matGlobal._43, m_dwFrame, m_pSoundInstanceVector);
		// NOTE : ¸ÅÆ®¸¯½º¿¡¼­ À§Ä¡¸¦ Á÷Á¢ ¾ò¾î¿Â´Ù - [levites]
	}
	++m_dwFrame;
}

struct FEffectUpdator
{
	BOOL isAlive;
	float fElapsedTime;
	FEffectUpdator(float fElapsedTime)
		: isAlive(FALSE), fElapsedTime(fElapsedTime)
	{
	}
	void operator () (CEffectElementBaseInstance * pInstance)
	{
		if (pInstance->Update(fElapsedTime))
			isAlive = TRUE;
	}
};

void CEffectInstance::OnUpdate()
{
	Transform();

	FEffectUpdator f(CTimer::Instance().GetCurrentSecond()-m_fLastTime);
	f = std::for_each(m_ParticleInstanceVector.begin(), m_ParticleInstanceVector.end(),f);
	f = std::for_each(m_MeshInstanceVector.begin(), m_MeshInstanceVector.end(),f);
	f = std::for_each(m_LightInstanceVector.begin(), m_LightInstanceVector.end(),f);
	m_isAlive = f.isAlive;

	m_fLastTime = CTimer::Instance().GetCurrentSecond();
}
//Régi

/*
void CEffectInstance::OnUpdate()
{
	Transform();

	float currentTime = CTimer::Instance().GetCurrentSecond();

	float elapsedTime = CTimer::Instance().GetElapsedSecond();

	bool bIsAnyEffectAlive = false;

	// Particle Instances
	for (auto& particle : m_ParticleInstanceVector)
	{
		if (particle->Update(elapsedTime))
			bIsAnyEffectAlive = true;
	}

	// Mesh Instances
	for (auto& mesh : m_MeshInstanceVector)
	{
		if (mesh->Update(elapsedTime))
			bIsAnyEffectAlive = true;
	}

	// Light Instances
	for (auto& light : m_LightInstanceVector)
	{
		if (light->Update(elapsedTime))
			bIsAnyEffectAlive = true;
	}

	// Always Alive állapot ellenõrzése (folyamatos effektek pl. Aura)
	m_isAlive = m_bAlwaysAlive || bIsAnyEffectAlive;

	m_fLastTime = currentTime;
}
*/
void CEffectInstance::OnRender()
{
	STATEMANAGER.SaveTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_NONE);
	STATEMANAGER.SaveTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_NONE);
	STATEMANAGER.SaveRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	STATEMANAGER.SaveRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	STATEMANAGER.SaveRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	STATEMANAGER.SaveRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	STATEMANAGER.SaveRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	STATEMANAGER.SaveRenderState(D3DRS_ZWRITEENABLE, FALSE);
	/////

    STATEMANAGER.SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
	STATEMANAGER.SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
	STATEMANAGER.SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
	STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
	STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
	STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
	STATEMANAGER.SetVertexShader(D3DFVF_XYZ|D3DFVF_TEX1);
	std::for_each(m_ParticleInstanceVector.begin(),m_ParticleInstanceVector.end(),std::mem_fn(&CEffectElementBaseInstance::Render));
	std::for_each(m_MeshInstanceVector.begin(),m_MeshInstanceVector.end(),std::mem_fn(&CEffectElementBaseInstance::Render));

	/////
	STATEMANAGER.RestoreTextureStageState(0, D3DTSS_MINFILTER);
	STATEMANAGER.RestoreTextureStageState(0, D3DTSS_MAGFILTER);
	STATEMANAGER.RestoreRenderState(D3DRS_ALPHABLENDENABLE);
	STATEMANAGER.RestoreRenderState(D3DRS_SRCBLEND);
	STATEMANAGER.RestoreRenderState(D3DRS_DESTBLEND);
	STATEMANAGER.RestoreRenderState(D3DRS_ALPHATESTENABLE);
	STATEMANAGER.RestoreRenderState(D3DRS_CULLMODE);
	STATEMANAGER.RestoreRenderState(D3DRS_ZWRITEENABLE);

	++ms_iRenderingEffectCount;
}

void CEffectInstance::SetGlobalMatrix(const D3DXMATRIX & c_rmatGlobal)
{
	m_matGlobal = c_rmatGlobal;
}

BOOL CEffectInstance::isAlive()
{
	return m_isAlive;
}
///*régi
void CEffectInstance::SetActive()
{
	std::for_each(
		m_ParticleInstanceVector.begin(),
		m_ParticleInstanceVector.end(),
		std::mem_fn(&CEffectElementBaseInstance::SetActive));
	std::for_each(
		m_MeshInstanceVector.begin(),
		m_MeshInstanceVector.end(),
		std::mem_fn(&CEffectElementBaseInstance::SetActive));
	std::for_each(
		m_LightInstanceVector.begin(),
		m_LightInstanceVector.end(),
		std::mem_fn(&CEffectElementBaseInstance::SetActive));
}
/*
void CEffectInstance::SetActive()
{
	m_isAlive = true;
	m_fLastTime = CTimer::Instance().GetCurrentSecond();

	for (auto& particle : m_ParticleInstanceVector)
		particle->SetActive();

	for (auto& mesh : m_MeshInstanceVector)
		mesh->SetActive();

	for (auto& light : m_LightInstanceVector)
		light->SetActive();
}
*/
void CEffectInstance::SetDeactive()
{
	std::for_each(
		m_ParticleInstanceVector.begin(),
		m_ParticleInstanceVector.end(),
		std::mem_fn(&CEffectElementBaseInstance::SetDeactive));
	std::for_each(
		m_MeshInstanceVector.begin(),
		m_MeshInstanceVector.end(),
		std::mem_fn(&CEffectElementBaseInstance::SetDeactive));
	std::for_each(
		m_LightInstanceVector.begin(),
		m_LightInstanceVector.end(),
		std::mem_fn(&CEffectElementBaseInstance::SetDeactive));
}
//*///régi
/*
void CEffectInstance::SetDeactive()
{
	m_isAlive = false;

	for (auto& particle : m_ParticleInstanceVector)
		particle->SetDeactive();

	for (auto& mesh : m_MeshInstanceVector)
		mesh->SetDeactive();

	for (auto& light : m_LightInstanceVector)
		light->SetDeactive();
}
*/

void CEffectInstance::__SetParticleData(CParticleSystemData * pData)
{
	CParticleSystemInstance * pInstance = CParticleSystemInstance::New();
	pInstance->SetDataPointer(pData);
	pInstance->SetLocalMatrixPointer(&m_matGlobal);
#ifdef ENABLE_AURA_COSTUME_SYSTEM
	pInstance->SetParticleScale(GetParticleScale());
#endif

	m_ParticleInstanceVector.push_back(pInstance);
}

void CEffectInstance::__SetMeshData(CEffectMeshScript * pMesh)
{
	CEffectMeshInstance * pMeshInstance = CEffectMeshInstance::New();
	pMeshInstance->SetDataPointer(pMesh);
	pMeshInstance->SetLocalMatrixPointer(&m_matGlobal);
#ifdef ENABLE_AURA_COSTUME_SYSTEM
	pMeshInstance->SetMeshScale(GetMeshScale());
#endif

	m_MeshInstanceVector.push_back(pMeshInstance);
}

void CEffectInstance::__SetLightData(CLightData* pData)
{
	CLightInstance * pInstance = CLightInstance::New();
	pInstance->SetDataPointer(pData);
	pInstance->SetLocalMatrixPointer(&m_matGlobal);

	m_LightInstanceVector.push_back(pInstance);
}

#ifdef ENABLE_SKILL_COLOR_SYSTEM
void CEffectInstance::SetEffectDataPointer(CEffectData * pEffectData, DWORD * dwSkillColor, DWORD EffectID)
#else
void CEffectInstance::SetEffectDataPointer(CEffectData * pEffectData)
#endif
{
	m_isAlive=true;

	m_pkEftData=pEffectData;

	m_fLastTime = CTimer::Instance().GetCurrentSecond();
	m_fBoundingSphereRadius = pEffectData->GetBoundingSphereRadius();
	m_v3BoundingSpherePosition = pEffectData->GetBoundingSpherePosition();

	if (m_fBoundingSphereRadius > 0.0f)
		CGraphicObjectInstance::RegisterBoundingSphere();

	DWORD i;

	for (i = 0; i < pEffectData->GetParticleCount(); ++i)
	{
		CParticleSystemData * pParticle = pEffectData->GetParticlePointer(i);

#ifdef ENABLE_SKILL_COLOR_SYSTEM
		if (dwSkillColor != NULL)
		{
			DWORD skill;

			if (i >= 5)
				skill = dwSkillColor[4];
			else
				skill = dwSkillColor[i];

			CParticleProperty* prob = pParticle->GetParticlePropertyPointer();
			if (skill != 99999999 && skill != 0)
			{
				D3DXCOLOR c = D3DXCOLOR(skill);
				D3DXCOLOR d;
				for (TTimeEventTableColor::iterator it = prob->m_TimeEventColor.begin(); it != prob->m_TimeEventColor.end(); ++it)
				{
					d = D3DXCOLOR(it->m_Value.m_dwColor);
					c.a = d.a;
					it->m_Value.m_dwColor = (DWORD)c;
				}
			}
			else if (skill == 0)
			{
				CParticleProperty* prob_backup = pParticle->GetParticlePropertyBackupPointer();
				for (TTimeEventTableColor::iterator it = prob->m_TimeEventColor.begin(); it != prob->m_TimeEventColor.end(); ++it)
					it->m_Value.m_dwColor = prob_backup->m_TimeEventColor[distance(prob->m_TimeEventColor.begin(), it)].m_Value.m_dwColor;
			}
		}
#endif

		__SetParticleData(pParticle);
	}

	for (i = 0; i < pEffectData->GetMeshCount(); ++i)
	{
		CEffectMeshScript * pMesh = pEffectData->GetMeshPointer(i);

		__SetMeshData(pMesh);
	}

	for (i = 0; i < pEffectData->GetLightCount(); ++i)
	{
		CLightData * pLight = pEffectData->GetLightPointer(i);

		__SetLightData(pLight);
	}

	m_pSoundInstanceVector = pEffectData->GetSoundInstanceVector();
}

bool CEffectInstance::GetBoundingSphere(D3DXVECTOR3 & v3Center, float & fRadius)
{
	v3Center.x = m_matGlobal._41 + m_v3BoundingSpherePosition.x;
	v3Center.y = m_matGlobal._42 + m_v3BoundingSpherePosition.y;
	v3Center.z = m_matGlobal._43 + m_v3BoundingSpherePosition.z;
	fRadius = m_fBoundingSphereRadius;
	return true;
}

void CEffectInstance::Clear()
{
	if (!m_ParticleInstanceVector.empty())
	{
		std::for_each(m_ParticleInstanceVector.begin(), m_ParticleInstanceVector.end(), CParticleSystemInstance::Delete);
		m_ParticleInstanceVector.clear();
	}

	if (!m_MeshInstanceVector.empty())
	{
		std::for_each(m_MeshInstanceVector.begin(), m_MeshInstanceVector.end(), CEffectMeshInstance::Delete);
		m_MeshInstanceVector.clear();
	}

	if (!m_LightInstanceVector.empty())
	{
		std::for_each(m_LightInstanceVector.begin(), m_LightInstanceVector.end(), CLightInstance::Delete);
		m_LightInstanceVector.clear();
	}

	__Initialize();
}

void CEffectInstance::__Initialize()
{
	m_isAlive = FALSE;
	m_dwFrame = 0;

#ifdef ENABLE_RENDER_TARGET
	m_ignoreFrustum = false;
#endif

#ifdef ENABLE_INGAME_WIKI_SYSTEM
	m_wikiIgnoreFrustum = false;
#endif

	m_pSoundInstanceVector = NULL;
	m_fBoundingSphereRadius = 0.0f;
	m_v3BoundingSpherePosition.x = m_v3BoundingSpherePosition.y = m_v3BoundingSpherePosition.z = 0.0f;

#ifdef ENABLE_AURA_COSTUME_SYSTEM
	m_fParticleScale = 1.0f;
	m_v3MeshScale.x = m_v3MeshScale.y = m_v3MeshScale.z = 1.0f;
#endif

	m_pkEftData = NULL;

	D3DXMatrixIdentity(&m_matGlobal);
}

CEffectInstance::CEffectInstance() 
{
	__Initialize();
}
CEffectInstance::~CEffectInstance()
{
	assert(m_ParticleInstanceVector.empty());
	assert(m_MeshInstanceVector.empty());
	assert(m_LightInstanceVector.empty());
}
