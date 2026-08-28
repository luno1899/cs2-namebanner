#include "hooks.h"

#include "namebanner.h"
#include "player/player.h"
#include "utils/interfaces.h"

#include "iserver.h"

SH_DECL_HOOK1_void(ISource2GameClients, ClientFullyConnect, SH_NOATTRIB, false, CPlayerSlot);
SH_DECL_HOOK1_void(ISource2GameClients, ClientSettingsChanged, SH_NOATTRIB, false, CPlayerSlot);
SH_DECL_HOOK5_void(ISource2GameClients, ClientDisconnect, SH_NOATTRIB, false, CPlayerSlot, ENetworkDisconnectionReason, const char *, uint64,
				   const char *);

namespace
{
	struct HookEntry
	{
		int id;
		const char *name;
	};

	CUtlVector<HookEntry> hookIds;

	void HookClientFullyConnect(CPlayerSlot slot)
	{
		if (!g_NameBanner.IsLoaded())
		{
			RETURN_META(MRES_IGNORED);
		}
		g_pPlayerManager->OnClientFullyConnect(slot);
		g_NameBanner.OnClientFullyConnect(slot);
		RETURN_META(MRES_IGNORED);
	}

	void HookClientSettingsChanged(CPlayerSlot slot)
	{
		if (!g_NameBanner.IsLoaded())
		{
			RETURN_META(MRES_IGNORED);
		}
		g_NameBanner.OnClientSettingsChanged(slot);
		RETURN_META(MRES_IGNORED);
	}

	void HookClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason, const char *, uint64, const char *)
	{
		if (!g_NameBanner.IsLoaded())
		{
			RETURN_META(MRES_IGNORED);
		}
		g_NameBanner.OnClientDisconnect(slot);
		g_pPlayerManager->OnClientDisconnect(slot);
		RETURN_META(MRES_IGNORED);
	}
} // namespace

bool hooks::Initialize(std::vector<std::string> &missing)
{
	auto add = [&](int id, const char *name)
	{
		if (id)
		{
			hookIds.AddToTail({id, name});
		}
		else
		{
			missing.emplace_back(std::string("The ") + name + " server hook could not be started.");
		}
	};
	add(SH_ADD_HOOK(ISource2GameClients, ClientFullyConnect, g_pSource2GameClients, SH_STATIC(HookClientFullyConnect), true),
		"fully connected player");
	add(SH_ADD_HOOK(ISource2GameClients, ClientSettingsChanged, g_pSource2GameClients, SH_STATIC(HookClientSettingsChanged), true),
		"player setting update");
	add(SH_ADD_HOOK(ISource2GameClients, ClientDisconnect, g_pSource2GameClients, SH_STATIC(HookClientDisconnect), true), "disconnecting player");

	if (!missing.empty())
	{
		Cleanup();
		return false;
	}
	return true;
}

bool hooks::Cleanup()
{
	bool removed = true;
	for (int i = hookIds.Count() - 1; i >= 0; --i)
	{
		if (SH_REMOVE_HOOK_ID(hookIds[i].id))
		{
			hookIds.Remove(i);
		}
		else
		{
			Warning("[NameBanner] The %s hook could not be removed yet. Metamod will try again during unload.\n", hookIds[i].name);
			removed = false;
		}
	}
	return removed;
}
