#pragma once
#include "partchams.h"

// Mirrors the aim bone enum used by Aimbot/SilentAim (0-7).
inline RobloxInstance HitboxChams_GetBonePart(const RobloxPlayer& player, int boneIdx)
{
    switch (boneIdx)
    {
        case 0: return player.Head;
        case 1: return player.HumanoidRootPart;
        case 2: return (player.RigType == 0) ? player.Left_Arm : player.Left_Hand;
        case 3: return (player.RigType == 0) ? player.Right_Arm : player.Right_Hand;
        case 4: return (player.RigType == 0) ? player.Left_Leg : player.Left_Foot;
        case 5: return (player.RigType == 0) ? player.Right_Leg : player.Right_Foot;
        case 6: return (player.RigType == 1) ? player.Lower_Torso : player.HumanoidRootPart;
        case 7: return (player.RigType == 1) ? player.Upper_Torso : player.HumanoidRootPart;
        default: return player.Head;
    }
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
