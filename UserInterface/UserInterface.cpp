#include "StdAfx.h"
#include "PythonApplication.h"
#include "ProcessScanner.h"
#include "PythonExceptionSender.h"
#include "resource.h"
#include "Version.h"

#ifdef _DEBUG
#include <crtdbg.h>
#endif

#include "../eterPack/EterPackManager.h"
#include "../eterLib/Util.h"
#include "../CWebBrowser/CWebBrowser.h"
#include "../eterBase/CPostIt.h"

#ifdef ENABLE_CONFIG_MODULE
	#include "PythonConfig.h"
#endif

#ifdef ENABLE_CLIENT_PERFORMANCE
	#include "PythonPlayerSettingsModule.h"
#endif

#ifdef ENABLE_PYTHON_DYNAMIC_MODULE_NAME
	#include "PythonDynamicModuleNames.h"
#endif

#ifdef NUBIRUS2_SHIELD
#include "ShieldHelper.h"
#endif

static unsigned int __stdcall WindowTitleRandomizerThread(void* lpParameter);

extern "C" {
extern int _fltused;
volatile int _AVOID_FLOATING_POINT_LIBRARY_BUG = _fltused;
};

extern "C" { FILE __iob_func[3] = {*stdin,*stdout,*stderr }; }

extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

#pragma comment(linker, "/NODEFAULTLIB:libci.lib")
#pragma comment(linker, "/NODEFAULTLIB:msvcrt.lib")


#pragma comment( lib, "version.lib" )
#pragma comment( lib, "python27.lib" )
#pragma comment( lib, "imagehlp.lib" )
#pragma comment( lib, "devil.lib" )
#pragma comment( lib, "granny2.lib" )
#pragma comment( lib, "mss32.lib" )
#pragma comment( lib, "winmm.lib" )
#pragma comment( lib, "imm32.lib" )
#pragma comment( lib, "oldnames.lib" )
#pragma comment( lib, "SpeedTreeRT.lib" )
#pragma comment( lib, "dinput8.lib" )
#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "ws2_32.lib" )
#pragma comment( lib, "strmiids.lib" )
#pragma comment( lib, "ddraw.lib" )
#pragma comment( lib, "dmoguids.lib" )

#ifdef ENABLE_FOXFS_ENCRYPT
	#pragma comment( lib, "lz4.lib" )
	#pragma comment( lib, "xxhash.lib" )
	#ifndef _DEBUG
		#pragma comment(lib,"FoxFS.lib")
	#else
		#pragma comment(lib,"FoxFS_d.lib")
	#endif
	#pragma comment( lib, "Iphlpapi.lib" )
#endif

#include <stdlib.h>
#include <cryptopp/cryptoppLibLink.h>

extern bool SetDefaultCodePage(DWORD codePage);

static const char *sc_apszPythonLibraryFilenames[] =
{
	"UserDict.pyc",
	"__future__.pyc",
	"copy_reg.pyc",
	"linecache.pyc",
	"ntpath.pyc",
	"os.pyc",
	"site.pyc",
	"stat.pyc",
	"string.pyc",
	"traceback.pyc",
	"types.pyc",
	"\n",
};

#include <aclapi.h>
#include <sddl.h>
#pragma comment(lib, "advapi32.lib")

static bool ApplyRestrictedDaclToCurrentProcess()
{
	PSID pUserSid = NULL;
	HANDLE hToken = NULL;
	bool ok = false;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
		return false;

	DWORD dwLen = 0;
	GetTokenInformation(hToken, TokenUser, NULL, 0, &dwLen);
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
		CloseHandle(hToken);
		return false;
	}

	std::vector<BYTE> buf(dwLen);
	if (!GetTokenInformation(hToken, TokenUser, buf.data(), dwLen, &dwLen)) {
		CloseHandle(hToken);
		return false;
	}

	TOKEN_USER* tu = reinterpret_cast<TOKEN_USER*>(buf.data());
	DWORD sidLen = GetLengthSid(tu->User.Sid);
	pUserSid = (PSID)LocalAlloc(LPTR, sidLen);
	if (!pUserSid) {
		CloseHandle(hToken);
		return false;
	}
	if (!CopySid(sidLen, pUserSid, tu->User.Sid)) {
		LocalFree(pUserSid);
		CloseHandle(hToken);
		return false;
	}

	// Build ACL: allow SYSTEM and current user PROCESS_ALL_ACCESS
	EXPLICIT_ACCESSA ea[2];
	ZeroMemory(ea, sizeof(ea));

	ea[0].grfAccessPermissions = PROCESS_ALL_ACCESS;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = NO_INHERITANCE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[0].Trustee.ptstrName = const_cast<LPSTR>("SYSTEM");

	ea[1].grfAccessPermissions = PROCESS_ALL_ACCESS;
	ea[1].grfAccessMode = SET_ACCESS;
	ea[1].grfInheritance = NO_INHERITANCE;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea[1].Trustee.ptstrName = (LPSTR)pUserSid;

	PACL pAcl = NULL;
	DWORD dwRes = SetEntriesInAclA(2, ea, NULL, &pAcl);
	if (dwRes == ERROR_SUCCESS && pAcl)
	{
		DWORD rc = SetSecurityInfo(
			GetCurrentProcess(),             // handle to object
			SE_KERNEL_OBJECT,                // object type: kernel object (process)
			DACL_SECURITY_INFORMATION,       // set DACL
			NULL, NULL,
			pAcl,
			NULL
		);

		if (rc == ERROR_SUCCESS)
			ok = true;

		LocalFree(pAcl);
	}

	LocalFree(pUserSid);
	CloseHandle(hToken);
	return ok;
}

//#include <fstream>
//#include <string>
//#include <shellapi.h>
//#include <regex>
//#include <thread>
//#include <chrono>
//#include <windows.h>
//std::vector<std::string> windowBlackList = {
//	"inject",
//	"lalaker",
//	"hlbot",
//	"eterwizard",
//	"etermgr",
//	"m2bob",
//	"netlimiter",
//	"metin2mod",
//	"wireshark",
//	"unpack",
//	"clicker",
//	"debugger",
//	"Wiersz polecenia",
//	"ymir work",
//	"stmod",
//	"hlb0t",
//	"metin2 bot",
//	"m24pro",
//	"HLBot",
//	"Metin2 bot"
//};
//static BOOL CALLBACK FindBlacklistWindow(HWND hWnd, LPARAM lparam)
//{
//	int length = GetWindowTextLength(hWnd);
//	char* buffer = new char[length + 1];
//	GetWindowTextA(hWnd, buffer, length + 1);
//	std::string windowTitle(buffer);
//	if (IsWindowVisible(hWnd) && length != 0)
//	{
//		for (const auto& blacklisted : windowBlackList)
//		{
//			if (std::regex_search(windowTitle, std::regex(blacklisted, std::regex_constants::icase)))
//			{
//				std::ofstream textfile("protection.txt", std::ios::out | std::ios::app);
//				textfile << "Protection Alert: Suspicious activity detected. Client terminated." << std::endl;
//				ShellExecuteA(NULL, "open", "protection.txt", NULL, NULL, SW_SHOWNORMAL);
//				TerminateProcess(GetCurrentProcess(), 0);
//				exit(0);
//			}
//		}
//	}
//	delete[] buffer;
//	return TRUE;
//}
//void ScanForBlockedWindows()
//{
//	while (true)
//	{
//		EnumWindows(FindBlacklistWindow, NULL);
//		std::this_thread::sleep_for(std::chrono::milliseconds(200));
//	}
//}
//void Protection()
//{
//	CreateThread(NULL, NULL, LPTHREAD_START_ROUTINE(ScanForBlockedWindows), NULL, NULL, 0);
//}

// ===== Shield pipe detection & helper (add to top of UserInterface.cpp) =====
static bool g_shieldPipeConnected = false;
static std::string g_shieldPipeName;

static std::string GetCmdOptionFromString(const std::string& cmdline, const std::string& opt)
{
	size_t pos = cmdline.find(opt);
	if (pos == std::string::npos) return std::string();
	pos += opt.size();
	if (pos < cmdline.size() && cmdline[pos] == '=') ++pos;
	// allow quotes
	if (pos < cmdline.size() && cmdline[pos] == '"') ++pos;
	size_t end = cmdline.find_first_of(" \"", pos);
	if (end == std::string::npos) end = cmdline.size();
	return cmdline.substr(pos, end - pos);
}

static bool TryConnectToPipe(const std::string& pipeName, DWORD timeoutMs = 2000)
{
	if (pipeName.empty()) return false;
	DWORD t0 = GetTickCount();
	while ((GetTickCount() - t0) < timeoutMs)
	{
		HANDLE h = CreateFileA(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (h != INVALID_HANDLE_VALUE)
		{
			// success -> we only need to know it's present. Close handle.
			CloseHandle(h);
			return true;
		}
		Sleep(100);
	}
	return false;
}

// Parse command line (full commandline) and try to connect to --shieldpipe argument
static void TryConnectShieldPipeFromCmdLine(const char* lpCmdLine)
{
	// first prefer explicit argument string from lpCmdLine
	std::string cmd;
	if (lpCmdLine && lpCmdLine[0] != '\0')
		cmd = std::string(lpCmdLine);
	else
		cmd = std::string(GetCommandLineA()); // fallback

	std::string pipe = GetCmdOptionFromString(cmd, "--shieldpipe");
	if (pipe.empty())
	{
		// also support --shieldpipe:"..."/--shieldpipe="..."
		pipe = GetCmdOptionFromString(cmd, "--shieldpipe:");
	}

	if (!pipe.empty())
	{
		if (TryConnectToPipe(pipe, 2000))
		{
			g_shieldPipeConnected = true;
			g_shieldPipeName = pipe;
		}
	}
}

// Expose helper for other translation units (optional)
extern "C" bool IsShieldPipeConnected()
{
	return g_shieldPipeConnected;
}
extern "C" const char* GetShieldPipeNameC()
{
	return g_shieldPipeConnected ? g_shieldPipeName.c_str() : NULL;
}
// ===== end shield helpers =====


bool CheckPythonLibraryFilenames()
{
	for (int i = 0;*sc_apszPythonLibraryFilenames[i] != '\n'; ++i)
	{
		std::string stFilename = "lib\\";
		stFilename += sc_apszPythonLibraryFilenames[i];

		if (_access(stFilename.c_str(), 0) != 0)
		{
			return false;
		}

		MoveFile(stFilename.c_str(), stFilename.c_str());
	}

	return true;
}

int Setup(LPSTR lpCmdLine);

#ifdef ENABLE_FOXFS_ENCRYPT
bool PackInitialize(const char *c_pszFolder)
{
	if (_access(c_pszFolder, 0) != 0)
		return true;

	std::string stFolder(c_pszFolder);
	stFolder += "/";

	CTextFileLoader::SetCacheMode();
	CEterPackManager::Instance().SetCacheMode();
	CEterPackManager::Instance().SetSearchMode(CEterPackManager::SEARCH_PACK);

	CSoundData::SetPackMode();
	CEterPackManager::Instance().RegisterPack("pack/ayalon_pack", "*");
	CEterPackManager::Instance().RegisterPack("pack/asenis", "*");
	CEterPackManager::Instance().RegisterPack("pack/bgm", "*");
	CEterPackManager::Instance().RegisterPack("pack/effect", "*");
	CEterPackManager::Instance().RegisterPack("pack/effektek", "*");
	CEterPackManager::Instance().RegisterPack("pack/guild", "*");
	CEterPackManager::Instance().RegisterPack("pack/icon", "*");
	CEterPackManager::Instance().RegisterPack("pack/item", "*");
	CEterPackManager::Instance().RegisterPack("pack/locale", "*");
	CEterPackManager::Instance().RegisterPack("pack/monster", "*");
	CEterPackManager::Instance().RegisterPack("pack/monster2", "*");
	CEterPackManager::Instance().RegisterPack("pack/npc", "*");
	CEterPackManager::Instance().RegisterPack("pack/npc_mount", "*");
	CEterPackManager::Instance().RegisterPack("pack/npc_pet", "*");
	CEterPackManager::Instance().RegisterPack("pack/npc2", "*");
	CEterPackManager::Instance().RegisterPack("pack/others", "*");
	CEterPackManager::Instance().RegisterPack("pack/others2", "*");
	CEterPackManager::Instance().RegisterPack("pack/outdoor", "*");
	CEterPackManager::Instance().RegisterPack("pack/pc", "*");
	CEterPackManager::Instance().RegisterPack("pack/pc2", "*");
	CEterPackManager::Instance().RegisterPack("pack/property", "*");
	CEterPackManager::Instance().RegisterPack("pack/sound", "*");
	CEterPackManager::Instance().RegisterPack("pack/terrainmaps", "*");
	CEterPackManager::Instance().RegisterPack("pack/textureset", "*");
	CEterPackManager::Instance().RegisterPack("pack/tree", "*");
	CEterPackManager::Instance().RegisterPack("pack/zone", "*");

	CEterPackManager::Instance().RegisterRootPack((stFolder + std::string("root")).c_str());
	return true;
}
#else
#ifdef ENABLE_LOAD_INDEX_BINARY
bool PackInitialize(const char *c_pszFolder)
{
	std::vector<std::vector<std::string>> indexVec
	{
		{ "pack/", "bgm" },
		{ "d:/ymir work/pc/", "characters" },
		{ "d:/ymir work/pc2/", "characters" },
		{ "d:/ymir work/effect/", "effect" },
		{ "d:/ymir work/guild/", "guild" },
		{ "icon/", "icon" },
		{ "d:/ymir work/item/", "item" },
		{ "locale/ro/", "locale" },
		{ "d:/ymir work/monster/", "monster" },
		{ "d:/ymir work/monster2/", "monster" },
		{ "d:/ymir work/npc/", "npc" },
		{ "d:/ymir work/npc2/", "npc" },
		{ "d:/ymir work/environment/", "others" },
		{ "d:/ymir work/special/", "others" },
		{ "d:/ymir work/ui/", "others" },
		{ "gm_guild_build/", "outdoor" },
		{ "maimetin2_map_empirewar_a01/", "outdoor" },
		{ "map_a2/", "outdoor" },
		{ "map_b_fielddungeon/", "outdoor" },
		{ "map_n_snowm_01/", "outdoor" },
		{ "map_n_snowm_02/", "outdoor" },
		{ "metin2_guild_village/", "outdoor" },
		{ "metin2_guild_village_01/", "outdoor" },
		{ "metin2_guild_village_02/", "outdoor" },
		{ "metin2_guild_village_03/", "outdoor" },
		{ "metin2_map/", "outdoor" },
		{ "metin2_map_a1/", "outdoor" },
		{ "metin2_map_a2_1/", "outdoor" },
		{ "metin2_map_a3/", "outdoor" },
		{ "metin2_map_b1/", "outdoor" },
		{ "metin2_map_b3/", "outdoor" },
		{ "metin2_map_bayblacksand/", "outdoor" },
		{ "metin2_map_c1/", "outdoor" },
		{ "metin2_map_c3/", "outdoor" },
		{ "metin2_map_capedragonhead/", "outdoor" },
		{ "metin2_map_dawnmistwood/", "outdoor" },
		{ "metin2_map_devilscatacomb/", "outdoor" },
		{ "metin2_map_deviltower1/", "outdoor" },
		{ "metin2_map_duel/", "outdoor" },
		{ "metin2_map_empirewar_a01/", "outdoor" },
		{ "metin2_map_empirewar01/", "outdoor" },
		{ "metin2_map_empirewar02/", "outdoor" },
		{ "metin2_map_empirewar03/", "outdoor" },
		{ "metin2_map_ew02/", "outdoor" },
		{ "metin2_map_guild_01/", "outdoor" },
		{ "metin2_map_guild_02/", "outdoor" },
		{ "metin2_map_guild_03/", "outdoor" },
		{ "metin2_map_guild_inside01/", "outdoor" },
		{ "metin2_map_milgyo/", "outdoor" },
		{ "metin2_map_milgyo_a/", "outdoor" },
		{ "metin2_map_monkeydungeon/", "outdoor" },
		{ "metin2_map_monkeydungeon_02/", "outdoor" },
		{ "metin2_map_monkeydungeon_03/", "outdoor" },
		{ "metin2_map_mt_thunder/", "outdoor" },
		{ "metin2_map_n_desert_01/", "outdoor" },
		{ "metin2_map_n_desert_02/", "outdoor" },
		{ "metin2_map_n_flame_01/", "outdoor" },
		{ "metin2_map_n_flame_02/", "outdoor" },
		{ "metin2_map_n_flame_dungeon_01/", "outdoor" },
		{ "metin2_map_n_snow_dungeon_01/", "outdoor" },
		{ "metin2_map_nusluck01/", "outdoor" },
		{ "metin2_map_oxevent/", "outdoor" },
		{ "metin2_map_shinsutest_01/", "outdoor" },
		{ "metin2_map_siege_01/", "outdoor" },
		{ "metin2_map_siege_02/", "outdoor" },
		{ "metin2_map_siege_03/", "outdoor" },
		{ "metin2_map_skipia_bossdungeon/", "outdoor" },
		{ "metin2_map_skipia_dungeon_01/", "outdoor" },
		{ "metin2_map_skipia_dungeon_02/", "outdoor" },
		{ "metin2_map_skipia_dungeon_boss/", "outdoor" },
		{ "metin2_map_spider_bossdungeon/", "outdoor" },
		{ "metin2_map_spiderdungeon/", "outdoor" },
		{ "metin2_map_spiderdungeon_02/", "outdoor" },
		{ "metin2_map_spiderdungeon_03/", "outdoor" },
		{ "metin2_map_sungzi/", "outdoor" },
		{ "metin2_map_sungzi_desert_01/", "outdoor" },
		{ "metin2_map_sungzi_desert_hill_01/", "outdoor" },
		{ "metin2_map_sungzi_desert_hill_02/", "outdoor" },
		{ "metin2_map_sungzi_desert_hill_03/", "outdoor" },
		{ "metin2_map_sungzi_flame_hill_01/", "outdoor" },
		{ "metin2_map_sungzi_flame_hill_02/", "outdoor" },
		{ "metin2_map_sungzi_flame_hill_03/", "outdoor" },
		{ "metin2_map_sungzi_milgyo/", "outdoor" },
		{ "metin2_map_sungzi_milgyo_pass_01/", "outdoor" },
		{ "metin2_map_sungzi_milgyo_pass_02/", "outdoor" },
		{ "metin2_map_sungzi_milgyo_pass_03/", "outdoor" },
		{ "metin2_map_sungzi_snow/", "outdoor" },
		{ "metin2_map_sungzi_snow_pass01/", "outdoor" },
		{ "metin2_map_sungzi_snow_pass02/", "outdoor" },
		{ "metin2_map_sungzi_snow_pass03/", "outdoor" },
		{ "metin2_map_t1/", "outdoor" },
		{ "metin2_map_t2/", "outdoor" },
		{ "metin2_map_t3/", "outdoor" },
		{ "metin2_map_t4/", "outdoor" },
		{ "metin2_map_trent/", "outdoor" },
		{ "metin2_map_trent_a/", "outdoor" },
		{ "metin2_map_trent02/", "outdoor" },
		{ "metin2_map_trent02_a/", "outdoor" },
		{ "metin2_map_wedding_01/", "outdoor" },
		{ "metin2_map_wl_01/", "outdoor" },
		{ "property/", "property" },
		{ "uiscript/", "root" },
		{ "sound/ambience/", "sound" },
		{ "sound/common/", "sound" },
		{ "sound/effect/", "sound" },
		{ "sound/monster/", "sound" },
		{ "sound/monster2/", "sound" },
		{ "sound/npc/", "sound" },
		{ "sound/npc2/", "sound" },
		{ "sound/pc/", "sound" },
		{ "sound/pc2/", "sound" },
		{ "sound/ui/", "sound" },
		{ "d:/ymir work/terrainmaps/", "terrainmaps" },
		{ "textureset/", "textureset" },
		{ "d:/ymir work/tree/", "tree" },
		{ "d:/ymir work/zone/", "zone" },
	};

	if (_access(c_pszFolder, 0) != 0)
		return true;

	std::string stFolder(c_pszFolder);
	stFolder += "/";

	const bool bPackFirst = TRUE;

	CEterPackManager::Instance().SetCacheMode();
	CEterPackManager::Instance().SetSearchMode(bPackFirst);

	CSoundData::SetPackMode();

	std::string strPackName, strTexCachePackName;

	for (auto& elem : indexVec)
	{
		const std::string& c_rstFolder = elem[0];
		const std::string& c_rstName = elem[1];

		strPackName = stFolder + c_rstName;
		strTexCachePackName = strPackName + "_texcache";

		CEterPackManager::Instance().RegisterPack(strPackName.c_str(), c_rstFolder.c_str());
		CEterPackManager::Instance().RegisterPack(strTexCachePackName.c_str(), c_rstFolder.c_str());
	}

	CEterPackManager::Instance().RegisterRootPack((stFolder + std::string("root")).c_str());

	NANOEND

		return true;
}
#else
bool PackInitialize(const char *c_pszFolder)
{
	NANOBEGIN
	if (_access(c_pszFolder, 0) != 0)
		return true;

	std::string stFolder(c_pszFolder);
	stFolder += "/";

	std::string stFileName(stFolder);
	stFileName += "Index";

	CMappedFile file;
	LPCVOID pvData;

	if (!file.Create(stFileName.c_str(), &pvData, 0, 0))
	{
		LogBoxf("FATAL ERROR! File not exist: %s", stFileName.c_str());
		TraceError("FATAL ERROR! File not exist: %s", stFileName.c_str());
		return true;
	}

	CMemoryTextFileLoader TextLoader;
	TextLoader.Bind(file.Size(), pvData);

	bool bPackFirst = TRUE;

	const std::string& strPackType = TextLoader.GetLineString(0);

	if (strPackType.compare("FILE") && strPackType.compare("PACK"))
	{
		TraceError("Pack/Index has invalid syntax. First line must be 'PACK' or 'FILE'");
		return false;
	}

	bPackFirst = TRUE;
	Tracef("ľË¸˛: ĆÄŔĎ ¸đµĺŔÔ´Ď´Ů.\n");

	CTextFileLoader::SetCacheMode();
	CEterPackManager::Instance().SetCacheMode();
	CEterPackManager::Instance().SetSearchMode(bPackFirst);

	CSoundData::SetPackMode();

	std::string strPackName, strTexCachePackName;
	for (DWORD i = 1; i < TextLoader.GetLineCount() - 1; i += 2)
	{
		const std::string & c_rstFolder = TextLoader.GetLineString(i);
		const std::string & c_rstName = TextLoader.GetLineString(i + 1);

		strPackName = stFolder + c_rstName;
		strTexCachePackName = strPackName + "_texcache";

		CEterPackManager::Instance().RegisterPack(strPackName.c_str(), c_rstFolder.c_str());
		CEterPackManager::Instance().RegisterPack(strTexCachePackName.c_str(), c_rstFolder.c_str());
	}

	CEterPackManager::Instance().RegisterRootPack((stFolder + std::string("root")).c_str());
	NANOEND
	return true;
}
#endif
#endif

static bool RunMainScript(CPythonLauncher& pyLauncher, const char *lpCmdLine)
{
#ifdef ENABLE_PYTHON_DYNAMIC_MODULE_NAME
	initPythonApi();
#endif
	initpack();
	initdbg();
	initime();
	initgrp();
	initgrpImage();
	initgrpText();
	initwndMgr();
	initudp();
	initapp();
	initsystem();
	initchr();
	initchrmgr();
	initPlayer();
	initItem();
	initNonPlayer();
	initTrade();
	initChat();
	initTextTail();
	initnet();
	initMiniMap();
	initProfiler();
	initEvent();
	initeffect();
	initfly();
	initsnd();
	initeventmgr();
	initshop();
	initskill();
	initquest();
	initBackground();
	initMessenger();
	initsafebox();
	initguild();
	initServerStateChecker();
	initShining();
#ifdef ENABLE_GUILD_RANK_SYSTEM
	initguildranking();
#endif
#ifdef ENABLE_CONFIG_MODULE
	initcfg();
#endif
#ifdef ENABLE_RENEWAL_SWITCHBOT
	initSwitchbot();
#endif
#ifdef ENABLE_RENEWAL_CUBE
	intcuberenewal();
#endif
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	initAcce();
#endif
#ifdef ENABLE_BIOLOG_SYSTEM
	initBiologManager();
#endif
#ifdef ENABLE_RENDER_TARGET
	initRenderTarget();
#endif
#ifdef ENABLE_INGAME_WIKI_SYSTEM
	initWiki();
#endif
#ifdef ENABLE_OFFLINESHOP_SEARCH_SYSTEM
	initprivateShopSearch();
#endif
#ifdef ENABLE_ACHIEVEMENT_SYSTEM
	InitAchievementModule();
#endif

	NANOBEGIN

	PyObject *builtins = PyImport_ImportModule("__builtin__");
#ifdef _DEBUG
	PyModule_AddIntConstant(builtins, "__DEBUG__", 1);
#else
	PyModule_AddIntConstant(builtins, "__DEBUG__", 0);
#endif
#ifdef ENABLE_PYTHON_DYNAMIC_MODULE_NAME
	PyModule_AddIntConstant(builtins, "__USE_DYNAMIC_MODULE__", 1);
#else
	PyModule_AddIntConstant(builtins, "__USE_DYNAMIC_MODULE__", 0);
#endif

	{
		std::string stRegisterCmdLine;

		const char *loginMark = "-cs";
		const char *loginMark_NonEncode = "-ncs";
		const char *seperator = " ";

		std::string stCmdLine;
		const int CmdSize = 3;
		std::vector<std::string> stVec;
		SplitLine(lpCmdLine,seperator,&stVec);

		if (CmdSize == stVec.size() && stVec[0]==loginMark)
		{
			char buf[MAX_PATH];
			base64_decode(stVec[2].c_str(),buf);
			stVec[2] = buf;
			string_join(seperator,stVec,&stCmdLine);
		}
		else if (CmdSize <= stVec.size() && stVec[0]==loginMark_NonEncode)
		{
			stVec[0] = loginMark;
			string_join(" ",stVec,&stCmdLine);
		}
		else
			stCmdLine = lpCmdLine;

		stRegisterCmdLine ="__COMMAND_LINE__ = ";
		stRegisterCmdLine+='"';
		stRegisterCmdLine+=stCmdLine;
		stRegisterCmdLine+='"';

		const CHAR* c_szRegisterCmdLine=stRegisterCmdLine.c_str();
		if (!pyLauncher.RunLine(c_szRegisterCmdLine))
		{
			TraceError("RegisterCommandLine Error");
			return false;
		}
	}
	{
		std::vector<std::string> stVec;
		SplitLine(lpCmdLine," " ,&stVec);

		if (stVec.size() != 0 && "--pause-before-create-window" == stVec[0])
		{
			system("pause");
		}
		if (!pyLauncher.RunFile("system.py"))
		{
			TraceError("RunMain Error");
			return false;
		}
	}

	NANOEND
	return true;
}

static bool Main(HINSTANCE hInstance, LPSTR lpCmdLine)
{
	DWORD dwRandSeed = time(NULL)+DWORD(GetCurrentProcess());
	srandom(dwRandSeed);
	srand(random());
	SetLogLevel(1);
#ifdef NUBIRUS2_SHIELD
	ShieldHelper shield_helper;
	ShieldHelper::Instance().AddFileEntry("miles/mss32.dll", "6400e224b8b44ece59a992e6d8233719");
	ShieldHelper::Instance().AddFileEntry("miles/mssa3d.m3d", "e089ce52b0617a6530069f22e0bdba2a");
	ShieldHelper::Instance().AddFileEntry("miles/mssds3d.m3d", "85267776d45dbf5475c7d9882f08117c");
	ShieldHelper::Instance().AddFileEntry("miles/mssdsp.flt", "cb71b1791009eca618e9b1ad4baa4fa9");
	ShieldHelper::Instance().AddFileEntry("miles/mssdx7.m3d", "2727e2671482a55b2f1f16aa88d2780f");
	ShieldHelper::Instance().AddFileEntry("miles/msseax.m3d", "788bd950efe89fa5166292bd6729fa62");
	ShieldHelper::Instance().AddFileEntry("miles/mssmp3.asi", "189576dfe55af3b70db7e3e2312cd0fd");
	ShieldHelper::Instance().AddFileEntry("miles/mssrsx.m3d", "7fae15b559eb91f491a5f75cfa103cd4");
	ShieldHelper::Instance().AddFileEntry("miles/msssoft.m3d", "bdc9ad58ade17dbd939522eee447416f");
	ShieldHelper::Instance().AddFileEntry("miles/mssvoice.asi", "3d5342edebe722748ace78c930f4d8a5");
	ShieldHelper::Instance().AddFileEntry("python27.dll", "fdc67f415f59af6404cf854c29098eca");

	if (!ShieldHelper::Instance().ActivateProtection())
	{
		MessageBox(NULL, "Nem tudod elindítani a játékot..", (IDS_APP_NAME, "Ayalon2 Rework"), MB_ICONSTOP);
		return false;
	}

#endif
	ApplyRestrictedDaclToCurrentProcess();
	TryConnectShieldPipeFromCmdLine(lpCmdLine);
	ilInit();

	if (!Setup(lpCmdLine))
		return false;

	//if (strstr(lpCmdLine, "--koKYMZSznWMPqsNMeXXD") == 0)
	//{
	//	MessageBox(NULL, "Használd a patchert..", (IDS_APP_NAME, "Ayalon2 Rework"), MB_ICONSTOP);
	//		return false;
	//}
#ifdef _DEBUG
	OpenConsoleWindow();
	OpenLogFile(true);
#else
	OpenLogFile(false);
#endif

	static CLZO lzo;
	static CEterPackManager EterPackManager;
#ifdef ENABLE_CLIENT_PERFORMANCE
	static CPythonPlayerSettingsModule PlayerSettings;
#endif

#ifdef ENABLE_CONFIG_MODULE
	static CPythonConfig m_pyConfig;
	m_pyConfig.Initialize("config.cfg");
#endif

	if (!PackInitialize("pack"))
	{
		LogBox("Pack Initialization failed. Check log.txt file..");
		return false;
	}

	CPythonApplication* app = new CPythonApplication;

	app->Initialize(hInstance);

	HANDLE hThr = (HANDLE)_beginthreadex(NULL, 0, WindowTitleRandomizerThread, NULL, 0, NULL);
	if (hThr) CloseHandle(hThr);

	bool ret=false;
	{
		CPythonLauncher pyLauncher;
		CPythonExceptionSender pyExceptionSender;
		SetExceptionSender(&pyExceptionSender);

		if (pyLauncher.Create())
		{
			ret=RunMainScript(pyLauncher, lpCmdLine);
		}

		app->Clear();

		timeEndPeriod(1);
		pyLauncher.Clear();
	}

	app->Destroy();
	delete app;
	
	return ret;
}

HANDLE CreateMetin2GameMutex()
{
	SECURITY_ATTRIBUTES sa;
	ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = FALSE;

	return CreateMutex(&sa, FALSE, "Metin2GameMutex");
}

static void DestroyMetin2GameMutex(HANDLE hMutex)
{
	if (hMutex)
	{
		ReleaseMutex(hMutex);
		hMutex = NULL;
	}
}

void __ErrorPythonLibraryIsNotExist()
{
	LogBoxf("FATAL ERROR!! Python Library file not exist!");
}

bool __IsTimeStampOption(LPSTR lpCmdLine)
{
	const char *TIMESTAMP = "/timestamp";
	return (strncmp(lpCmdLine, TIMESTAMP, strlen(TIMESTAMP))==0);
}

void __PrintTimeStamp()
{
#ifdef _DEBUG
	LogBoxf("METIN2 BINARY DEBUG VERSION %s ( MS C++ %d Compiled )", __TIMESTAMP__, _MSC_VER);
#else
	LogBoxf("METIN2 BINARY DISTRIBUTE VERSION %s ( MS C++ %d Compiled )", __TIMESTAMP__, _MSC_VER);
#endif
}

bool __IsLocaleOption(LPSTR lpCmdLine)
{
	return (strcmp(lpCmdLine, "--locale") == 0);
}

bool __IsLocaleVersion(LPSTR lpCmdLine)
{
	return (strcmp(lpCmdLine, "--perforce-revision") == 0);
}


#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

//HLBOT SHIELD KOBRA21+50 #LICENSE\\

#include <windows.h>
#include <tlhelp32.h>

typedef void (*StartHLBotShieldFunc)();
// PE fejléc törlése a memóriából (anti-dump)
void ErasePEHeader()
{
	HMODULE hModule = GetModuleHandle(NULL);
	if (!hModule)
		return;

	DWORD oldProtect;
	IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)hModule;
	IMAGE_NT_HEADERS* ntHeader = (IMAGE_NT_HEADERS*)((BYTE*)hModule + dosHeader->e_lfanew);

	if (VirtualProtect((void*)hModule, ntHeader->OptionalHeader.SizeOfHeaders, PAGE_READWRITE, &oldProtect))
	{
		SecureZeroMemory((void*)hModule, ntHeader->OptionalHeader.SizeOfHeaders);
		VirtualProtect((void*)hModule, ntHeader->OptionalHeader.SizeOfHeaders, oldProtect, &oldProtect);
	}
}
// Debugger, IDA, CE detektálás
bool IsDebuggerDetected()
{
	const char* targets[] = {
		"ida.exe", "ida64.exe", "idag.exe" "RextBot","Cypor", "Cheat Enginge",
		"CheatEngine.exe", "HLBot", "Cheat Engine.exe",
		"x64dbg.exe", "x32dbg.exe", "cheatengine.exe", "ollydbg.exe"
	};

	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snap == INVALID_HANDLE_VALUE)
		return false;

	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(entry);

	if (!Process32First(snap, &entry)) {
		CloseHandle(snap);
		return false;
	}

	do {
		for (auto& name : targets)
			if (_stricmp(entry.szExeFile, name) == 0) {
				CloseHandle(snap);
				return true;
			}
	} while (Process32Next(snap, &entry));

	CloseHandle(snap);
	return false;
}

#include <string>
#include <process.h>

static std::string MakeRandomWindowTitle()
{
	char buf[64];
	DWORD pid = GetCurrentProcessId();
	DWORD tick = GetTickCount();
	snprintf(buf, sizeof(buf), "win_%u_%u", pid % 100000, tick % 1000000);
	return std::string(buf);
}

static BOOL CALLBACK EnumWindowsProc_FindAndRename(HWND hWnd, LPARAM lParam)
{
	DWORD pid = 0;
	GetWindowThreadProcessId(hWnd, &pid);
	if (pid != GetCurrentProcessId())
		return TRUE;

	// csak látható top-level ablakokra
	if (!IsWindowVisible(hWnd))
		return TRUE;

	std::string newTitle = MakeRandomWindowTitle();
	SetWindowTextA(hWnd, newTitle.c_str());
	// egyszer átneveztük, visszatérve true is ok - de folytatjuk, hátha több ablak is kell
	return TRUE;
}

unsigned int __stdcall WindowTitleRandomizerThread(void*)
{
	// próbáljunk találni és átírni az ablak címét 5s-ig, majd minden 10s-ig frissítjük
	for (int i = 0; i < 50; ++i)
	{
		EnumWindows(EnumWindowsProc_FindAndRename, 0);
		Sleep(100);
	}

	// hosszabb távon is frissítünk időnként (opcionális)
	while (true)
	{
		EnumWindows(EnumWindowsProc_FindAndRename, 0);
		Sleep(10000);
	}
	return 0;
}

// Funkció ami meghívja a DLL-t, és exportált védelmi függvényt
void CallHLBotShield()
{

	if (IsDebuggerDetected())
	{
		MessageBoxA(NULL, "Debugger észlelve!", "HLBotShield", MB_ICONERROR);
		ExitProcess(0);
	}

	// XOR-ozott DLL név (ne lehessen keresni)
	unsigned char dll[] = { 'H' ^ 0x12, 'L' ^ 0x23, 'B' ^ 0x34, 'o' ^ 0x45, 't' ^ 0x56, 'S' ^ 0x67, 'h' ^ 0x78, 'i' ^ 0x89, 'e' ^ 0x9A, 'l' ^ 0xAB, 'd' ^ 0xBC, '.' ^ 0xCD, 'd' ^ 0xDE, 'l' ^ 0xEF, 'l' ^ 0xF0, 0 };
	unsigned char keys[] = { 0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78, 0x89, 0x9A, 0xAB, 0xBC, 0xCD, 0xDE, 0xEF, 0xF0 };

	for (int i = 0; dll[i]; ++i)
		dll[i] ^= keys[i];

	HMODULE hDll = LoadLibraryA((LPCSTR)dll);
	if (!hDll)
	{
		MessageBoxA(NULL, "Hiányzik egy függőség! (VCRedist Telepítő a kliensben)", "HLBotShield", MB_ICONERROR);
		ExitProcess(0);
	}

	// XOR-ozott exportnév: "StartHLBotShield"
	char obf[] = { 'S' ^ 0x5A, 't' ^ 0x5A, 'a' ^ 0x5A, 'r' ^ 0x5A, 't' ^ 0x5A, 'H' ^ 0x5A, 'L' ^ 0x5A, 'B' ^ 0x5A, 'o' ^ 0x5A, 't' ^ 0x5A, 'S' ^ 0x5A, 'h' ^ 0x5A, 'i' ^ 0x5A, 'e' ^ 0x5A, 'l' ^ 0x5A, 'd' ^ 0x5A, 0 };
	for (int i = 0; obf[i] != 0; ++i)
		obf[i] ^= 0x5A;

	StartHLBotShieldFunc StartFunc = (StartHLBotShieldFunc)GetProcAddress(hDll, obf);
	if (!StartFunc)
	{
		MessageBoxA(NULL, "Védelem hibás: export hiányzik!", "HLBotShield", MB_ICONERROR);
		ExitProcess(0);
	}

	StartFunc();
}

DWORD WINAPI DelayedShieldLoader(LPVOID)
{
	// véletlen késleltetés (4-12s)
	srand((unsigned)time(NULL) ^ GetTickCount());
	int delayMs = 1000 + (rand() % 8000);
	Sleep(delayMs);

	// Load és Start hívás (ugyanaz mint a CallHLBotShield)
	// XOR-olt név visszafejtése, stb. másold be a meglévő CallHLBotShield logikáját ide
	CallHLBotShield();
	return 0;
}



int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// ---- DEBUG heap (opcionális, de ajánlott Debug buildben) ----
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF
		| _CRTDBG_LEAK_CHECK_DF
		| _CRTDBG_CHECK_ALWAYS_DF
		| _CRTDBG_DELAY_FREE_MEM_DF);
#endif

	LoadConfig ("locale.cfg");
	SetDefaultCodePage(GetCodePage());

	int nArgc = 0;
	PCHAR* szArgv = CommandLineToArgv( lpCmdLine, &nArgc );
	WebBrowser_Startup(hInstance);

	if (!CheckPythonLibraryFilenames())
	{
		__ErrorPythonLibraryIsNotExist();
		goto Clean;
	}

	CreateThread(NULL, 0, DelayedShieldLoader, NULL, 0, NULL);
	//Protection();


	Main(hInstance, lpCmdLine);
	WebBrowser_Cleanup();
	::CoUninitialize();

Clean:
	SAFE_FREE_GLOBAL(szArgv);
	//ErasePEHeader();
	return 0;
}

static void GrannyError(granny_log_message_type Type,
	granny_log_message_origin Origin,
	char const* File,
	granny_int32x Line,
	char const* Message,
	void* UserData)
{
	TraceError("GRANNY: %s", Message);
}

int Setup(LPSTR lpCmdLine)
{
	TIMECAPS tc;
	UINT wTimerRes;

	if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR)
		return 0;

	wTimerRes = MINMAX(tc.wPeriodMin, 1, tc.wPeriodMax);
	timeBeginPeriod(wTimerRes); 

	granny_log_callback Callback;
	Callback.Function = nullptr;
	Callback.UserData = 0;
	GrannySetLogCallback(&Callback);
	return 1;
}
