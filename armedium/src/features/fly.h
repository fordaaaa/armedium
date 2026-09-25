#pragma once
#include "../rbx/globals/globals.h"
#include "../rbx/globals/options.h"
#include "gravity.h"
#include <thread>
#include <chrono>

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

inline void Fly_Deactivate(uintptr_t humanoid)
{
    if (humanoid)
    {
        Memory->write<bool>(humanoid + Offsets::Humanoid::PlatformStand, false);
        Memory->write<bool>(humanoid + Offsets::Humanoid::AutoRotate, true);
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
                    Fly_Deactivate(humanoid.address);
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

            // Camera-relative wish direction.
            Vectors::Vector3 move(0, 0, 0);
            auto camera = Globals::Roblox::Camera;
            if (camera.address)
            {
                sCFrame cam = camera.CFrame();
                Vectors::Vector3 forward = cam.GetLookVector();
                Vectors::Vector3 right = cam.GetRightVector();
                forward.y = 0.f;
                forward = forward.Normalize();
                right = right.Normalize();

                if (GetAsyncKeyState('W') & 0x8000) move = move + forward;
                if (GetAsyncKeyState('S') & 0x8000) move = move - forward;
                if (GetAsyncKeyState('A') & 0x8000) move = move - right;
                if (GetAsyncKeyState('D') & 0x8000) move = move + right;
            }
            if (GetAsyncKeyState(VK_SPACE) & 0x8000 || GetAsyncKeyState('E') & 0x8000) move.y += 1.f;
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000 || GetAsyncKeyState('C') & 0x8000) move.y -= 1.f;

            if (move.x != 0.f || move.y != 0.f || move.z != 0.f)
            {
                move = move.Normalize() * (0.01f * Options::Fly::Speed);
                sCFrame cf = Memory->read<sCFrame>(primitive + Offsets::Primitive::Rotation);
                cf.x += move.x;
                cf.y += move.y;
                cf.z += move.z;
                Memory->write<sCFrame>(primitive + Offsets::Primitive::Rotation, cf);
            }

            FlyState::WasActive = true;
        }
        catch (...)
        {
        }
    }
}
