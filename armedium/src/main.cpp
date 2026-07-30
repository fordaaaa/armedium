#include <iostream>
#include <thread>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <windows.h>
#include "Memory/MemoryManager.h"
#include "overlay/renderer.h"
#include "features/misc.h"
#include "features/hitboxexpander.h"
#include "features/fly.h"
#include "features/speed.h"
#include "rbx/Caches/playercache.h"
#include "rbx/Caches/playerobjectscache.h"
#include "rbx/Caches/TPHandler.h"
#include "rbx/globals/globals.h"
#include "rbx/configs/configs.h"
#include "rbx/Webhook.h"

bool IsGameRunning(const wchar_t* windowTitle)
{
    HWND hwnd = FindWindowW(NULL, windowTitle);
    return hwnd != NULL;
}

std::string GetExecutableDir()
{
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::filesystem::path exePath(path);
    return exePath.parent_path().string();
}

void log(const std::string& message, int type = 0)
{
    std::string prefix;
    switch (type)
    {
    case 0: prefix = "[*]"; break;
    case 1: prefix = "[+]"; break;
    case 2: prefix = "[-]"; break;
    default: prefix = ""; break;
    }
    std::cout << prefix << " " << message << std::endl;
    SendWebhookAsync(message, type);
}

template<typename T>
std::string toHexString(T value, bool prefix = false, bool uppercase = false)
{
    std::stringstream stream;
    if (uppercase)
        stream << std::uppercase;

    if (prefix)
        stream << "0x";

    stream << std::hex << value;
    return stream.str();
}

int main()
{
    if (!IsGameRunning(L"Roblox"))
    {
        log("Roblox not found!", 2);
        log("Waiting for Roblox...", 0);
        while (!IsGameRunning(L"Roblox"))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    log("Roblox found!", 1);

    log("Attaching to Roblox...", 0);
    if (!Memory->attachToProcess("RobloxPlayerBeta.exe"))
    {
        log("Failed to attach to Roblox!", 2);
        log("Press any key to exit...", 0);
        std::cin.get();
        return -1;
    }

    log("Succesfully attached!", 1);

    if (Memory->getProcessId("RobloxPlayerBeta.exe") == 0)
    {
        log("Failed to get Roblox's PID!", 2);
        log("Press any key to exit...", 0);
        std::cin.get();
        return -1;
    }

    log(std::string("Roblox PID -> " + std::to_string(Memory->getProcessId())), 1);
    log(std::string("Roblox Base Address -> 0x" + toHexString(std::to_string(Memory->getBaseAddress()), false, true)), 1);

    Globals::executablePath = GetExecutableDir();
    Globals::configsPath = Globals::executablePath + "\\configs";
    std::filesystem::create_directories(Globals::configsPath);

    std::thread(ShowImgui).detach();
    log("Overlay started!", 1);

    log("Waiting for game to load...", 0);

    RobloxInstance dataModel(0);
    int gameLoadRetries = 0;

    while (true)
    {
        auto fakeDataModel = Memory->read<uintptr_t>(Memory->getBaseAddress() + Offsets::FakeDataModel::Pointer);
        if (fakeDataModel != 0)
        {
            auto dataModelAddr = Memory->read<uintptr_t>(fakeDataModel + Offsets::FakeDataModel::RealDataModel);
            if (dataModelAddr != 0)
            {
                dataModel = RobloxInstance(dataModelAddr);
                auto name = dataModel.Name();

                // Check by name (Ugc = in-game)
                if (name == "Ugc")
                    break;

                // Fallback: if name is not empty and not LuaApp, and PlaceId is valid
                int placeId = Memory->read<int>(dataModelAddr + Offsets::DataModel::PlaceId);
                if (!name.empty() && name != "LuaApp" && placeId > 0)
                    break;
            }
        }

        gameLoadRetries++;
        if (gameLoadRetries > 300)
        {
            log("Game load timeout, continuing anyway...", 2);
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    Globals::Roblox::DataModel = dataModel;

    auto visualEngine = Memory->read<uintptr_t>(Memory->getBaseAddress() + Offsets::VisualEngine::Pointer);

    int veRetries = 0;
    while (visualEngine == 0)
    {
        visualEngine = Memory->read<uintptr_t>(Memory->getBaseAddress() + Offsets::VisualEngine::Pointer);
        veRetries++;
        if (veRetries > 300) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    Globals::Roblox::VisualEngine = visualEngine;

    if (Globals::Roblox::DataModel.address)
    {
        Globals::Roblox::Workspace = Globals::Roblox::DataModel.FindFirstChildWhichIsA("Workspace");
        Globals::Roblox::Players = Globals::Roblox::DataModel.FindFirstChildWhichIsA("Players");
        if (Globals::Roblox::Workspace.address)
            Globals::Roblox::Camera = Globals::Roblox::Workspace.FindFirstChildWhichIsA("Camera");
        if (Globals::Roblox::Players.address)
            Globals::Roblox::LocalPlayer = RobloxInstance(Memory->read<uintptr_t>(Globals::Roblox::Players.address + Offsets::Player::LocalPlayer));
        Globals::Roblox::lastPlaceID = Memory->read<int>(Globals::Roblox::DataModel.address + Offsets::DataModel::PlaceId);
    }

    log(std::string("DataModel -> 0x" + toHexString(std::to_string(Globals::Roblox::DataModel.address), false, true)), 1);
    log(std::string("VisualEngine -> 0x" + toHexString(std::to_string(Globals::Roblox::VisualEngine), false, true)), 1);

    log(std::string("Workspace -> 0x" + toHexString(std::to_string(Globals::Roblox::Workspace.address), false, true)), 1);
    log(std::string("Players -> 0x" + toHexString(std::to_string(Globals::Roblox::Players.address), false, true)), 1);
    log(std::string("Camera -> 0x" + toHexString(std::to_string(Globals::Roblox::Camera.address), false, true)), 1);

    if (Globals::Roblox::LocalPlayer.address)
        log(std::string("Logged in as " + Globals::Roblox::LocalPlayer.Name()), 1);

    Globals::Initialized = true;

    if (std::filesystem::exists(Globals::configsPath + "\\autosave.json"))
    {
        log("Loading auto-saved config...", 0);
        LoadConfig("autosave.json");
    }

    log("Starting cheat...", 1);
    
    std::thread(CachePlayers).detach();
    std::thread(CachePlayerObjects).detach();
    std::thread(TPHandler).detach();
    std::thread(MiscLoop).detach();
    std::thread(RunHitboxExpander).detach();
    std::thread(FlyLoop).detach();
    std::thread(SpeedLoop).detach();

    SendWebhookAsync("Started", 1);

    std::cin.get();

    return 1;
}