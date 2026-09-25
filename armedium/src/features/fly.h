#pragma once
#include "../rbx/globals/globals.h"
#include "../rbx/globals/options.h"
#include "gravity.h"
#include <thread>
#include <chrono>
#include <cmath>

// ─── CFrame fly ────────────────────────────────────────────────────────────
// Moves the HumanoidRootPart CFrame every tick (camera-relative WASD,
// Space/E up, Shift/C down). Gravity is zeroed through the World object
// while active and restored on exit — never write Workspace::Gravity, that
// offset is stale and points at an unrelated field.

namespace FlyState
{
    inline bool WasActive = false;
    inline bool GravitySaved = false;
    inline float SavedGravity = 196.2f;
    inline float SavedReadOnlyGravity = 196.2f;
    inline bool WasKeyPressed = false;
}

inline bool Fly_UpdateToggle()
{
    if (Options::Fly::FlyKey != 0)
    {
        bool pressed = (GetAsyncKeyState(Options::Fly::FlyKey) & 0x8000) != 0;
        if (Options::Fly::ToggleType == 1)
        {
            if (pressed && !FlyState::WasKeyPressed)
                Options::Fly::Toggled = !Options::Fly::Toggled;
            FlyState::WasKeyPressed = pressed;
        }
        else
        {
            Options::Fly::Toggled = pressed;
        }
    }
    else
    {
        Options::Fly::Toggled = Options::Fly::Enabled;
    }
    return Options::Fly::Enabled && Options::Fly::Toggled;
}

inline void Fly_Deactivate(uintptr_t humanoid, uintptr_t primitive)
{
    if (humanoid)
    {
        Memory->write<bool>(humanoid + Offsets::Humanoid::PlatformStand, false);
        Memory->write<bool>(humanoid + Offsets::Humanoid::AutoRotate, true);
    }
    if (primitive)
    {
        Memory->write<Vectors::Vector3>(primitive + Offsets::Primitive::AssemblyLinearVelocity, Vectors::Vector3(0, 0, 0));
        Memory->write<Vectors::Vector3>(primitive + Offsets::Primitive::AssemblyAngularVelocity, Vectors::Vector3(0, 0, 0));
    }
    if (FlyState::GravitySaved)
    {
        Gravity::Write(FlyState::SavedGravity);
        Gravity::WriteRO(FlyState::SavedReadOnlyGravity);
        FlyState::GravitySaved = false;
    }
    FlyState::WasActive = false;
}

void FlyLoop()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        bool active = Fly_UpdateToggle();

        try
        {
            auto localPlayer = Globals::Roblox::LocalPlayer;
            if (!localPlayer.address) { if (FlyState::WasActive) FlyState::WasActive = false; continue; }

            auto character = localPlayer.Character();
            if (!character.address) { if (FlyState::WasActive) FlyState::WasActive = false; continue; }

            auto humanoid = character.FindFirstChildWhichIsA("Humanoid");
            if (!humanoid.address) continue;

            auto rootPart = character.FindFirstChild("HumanoidRootPart");
            if (!rootPart.address) continue;

            uintptr_t primitive = Memory->read<uintptr_t>(rootPart.address + Offsets::BasePart::Primitive);
            if (!primitive) continue;

            if (!active)
            {
                if (FlyState::WasActive)
                    Fly_Deactivate(humanoid.address, primitive);
                continue;
            }

            // Freeze animation + kill gravity.
            Memory->write<bool>(humanoid.address + Offsets::Humanoid::PlatformStand, true);
            Memory->write<bool>(humanoid.address + Offsets::Humanoid::AutoRotate, false);
            if (!FlyState::GravitySaved)
            {
                FlyState::SavedGravity = Gravity::Read();
                FlyState::SavedReadOnlyGravity = Gravity::ReadRO();
                FlyState::GravitySaved = true;
            }
            Gravity::Write(0.f);
            Gravity::WriteRO(0.f);

            // Camera-relative wish direction, then carried by physics velocity.
            // Velocity-primary (no CFrame teleports): teleports fight the
            // physics body and read as jitter/rubber-banding. Gravity stays
            // zeroed so velocity is the only force.
            Vectors::Vector3 wish(0, 0, 0);
            auto camera = Globals::Roblox::Camera;
            if (camera.address)
            {
                sCFrame cam = camera.CFrame();
                Vectors::Vector3 forward = cam.GetLookVector();
                Vectors::Vector3 right = cam.GetRightVector();
                forward.y = 0.f;
                if (forward.Magnitude() > 1e-6f && right.Magnitude() > 1e-6f)
                {
                    forward = forward.Normalize();
                    right = right.Normalize();
                    if (GetAsyncKeyState('W') & 0x8000) wish = wish + forward;
                    if (GetAsyncKeyState('S') & 0x8000) wish = wish - forward;
                    if (GetAsyncKeyState('A') & 0x8000) wish = wish - right;
                    if (GetAsyncKeyState('D') & 0x8000) wish = wish + right;
                }
            }
            if (GetAsyncKeyState(VK_SPACE) & 0x8000 || GetAsyncKeyState('E') & 0x8000) wish.y += 1.f;
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000 || GetAsyncKeyState('C') & 0x8000) wish.y -= 1.f;

            Vectors::Vector3 targetVel(0, 0, 0);
            if (wish.Magnitude() > 1e-6f)
            {
                wish = wish.Normalize();
                float mag = wish.Magnitude();
                // Normalize() on a garbage vector could still yield NaN if the
                // camera matrix is mid-update — never feed NaN to physics.
                if (std::isfinite(mag) && mag > 1e-6f && std::isfinite(wish.x + wish.y + wish.z))
                    targetVel = wish * Options::Fly::Speed;
            }
            Memory->write<Vectors::Vector3>(primitive + Offsets::Primitive::AssemblyLinearVelocity, targetVel);
            Memory->write<Vectors::Vector3>(primitive + Offsets::Primitive::AssemblyAngularVelocity, Vectors::Vector3(0, 0, 0));

            FlyState::WasActive = true;
        }
        catch (...)
        {
        }
    }
}
