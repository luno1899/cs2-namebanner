#pragma once

#include "common.h"

class CGameResourceService;
class IVEngineServer2;

namespace interfaces
{
	inline CGameResourceService *pGameResourceServiceServer = nullptr;
	inline IVEngineServer2 *pEngine = nullptr;

	template<typename T>
	inline void Find(ISmmAPI *ismm, CreateInterfaceFn factory, const char *version, const char *plainName, T *&result,
					 std::vector<std::string> &missing)
	{
		result = static_cast<T *>(ismm->VInterfaceMatch(factory, version));
		if (!result)
		{
			missing.emplace_back(std::string("The server did not provide ") + plainName + ".");
		}
	}

	inline void Initialize(ISmmAPI *ismm, std::vector<std::string> &missing)
	{
		Find(ismm, ismm->GetEngineFactory(), CVAR_INTERFACE_VERSION, "the console variable interface", g_pCVar, missing);
		Find(ismm, ismm->GetEngineFactory(), GAMERESOURCESERVICESERVER_INTERFACE_VERSION, "the game resource service", pGameResourceServiceServer,
			 missing);
		Find(ismm, ismm->GetServerFactory(), INTERFACEVERSION_SERVERGAMECLIENTS, "the game client interface", g_pSource2GameClients, missing);
		Find(ismm, ismm->GetEngineFactory(), INTERFACEVERSION_VENGINESERVER, "the engine server interface", pEngine, missing);
		Find(ismm, ismm->GetEngineFactory(), NETWORKSERVERSERVICE_INTERFACE_VERSION, "the network server interface", g_pNetworkServerService,
			 missing);
		Find(ismm, ismm->GetEngineFactory(), SCHEMASYSTEM_INTERFACE_VERSION, "the schema interface", g_pSchemaSystem, missing);
		Find(ismm, ismm->GetFileSystemFactory(), FILESYSTEM_INTERFACE_VERSION, "the filesystem interface", g_pFullFileSystem, missing);
	}
} // namespace interfaces
