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

		// Only react to real transitions: an actual teleport (name == "LuaApp"
		// = loading screen) or a VALID new place id. A placeId of 0 (read
		// during loading or on placeholder places) must never trigger the wait
		// loop below - otherwise the outer poll re-enters it every 100ms and
		// ResetRuntimeState() wipes the aim targets in a tight loop, which
		// kills silent aim / aimbot during any transition.
		if (name != "LuaApp" && (placeId <= 0 || placeId == Globals::Roblox::lastPlaceID))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		// Wait until the new place is actually loaded before touching any
		// globals. If we time out (300 retries), skip the refresh entirely:
		// mutating Globals/caches from a half-loaded DataModel is worse than
		// doing nothing, and staying in the loading state will re-trigger
		// this block on the next poll anyway.
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
			// The new place still hasn't loaded (long loading screen, or a
			// placeholder place that reports placeId 0). Never point the caches
			// at the half-loaded DataModel - instead zero every cached pointer
			// so no feature keeps dereferencing the OLD place's (potentially
			// freed/reused) objects. lastPlaceID is left untouched so the real
			// transition still triggers the full refresh once the place loads.
			Globals::Roblox::DataModel = RobloxInstance(0);
			Globals::Roblox::VisualEngine = 0;
			Globals::Roblox::Workspace = RobloxInstance(0);
			Globals::Roblox::Players = RobloxInstance(0);
			Globals::Roblox::Camera = RobloxInstance(0);
			Globals::Roblox::LocalPlayer = RobloxInstance(0);
			ResetRuntimeState();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		Globals::Roblox::DataModel = dataModel;

		auto visualEngine = Memory->read<uintptr_t>(Memory->getBaseAddress() + Offsets::VisualEngine::Pointer);

		int veRetries = 0;
		while (visualEngine == 0)
		{
			visualEngine = Memory->read<uintptr_t>(Memory->getBaseAddress() + Offsets::VisualEngine::Pointer);
			veRetries++;
			if (veRetries > 300)
				break;
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}

		Globals::Roblox::VisualEngine = visualEngine;

		Globals::Roblox::Workspace = Globals::Roblox::DataModel.FindFirstChildWhichIsA("Workspace");
		Globals::Roblox::Players = Globals::Roblox::DataModel.FindFirstChildWhichIsA("Players");
		if (Globals::Roblox::Workspace.address)
			Globals::Roblox::Camera = Globals::Roblox::Workspace.FindFirstChildWhichIsA("Camera");
		if (Globals::Roblox::Players.address)
			Globals::Roblox::LocalPlayer = RobloxInstance(Memory->read<uintptr_t>(Globals::Roblox::Players.address + Offsets::Player::LocalPlayer));

		Globals::Roblox::lastPlaceID = Memory->read<int>(Globals::Roblox::DataModel.address + Offsets::DataModel::PlaceId);

		// New game / teleport: wipe player caches, wall-check snapshot,
		// current aim targets, etc. so nothing carries over from the old place.
		ResetRuntimeState();
	}
}
