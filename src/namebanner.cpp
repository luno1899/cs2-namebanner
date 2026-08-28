#include "namebanner.h"

#include "utils/interfaces.h"
#include "utils/utils.h"

#include "iserver.h"
#include "KeyValues.h"

NameBannerPlugin g_NameBanner;

PLUGIN_EXPOSE(NameBannerPlugin, g_NameBanner);

Player *g_players[MAXPLAYERS + 1] {};

namespace
{
	IAdminApi *g_pAdmin {};

	void CreatePlayers()
	{
		if (g_players[0])
		{
			return;
		}
		for (i32 i = 0; i <= MAXPLAYERS; ++i)
		{
			g_players[i] = new Player(i);
		}
	}

	void ReplaceAll(std::string &value, const std::string &placeholder, const std::string &replacement)
	{
		for (size_t position = 0; (position = value.find(placeholder, position)) != std::string::npos; position += replacement.size())
		{
			value.replace(position, placeholder.size(), replacement);
		}
	}

	bool IsEligibleHuman(Player *player)
	{
		return player && player->index >= 1 && player->index <= MAXPLAYERS && player->GetController() && !player->IsFakeClient()
			   && !player->IsCSTV();
	}

	std::string JoinReasons(const std::vector<std::string> &reasons)
	{
		std::string message;
		for (const auto &reason : reasons)
		{
			if (!message.empty())
			{
				message += ' ';
			}
			message += reason;
		}
		return message;
	}
} // namespace

// -- Player ------------------------------------------------------------

CBasePlayerController *Player::GetController()
{
	if (!GameEntitySystem())
	{
		return nullptr;
	}
	auto *entity = static_cast<CBaseEntity *>(GameEntitySystem()->GetEntityInstance(CEntityIndex(index)));
	return entity && entity->IsController() ? static_cast<CBasePlayerController *>(entity) : nullptr;
}

CServerSideClient *Player::GetClient()
{
	return g_pNBUtils ? g_pNBUtils->GetClientBySlot(GetPlayerSlot()) : nullptr;
}

bool Player::IsInGame()
{
	auto *client = GetClient();
	return client && client->IsInGame();
}

bool Player::IsFakeClient()
{
	auto *client = GetClient();
	return client && client->IsFakeClient();
}

bool Player::IsCSTV()
{
	auto *client = GetClient();
	return client && client->IsHLTV();
}

const char *Player::GetName()
{
	auto *client = GetClient();
	sanitizedName = client ? client->GetClientName() : "<unknown>";
	sanitizedName.Trim();
	return sanitizedName.Get();
}

u64 Player::GetSteamId64()
{
	auto *client = GetClient();
	if (client)
	{
		return client->GetClientSteamID().ConvertToUint64();
	}
	auto *controller = GetController();
	return controller ? controller->m_steamID() : 0;
}

Player *PlayerFromSlot(CPlayerSlot slot)
{
	const i32 index = slot.Get() + 1;
	return index >= 1 && index <= MAXPLAYERS ? g_players[index] : nullptr;
}

// -- Client lifecycle hooks ----------------------------------------------

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
		if (g_NameBanner.IsLoaded())
		{
			g_NameBanner.OnClientFullyConnect(slot);
		}
		RETURN_META(MRES_IGNORED);
	}

	void HookClientSettingsChanged(CPlayerSlot slot)
	{
		if (g_NameBanner.IsLoaded())
		{
			g_NameBanner.OnClientSettingsChanged(slot);
		}
		RETURN_META(MRES_IGNORED);
	}

	void HookClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason, const char *, uint64, const char *)
	{
		if (g_NameBanner.IsLoaded())
		{
			g_NameBanner.OnClientDisconnect(slot);
		}
		RETURN_META(MRES_IGNORED);
	}

	bool InitializeHooks(std::vector<std::string> &missing)
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
		add(SH_ADD_HOOK(ISource2GameClients, ClientDisconnect, g_pSource2GameClients, SH_STATIC(HookClientDisconnect), true),
			"disconnecting player");
		return missing.empty();
	}

	bool CleanupHooks()
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
} // namespace

// -- Plugin ----------------------------------------------------------------

bool NameBannerPlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();
	if (late)
	{
		if (!Activate(error, maxlen, true))
		{
			return false;
		}
		ismm->AddListener(this, this);
	}
	else
	{
		activationPending = true;
	}
	return true;
}

void NameBannerPlugin::AllPluginsLoaded()
{
	if (!activationPending)
	{
		return;
	}
	activationPending = false;
	char error[1024] {};
	if (!Activate(error, sizeof(error), false))
	{
		activationError = error[0] ? error : "NameBanner could not finish loading after the game became ready.";
		Msg("[NameBanner] NameBanner is not running. %s\n", activationError.c_str());
		return;
	}
	g_SMAPI->AddListener(this, this);
}

bool NameBannerPlugin::QueryRunning(char *error, size_t maxlen)
{
	if (loaded)
	{
		return true;
	}
	const char *reason = activationError.empty() ? "NameBanner is waiting for the game to finish starting." : activationError.c_str();
	if (error && maxlen)
	{
		snprintf(error, maxlen, "%s", reason);
	}
	return false;
}

bool NameBannerPlugin::Activate(char *error, size_t maxlen, bool late)
{
	std::vector<std::string> missing;
	if (!g_SHPtr)
	{
		missing.emplace_back("Metamod's hook service is unavailable.");
	}

	interfaces::Initialize(g_SMAPI, missing);
	utils::Initialize(missing);

	if (g_pGameConfig)
	{
		if (g_pGameConfig->GetOffset("GameEntitySystem") < 0)
		{
			missing.emplace_back("The game entity system offset is unavailable.");
		}
		if (g_pGameConfig->GetOffset("ClientOffset") < 0)
		{
			missing.emplace_back("The player list offset is unavailable.");
		}
		if (g_pGameConfig->GetOffset("IsEntityPawn") < 0)
		{
			missing.emplace_back("The entity pawn check offset is unavailable.");
		}
		if (g_pGameConfig->GetOffset("IsEntityController") < 0)
		{
			missing.emplace_back("The entity controller check offset is unavailable.");
		}
	}

	int ret = 0;
	g_pAdmin = static_cast<IAdminApi *>(g_SMAPI->MetaFactory(Admin_INTERFACE, &ret, nullptr));
	if (ret == META_IFACE_FAILED || !g_pAdmin)
	{
		missing.emplace_back("The Admin System plugin (Pisex/cs2-admin_system) is not loaded, so bans have nowhere to go.");
	}

	if (!missing.empty())
	{
		if (error && maxlen)
		{
			snprintf(error, maxlen, "%s", JoinReasons(missing).c_str());
		}
		for (const auto &reason : missing)
		{
			Msg("[NameBanner] %s\n", reason.c_str());
		}
		CleanupRuntime();
		return false;
	}

	CreatePlayers();
	LoadConfig();
	playerData = {};
	alreadyBanned = {};

	if (!InitializeHooks(missing))
	{
		if (error && maxlen)
		{
			snprintf(error, maxlen, "%s", JoinReasons(missing).c_str());
		}
		for (const auto &reason : missing)
		{
			Msg("[NameBanner] %s\n", reason.c_str());
		}
		CleanupRuntime();
		return false;
	}

	loaded = true;
	if (late)
	{
		for (i32 i = 1; i <= MAXPLAYERS; ++i)
		{
			auto *player = g_players[i];
			if (player && player->IsInGame())
			{
				OnClientFullyConnect(player->GetPlayerSlot());
			}
		}
	}
	Msg("[NameBanner] NameBanner %s loaded successfully.\n", PLUGIN_FULL_VERSION);
	return true;
}

void NameBannerPlugin::LoadConfig()
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
}

void NameBannerPlugin::CleanupRuntime()
{
	CleanupHooks();
	utils::Cleanup();
	loaded = false;
}

bool NameBannerPlugin::Unload(char *error, size_t maxlen)
{
	if (!CleanupHooks())
	{
		const char *reason = "NameBanner could not unload because one of its server hooks could not be removed safely.";
		if (error && maxlen)
		{
			snprintf(error, maxlen, "%s", reason);
		}
		Msg("[NameBanner] %s\n", reason);
		return false;
	}
	CleanupRuntime();
	Msg("[NameBanner] NameBanner unloaded successfully.\n");
	return true;
}

bool NameBannerPlugin::Pause(char *error, size_t maxlen)
{
	const char *reason = "NameBanner cannot be paused safely. Unload it instead.";
	if (error && maxlen)
	{
		snprintf(error, maxlen, "%s", reason);
	}
	return false;
}

bool NameBannerPlugin::Unpause(char *error, size_t maxlen)
{
	const char *reason = "NameBanner is not paused.";
	if (error && maxlen)
	{
		snprintf(error, maxlen, "%s", reason);
	}
	return false;
}

// -- Name-change detection --------------------------------------------------

void NameBannerPlugin::OnClientFullyConnect(CPlayerSlot slot)
{
	auto *player = PlayerFromSlot(slot);
	if (!IsEligibleHuman(player) || !player->IsInGame())
	{
		return;
	}
	auto &data = playerData[player->index];
	data = {};
	const char *name = player->GetName();
	if (name)
	{
		data.lastName = name;
		data.initialized = true;
	}
}

void NameBannerPlugin::OnClientSettingsChanged(CPlayerSlot slot)
{
	auto *player = PlayerFromSlot(slot);
	if (!IsEligibleHuman(player) || !player->IsInGame())
	{
		return;
	}
	const char *currentName = player->GetName();
	if (!currentName)
	{
		return;
	}

	auto &data = playerData[player->index];
	if (!data.initialized)
	{
		data.lastName = currentName;
		data.initialized = true;
		return;
	}
	if (data.lastName == currentName)
	{
		return;
	}
	data.lastName = currentName;

	const auto now = Clock::now();
	const auto window = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<float>(windowSeconds));
	while (!data.changes.empty() && now - data.changes.front() >= window)
	{
		data.changes.pop_front();
	}
	data.changes.push_back(now);

	const std::size_t threshold_ = static_cast<std::size_t>((std::max)(1, threshold));
	if (data.changes.size() >= threshold_)
	{
		const std::size_t changes = data.changes.size();
		data.changes.clear();
		BanForNameChanging(player, changes);
	}
}

void NameBannerPlugin::OnClientDisconnect(CPlayerSlot slot)
{
	auto *player = PlayerFromSlot(slot);
	if (player && player->index >= 1 && player->index <= MAXPLAYERS)
	{
		playerData[player->index] = {};
		alreadyBanned[player->index] = false;
	}
}

void NameBannerPlugin::BanForNameChanging(Player *player, std::size_t changes)
{
	if (!player || player->index < 1 || player->index > MAXPLAYERS || !g_pAdmin)
	{
		return;
	}
	if (alreadyBanned[player->index])
	{
		return;
	}

	const std::string playerName = player->GetName();
	std::string reason = banReason;
	ReplaceAll(reason, "{changes}", std::to_string(changes));

	g_pAdmin->AddPlayerPunishment(player->GetPlayerSlot().Get(), RT_BAN, banMinutes, reason.c_str());
	alreadyBanned[player->index] = true;
	Msg("[NameBanner] Banned %s for changing names %zu times within the rolling window.\n", playerName.c_str(), changes);
}
