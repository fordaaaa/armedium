#pragma once
#include <cstdint>
#include <Windows.h>
#include <thread>
#include <chrono>
#include "../overlay/utils/W2S.h"
#include "../rbx/globals/options.h"
#include "../rbx/globals/globals.h"
#include "../rbx/math/math.h"
#include "../overlay/imgui/KeyBind.h"
#include "aimbot.h" // reuse GetVelocity
#include "wallcheck.h"

inline RobloxInstance SilentAim_GetTargetPart(const RobloxPlayer& player, int boneIdx)
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

inline Vectors::Vector3 SilentAim_GetAimPos(const RobloxPlayer& player)
{
    RobloxInstance part = SilentAim_GetTargetPart(player, Options::SilentAim::TargetBone);
    if (!part.address)
        return Vectors::Vector3{ 0.f, 0.f, 0.f };

    Vectors::Vector3 basePos = part.Position();

    if (Options::SilentAim::Prediction)
    {
        Vectors::Vector3 velocity = GetVelocity(part);
        basePos.x += velocity.x / Options::SilentAim::PredictionX;
        basePos.y += velocity.y / Options::SilentAim::PredictionY;
        basePos.z += velocity.z / Options::SilentAim::PredictionX;
    }

    return basePos;
}

inline RobloxPlayer SilentAim_GetClosestPlayer()
{
    RobloxPlayer target;
    float maxDistance = FLT_MAX;
    auto localTeam = Globals::Roblox::LocalPlayer.Team();
    std::string localTeamColor;
    if (localTeam.address != 0)
    {
        localTeamColor = Memory->readString(Memory->read<uintptr_t>(localTeam.address + Offsets::Team::BrickColorName));
    }
    auto localCharacter = Globals::Roblox::LocalPlayer.Character();
    auto localHRP = localCharacter.FindFirstChild("HumanoidRootPart");
    if (!localHRP.address)
        return target;

    POINT p;
    GetCursorPos(&p);

    Vectors::Vector3 camPos{};
    bool needCam = Options::SilentAim::VisibleOnly && Options::WallCheck::Enabled;
    if (needCam) camPos = WallCheck_GetCameraPosition();

    for (auto& player : Globals::Caches::CachedPlayerObjects)
    {
        if (!player.HumanoidRootPart.address)
            continue;
        if (player.address == Globals::Roblox::LocalPlayer.address)
            continue;
        if (Options::SilentAim::TeamCheck && !player.TeamColor.empty() && !localTeamColor.empty() &&
            player.TeamColor == localTeamColor)
            continue;
        if (player.Health == 0)
            continue;
        if (Options::SilentAim::DownedCheck && player.Health > 0 && player.Health <= 5.0f)
            continue;

        Vectors::Vector3 aimPos = SilentAim_GetAimPos(player);
        if (needCam)
        {
            RobloxInstance aimPart = SilentAim_GetTargetPart(player, Options::SilentAim::TargetBone);
            if (!IsPointVisible(camPos, aimPos, nullptr, aimPart.address))
                continue;
        }
        auto aimPos2D = WorldToScreen(aimPos);
        if (aimPos2D.x == -1 && aimPos2D.y == -1)
            continue;

        Vectors::Vector3 diff = localHRP.Position() - aimPos;
        float distance3D = diff.Magnitude();
        if (distance3D > Options::SilentAim::Range)
            continue;

        float distance = aimPos2D.Distance({ static_cast<float>(p.x), static_cast<float>(p.y) });
        if (distance < maxDistance && distance <= Options::SilentAim::FOV)
        {
            maxDistance = distance;
            target = player;
        }
    }
    return target;
}

inline void SilentAim_Apply(const RobloxPlayer& target)
{
    RobloxInstance targetPart = SilentAim_GetTargetPart(target, Options::SilentAim::TargetBone);
    if (!targetPart.address)
        return;

    Vectors::Vector3 aimPos = SilentAim_GetAimPos(target);

    Vectors::Vector3 camPos = Memory->read<Vectors::Vector3>(
        Globals::Roblox::Camera.address + Offsets::Camera::Position);

    float aimDist = (aimPos - camPos).Magnitude();
    if (aimDist < 0.001f)
        return;

    // Visibility gate: never spoof while the target part is behind a wall
    if (Options::SilentAim::VisibleOnly && Options::WallCheck::Enabled &&
        !IsPointVisible(camPos, aimPos, nullptr, targetPart.address))
        return;

    uintptr_t mouseAddr = Memory->read<uintptr_t>(
        Globals::Roblox::LocalPlayer.address + Offsets::Player::Mouse);
    if (!mouseAddr)
        return;

    // Method 0: PlayerMouse.Hit + Target overwrite (stable, reliable default)
    if (Options::SilentAim::Method == 0)
    {
        sCFrame hitCFrame = LookAt(camPos, aimPos);

        Memory->write<sCFrame>(mouseAddr + Offsets::PlayerMouse::Hit, hitCFrame);
        Memory->write<uintptr_t>(mouseAddr + Offsets::PlayerMouse::Target, targetPart.address);
    }

    // Method 1: also overwrite UnitRay (Ray = origin Vector3 + direction Vector3)
    if (Options::SilentAim::Method == 1)
    {
        Vectors::Vector3 dir = aimPos - camPos;
        dir = dir * (1.0f / aimDist);

        sCFrame hitCFrame = LookAt(camPos, aimPos);

        Memory->write<sCFrame>(mouseAddr + Offsets::PlayerMouse::Hit, hitCFrame);
        Memory->write<uintptr_t>(mouseAddr + Offsets::PlayerMouse::Target, targetPart.address);
        Memory->write<Vectors::Vector3>(mouseAddr + Offsets::PlayerMouse::UnitRay, camPos);
        Memory->write<Vectors::Vector3>(mouseAddr + Offsets::PlayerMouse::UnitRay + 0xc, dir);
    }

    // Hitbox on Fire: inflate target part briefly on left-mouse rising edge
    if (Options::SilentAim::HitboxOnFire)
    {
        static bool prevMouseDown = false;
        static uintptr_t inflatedPrimitive = 0;
        static Vectors::Vector3 origSize{};
        static int restoreFrames = 0;

        bool mouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        if (mouseDown && !prevMouseDown && inflatedPrimitive == 0)
        {
            uintptr_t primitiveAddr = Memory->read<uintptr_t>(
                targetPart.address + Offsets::BasePart::Primitive);
            if (primitiveAddr)
            {
                origSize = Memory->read<Vectors::Vector3>(primitiveAddr + Offsets::BasePart::Size);
                float m = Options::SilentAim::HitboxMult;
                Vectors::Vector3 inflated{ origSize.x * m, origSize.y * m, origSize.z * m };
                Memory->write<Vectors::Vector3>(primitiveAddr + Offsets::BasePart::Size, inflated);
                inflatedPrimitive = primitiveAddr;
                restoreFrames = Options::SilentAim::HitboxFrames;
            }
        }
        prevMouseDown = mouseDown;

        if (restoreFrames > 0)
        {
            restoreFrames--;
            if (restoreFrames == 0 && inflatedPrimitive)
            {
                Memory->write<Vectors::Vector3>(inflatedPrimitive + Offsets::BasePart::Size, origSize);
                inflatedPrimitive = 0;
            }
        }
    }
}

inline void RunSilentAim()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (!Options::SilentAim::Enabled)
            continue;

        try
        {
            // Key handling (mirrors aimbot pattern)
            static bool wasKeyPressed = false;
            bool isKeyPressed = (Options::SilentAim::Key != 0) &&
                ((GetAsyncKeyState(Options::SilentAim::Key) & 0x8000) != 0);

            if (Options::SilentAim::ToggleType == 1)
            {
                if (isKeyPressed && !wasKeyPressed)
                    Options::SilentAim::Toggled = !Options::SilentAim::Toggled;
                wasKeyPressed = isKeyPressed;
                if (!Options::SilentAim::Toggled)
                {
                    Options::SilentAim::CurrentTarget = 0;
                    continue;
                }
            }
            else
            {
                // Hold mode: only active while key held (or when Key==0, always active)
                if (Options::SilentAim::Key != 0 && !isKeyPressed)
                {
                    Options::SilentAim::CurrentTarget = 0;
                    Options::SilentAim::Toggled = false;
                    continue;
                }
            }

            RobloxPlayer target = SilentAim_GetClosestPlayer();
            if (target.address == 0)
            {
                Options::SilentAim::CurrentTarget = 0;
                continue;
            }

            Options::SilentAim::CurrentTarget = target.address;
            SilentAim_Apply(target);
        }
        catch (...)
        {
        }
    }
}
