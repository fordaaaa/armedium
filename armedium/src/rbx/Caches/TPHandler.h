#pragma once
#include "../globals/options.h"
#include "../globals/globals.h"
#include "../../features/wallcheck.h"
#include <thread>
#include <vector>


inline void TPHandler()
{
	while (true)
	{
		try
		{
			auto fakeDataModel = Memory->read<uintptr_t>(Memory->getBaseAddress() + Offsets::FakeDataModel::Pointer);
			if (fakeDataModel == 0)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}
			auto dataModelAddr = Memory->read<uintptr_t>(fakeDataModel + Offsets::FakeDataModel::RealDataModel);
			if (dataModelAddr == 0)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}
			RobloxInstance dataModel(dataModelAddr);
			auto placeId = Memory->read<int>(dataModel.address + Offsets::DataModel::PlaceId);
			auto name = dataModel.Name();

			// Detect a real transition: the LuaApp loading screen, a new
			// place id, or a brand-new DataModel instance. The last one
			// matters for re-joining the SAME place on a fresh server
			// (matchmaking queues): the place id stays identical, but the
			// old DataModel is freed, so every cached pointer would silently
			// go stale. Placeholder places (placeId 0, never committed) are
			// excluded - for them a changed DataModel address is just a
			// re-allocation, and reacting would spam the wait loop below.
			bool transitioning = (name == "LuaApp")
				|| (name != "LuaApp" && placeId > 0 && placeId != Globals::Roblox::lastPlaceID)
				|| (Globals::Roblox::lastPlaceID > 0
					&& Globals::Roblox::DataModel.address != 0
					&& dataModelAddr != Globals::Roblox::DataModel.address);

			if (transitioning)
			{
				// The old place is gone: zero every cached pointer right
				// away so no feature keeps reading the freed/reused old
				// DataModel while the new one loads. lastPlaceID is left
				// untouched, so this flow keeps re-triggering until the new
				// place actually commits below.
				Globals::Roblox::DataModel = RobloxInstance(0);
				Globals::Roblox::VisualEngine = 0;
				Globals::Roblox::Workspace = RobloxInstance(0);
				Globals::Roblox::Players = RobloxInstance(0);
				Globals::Roblox::Camera = RobloxInstance(0);
				Globals::Roblox::LocalPlayer = RobloxInstance(0);
				ResetRuntimeState();

				// Wait until the new place is actually loaded before
				// resolving anything from it. On timeout (300 retries), the
				// pointers stay zeroed and the next poll re-enters this
				// block once the place really loads.
				bool newPlaceReady = false;
				int tpRetries = 0;
				while (true)
				{
					fakeDataModel = Memory->read<uintptr_t>(Memory->getBaseAddress() + Offsets::FakeDataModel::Pointer);
					if (fakeDataModel != 0)
					{
						dataModelAddr = Memory->read<uintptr_t>(fakeDataModel + Offsets::FakeDataModel::RealDataModel);
						if (dataModelAddr != 0)
						{
							dataModel = RobloxInstance(dataModelAddr);
							name = dataModel.Name();
							if (name == "Ugc")
							{
								newPlaceReady = true;
								break;
							}

							int newPlaceId = Memory->read<int>(dataModelAddr + Offsets::DataModel::PlaceId);
							if (!name.empty() && name != "LuaApp" && newPlaceId > 0)
							{
								newPlaceReady = true;
								break;
							}
						}
					}

					tpRetries++;
					if (tpRetries > 300)
						break;
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				}

				if (!newPlaceReady)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					continue;
				}

				// Resolve the full pointer set from the freshly loaded
				// DataModel...
				Globals::Roblox::DataModel = dataModel;

				uintptr_t visualEngine = Memory->read<uintptr_t>(Memory->getBaseAddress() + Offsets::VisualEngine::Pointer);
				Globals::Roblox::VisualEngine = visualEngine;

				RobloxInstance workspace = Globals::Roblox::DataModel.FindFirstChildWhichIsA("Workspace");
				RobloxInstance players = Globals::Roblox::DataModel.FindFirstChildWhichIsA("Players");
				RobloxInstance camera;
				if (workspace.address)
					camera = workspace.FindFirstChildWhichIsA("Camera");
				RobloxInstance localPlayer;
				if (players.address)
					localPlayer = RobloxInstance(Memory->read<uintptr_t>(players.address + Offsets::Player::LocalPlayer));

				// ...but only COMMIT once the core pointers actually
				// resolved. A half-loaded place (services not created yet)
				// must not become the stable state: committing lastPlaceID
				// there would leave every feature dead until the next
				// teleport. Instead, stay in the transition state and retry
				// on the next poll.
				if (visualEngine == 0 || !workspace.address || !players.address || !localPlayer.address)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(500));
					continue;
				}

				Globals::Roblox::Workspace = workspace;
				Globals::Roblox::Players = players;
				Globals::Roblox::Camera = camera;
				Globals::Roblox::LocalPlayer = localPlayer;
				Globals::Roblox::lastPlaceID = Memory->read<int>(Globals::Roblox::DataModel.address + Offsets::DataModel::PlaceId);

				// New game / teleport: wipe player caches, wall-check
				// snapshot, current aim targets, etc. so nothing carries
				// over from the old place.
				ResetRuntimeState();
			}
			else
			{
				// Steady state: lazily fill in anything still missing
				// (a camera that spawned late, a VisualEngine that was 0
				// at commit time, a place that half-loaded earlier).
				if (Globals::Roblox::VisualEngine == 0)
					Globals::Roblox::VisualEngine = Memory->read<uintptr_t>(Memory->getBaseAddress() + Offsets::VisualEngine::Pointer);
				if (!Globals::Roblox::Workspace.address)
					Globals::Roblox::Workspace = Globals::Roblox::DataModel.FindFirstChildWhichIsA("Workspace");
				if (!Globals::Roblox::Players.address)
					Globals::Roblox::Players = Globals::Roblox::DataModel.FindFirstChildWhichIsA("Players");
				if (!Globals::Roblox::Camera.address && Globals::Roblox::Workspace.address)
					Globals::Roblox::Camera = Globals::Roblox::Workspace.FindFirstChildWhichIsA("Camera");
				if (!Globals::Roblox::LocalPlayer.address && Globals::Roblox::Players.address)
					Globals::Roblox::LocalPlayer = RobloxInstance(Memory->read<uintptr_t>(Globals::Roblox::Players.address + Offsets::Player::LocalPlayer));
			}
		}
		catch (...)
		{
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}
