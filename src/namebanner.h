#pragma once

#include "common.h"
#include "version_gen.h"
#include "include/admin.h"
#include "sdk/entity/cbaseplayercontroller.h"
#include "sdk/serversideclient.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <deque>
#include <string>

// The controller entity index is the player slot plus one.
class Player
{
public:
	explicit Player(i32 i) : index(i) {}

	CBasePlayerController *GetController();

	CPlayerSlot GetPlayerSlot() const
	{
		return index - 1;
	}

	CServerSideClient *GetClient();

	bool IsInGame();
	bool IsFakeClient();
	bool IsCSTV();

	const char *GetName();
	u64 GetSteamId64();

	const i32 index;

private:
	CUtlString sanitizedName;
};

extern Player *g_players[MAXPLAYERS + 1];
Player *PlayerFromSlot(CPlayerSlot slot);

using Clock = std::chrono::steady_clock;

struct NameChangerPlayerData
{
	std::string lastName;
	std::deque<Clock::time_point> changes;
	bool initialized {};
};

class NameBannerPlugin final : public ISmmPlugin, public IMetamodListener
{
public:
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late) override;
	void AllPluginsLoaded() override;
	bool QueryRunning(char *error, size_t maxlen) override;
	bool Unload(char *error, size_t maxlen) override;
	bool Pause(char *error, size_t maxlen) override;
	bool Unpause(char *error, size_t maxlen) override;

	const char *GetAuthor() override
	{
		return PLUGIN_AUTHOR;
	}

	const char *GetName() override
	{
		return PLUGIN_DISPLAY_NAME;
	}

	const char *GetDescription() override
	{
		return PLUGIN_DESCRIPTION;
	}

	const char *GetURL() override
	{
		return PLUGIN_URL;
	}

	const char *GetLicense() override
	{
		return PLUGIN_LICENSE;
	}

	const char *GetVersion() override
	{
		return PLUGIN_FULL_VERSION;
	}

	const char *GetDate() override
	{
		return __DATE__;
	}

	const char *GetLogTag() override
	{
		return PLUGIN_LOGTAG;
	}

	void OnLevelInit(char const *, char const *, char const *, char const *, bool, bool) override {}
	void OnLevelShutdown() override {}

	void OnClientFullyConnect(CPlayerSlot slot);
	void OnClientSettingsChanged(CPlayerSlot slot);
	void OnClientDisconnect(CPlayerSlot slot);

	bool IsLoaded() const
	{
		return loaded;
	}

private:
	bool Activate(char *error, size_t maxlen, bool late);
	void CleanupRuntime();
	void LoadConfig();
	void BanForNameChanging(Player *player, std::size_t changes);

	bool loaded {};
	bool activationPending {};
	std::string activationError;
	std::array<NameChangerPlayerData, MAXPLAYERS + 1> playerData;
	std::array<bool, MAXPLAYERS + 1> alreadyBanned {};

	// Config (configs/NameBanner/settings.ini).
	int threshold {5};
	float windowSeconds {60.0f};
	int banMinutes {0};
	std::string banReason {"Changed name too many times in under a minute"};
};

extern NameBannerPlugin g_NameBanner;
