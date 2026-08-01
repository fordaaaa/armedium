#pragma once
#include <Windows.h>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#include "../../Memory/MemoryManager.h"
#include "../SDK/sdk.h"

struct RobloxPlayer
{
    uintptr_t address = 0;
    int RigType = 0;
    std::string Name = "";
    std::string DisplayName = "";
    std::string ToolName = "";
    int64_t UserId = 0;
    float Health = 0.0f;
    float MaxHealth = 0.0f;
    RobloxInstance Team = RobloxInstance(0);
    std::string TeamColor = "";
    int64_t GroupId = 0;
    RobloxInstance Character = RobloxInstance(0);
    RobloxInstance Humanoid = RobloxInstance(0);
    RobloxInstance Head = RobloxInstance(0);
    RobloxInstance HumanoidRootPart = RobloxInstance(0);
    RobloxInstance Left_Arm = RobloxInstance(0);
    RobloxInstance Left_Leg = RobloxInstance(0);
    RobloxInstance Right_Arm = RobloxInstance(0);
    RobloxInstance Right_Leg = RobloxInstance(0);
    RobloxInstance Torso = RobloxInstance(0);
    RobloxInstance Upper_Torso = RobloxInstance(0);
    RobloxInstance Lower_Torso = RobloxInstance(0);
    RobloxInstance Right_Upper_Arm = RobloxInstance(0);
    RobloxInstance Right_Lower_Arm = RobloxInstance(0);
    RobloxInstance Right_Hand = RobloxInstance(0);
    RobloxInstance Left_Upper_Arm = RobloxInstance(0);
    RobloxInstance Left_Lower_Arm = RobloxInstance(0);
    RobloxInstance Left_Hand = RobloxInstance(0);
    RobloxInstance Right_Upper_Leg = RobloxInstance(0);
    RobloxInstance Right_Lower_Leg = RobloxInstance(0);
    RobloxInstance Right_Foot = RobloxInstance(0);
    RobloxInstance Left_Upper_Leg = RobloxInstance(0);
    RobloxInstance Left_Lower_Leg = RobloxInstance(0);
    RobloxInstance Left_Foot = RobloxInstance(0);
};

namespace Globals
{
    namespace Roblox
    {
        inline RobloxInstance DataModel(0);
        inline uintptr_t VisualEngine;
        inline RobloxInstance Workspace(0);
        inline RobloxInstance Players(0);
        inline RobloxInstance Camera(0);
        inline RobloxInstance LocalPlayer(0);
        inline int lastPlaceID;
    }
    namespace Caches
    {
        // Guards CachedPlayers / CachedPlayerObjects: the cache threads rebuild
        // them on their own schedules (5s / 500ms) while the render/aim threads
        // iterate them every frame, and ResetRuntimeState() clears them on
        // teleport - unlocked concurrent clear/assign/iterate is UB. Writers
        // swap under this lock; readers take a lock-protected snapshot via
        // SnapshotCachedPlayers() / SnapshotCachedPlayerObjects().
        inline std::mutex Mutex;
        inline std::vector<RobloxInstance> CachedPlayers;
        inline std::vector<RobloxPlayer> CachedPlayerObjects;
    }
    inline std::string executablePath;
    inline std::string configsPath;
    inline bool Initialized = false;
}

inline std::vector<RobloxInstance> SnapshotCachedPlayers()
{
    std::lock_guard<std::mutex> lock(Globals::Caches::Mutex);
    return Globals::Caches::CachedPlayers;
}

inline std::vector<RobloxPlayer> SnapshotCachedPlayerObjects()
{
    std::lock_guard<std::mutex> lock(Globals::Caches::Mutex);
    return Globals::Caches::CachedPlayerObjects;
}
