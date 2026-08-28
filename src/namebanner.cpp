#include "namebanner.h"

#include "settings.h"
#include "utils/hooks.h"
#include "utils/interfaces.h"
#include "utils/utils.h"
#include "player/player.h"

#include "eiface.h"

NameBannerPlugin g_NameBanner;

PLUGIN_EXPOSE(NameBannerPlugin, g_NameBanner);

namespace
{
	void ReplaceAll(std::string &value, const std::string &placeholder, const std::string &replacement)
	{
		for (size_t position = 0; (position = value.find(placeholder, position)) != std::string::npos; position += replacement.size())
		{
			value.replace(position, placeholder.size(), replacement);
		}
	}

	void BanCallback(Player *player, std::size_t changes)
	{
		g_NameBanner.BanForNameChanging(player, changes);
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

bool NameBannerPlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();
	settings::Initialize();
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

	ConVar_Register();
	convarsRegistered = true;
	nameChanger.Load(BanCallback);

	if (!hooks::Initialize(missing))
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
	alreadyBanned = {};
	if (late)
	{
		for (i32 i = 1; i <= MAXPLAYERS; ++i)
		{
			auto *player = g_pPlayerManager->players[i];
			if (player && player->IsInGame())
			{
				OnClientFullyConnect(player->GetPlayerSlot());
			}
		}
	}
	Msg("[NameBanner] NameBanner %s loaded successfully.\n", PLUGIN_FULL_VERSION);
	return true;
}

void NameBannerPlugin::CleanupRuntime()
{
	hooks::Cleanup();
	nameChanger.Unload();
	if (convarsRegistered)
	{
		ConVar_Unregister();
		convarsRegistered = false;
	}
	utils::Cleanup();
	loaded = false;
}

bool NameBannerPlugin::Unload(char *error, size_t maxlen)
{
	if (!hooks::Cleanup())
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
	settings::Shutdown();
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

void NameBannerPlugin::OnClientFullyConnect(CPlayerSlot slot)
{
	nameChanger.OnClientReady(g_pPlayerManager->ToPlayer(slot));
}

void NameBannerPlugin::OnClientSettingsChanged(CPlayerSlot slot)
{
	nameChanger.OnClientSettingsChanged(g_pPlayerManager->ToPlayer(slot));
}

void NameBannerPlugin::OnClientDisconnect(CPlayerSlot slot)
{
	auto *player = g_pPlayerManager->ToPlayer(slot);
	nameChanger.OnClientDisconnect(player);
	if (player && player->index >= 1 && player->index <= MAXPLAYERS)
	{
		alreadyBanned[player->index] = false;
	}
}

void NameBannerPlugin::BanForNameChanging(Player *player, std::size_t changes)
{
	if (!player || player->index < 1 || player->index > MAXPLAYERS || !interfaces::pEngine)
	{
		return;
	}
	if (alreadyBanned[player->index])
	{
		return;
	}

	const std::string playerName = player->GetName();
	const std::uint64_t steamId = player->GetSteamId64(false);
	if (!steamId)
	{
		Msg("[NameBanner] %s changed names %zu times but has no SteamID64 yet; skipping the ban.\n", playerName.c_str(), changes);
		return;
	}

	const char *commandTemplate = settings::GetBanCommand();
	if (!commandTemplate || !*commandTemplate)
	{
		Msg("[NameBanner] %s changed names %zu times but nb_ban_command is empty; no ban was sent.\n", playerName.c_str(), changes);
		return;
	}

	const int userId = interfaces::pEngine->GetPlayerUserId(player->GetPlayerSlot()).Get();
	std::string command = commandTemplate;
	ReplaceAll(command, "{steamid64}", std::to_string(steamId));
	ReplaceAll(command, "{userid}", std::to_string(userId));
	ReplaceAll(command, "{changes}", std::to_string(changes));
	command.push_back('\n');
	interfaces::pEngine->ServerCommand(command.c_str());
	alreadyBanned[player->index] = true;
	Msg("[NameBanner] Banned %s (SteamID64 %llu) for changing names %zu times within the rolling window.\n", playerName.c_str(),
		static_cast<unsigned long long>(steamId), changes);
}
