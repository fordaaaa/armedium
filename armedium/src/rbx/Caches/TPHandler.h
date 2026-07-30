#pragma once
#include "../globals/options.h"
#include "../globals/globals.h"
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
		auto dataModel = RobloxInstance(Memory->read<uintptr_t>(fakeDataModel + Offsets::FakeDataModel::RealDataModel));
		if (!dataModel || dataModel.address == 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}
		auto placeId = Memory->read<int>(dataModel.address + Offsets::DataModel::PlaceId);
		auto name = dataModel.Name();

		if (name == "LuaApp" || placeId != Globals::Roblox::lastPlaceID)
		{
			while (true)
			{
				fakeDataModel = Memory->read<uintptr_t>(Memory->getBaseAddress() + Offsets::FakeDataModel::Pointer);
				if (fakeDataModel != 0)
				{
					dataModel = RobloxInstance(Memory->read<uintptr_t>(fakeDataModel + Offsets::FakeDataModel::RealDataModel));
					if (dataModel && dataModel.address != 0 && dataModel.Name() == "Ugc")
						break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}

			Globals::Roblox::DataModel = dataModel;

			auto visualEngine = Memory->read<uintptr_t>(Memory->getBaseAddress() + Offsets::VisualEngine::Pointer);

			while (visualEngine == 0)
			{
				visualEngine = Memory->read<uintptr_t>(Memory->getBaseAddress() + Offsets::VisualEngine::Pointer);
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}

			Globals::Roblox::VisualEngine = visualEngine;

			Globals::Roblox::Workspace = Globals::Roblox::DataModel.FindFirstChildWhichIsA("Workspace");
			Globals::Roblox::Players = Globals::Roblox::DataModel.FindFirstChildWhichIsA("Players");
			Globals::Roblox::Camera = Globals::Roblox::Workspace.FindFirstChildWhichIsA("Camera");

			Globals::Roblox::LocalPlayer = RobloxInstance(Memory->read<uintptr_t>(Globals::Roblox::Players.address + Offsets::Player::LocalPlayer));

			Globals::Roblox::lastPlaceID = Memory->read<int>(Globals::Roblox::DataModel.address + Offsets::DataModel::PlaceId);;

			Globals::Caches::CachedPlayers.clear();
			Globals::Caches::CachedPlayerObjects.clear();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

