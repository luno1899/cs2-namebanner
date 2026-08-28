#pragma once

#include "common.h"

namespace settings
{
	void Initialize();
	void Shutdown();

	// Number of distinct name changes within the rolling window that triggers a ban.
	int GetNameChangeThreshold();
	// Size of the rolling window, in seconds.
	float GetNameChangeWindowSeconds();
	// Server command template run to ban the player. Supports {steamid64}, {userid}, {changes}.
	const char *GetBanCommand();
} // namespace settings
