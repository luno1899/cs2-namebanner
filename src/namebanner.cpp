#include "namebanner.h"

#include <eiface.h>
#include <filesystem.h>
#include <KeyValues.h>

NameBanner g_NameBanner;
PLUGIN_EXPOSE(NameBanner, g_NameBanner);

static IVEngineServer2 *g_pEngine = nullptr;
static IServerGameClients *g_pGameClients = nullptr;
static IPlayersApi *g_pPlayers = nullptr;
static IAdminApi *g_pAdmin = nullptr;
static int g_settingsChangedHookId = 0;

SH_DECL_HOOK1_void(IServerGameClients, ClientSettingsChanged, SH_NOATTRIB, false, CPlayerSlot);

static void HookClientSettingsChangedPost(CPlayerSlot slot)
{
	g_NameBanner.OnClientSettingsChanged(slot.Get());
	RETURN_META(MRES_IGNORED);
}

static void ReplaceAll(std::string &value, const std::string &placeholder, const std::string &replacement)
{
	for (size_t pos = 0; (pos = value.find(placeholder, pos)) != std::string::npos; pos += replacement.size())
	{
		value.replace(pos, placeholder.size(), replacement);
	}
}

bool NameBanner::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, g_pEngine, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetServerFactory, g_pGameClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	Msg("[NameBanner] %s loaded (late=%d), waiting for the other plugins.\n", PLUGIN_FULL_VERSION, late ? 1 : 0);
	return true;
}

bool NameBanner::Unload(char *error, size_t maxlen)
{
	if (g_settingsChangedHookId)
	{
		SH_REMOVE_HOOK_ID(g_settingsChangedHookId);
		g_settingsChangedHookId = 0;
	}
	// Utils drops our IPlayerListener on its own when Metamod unloads this plugin.
	return true;
}

void NameBanner::AllPluginsLoaded()
{
	auto fail = [&](const char *message)
	{
		ConColorMsg(Color(255, 0, 0, 255), "[NameBanner] %s\n", message);
		if (g_pEngine)
		{
			std::string command = "meta unload " + std::to_string(g_PLID);
			g_pEngine->ServerCommand(command.c_str());
		}
	};

	LoadConfig();

	int ret = 0;
	g_pPlayers = static_cast<IPlayersApi *>(g_SMAPI->MetaFactory(PLAYERS_INTERFACE, &ret, nullptr));
	if (ret == META_IFACE_FAILED || !g_pPlayers)
	{
		fail("Missing Utils plugin (Pisex/cs2-menus 1.9.2 or newer).");
		return;
	}

	g_pAdmin = static_cast<IAdminApi *>(g_SMAPI->MetaFactory(Admin_INTERFACE, &ret, nullptr));
	if (ret == META_IFACE_FAILED || !g_pAdmin)
	{
		fail("Missing Admin System plugin (Pisex/cs2-admin_system).");
		return;
	}

	Msg("[NameBanner] Utils players API found (%p), Admin System %.2f found (%p).\n", static_cast<void *>(g_pPlayers),
		g_pAdmin->GetPluginVersion(), static_cast<void *>(g_pAdmin));
	Msg("[NameBanner] Config: threshold=%d window=%.0fs ban_minutes=%d debug=%d\n", threshold, windowSeconds, banMinutes, debug ? 1 : 0);
	g_pPlayers->AddListener(g_PLID, this);
	g_settingsChangedHookId = SH_ADD_HOOK(IServerGameClients, ClientSettingsChanged, g_pGameClients,
		SH_STATIC(HookClientSettingsChangedPost), true);
	if (!g_settingsChangedHookId)
	{
		fail("Could not install the post ClientSettingsChanged hook.");
		return;
	}
	Msg("[NameBanner] Utils listener and post ClientSettingsChanged hook registered.\n");

	// Players who were already connected when the plugin loaded.
	for (int slot = 0; slot < MAXPLAYERS; ++slot)
	{
		SeedBaseline(slot);
	}
}

void NameBanner::LoadConfig()
{
	KeyValues::AutoDelete kv("NameBanner");
	if (!g_pFullFileSystem || !kv->LoadFromFile(g_pFullFileSystem, "addons/configs/NameBanner/settings.ini"))
	{
		Msg("[NameBanner] No configs/NameBanner/settings.ini found, using defaults.\n");
		return;
	}
	threshold = kv->GetInt("threshold", threshold);
	windowSeconds = kv->GetFloat("window_seconds", windowSeconds);
	banMinutes = kv->GetInt("ban_minutes", banMinutes);
	banReason = kv->GetString("reason", banReason.c_str());
	debug = kv->GetBool("debug", debug);
}

bool NameBanner::IsEligible(int slot) const
{
	return slot >= 0 && slot < MAXPLAYERS && g_pPlayers && g_pPlayers->IsInGame(slot) && !g_pPlayers->IsFakeClient(slot);
}

// This is called from a post-hook, so the engine has already applied the new userinfo.
const char *NameBanner::ClientName(int slot) const
{
	return g_pEngine ? g_pEngine->GetClientConVarValue(CPlayerSlot(slot), "name") : nullptr;
}

void NameBanner::SeedBaseline(int slot)
{
	if (slot < 0 || slot >= MAXPLAYERS)
	{
		return;
	}
	auto &data = playerData[slot];
	data = {};
	if (!IsEligible(slot))
	{
		return;
	}
	if (const char *name = ClientName(slot))
	{
		data.lastName = name;
		data.initialized = true;
	}
}

void NameBanner::ClientFullyConnect(int slot)
{
	if (debug)
	{
		Msg("[NameBanner] ClientFullyConnect slot %d\n", slot);
	}
	SeedBaseline(slot);
}

void NameBanner::ClientDisconnect(int slot)
{
	if (debug)
	{
		Msg("[NameBanner] ClientDisconnect slot %d\n", slot);
	}
	if (slot >= 0 && slot < MAXPLAYERS)
	{
		playerData[slot] = {};
	}
}

void NameBanner::OnClientSettingsChanged(int slot)
{
	if (!IsEligible(slot))
	{
		if (debug)
		{
			Msg("[NameBanner] ClientSettingsChanged post slot %d ignored (not eligible)\n", slot);
		}
		return;
	}
	const char *name = ClientName(slot);
	if (!name)
	{
		return;
	}

	auto &data = playerData[slot];
	if (!data.initialized)
	{
		data.lastName = name;
		data.initialized = true;
		return;
	}
	if (data.lastName == name)
	{
		return;
	}
	const std::string previousName = data.lastName;
	data.lastName = name;

	const auto now = std::chrono::steady_clock::now();
	const auto window = std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<float>(windowSeconds));
	while (!data.changes.empty() && now - data.changes.front() >= window)
	{
		data.changes.pop_front();
	}
	data.changes.push_back(now);
	if (debug)
	{
		Msg("[NameBanner] slot %d name changed '%s' -> '%s'; count=%zu/%d within %.0fs\n", slot,
			previousName.c_str(), name, data.changes.size(), (std::max)(1, threshold), windowSeconds);
	}

	if (data.changes.size() >= static_cast<size_t>((std::max)(1, threshold)))
	{
		const size_t changes = data.changes.size();
		data.changes.clear();
		Ban(slot, name, changes);
	}
}

void NameBanner::Ban(int slot, const char *name, size_t changes)
{
	auto &data = playerData[slot];
	if (data.banned || !g_pAdmin)
	{
		return;
	}
	data.banned = true;

	std::string reason = banReason;
	ReplaceAll(reason, "{changes}", std::to_string(changes));
	Msg("[NameBanner] Threshold reached for %s: %zu changes within %.0f seconds; sending ban to Admin System.\n",
		name, changes, windowSeconds);
	g_pAdmin->AddPlayerPunishment(slot, RT_BAN, banMinutes, reason.c_str());
	Msg("[NameBanner] Admin System accepted the ban request for %s (minutes=%d, reason='%s').\n", name, banMinutes,
		reason.c_str());
}
