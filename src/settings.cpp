#include "settings.h"

#include "convar.h"

namespace
{
	struct Configuration
	{
		CConVar<int> threshold {"nb_namechanger_threshold", FCVAR_NONE, "Name changes within the rolling window that trigger a ban", 5};
		CConVar<float> windowSeconds {"nb_namechanger_window", FCVAR_NONE, "Rolling window size, in seconds", 60.0f};
		CConVar<CUtlString> banCommand {"nb_ban_command", FCVAR_NONE,
										"Server command run to ban a player. Supports {steamid64}, {userid}, {changes}",
										CUtlString("css_addban {steamid64} 0 NameBanner: changed name {changes} times in under a minute")};
	};

	Configuration *configuration {};
} // namespace

void settings::Initialize()
{
	if (!configuration)
	{
		configuration = new Configuration;
	}
}

void settings::Shutdown()
{
	delete configuration;
	configuration = nullptr;
}

int settings::GetNameChangeThreshold()
{
	return configuration ? configuration->threshold.GetInt() : 5;
}

float settings::GetNameChangeWindowSeconds()
{
	return configuration ? configuration->windowSeconds.GetFloat() : 60.0f;
}

const char *settings::GetBanCommand()
{
	return configuration ? configuration->banCommand.Get().Get() : "";
}
