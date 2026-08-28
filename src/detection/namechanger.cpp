#include "namechanger.h"

#include "player/player.h"
#include "settings.h"

namespace detection
{
	bool IsEligibleHuman(Player *player)
	{
		return player && player->index >= 1 && player->index <= MAXPLAYERS && player->GetController() && !player->IsFakeClient()
			   && !player->IsCSTV();
	}

	void NameChangerModule::Load(BanCallback banCallback)
	{
		ban = banCallback;
		Reset();
	}

	void NameChangerModule::Unload()
	{
		playerData = {};
		ban = nullptr;
	}

	void NameChangerModule::Reset()
	{
		playerData = {};
	}

	void NameChangerModule::OnClientReady(Player *player)
	{
		if (!IsEligibleHuman(player) || !player->IsInGame())
		{
			return;
		}
		auto &data = playerData[player->index];
		data = {};
		const char *name = player->GetClient()->GetClientName();
		if (name)
		{
			data.lastName = name;
			data.initialized = true;
		}
	}

	void NameChangerModule::OnClientSettingsChanged(Player *player)
	{
		if (!IsEligibleHuman(player) || !player->IsInGame())
		{
			return;
		}
		const char *currentName = player->GetClient()->GetClientName();
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
		const auto window = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<float>(settings::GetNameChangeWindowSeconds()));
		while (!data.changes.empty() && now - data.changes.front() >= window)
		{
			data.changes.pop_front();
		}
		data.changes.push_back(now);

		const std::size_t threshold = static_cast<std::size_t>((std::max)(1, settings::GetNameChangeThreshold()));
		if (data.changes.size() >= threshold)
		{
			const std::size_t changes = data.changes.size();
			data.changes.clear();
			if (ban)
			{
				ban(player, changes);
			}
		}
	}

	void NameChangerModule::OnClientDisconnect(Player *player)
	{
		if (player && player->index >= 1 && player->index <= MAXPLAYERS)
		{
			playerData[player->index] = {};
		}
	}
} // namespace detection
