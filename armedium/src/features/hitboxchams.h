#pragma once
#include "partchams.h"
#include "../rbx/BoneRegistry.h"

// Resolves bone index to RobloxInstance — delegates to BoneRegistry.
inline RobloxInstance HitboxChams_GetBonePart(const RobloxPlayer& player, int boneIdx)
{
    return GetAimTargetBone(player, boneIdx);
}

inline void RenderHitboxChams(ImDrawList* dl, bool menuOpen = false)
{
    if (!Options::HitboxChams::Enabled)
        return;
    if (Options::HitboxChams::OnlyWhenMenuClosed && menuOpen)
        return;
    auto players = SnapshotCachedPlayerObjects();
    if (!dl || players.empty())
        return;

    std::string localTeamColor;
    if (Options::HitboxChams::TeamCheck)
        localTeamColor = Chams_GetLocalTeamColor();

    int r = (int)(Options::HitboxChams::Color[0] * 255.f);
    int g = (int)(Options::HitboxChams::Color[1] * 255.f);
    int b = (int)(Options::HitboxChams::Color[2] * 255.f);
    int a = (int)(Options::HitboxChams::Alpha * 255.f);
    ImU32 color = IM_COL32(r, g, b, a);
    ImU32 fill = Options::HitboxChams::Filled ? IM_COL32(r, g, b, (int)(a * 0.35f)) : 0;

    // Determine which aim target to highlight. Prefer silent aim when active.
    uintptr_t targetAddress = 0;
    int targetBone = Options::HitboxChams::PartSelect;
    if (Options::SilentAim::Enabled && Options::SilentAim::CurrentTarget != 0)
    {
        targetAddress = Options::SilentAim::CurrentTarget;
        targetBone = Options::SilentAim::TargetBone;
    }
    else if (Options::Aimbot::Aimbot && Options::Aimbot::CurrentTarget.address != 0)
    {
        targetAddress = Options::Aimbot::CurrentTarget.address;
        targetBone = Options::Aimbot::TargetBone;
    }
    ImU32 targetColor = IM_COL32(255, 50, 50, a);

    for (auto& player : players)
    {
        try
        {
            if (IsLocalPlayerEntry(player))
                continue;
            if (player.Health <= 0)
                continue;
            if (Options::HitboxChams::TeamCheck && Chams_IsTeammate(player, localTeamColor))
                continue;

            DrawPartBox(dl, HitboxChams_GetBonePart(player, Options::HitboxChams::PartSelect),
                color, Options::HitboxChams::Thickness, fill);

            if (Options::HitboxChams::HighlightTarget && player.address == targetAddress)
            {
                DrawPartBox(dl, HitboxChams_GetBonePart(player, targetBone),
                    targetColor, Options::HitboxChams::Thickness + 1.0f, fill);
            }
        }
        catch (...)
        {
        }
    }
}
