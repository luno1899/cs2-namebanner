#pragma once

#include "common.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <deque>
#include <string>

class Player;

namespace detection
{
	using Clock = std::chrono::steady_clock;
	// Called once a player's name-change count reaches the configured threshold.
	using BanCallback = void (*)(Player *player, std::size_t changes);

	bool IsEligibleHuman(Player *player);

	struct NameChangerPlayerData
	{
		std::string lastName;
		std::deque<Clock::time_point> changes;
		bool initialized {};
	};

	// Detects clients that change their visible name too many times within a rolling window.
	class NameChangerModule
	{
	public:
		void Load(BanCallback ban);
		void Unload();
		void Reset();
		void OnClientReady(Player *player);
		void OnClientSettingsChanged(Player *player);
		void OnClientDisconnect(Player *player);

	private:
		BanCallback ban {};
		std::array<NameChangerPlayerData, MAXPLAYERS + 1> playerData;
	};
} // namespace detection
