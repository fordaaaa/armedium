#pragma once
#include <Windows.h>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#include "../../Memory/MemoryManager.h"
#include "../SDK/sdk.h"
#include "../BoneRegistry.h"

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

    // ─── Universal Bone Array ──────────────────────────────────────────
    // Indexed by BoneId enum. Populated by CachePlayerObjects(). All
    // aiming, ESP, and chams code should use GetBone() / GetAimTargetBone()
    // from BoneRegistry.h instead of the legacy named fields below.
    std::array<RobloxInstance, (size_t)BoneId::COUNT> Bones{};

    // ─── Legacy bone aliases (index into Bones[]) ──────────────────────
    // Still readable for existing code during transition. Prefer Bones[]
    // or GetBone() for new code.
    RobloxInstance& Head            = Bones[(uint8_t)BoneId::Head];
    RobloxInstance& HumanoidRootPart = Bones[(uint8_t)BoneId::HumanoidRootPart];
    RobloxInstance& Torso           = Bones[(uint8_t)BoneId::Torso];
    RobloxInstance& Upper_Torso     = Bones[(uint8_t)BoneId::UpperTorso];
    RobloxInstance& Lower_Torso     = Bones[(uint8_t)BoneId::LowerTorso];
    RobloxInstance& Left_Arm        = Bones[(uint8_t)BoneId::LeftArm];
    RobloxInstance& Right_Arm       = Bones[(uint8_t)BoneId::RightArm];
    RobloxInstance& Left_Hand       = Bones[(uint8_t)BoneId::LeftHand];
    RobloxInstance& Right_Hand      = Bones[(uint8_t)BoneId::RightHand];
    RobloxInstance& Left_Leg        = Bones[(uint8_t)BoneId::LeftLeg];
    RobloxInstance& Right_Leg       = Bones[(uint8_t)BoneId::RightLeg];
    RobloxInstance& Left_Foot       = Bones[(uint8_t)BoneId::LeftFoot];
    RobloxInstance& Right_Foot      = Bones[(uint8_t)BoneId::RightFoot];
    RobloxInstance& Left_Upper_Arm  = Bones[(uint8_t)BoneId::LeftUpperArm];
    RobloxInstance& Left_Lower_Arm  = Bones[(uint8_t)BoneId::LeftLowerArm];
    RobloxInstance& Right_Upper_Arm = Bones[(uint8_t)BoneId::RightUpperArm];
    RobloxInstance& Right_Lower_Arm = Bones[(uint8_t)BoneId::RightLowerArm];
    RobloxInstance& Left_Upper_Leg  = Bones[(uint8_t)BoneId::LeftUpperLeg];
    RobloxInstance& Left_Lower_Leg  = Bones[(uint8_t)BoneId::LeftLowerLeg];
    RobloxInstance& Right_Upper_Leg = Bones[(uint8_t)BoneId::RightUpperLeg];
    RobloxInstance& Right_Lower_Leg = Bones[(uint8_t)BoneId::RightLowerLeg];
};

// Resolve a BoneId to its RobloxInstance for a given player.
// Returns RobloxInstance(0) if the bone doesn't exist for this player's rig.
inline RobloxInstance GetBone(const RobloxPlayer& player, BoneId id)
{
    return player.Bones[(uint8_t)id];
}

// Get the RobloxInstance for an aim-target bone index (0-7).
// This replaces the old GetTargetBonePart() / SilentAim_GetTargetPart().
inline RobloxInstance GetAimTargetBone(const RobloxPlayer& player, int boneIdx)
{
    if (boneIdx < 0 || boneIdx >= AIM_TARGET_BONE_COUNT)
        return player.Bones[(uint8_t)BoneId::Head];

    BoneId id = AIM_TARGET_BONES[boneIdx];
    RobloxInstance part = player.Bones[(uint8_t)id];

    // For R15-only bones on R6, fall back to HumanoidRootPart
    if (!part.address && !BoneValidForRig(id, player.RigType))
        part = player.Bones[(uint8_t)BoneId::HumanoidRootPart];

    // If still nothing, fall back to Head
    if (!part.address)
        part = player.Bones[(uint8_t)BoneId::Head];

    return part;
}

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

// True if the cached player entry is the local player, either by Player-object
// address or by character address. The character check matters: entries that
// got into the cache via the NPC path (e.g. right after a teleport, when the
// local character can be scanned into Workspace before LocalPlayer is
// refreshed) carry the CHARACTER address, not the Player-object address, so
// comparing only against LocalPlayer.address would miss them and the cheat
// would ESP/aim at your own character.
inline bool IsLocalPlayerEntry(const RobloxPlayer& player)
{
    if (player.address != 0 && player.address == Globals::Roblox::LocalPlayer.address)
        return true;
    if (player.Character.address != 0)
    {
        RobloxInstance localChar = Globals::Roblox::LocalPlayer.Character();
        return player.Character.address == localChar.address;
    }
    return false;
}
