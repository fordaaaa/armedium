#pragma once
#include "../rbx/globals/globals.h"
#include "../rbx/globals/options.h"
#include <thread>
#include <chrono>

void FlyLoop()
{
    static bool wasActive = false;

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

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

            uintptr_t primitive = Memory->read<uintptr_t>(humanoidRootPart.address + Offsets::BasePart::Primitive);
            if (!primitive) continue;

            if (active)
            {
                Memory->write<bool>(humanoid.address + Offsets::Humanoid::PlatformStand, true);
                Memory->write<bool>(humanoid.address + Offsets::Humanoid::AutoRotate, false);
                Memory->write<Vectors::Vector3>(primitive + Offsets::Primitive::AssemblyAngularVelocity, Vectors::Vector3(0, 0, 0));

                auto camera = Globals::Roblox::Camera;
                if (camera.address)
                {
                    auto camCFrame = camera.CFrame();
                    Vectors::Vector3 forward = camCFrame.GetLookVector();
                    Vectors::Vector3 right = camCFrame.GetRightVector();
                    forward.y = 0.0f;
                    right.y = 0.0f;
                    forward = forward.Normalize();
                    right = right.Normalize();

                    Vectors::Vector3 moveDir(0, 0, 0);
                    if (GetAsyncKeyState('W') & 0x8000) moveDir = moveDir + forward;
                    if (GetAsyncKeyState('S') & 0x8000) moveDir = moveDir - forward;
                    if (GetAsyncKeyState('A') & 0x8000) moveDir = moveDir - right;
                    if (GetAsyncKeyState('D') & 0x8000) moveDir = moveDir + right;
                    if (GetAsyncKeyState(VK_SPACE) & 0x8000) moveDir.y += 1.0f;
                    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) moveDir.y -= 1.0f;

                    float mag = moveDir.Magnitude();
                    Vectors::Vector3 velocity(0, 0, 0);
                    if (mag > 0.01f)
                        velocity = moveDir * (Options::Fly::Speed / mag);
                    else
                        velocity = Vectors::Vector3(0, 3.0f, 0);

                    Memory->write<Vectors::Vector3>(primitive + Offsets::Primitive::AssemblyLinearVelocity, velocity);
                }

                wasActive = true;
            }
            else if (wasActive)
            {
                Memory->write<bool>(humanoid.address + Offsets::Humanoid::PlatformStand, false);
                Memory->write<bool>(humanoid.address + Offsets::Humanoid::AutoRotate, true);
                Memory->write<Vectors::Vector3>(primitive + Offsets::Primitive::AssemblyAngularVelocity, Vectors::Vector3(0, 0, 0));
                Memory->write<Vectors::Vector3>(primitive + Offsets::Primitive::AssemblyLinearVelocity, Vectors::Vector3(0, 0, 0));
                wasActive = false;
            }
        }
        catch (...)
        {
        }
    }
}
