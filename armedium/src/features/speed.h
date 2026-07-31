#pragma once
#include "../rbx/globals/globals.h"
#include "../rbx/globals/options.h"
#include <thread>
#include <chrono>

void SpeedLoop()
{
    static bool wasActive = false;

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (Options::WalkSpeed::WalkSpeedKey != 0)
        {
            static bool wasKeyPressed = false;
            bool isKeyPressed = (GetAsyncKeyState(Options::WalkSpeed::WalkSpeedKey) & 0x8000) != 0;
            bool keyDown = isKeyPressed && !wasKeyPressed;

            if (Options::WalkSpeed::ToggleType == 1)
            {
                if (keyDown)
                    Options::WalkSpeed::Toggled = !Options::WalkSpeed::Toggled;
                wasKeyPressed = isKeyPressed;
            }
            else
            {
                Options::WalkSpeed::Toggled = isKeyPressed;
            }
        }
        else
        {
            Options::WalkSpeed::Toggled = Options::WalkSpeed::Enabled;
        }

        bool active = Options::WalkSpeed::Enabled && Options::WalkSpeed::Toggled;

        try
        {
            auto localPlayer = Globals::Roblox::LocalPlayer;
            if (!localPlayer.address) continue;

            auto character = localPlayer.Character();
            if (!character.address) continue;

            auto humanoid = character.FindFirstChildWhichIsA("Humanoid");
            if (!humanoid.address) continue;

            if (active)
            {
                humanoid.SetWalkspeed(Options::WalkSpeed::Speed);
                wasActive = true;
            }
            else if (wasActive)
            {
                humanoid.SetWalkspeed(16.0f);
                wasActive = false;
            }
        }
        catch (...)
        {
        }
    }
}
