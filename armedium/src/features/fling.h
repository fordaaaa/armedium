#pragma once
#include "../rbx/globals/globals.h"
#include "../rbx/globals/options.h"
#include <thread>
#include <chrono>

void FlingLoop()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (Options::Fling::FlingKey != 0 && Options::Fling::HoldKey)
        {
            Options::Fling::Toggled = (GetAsyncKeyState(Options::Fling::FlingKey) & 0x8000) != 0;
        }
        else if (Options::Fling::FlingKey != 0)
        {
            static bool wasKeyPressed = false;
            bool isPressed = (GetAsyncKeyState(Options::Fling::FlingKey) & 0x8000) != 0;
            if (Options::Fling::ToggleType == 1)
            {
                if (isPressed && !wasKeyPressed)
                    Options::Fling::Toggled = !Options::Fling::Toggled;
                wasKeyPressed = isPressed;
            }
            else
            {
                Options::Fling::Toggled = isPressed;
            }
        }

        if (!Options::Fling::Enabled)
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

            uintptr_t primitive = Memory->read<uintptr_t>(humanoidRootPart.address + Offsets::BasePart::Primitive);
            if (!primitive) continue;

            if (Options::Fling::Toggled)
            {
                Memory->write<bool>(humanoid.address + Offsets::Humanoid::PlatformStand, true);
                Memory->write<bool>(humanoid.address + Offsets::Humanoid::AutoRotate, false);

                // Spin fast on Y axis to fling others
                Memory->write<Vectors::Vector3>(primitive + Offsets::Primitive::AssemblyAngularVelocity,
                    Vectors::Vector3(0, Options::Fling::Speed, 0));

                // Keep position stable (prevent falling through floor)
                Vectors::Vector3 pos = Memory->read<Vectors::Vector3>(primitive + Offsets::Primitive::Position);
                Memory->write<Vectors::Vector3>(primitive + Offsets::BasePart::AssemblyLinearVelocity,
                    Vectors::Vector3(0, 0, 0));
            }
            else
            {
                Memory->write<bool>(humanoid.address + Offsets::Humanoid::AutoRotate, true);
            }
        }
        catch (...)
        {
        }
    }
}

void AntiFlingLoop()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        if (!Options::AntiFling::Enabled)
            continue;

        try
        {
            auto localPlayer = Globals::Roblox::LocalPlayer;
            if (!localPlayer.address) continue;

            auto character = localPlayer.Character();
            if (!character.address) continue;

            if (Options::AntiFling::Mode == 0)
            {
                auto parts = character.GetChildren();
                for (auto& part : parts)
                {
                    uintptr_t primitive = Memory->read<uintptr_t>(part.address + Offsets::BasePart::Primitive);
                    if (primitive)
                    {
                        uint8_t collideFlags = Memory->read<uint8_t>(primitive + Offsets::BasePart::CanCollide);
                        if (collideFlags & 0x8)
                            Memory->write<uint8_t>(primitive + Offsets::BasePart::CanCollide, collideFlags & ~0x8);
                        uint8_t queryFlags = Memory->read<uint8_t>(primitive + Offsets::BasePart::CanQuery);
                        if (queryFlags & 0x1)
                            Memory->write<uint8_t>(primitive + Offsets::BasePart::CanQuery, queryFlags & ~0x1);
                    }
                }
            }
            else if (Options::AntiFling::Mode == 1)
            {
                auto humanoidRootPart = character.FindFirstChild("HumanoidRootPart");
                if (!humanoidRootPart.address) continue;

                uintptr_t primitive = Memory->read<uintptr_t>(humanoidRootPart.address + Offsets::BasePart::Primitive);
                if (!primitive) continue;

                Memory->write<Vectors::Vector3>(primitive + Offsets::Primitive::AssemblyAngularVelocity,
                    Vectors::Vector3(0, 0, 0));
                Memory->write<Vectors::Vector3>(primitive + Offsets::BasePart::AssemblyLinearVelocity,
                    Vectors::Vector3(0, 0, 0));
            }
        }
        catch (...)
        {
        }
    }
}
