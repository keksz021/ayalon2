#include "StdAfx.h"

#include <io.h>
#include <assert.h>

#include "EterPackManager.h"

#include "../eterBase/Debug.h"
#include "../eterBase/CRC32.h"

#define PATH_ABSOLUTE_YMIRWORK1	"d:/ymir work/"
#define PATH_ABSOLUTE_YMIRWORK2	"d:\\ymir work\\"

#ifdef ENABLE_FOXFS_ENCRYPT
namespace FoxFS
{
	enum
	{
		ERROR_OK = 0,
		ERROR_BASE_CODE = 0,
		ERROR_FILE_WAS_NOT_FOUND = ERROR_BASE_CODE + 1,
		ERROR_CORRUPTED_FILE = ERROR_BASE_CODE + 2,
		ERROR_MISSING_KEY = ERROR_BASE_CODE + 3,
		ERROR_MISSING_IV = ERROR_BASE_CODE + 4,
		ERROR_DECRYPTION_HAS_FAILED = ERROR_BASE_CODE + 5,
		ERROR_DECOMPRESSION_FAILED = ERROR_BASE_CODE + 6,
		ERROR_ARCHIVE_NOT_FOUND = ERROR_BASE_CODE + 7,
		ERROR_ARCHIVE_NOT_READABLE = ERROR_BASE_CODE + 8,
		ERROR_ARCHIVE_INVALID = ERROR_BASE_CODE + 9,
		ERROR_ARCHIVE_ACCESS_DENIED = ERROR_BASE_CODE + 10,
		ERROR_KEYSERVER_SOCKET = ERROR_BASE_CODE + 11,
		ERROR_KEYSERVER_CONNECTION = ERROR_BASE_CODE + 12,
		ERROR_KEYSERVER_RESPONSE = ERROR_BASE_CODE + 13,
		ERROR_KEYSERVER_TIMEOUT = ERROR_BASE_CODE + 14,
		ERROR_UNKNOWN = ERROR_BASE_CODE + 15
	};
}

const char *white_file_list[] = { "mark\10_0.tga" "mark\250_0.tga" };

bool isWhiteFile(const char *c_szFileName)
{
	for (int i = 0; i < ARRAYSIZE(white_file_list); i++)
	{
		if (stricmp(c_szFileName, white_file_list[i]) == 0)
		{
			return true;
		}
	}
	return false;
}

const char *white_file_list_ext[] = { "xml", "tga", "png", "bmp", "mp3", "jpg" };

bool isWhiteFileExt(const char *c_szFileName)
{
	for (int i = 0; i < ARRAYSIZE(white_file_list_ext); i++)
	{
		auto v = std::string(c_szFileName);
		std::string ext = CFileNameHelper::GetExtension(v);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		std::string whiteExt = std::string(white_file_list_ext[i]);

		if (ext == whiteExt)
			return true;
	}
	return false;
}
#endif

CEterPack* CEterPackManager::FindPack(const char *c_szPathName)
{
	std::string strFileName;

	if (0 == ConvertFileName(c_szPathName, strFileName))
	{
		return &m_RootPack;
	}
	else
	{
		for (TEterPackMap::iterator itor = m_DirPackMap.begin(); itor != m_DirPackMap.end(); ++itor)
		{
			const std::string & c_rstrName = itor->first;
			CEterPack * pEterPack = itor->second;

			if (CompareName(c_rstrName.c_str(), c_rstrName.length(), strFileName.c_str()))
			{
				return pEterPack;
			}
		}
	}

	return NULL;
}

void CEterPackManager::SetCacheMode()
{
	m_isCacheMode=true;
}

void CEterPackManager::SetRelativePathMode()
{
	m_bTryRelativePath = true;
}


// StringPath std::string ¹öÀü
int CEterPackManager::ConvertFileName(const char *c_szFileName, std::string & rstrFileName)
{
	rstrFileName = c_szFileName;
	stl_lowers(rstrFileName);

	int iCount = 0;

	for (DWORD i = 0; i < rstrFileName.length(); ++i)
	{
		if (rstrFileName[i] == '/')
			++iCount;
		else if (rstrFileName[i] == '\\')
		{
			rstrFileName[i] = '/';
			++iCount;
		}
	}

	return iCount;
}

bool CEterPackManager::CompareName(const char *c_szDirectoryName, DWORD /*dwLength*/, const char *c_szFileName)
{
	const char *c_pszSrc = c_szDirectoryName;
	const char *c_pszCmp = c_szFileName;

	while (*c_pszSrc)
	{
		if (*(c_pszSrc++) != *(c_pszCmp++))
			return false;

		if (!*c_pszCmp)
			return false;
	}

	return true;
}

void CEterPackManager::LoadStaticCache(const char *c_szFileName)
{
	if (!m_isCacheMode)
		return;

	std::string strFileName;
	if (0 == ConvertFileName(c_szFileName, strFileName))
	{
		return;
	}
	
	DWORD dwFileNameHash = GetCRC32(strFileName.c_str(), strFileName.length());

	std::unordered_map<DWORD, SCache>::iterator f = m_kMap_dwNameKey_kCache.find(dwFileNameHash);
	if (m_kMap_dwNameKey_kCache.end() != f)
		return;	
	
	CMappedFile kMapFile;
	const void* c_pvData;
	if (!Get(kMapFile, c_szFileName, &c_pvData))
		return;

	SCache kNewCache;
	kNewCache.m_dwBufSize = kMapFile.Size();
	kNewCache.m_abBufData = new BYTE[kNewCache.m_dwBufSize];
	memcpy(kNewCache.m_abBufData, c_pvData, kNewCache.m_dwBufSize);
	m_kMap_dwNameKey_kCache.insert(std::unordered_map<DWORD, SCache>::value_type(dwFileNameHash, kNewCache));
}

CEterPackManager::SCache* CEterPackManager::__FindCache(DWORD dwFileNameHash)
{	
	std::unordered_map<DWORD, SCache>::iterator f=m_kMap_dwNameKey_kCache.find(dwFileNameHash);
	if (m_kMap_dwNameKey_kCache.end()==f)
		return NULL;

	return &f->second;
}

void	CEterPackManager::__ClearCacheMap()
{
	std::unordered_map<DWORD, SCache>::iterator i;

	for (i = m_kMap_dwNameKey_kCache.begin(); i != m_kMap_dwNameKey_kCache.end(); ++i)
		delete [] i->second.m_abBufData;

	m_kMap_dwNameKey_kCache.clear();
}

struct TimeChecker
{
	TimeChecker(const char *name) : name(name)
	{
		baseTime = timeGetTime();
	}
	~TimeChecker()
	{
		printf("load %s (%d)\n", name, timeGetTime() - baseTime);
	}

	const char *name;
	DWORD baseTime;
};

bool CEterPackManager::Get(CMappedFile& rMappedFile, const char *c_szFileName, LPCVOID* pData)
{
#ifdef ENABLE_FOXFS_ENCRYPT
	if (m_iSearchMode == SEARCH_FILE)
	{
		if (GetFromFile(rMappedFile, c_szFileName, pData))
		{
			return true;
		}

		return GetFromPack(rMappedFile, c_szFileName, pData);
	}
	else
	{
		if (isExistInPack(c_szFileName))
		{
			return GetFromPack(rMappedFile, c_szFileName, pData);
		}
		else if (isExist(c_szFileName))
		{
			return GetFromFile(rMappedFile, c_szFileName, pData);
		}
	}
	return false;
#else
	if (m_iSearchMode == SEARCH_PACK_FIRST)
	{
		if (GetFromPack(rMappedFile, c_szFileName, pData))
			return true;

		if (c_szFileName[1] != ':' && GetFromFile(rMappedFile, c_szFileName, pData))
		{
			TraceError("%s", c_szFileName); // only for log. it's not an error.
			return true;
		}
	}

	if (m_iSearchMode == SEARCH_FILE_FIRST)
	{
		if (GetFromFile(rMappedFile, c_szFileName, pData))
			return true;

		return GetFromPack(rMappedFile, c_szFileName, pData);
	}

	return false;
#endif
}

struct FinderLock
{
	FinderLock(CRITICAL_SECTION& cs) : p_cs(&cs)
	{
		EnterCriticalSection(p_cs);
	}

	~FinderLock()
	{
		LeaveCriticalSection(p_cs);
	}

	CRITICAL_SECTION* p_cs;
};



//bool CEterPackManager::GetFromPack(CMappedFile & rMappedFile, const char *c_szFileName, LPCVOID * pData)
//{
//#ifdef ENABLE_FOXFS_ENCRYPT
//	assert(c_szFileName);
//
//	FinderLock lock(m_csFinder);
//
//	if (m_pFoxFS)
//	{
//		int errorCodeSize = 0;
//		if ((errorCodeSize = FoxFS_ExistsA(m_pFoxFS, c_szFileName)) == FoxFS::ERROR_OK)
//		{
//			unsigned int dwSize = FoxFS_SizeA(m_pFoxFS, c_szFileName), dwReadSize = 0;
//			BYTE* pbData = new BYTE[dwSize + 1];
//			int errorCode = 0;
//			if ((errorCode = FoxFS_GetA(m_pFoxFS, c_szFileName, pbData, dwSize, &dwReadSize)) == FoxFS::ERROR_OK)
//			{
//				pbData[dwReadSize] = 0;
//				*pData = pbData;
//				rMappedFile.Link(dwReadSize, pbData);
//				return true;
//			}
//			else {
//				TraceError("FoxFS - Could not get file %s Error Code %d", c_szFileName, errorCode);
//			}
//			delete[] pbData;
//		}
//		else {
//			//TraceError("FoxFS: File not existing %s Error Code %d", c_szFileName, errorCodeSize);
//		}
//	}
//	else
//	{
//		TraceError("FoxFS: Not initialized!");
//	}
//
//	return false;
//
//#else
//	FinderLock lock(m_csFinder);
//
//	static std::string strFileName;
//	
//	if (0 == ConvertFileName(c_szFileName, strFileName))
//	{
//		return m_RootPack.Get(rMappedFile, strFileName.c_str(), pData);
//	}
//	else
//	{
//		DWORD dwFileNameHash = GetCRC32(strFileName.c_str(), strFileName.length());
//		SCache* pkCache = __FindCache(dwFileNameHash);
//
//		if (pkCache)
//		{
//			rMappedFile.Link(pkCache->m_dwBufSize, pkCache->m_abBufData);
//			return true;
//		}
//
//		CEterFileDict::Item* pkFileItem = m_FileDict.GetItem(dwFileNameHash, strFileName.c_str());
//
//		if (pkFileItem)
//			if (pkFileItem->pkPack)
//			{
//				bool r = pkFileItem->pkPack->Get2(rMappedFile, strFileName.c_str(), pkFileItem->pkInfo, pData);	
//				//pkFileItem->pkPack->ClearDataMemoryMap();
//				return r;
//			}
//	}
//#ifdef _DEBUG
//	TraceError("CANNOT_FIND_PACK_FILE [%s]", strFileName.c_str());
//#endif
//
//	return false;
//#endif
//}

bool CEterPackManager::GetFromPack(CMappedFile& rMappedFile, const char* c_szFileName, LPCVOID* pData)
{
#ifdef ENABLE_FOXFS_ENCRYPT
	assert(c_szFileName);
	FinderLock lock(m_csFinder);

	if (!m_pFoxFS)
	{
		TraceError("[PACK] FoxFS not initialized!");
		return false;
	}

	// Létezik?
	int existsCode = FoxFS_ExistsA(m_pFoxFS, c_szFileName);
	if (existsCode != FoxFS::ERROR_OK)
	{
		// TraceError("[PACK] Not found: %s (code=%d)", c_szFileName, existsCode);
		return false;
	}

	// Méret lekérdezés + basic guard
	unsigned int dwSize = FoxFS_SizeA(m_pFoxFS, c_szFileName);
	if (dwSize == 0)
	{
	//	TraceError("[PACK] 0-byte file: %s", c_szFileName);
		return false;
	}

	// Irreális méretû bejegyzés ellen védünk (korrupt pack)
	if (dwSize > 256u * 1024u * 1024u) // 256 MB
	{
	//	TraceError("[PACK] insane size=%u for %s (skip)", dwSize, c_szFileName);
		return false;
	}

	// Buffer allokáció védetten
	BYTE* pbData = nullptr;
	try
	{
		pbData = new BYTE[dwSize + 1];
	}
	catch (const std::bad_alloc&)
	{
	//	TraceError("[PACK] bad_alloc while allocating %u bytes for %s", dwSize, c_szFileName);
		return false;
	}

	// Beolvasás FoxFS-bõl – try/catch, hogy ne dobjon kivételt kifelé
	unsigned int dwReadSize = 0;
	try
	{
		int errorCode = FoxFS_GetA(m_pFoxFS, c_szFileName, pbData, dwSize, &dwReadSize);
		if (errorCode != FoxFS::ERROR_OK)
		{
	//		TraceError("[PACK] FoxFS_GetA failed: %s (code=%d, size=%u)", c_szFileName, errorCode, dwSize);
			delete[] pbData;
			return false;
		}
	}
	catch (const std::bad_alloc&)
	{
	//	TraceError("[PACK] std::bad_alloc while reading %s (size=%u)", c_szFileName, dwSize);
		delete[] pbData;
		return false;
	}
	catch (...)
	{
	//	TraceError("[PACK] Unknown exception in FoxFS_GetA: %s", c_szFileName);
		delete[] pbData;
		return false;
	}

	// Overflow guard
	if (dwReadSize > dwSize)
	{
	//	TraceError("[PACK] read overflow: %s (read=%u > alloc=%u)", c_szFileName, dwReadSize, dwSize);
		delete[] pbData;
		return false;
	}

	// NULL-terminátor a kényelmesebb feldolgozáshoz
	pbData[dwReadSize] = 0;

	// átadás a mapped file-nak (õ birtokolja a buffert)
	*pData = pbData;
	rMappedFile.Link(dwReadSize, pbData);

	// 256 MB -> túl nagy
	if (dwSize > 256u * 1024u * 1024u)
	{
	//	TraceError("[PACK] insane size=%u for %s (skip)", dwSize, c_szFileName);
		return false;
	}

	// plusz finomabb védelem textúrákra:
	const char* ext = strrchr(c_szFileName, '.');
	if (ext && (!_stricmp(ext, ".dds") || !_stricmp(ext, ".tga"))) {
		if (dwSize > 8u * 1024u * 1024u) {
	//		TraceError("[PACK] texture too large (%u bytes): %s -> skipped", dwSize, c_szFileName);
			return false;
		}
	}



#ifdef _DEBUG
	TraceError("[PACK OK] %s (read=%u bytes)", c_szFileName, dwReadSize);
#endif
	return true;

#else
	// --- Eredeti EterPack fallback ---
	FinderLock lock(m_csFinder);
	static std::string strFileName;

	if (0 == ConvertFileName(c_szFileName, strFileName))
		return m_RootPack.Get(rMappedFile, strFileName.c_str(), pData);

	DWORD dwFileNameHash = GetCRC32(strFileName.c_str(), strFileName.length());
	SCache* pkCache = __FindCache(dwFileNameHash);

	if (pkCache)
	{
		rMappedFile.Link(pkCache->m_dwBufSize, pkCache->m_abBufData);
		return true;
	}

	CEterFileDict::Item* pkFileItem = m_FileDict.GetItem(dwFileNameHash, strFileName.c_str());
	if (pkFileItem && pkFileItem->pkPack)
		return pkFileItem->pkPack->Get2(rMappedFile, strFileName.c_str(), pkFileItem->pkInfo, pData);

#ifdef _DEBUG
	TraceError("CANNOT_FIND_PACK_FILE [%s]", strFileName.c_str());
#endif
	return false;
#endif
}



const time_t g_tCachingInterval = 10; // 10ÃÊ
void CEterPackManager::ArrangeMemoryMappedPack()
{
	//time_t curTime = time(NULL);
	//CEterFileDict::TDict dict = m_FileDict.GetDict();
	//for (CEterFileDict::TDict::iterator it = dict.begin(); it != dict.end(); ++it)
	//{
	//	CEterFileDict::Item &rFileItem = it->second;
	//	CEterPack* pkPack = rFileItem.pkPack;
	//	if (pkPack)
	//	{
	//		if (curTime - pkPack->GetLastAccessTime() > g_tCachingInterval)
	//		{
	//			pkPack->ClearDataMemoryMap();
	//		}
	//	}
	//}
}

bool CEterPackManager::GetFromFile(CMappedFile & rMappedFile, const char *c_szFileName, LPCVOID * pData)
{
	return rMappedFile.Create(c_szFileName, pData, 0, 0) ? true : false;
}

bool CEterPackManager::isExistInPack(const char *c_szFileName)
{
#ifdef ENABLE_FOXFS_ENCRYPT
	assert(c_szFileName);

	if (m_pFoxFS)
	{
		int errorCodeSize = 0;
		if ((errorCodeSize = FoxFS_ExistsA(m_pFoxFS, c_szFileName)) == FoxFS::ERROR_OK)
		{
			return true;
		}
		else
		{
			//Tracenf("FoxFS :  File not exists %s with error code %d", c_szFileName, errorCodeSize);
		}
	}
	else
	{
		TraceError("FoxFS: Not initialized!");
	}

	return false;
#else
	std::string strFileName;

	if (0 == ConvertFileName(c_szFileName, strFileName))
	{
		return m_RootPack.IsExist(strFileName.c_str());
	}
	else
	{
		DWORD dwFileNameHash = GetCRC32(strFileName.c_str(), strFileName.length());
		CEterFileDict::Item* pkFileItem = m_FileDict.GetItem(dwFileNameHash, strFileName.c_str());

		if (pkFileItem)
			if (pkFileItem->pkPack)
				return pkFileItem->pkPack->IsExist(strFileName.c_str());
	}

	return false;
#endif
}

bool CEterPackManager::isExist(const char *c_szFileName)
{
#ifdef ENABLE_FOXFS_ENCRYPT
	if (m_iSearchMode == SEARCH_PACK)
	{
		if (isWhiteFile(c_szFileName) || isWhiteFileExt(c_szFileName))
		{
			return isExistInPack(c_szFileName) || (_access(c_szFileName, 0) == 0);
		}
		return isExistInPack(c_szFileName);
	}

	if (_access(c_szFileName, 0) == 0)
		return true;
#endif

	return isExistInPack(c_szFileName);
}

void CEterPackManager::RegisterRootPack(const char *c_szName)
{
#ifdef ENABLE_FOXFS_ENCRYPT
	assert(c_szName);
	if (m_pFoxFS)
	{
		int errorCode = 0;
		if ((errorCode = FoxFS_LoadA(m_pFoxFS, c_szName)) != FoxFS::ERROR_OK)
		{
			TraceError("%s: Error Code %d", c_szName, errorCode);
		}
	}
	else
	{
		TraceError("FoxFS: Not initialized!");
	}
#else
	if (!m_RootPack.Create(m_FileDict, c_szName, ""))
	{
		TraceError("%s: Pack file does not exist", c_szName);
	}
#endif
}

const char *CEterPackManager::GetRootPackFileName()
{
	return m_RootPack.GetDBName();
}

bool CEterPackManager::RegisterPackWhenPackMaking(const char *c_szName, const char *c_szDirectory, CEterPack* pPack)
{
	m_PackMap.insert(TEterPackMap::value_type(c_szName, pPack));
	m_PackList.push_front(pPack);

	m_DirPackMap.insert(TEterPackMap::value_type(c_szDirectory, pPack));
	return true;
}


bool CEterPackManager::RegisterPack(const char *c_szName, const char *c_szDirectory, const BYTE* c_pbIV)
{

#ifdef ENABLE_FOXFS_ENCRYPT
	assert(c_szName);

	if (!m_pFoxFS)
		m_pFoxFS = FoxFS_Create();

	int errorCode = FoxFS_LoadA(m_pFoxFS, c_szName);
	if (errorCode != FoxFS::ERROR_OK)
	{
		TraceError("[FoxFS] Failed to load pack %s (Error %d)", c_szName, errorCode);
		return false;
	}

	//TraceError("[FoxFS] Successfully registered pack %s", c_szName);
	return true; // <-- EDDIG HIÁNYZOTT!
#else
	CEterPack * pEterPack = NULL;
	{
		TEterPackMap::iterator itor = m_PackMap.find(c_szName);

		if (m_PackMap.end() == itor)
		{
			bool bReadOnly = true;

			pEterPack = new CEterPack;
			if (pEterPack->Create(m_FileDict, c_szName, c_szDirectory, bReadOnly, c_pbIV))
			{
				m_PackMap.insert(TEterPackMap::value_type(c_szName, pEterPack));
			}
			else
			{
#ifdef _DEBUG
				Tracef("The eterpack doesn't exist [%s]\n", c_szName);
#endif
				delete pEterPack;
				pEterPack = NULL;
				return false;
			}
		}
		else
		{
			pEterPack = itor->second;
		}
	}

	if (c_szDirectory && c_szDirectory[0] != '*')
	{
		TEterPackMap::iterator itor = m_DirPackMap.find(c_szDirectory);
		if (m_DirPackMap.end() == itor)
		{
			m_PackList.push_front(pEterPack);
			m_DirPackMap.insert(TEterPackMap::value_type(c_szDirectory, pEterPack));
		}
	}

	return true;
#endif
}

void CEterPackManager::SetSearchMode(bool bPackFirst)
{
	m_iSearchMode = bPackFirst ? SEARCH_PACK_FIRST : SEARCH_FILE_FIRST;
}

int CEterPackManager::GetSearchMode()
{
	return m_iSearchMode;
}

CEterPackManager::CEterPackManager() : m_bTryRelativePath(false), m_iSearchMode(SEARCH_FILE_FIRST), m_isCacheMode(false)
{
	InitializeCriticalSection(&m_csFinder);
#ifdef ENABLE_FOXFS_ENCRYPT
	m_pFoxFS = FoxFS_Create();
#endif
}

CEterPackManager::~CEterPackManager()
{
	__ClearCacheMap();

	TEterPackMap::iterator i = m_PackMap.begin();
	TEterPackMap::iterator e = m_PackMap.end();
	while (i != e)
	{
		delete i->second;
		i++;
	}

	DeleteCriticalSection(&m_csFinder);

#ifdef ENABLE_FOXFS_ENCRYPT
	if (m_pFoxFS)
	{
		FoxFS_Destroy(m_pFoxFS);
	}
#endif
}
