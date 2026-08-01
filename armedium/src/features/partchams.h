#pragma once
#include <string>
#include <vector>
#include <cmath>
#include "../overlay/utils/W2S.h"
#include "../overlay/imgui/imgui.h"
#include "../rbx/globals/options.h"
#include "../rbx/globals/globals.h"

// Project the part's real 3D box (from its primitive Size + CFrame) onto the overlay.
// Offscreen corners are skipped per-edge; at least 2 valid corners are required.
// fillColor (optional) draws the front/back faces at low alpha.
inline void DrawPartBox(ImDrawList* dl, const RobloxInstance& part, ImU32 color, float thickness, ImU32 fillColor = 0)
{
    if (!part.address || !dl)
        return;

    try
    {
        uintptr_t primitive = Memory->read<uintptr_t>(part.address + Offsets::BasePart::Primitive);
        if (!primitive)
            return;

        Vectors::Vector3 size = Memory->read<Vectors::Vector3>(primitive + Offsets::BasePart::Size);
        if (size.x < 0.01f || size.y < 0.01f || size.z < 0.01f ||
            size.x > 50.f || size.y > 50.f || size.z > 50.f)
            return;

        Vectors::Vector3 center = part.Position();
        sCFrame cf = part.CFrame();
        Vectors::Vector3 hw = cf.GetRightVector() * (size.x * 0.5f);
        Vectors::Vector3 hh = cf.GetUpVector() * (size.y * 0.5f);
        Vectors::Vector3 hd = cf.GetLookVector() * (size.z * 0.5f);

        Vectors::Vector3 corners3D[8] = {
            center + hw + hh + hd,
            center - hw + hh + hd,
            center - hw - hh + hd,
            center + hw - hh + hd,
            center + hw + hh - hd,
            center - hw + hh - hd,
            center - hw - hh - hd,
            center + hw - hh - hd
        };

        ImVec2 corners2D[8];
        int validCount = 0;
        for (int i = 0; i < 8; i++)
        {
            Vectors::Vector2 screen = WorldToScreen(corners3D[i]);
            if (screen.x != -1 && screen.y != -1)
            {
                corners2D[i] = ImVec2(screen.x, screen.y);
                validCount++;
            }
            else
            {
                corners2D[i] = ImVec2(-1.f, -1.f);
            }
        }

        if (validCount < 2)
            return;

        auto valid = [&](int i) { return corners2D[i].x != -1.f && corners2D[i].y != -1.f; };
        auto drawEdge = [&](int a, int b) {
            if (valid(a) && valid(b))
                dl->AddLine(corners2D[a], corners2D[b], color, thickness);
        };
        auto drawQuad = [&](int a, int b, int c, int d) {
            if (valid(a) && valid(b) && valid(c) && valid(d))
                dl->AddQuadFilled(corners2D[a], corners2D[b], corners2D[c], corners2D[d], fillColor);
        };

        if (fillColor != 0)
        {
            drawQuad(0, 1, 2, 3); // front
            drawQuad(4, 5, 6, 7); // back
        }

        drawEdge(0, 1); drawEdge(1, 2); drawEdge(2, 3); drawEdge(3, 0);
        drawEdge(4, 5); drawEdge(5, 6); drawEdge(6, 7); drawEdge(7, 4);
        drawEdge(0, 4); drawEdge(1, 5); drawEdge(2, 6); drawEdge(3, 7);
    }
    catch (...)
    {
    }
}

// Local player's team brick-color name (mirrors esp.h). Empty string when teamless.
inline std::string Chams_GetLocalTeamColor()
{
    auto localTeam = Globals::Roblox::LocalPlayer.Team();
    if (localTeam.address == 0)
        return "";
    return Memory->readString(Memory->read<uintptr_t>(localTeam.address + Offsets::Team::BrickColorName));
}

inline bool Chams_IsTeammate(const RobloxPlayer& player, const std::string& localTeamColor)
{
    if (localTeamColor.empty() || player.TeamColor.empty())
        return false;
    return player.TeamColor == localTeamColor;
}

// Part chams selection modes
enum PartChamsSelect
{
    PartChams_Head = 0,
    PartChams_Torso = 1,
    PartChams_Arms = 2,
    PartChams_Legs = 3,
    PartChams_All = 4
};

// Collect the parts for the given selection mode, mapping R6/R15 cached parts correctly.
inline void Chams_CollectParts(const RobloxPlayer& player, int mode, std::vector<RobloxInstance>& out)
{
    out.clear();

    auto add = [&](const RobloxInstance& part) {
        if (part.address)
            out.push_back(part);
    };

    if (mode == PartChams_Head || mode == PartChams_All)
        add(player.Head);

    if (mode == PartChams_Torso || mode == PartChams_All)
    {
        if (player.RigType == 0)
            add(player.Torso);
        else
        {
            add(player.Upper_Torso);
            add(player.Lower_Torso);
        }
    }

    if (mode == PartChams_Arms || mode == PartChams_All)
    {
        if (player.RigType == 0)
        {
            add(player.Left_Arm);
            add(player.Right_Arm);
        }
        else
        {
            add(player.Left_Upper_Arm);
            add(player.Left_Lower_Arm);
            add(player.Left_Hand);
            add(player.Right_Upper_Arm);
            add(player.Right_Lower_Arm);
            add(player.Right_Hand);
        }
    }

    if (mode == PartChams_Legs || mode == PartChams_All)
    {
        if (player.RigType == 0)
        {
            add(player.Left_Leg);
            add(player.Right_Leg);
        }
        else
        {
            add(player.Left_Upper_Leg);
            add(player.Left_Lower_Leg);
            add(player.Left_Foot);
            add(player.Right_Upper_Leg);
            add(player.Right_Lower_Leg);
            add(player.Right_Foot);
        }
    }
}

inline void RenderPartChams(ImDrawList* dl, bool menuOpen = false)
{
    if (!Options::PartChams::Enabled)
        return;
    if (Options::PartChams::OnlyWhenMenuClosed && menuOpen)
        return;
    auto players = SnapshotCachedPlayerObjects();
    if (!dl || players.empty())
        return;

    std::string localTeamColor;
    if (Options::PartChams::TeamCheck)
        localTeamColor = Chams_GetLocalTeamColor();

    int r = (int)(Options::PartChams::Color[0] * 255.f);
    int g = (int)(Options::PartChams::Color[1] * 255.f);
    int b = (int)(Options::PartChams::Color[2] * 255.f);
    int a = (int)(Options::PartChams::Alpha * 255.f);
    ImU32 color = IM_COL32(r, g, b, a);
    ImU32 fill = Options::PartChams::Filled ? IM_COL32(r, g, b, (int)(a * 0.35f)) : 0;

    std::vector<RobloxInstance> parts;
    for (auto& player : players)
    {
        try
        {
            if (IsLocalPlayerEntry(player))
                continue;
            if (player.Health <= 0)
                continue;
            if (Options::PartChams::TeamCheck && Chams_IsTeammate(player, localTeamColor))
                continue;

            Chams_CollectParts(player, Options::PartChams::PartSelect, parts);
            for (auto& part : parts)
                DrawPartBox(dl, part, color, Options::PartChams::Thickness, fill);
        }
        catch (...)
        {
        }
    }
}
