#pragma once

#include "common.h"
#include "version_gen.h"
#include "include/admin.h"
#include "include/players.h"

struct NameChangerPlayerData
{
	std::string lastName;
	std::deque<std::chrono::steady_clock::time_point> changes;
	bool initialized {};
	bool banned {};
};

class NameBanner final : public ISmmPlugin, public IPlayerListener
{
public:
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late) override;
	bool Unload(char *error, size_t maxlen) override;
	void AllPluginsLoaded() override;

	// IPlayerListener (dispatched by the Utils plugin).
	void ClientFullyConnect(int slot) override;
	void ClientSettingsChanged(int slot) override;
	void ClientDisconnect(int slot) override;

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

private:
	bool IsEligible(int slot) const;
	const char *ClientName(int slot) const;
	void SeedBaseline(int slot);
	void LoadConfig();
	void Ban(int slot, const char *name, size_t changes);

	std::array<NameChangerPlayerData, MAXPLAYERS> playerData;

	// Config (configs/NameBanner/settings.ini).
	int threshold {10};
	float windowSeconds {20.0f};
	int banMinutes {0};
	bool debug {};
	std::string banReason {"NameBanner"};
};
