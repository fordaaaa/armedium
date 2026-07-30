#pragma once
#include "../rbx/globals/globals.h"
#include "../rbx/globals/options.h"
#include <thread>
#include <chrono>

void FlyLoop()
{
    static bool wasToggled = false;

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (Options::Fly::FlyKey != 0 && Options::Fly::HoldKey)
        {
            bool isPressed = (GetAsyncKeyState(Options::Fly::FlyKey) & 0x8000) != 0;
            Options::Fly::Toggled = isPressed;
        }
        else if (Options::Fly::FlyKey != 0)
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

        if (!Options::Fly::Enabled)
            continue;

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

            auto camera = Globals::Roblox::Camera;
            if (!camera.address) continue;

            auto camCFrame = camera.CFrame();
            Vectors::Vector3 forward(-camCFrame.r02, -camCFrame.r12, -camCFrame.r22);
            Vectors::Vector3 right(camCFrame.r00, camCFrame.r10, camCFrame.r20);
            Vectors::Vector3 up(camCFrame.r01, camCFrame.r11, camCFrame.r21);

            uintptr_t primitive = Memory->read<uintptr_t>(humanoidRootPart.address + Offsets::BasePart::Primitive);
            if (!primitive) continue;

            if (Options::Fly::Toggled)
            {
                Memory->write<bool>(humanoid.address + Offsets::Humanoid::PlatformStand, true);
                Memory->write<bool>(humanoid.address + Offsets::Humanoid::AutoRotate, false);
                Memory->write<Vectors::Vector3>(primitive + Offsets::Primitive::AssemblyAngularVelocity, Vectors::Vector3(0, 0, 0));
                wasToggled = true;

                float speed = Options::Fly::Speed;

                Vectors::Vector3 moveDir(0, 0, 0);
                if (GetAsyncKeyState('W') & 0x8000) moveDir = moveDir + forward;
                if (GetAsyncKeyState('S') & 0x8000) moveDir = moveDir - forward;
                if (GetAsyncKeyState('A') & 0x8000) moveDir = moveDir - right;
                if (GetAsyncKeyState('D') & 0x8000) moveDir = moveDir + right;

                if (GetAsyncKeyState(VK_SPACE) & 0x8000)
                    moveDir = moveDir + up;
                if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                    moveDir = moveDir - up;

                float mag = moveDir.Magnitude();
                Vectors::Vector3 velocity(0, 0, 0);
                if (mag > 0)
                    velocity = moveDir * (speed / mag);

                Memory->write<Vectors::Vector3>(primitive + Offsets::BasePart::AssemblyLinearVelocity, velocity);
            }
            else if (wasToggled)
            {
                Memory->write<bool>(humanoid.address + Offsets::Humanoid::PlatformStand, false);
                Memory->write<bool>(humanoid.address + Offsets::Humanoid::AutoRotate, true);
                wasToggled = false;
            }
        }
        catch (...)
        {
        }
    }
}
