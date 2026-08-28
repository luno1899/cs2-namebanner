#include "utils.h"

#include "sdk/cgameresourceserviceserver.h"
#include "sdk/serversideclient.h"
#include "utils/gameconfig.h"
#include "iserver.h"

CGameConfig *g_pGameConfig = nullptr;
static NBUtils s_NBUtils;
NBUtils *g_pNBUtils = &s_NBUtils;

void utils::Initialize(std::vector<std::string> &missing)
{
	if (!interfaces::pEngine || !g_pFullFileSystem)
	{
		return;
	}

	CBufferStringGrowable<256> gameDirectory;
	interfaces::pEngine->GetGameDir(gameDirectory);
	g_pGameConfig = new CGameConfig(CGameConfig::GetDirectoryName(gameDirectory.Get()), "addons/namebanner/gamedata/namebanner.games.txt");

	char error[256] {};
	if (!g_pGameConfig->Init(g_pFullFileSystem, error, sizeof(error)))
	{
		missing.emplace_back(std::string("The NameBanner game data could not be read: ") + error + ".");
	}
}

void utils::Cleanup()
{
	delete g_pGameConfig;
	g_pGameConfig = nullptr;
}

CUtlVector<CServerSideClient *> *NBUtils::GetClientList()
{
	if (!g_pNetworkServerService || !g_pGameConfig || g_pGameConfig->GetOffset("ClientOffset") < 0)
	{
		return nullptr;
	}
	auto *server = g_pNetworkServerService->GetIGameServer();
	return server ? reinterpret_cast<CUtlVector<CServerSideClient *> *>(reinterpret_cast<char *>(server) + g_pGameConfig->GetOffset("ClientOffset"))
				  : nullptr;
}

CServerSideClient *NBUtils::GetClientBySlot(CPlayerSlot slot)
{
	auto *clients = GetClientList();
	return clients && slot.Get() >= 0 && slot.Get() < clients->Count() ? clients->Element(slot.Get()) : nullptr;
}

CGameEntitySystem *GameEntitySystem()
{
	return interfaces::pGameResourceServiceServer ? interfaces::pGameResourceServiceServer->GetGameEntitySystem() : nullptr;
}
