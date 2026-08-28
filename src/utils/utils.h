#pragma once

#include "common.h"
#include "utils/interfaces.h"

class CGameConfig;
class CServerSideClient;

class NBUtils
{
public:
	CUtlVector<CServerSideClient *> *GetClientList();
	CServerSideClient *GetClientBySlot(CPlayerSlot slot);
};

extern NBUtils *g_pNBUtils;
extern CGameConfig *g_pGameConfig;

namespace utils
{
	// Reads gamedata/namebanner.games.txt and resolves the offsets we need
	// (no signature scanning is required for name-change detection).
	void Initialize(std::vector<std::string> &missing);
	void Cleanup();
} // namespace utils

CGameEntitySystem *GameEntitySystem();
