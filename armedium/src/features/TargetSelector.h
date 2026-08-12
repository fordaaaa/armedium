#pragma once
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <cfloat>
#include "../rbx/globals/globals.h"
#include "../rbx/BoneRegistry.h"
#include "../overlay/utils/W2S.h"
#include "wallcheck.h"

// ─── Unified Target Selection ──────────────────────────────────────────────
// Shared by aimbot and silent aim. One function picks the target using a
// configurable strategy (closest-to-crosshair, lowest-health, nearest-3D).
// Eliminates the ~80% duplicated code that previously lived in both
// aimbot.h::GetClosestPlayer() and silentaim.h::SilentAim_GetClosestPlayer().
//
// Usage:
//   TargetSelector selector;
//   selector.TeamCheck   = Options::Aimbot::TeamCheck;
//   selector.FOV         = Options::Aimbot::FOV;
//   selector.Range       = Options::Aimbot::Range;
//   selector.Priority    = 0; // 0 = crosshair, 1 = lowest health
//   selector.VisibleOnly = Options::Aimbot::VisibleOnly;
//   ...
//   RobloxPlayer target = selector.FindTarget();

enum class TargetPriority : int
{
    ClosestToCrosshair = 0,
    LowestHealth       = 1,
};

struct TargetSelector
{
    bool   TeamCheck      = true;
    bool   VisibleOnly    = true;
    bool   DownedCheck    = false;
    float  FOV            = 300.f;
    float  Range          = 5000.f;
    TargetPriority Priority = TargetPriority::ClosestToCrosshair;
    int    TargetBoneIdx  = 0;             // which AIM_TARGET_BONES[] index to aim at
    float  HeadChance     = 70.f;          // for part randomizer
    bool   PartRandomizer = false;
    bool   Prediction     = false;
    float  PredictionX    = 10.f;
    float  PredictionY    = 10.f;

    // Returns the position to aim at for a given player, including prediction.
    Vectors::Vector3 GetAimPos(const RobloxPlayer& player) const
    {
        RobloxInstance part = GetAimTargetBone(player, TargetBoneIdx);
        if (!part.address)
            return Vectors::Vector3{};

        Vectors::Vector3 basePos = part.Position();

        if (Prediction)
        {
            Vectors::Vector3 velocity = GetVelocityFromPart(part);
            basePos.x += velocity.x / PredictionX;
            basePos.y += velocity.y / PredictionY;
            basePos.z += velocity.z / PredictionX;
        }

        return basePos;
    }

    // Find the best target from the current player cache.
    RobloxPlayer FindTarget() const
    {
        RobloxPlayer target;
        float bestScore = FLT_MAX;

        std::string localTeamColor;
        auto localTeam = Globals::Roblox::LocalPlayer.Team();
        if (localTeam.address != 0 && TeamCheck)
            localTeamColor = Memory->readString(Memory->read<uintptr_t>(
                localTeam.address + Offsets::Team::BrickColorName));

        auto localCharacter = Globals::Roblox::LocalPlayer.Character();
        auto localHRP = localCharacter.FindFirstChild("HumanoidRootPart");

        Vectors::Vector3 camPos{};
        bool needCam = VisibleOnly && Options::WallCheck::Enabled;
        if (needCam) camPos = WallCheck_GetCameraPosition();

        POINT cursor;
        GetCursorPos(&cursor);

        auto players = SnapshotCachedPlayerObjects();
        for (auto& player : players)
        {
            if (!player.HumanoidRootPart.address)
                continue;
            if (IsLocalPlayerEntry(player))
                continue;

            // Team check
            if (TeamCheck && !player.TeamColor.empty() && !localTeamColor.empty() &&
                player.TeamColor == localTeamColor)
                continue;

            // Dead check
            if (player.Health == 0)
                continue;

            // Downed check
            if (DownedCheck && player.Health > 0 && player.Health <= 5.0f)
                continue;

            // Get aim position
            Vectors::Vector3 aimPos = GetAimPos(player);
            if (aimPos.x == 0.f && aimPos.y == 0.f && aimPos.z == 0.f)
                continue;

            // Visibility check
            if (needCam)
            {
                RobloxInstance aimPart = GetAimTargetBone(player, TargetBoneIdx);
                if (!IsPointVisible(camPos, aimPos, nullptr, aimPart.address))
                    continue;
            }

            // 2D screen check
            Vectors::Vector2 aimPos2D = WorldToScreen(aimPos);
            if (aimPos2D.x == -1 && aimPos2D.y == -1)
                continue;

            // 3D range check (only if we have a local HRP)
            if (localHRP.address && Range > 0)
            {
                float dist3D = (localHRP.Position() - aimPos).Magnitude();
                if (dist3D > Range)
                    continue;
            }

            // Score / distance check
            float dist2D = aimPos2D.Distance({ (float)cursor.x, (float)cursor.y });
            if (dist2D > FOV)
                continue;

            float score;
            if (Priority == TargetPriority::LowestHealth)
                score = player.Health;  // lower = better
            else
                score = dist2D;         // closer to crosshair = better

            if (score < bestScore)
            {
                bestScore = score;
                target = player;
            }
        }

        return target;
    }

private:
    // Velocity helper — mirrors aimbot.h::GetVelocity
    static Vectors::Vector3 GetVelocityFromPart(const RobloxInstance& part)
    {
        if (!part.address)
            return Vectors::Vector3{};
        uintptr_t primitiveAddr = Memory->read<uintptr_t>(
            part.address + Offsets::BasePart::Primitive);
        if (!primitiveAddr)
            return Vectors::Vector3{};
        return Memory->read<Vectors::Vector3>(
            primitiveAddr + Offsets::BasePart::AssemblyLinearVelocity);
    }
};
