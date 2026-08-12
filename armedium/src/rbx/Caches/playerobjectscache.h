#pragma once
#include <thread>
#include "../globals/options.h"
#include "../../rbx/globals/globals.h"

inline void CachePlayerObjects()
{
	std::vector<RobloxPlayer> tempList;

	while (true)
	{
		try
		{
			tempList.clear();

			auto playersSnapshot = SnapshotCachedPlayers();
			if (playersSnapshot.empty())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}

			for (auto& player : playersSnapshot)
			{
				RobloxPlayer p;

				if (!player || player.address == 0)
					continue;

				p.address = player.address;

				// Check if this is a Player object or a Model (NPC)
				std::string className = player.Class();
				bool isNPC = (className == "Model");

				if (isNPC)
				{
					p.Name = player.Name();
					p.DisplayName = "";
					p.UserId = 0;
					p.Team = RobloxInstance(0);
					p.Character = player;
					p.Humanoid = p.Character.FindFirstChildWhichIsA("Humanoid");
					p.Health = Memory->read<float>(p.Humanoid.address + Offsets::Humanoid::Health);
					p.MaxHealth = Memory->read<float>(p.Humanoid.address + Offsets::Humanoid::MaxHealth);
				}
				else
				{
					p.Name = player.Name();
					p.DisplayName = Memory->readString(Memory->read<uintptr_t>(player.address + Offsets::Player::DisplayName));
					p.UserId = Memory->read<int64_t>(player.address + Offsets::Player::UserId);
					p.Team = player.Team();
					auto pTeam = p.Team;
					if (pTeam.address != 0)
						p.TeamColor = Memory->readString(Memory->read<uintptr_t>(pTeam.address + Offsets::Team::BrickColorName));
					p.GroupId = Memory->read<int64_t>(player.address + Offsets::Player::GroupId);
					p.Character = player.Character();
					p.Humanoid = p.Character.FindFirstChildWhichIsA("Humanoid");
					p.Health = player.Health();
					p.MaxHealth = player.MaxHealth();
				}

				p.RigType = p.Humanoid.RigType();

				// ─── Populate Bones[] from registry ────────────────────────
				// Single loop over the bone registry replaces the old
				// rigid switch. Works for R6, R15, and any future rig.
				for (uint8_t i = 0; i < (uint8_t)BoneId::COUNT; i++)
				{
					const auto& entry = BONE_REGISTRY[i];
					const char* partName = (p.RigType == 0) ? entry.r6Name : entry.r15Name;

					if (partName)
						p.Bones[i] = p.Character.FindFirstChild(partName);
				}

				// Held tool name
				if (p.Character.address != 0)
				{
					auto tool = p.Character.FindFirstChildWhichIsA("Tool");
					if (tool.address != 0)
						p.ToolName = tool.Name();
					else
						p.ToolName = "";
				}

				tempList.push_back(p);
			}
		}
		catch (...)
		{
		}

		{
			std::lock_guard<std::mutex> lock(Globals::Caches::Mutex);
			Globals::Caches::CachedPlayerObjects.clear();
			Globals::Caches::CachedPlayerObjects = tempList;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}
