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

		if (name == "LuaApp" || placeId != Globals::Roblox::lastPlaceID)
		{
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
							break;

						int newPlaceId = Memory->read<int>(dataModelAddr + Offsets::DataModel::PlaceId);
						if (!name.empty() && name != "LuaApp" && newPlaceId > 0)
							break;
					}
				}

				tpRetries++;
				if (tpRetries > 300)
					break;
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

