#pragma once
#include "../rbx/globals/globals.h"
#include "../rbx/globals/options.h"
#include <thread>
#include <chrono>

// ─── CFrame-based Fly ──────────────────────────────────────────────────────
// Instead of fighting gravity with AssemblyLinearVelocity (which resets every
// physics step), we directly move the HumanoidRootPart CFrame each frame.
// This is effectively a per-frame teleport — zero gravity interaction, works
// on any Roblox version regardless of physics changes.
//
// PlatformStand is still set to freeze humanoid animation, and gravity is
// temporarily zeroed while flying to prevent the physics engine from applying
// downward acceleration between frames.

void FlyLoop()
{
    static bool wasActive = false;
    static float  savedGravity = 196.2f;
    static float  savedReadOnlyGravity = 196.2f;
    static bool   gravitySaved = false;

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Key handling
        if (Options::Fly::FlyKey != 0)
        {
            static bool wasKeyPressed = false;
            bool isPressed = (GetAsyncKeyState(Options::Fly::FlyKey) & 0x8000) != 0;
            if (Options::Fly::ToggleType == 1)
            {
                if (isPressed && !wasKeyPressed)
                    Options::Fly::Toggled = !Options::Fly::Toggled;
                wasKeyPressed = isPressed;
            }
            else
            {
                Options::Fly::Toggled = isPressed;
            }
        }
        else
        {
            Options::Fly::Toggled = Options::Fly::Enabled;
        }

        bool active = Options::Fly::Enabled && Options::Fly::Toggled;

        try
        {
            auto localPlayer = Globals::Roblox::LocalPlayer;
            if (!localPlayer.address) continue;

            auto character = localPlayer.Character();
            if (!character.address) continue;

            auto humanoid = character.FindFirstChildWhichIsA("Humanoid");
            if (!humanoid.address) continue;

            auto humanoidRootPart = character.FindFirstChild("HumanoidRootPart");
            if (!humanoidRootPart.address) continue;

            uintptr_t primitive = Memory->read<uintptr_t>(
                humanoidRootPart.address + Offsets::BasePart::Primitive);
            if (!primitive) continue;

            if (active)
            {
                // Freeze animation
                Memory->write<bool>(humanoid.address + Offsets::Humanoid::PlatformStand, true);
                Memory->write<bool>(humanoid.address + Offsets::Humanoid::AutoRotate, false);

                // Zero out gravity so the physics engine doesn't fight us
                if (Globals::Roblox::Workspace.address != 0)
                {
                    if (!gravitySaved)
                    {
                        savedGravity = Memory->read<float>(
                            Globals::Roblox::Workspace.address + Offsets::Workspace::Gravity);
                        savedReadOnlyGravity = Memory->read<float>(
                            Globals::Roblox::Workspace.address + Offsets::Workspace::ReadOnlyGravity);
                        gravitySaved = true;
                    }
                    Memory->write<float>(
                        Globals::Roblox::Workspace.address + Offsets::Workspace::Gravity, 0.f);
                    Memory->write<float>(
                        Globals::Roblox::Workspace.address + Offsets::Workspace::ReadOnlyGravity, 0.f);
                }

                // ─── CFrame-based movement ───────────────────────────
                auto camera = Globals::Roblox::Camera;
                Vectors::Vector3 move(0, 0, 0);

                if (camera.address)
                {
                    auto camCFrame = camera.CFrame();
                    Vectors::Vector3 forward = camCFrame.GetLookVector();
                    Vectors::Vector3 right = camCFrame.GetRightVector();
                    forward.y = 0.0f;
                    forward = forward.Normalize();
                    right = right.Normalize();

                    if (GetAsyncKeyState('W') & 0x8000) move = move + forward;
                    if (GetAsyncKeyState('S') & 0x8000) move = move - forward;
                    if (GetAsyncKeyState('A') & 0x8000) move = move - right;
                    if (GetAsyncKeyState('D') & 0x8000) move = move + right;
                }

                if (GetAsyncKeyState(VK_SPACE) & 0x8000)  move.y += 1.0f;
                if (GetAsyncKeyState(VK_SHIFT) & 0x8000)  move.y -= 1.0f;

                // Scale by speed and frame time (~10ms)
                float dt = 0.01f * Options::Fly::Speed;
                if (move.x != 0.f || move.y != 0.f || move.z != 0.f)
                    move = move.Normalize() * dt;

                // Read current CFrame, apply delta, write back
                uintptr_t primitiveAddr = Memory->read<uintptr_t>(
                    humanoidRootPart.address + Offsets::BasePart::Primitive);
                if (primitiveAddr)
                {
                    sCFrame currentCFrame = Memory->read<sCFrame>(
                        primitiveAddr + Offsets::BasePart::Rotation);

                    currentCFrame.px += move.x;
                    currentCFrame.py += move.y;
                    currentCFrame.pz += move.z;

                    Memory->write<sCFrame>(
                        primitiveAddr + Offsets::BasePart::Rotation, currentCFrame);
                    Memory->write<Vectors::Vector3>(
                        primitiveAddr + Offsets::BasePart::Position, {0, 0, 0}); // zero positional offset
                }

                wasActive = true;
            }
            else if (wasActive)
            {
                // Restore state
                Memory->write<bool>(humanoid.address + Offsets::Humanoid::PlatformStand, false);
                Memory->write<bool>(humanoid.address + Offsets::Humanoid::AutoRotate, true);

                // Restore gravity
                if (gravitySaved && Globals::Roblox::Workspace.address != 0)
                {
                    Memory->write<float>(
                        Globals::Roblox::Workspace.address + Offsets::Workspace::Gravity, savedGravity);
                    Memory->write<float>(
                        Globals::Roblox::Workspace.address + Offsets::Workspace::ReadOnlyGravity, savedReadOnlyGravity);
                    gravitySaved = false;
                }

                wasActive = false;
            }
        }
        catch (...)
        {
        }
    }
}
