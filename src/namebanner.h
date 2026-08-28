#pragma once

#include "common.h"
#include "detection/namechanger.h"
#include "version_gen.h"

#include <array>
#include <cstddef>
#include <string>

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
	void BanForNameChanging(Player *player, std::size_t changes);

	bool IsLoaded() const
	{
		return loaded;
	}

private:
	bool Activate(char *error, size_t maxlen, bool late);
	void CleanupRuntime();

	bool loaded {};
	bool activationPending {};
	bool convarsRegistered {};
	std::string activationError;
	detection::NameChangerModule nameChanger;
	std::array<bool, MAXPLAYERS + 1> alreadyBanned {};
};

extern NameBannerPlugin g_NameBanner;
