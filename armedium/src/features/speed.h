#pragma once
#include "../rbx/globals/globals.h"
#include "../rbx/globals/options.h"
#include <thread>
#include <chrono>

void SpeedLoop()
{
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

        if (!Options::WalkSpeed::Enabled)
            continue;

        try
        {
            auto localPlayer = Globals::Roblox::LocalPlayer;
            if (!localPlayer.address) continue;

            auto character = localPlayer.Character();
            if (!character.address) continue;

            auto humanoid = character.FindFirstChildWhichIsA("Humanoid");
            if (!humanoid.address) continue;

            humanoid.SetWalkspeed(Options::WalkSpeed::Toggled ? Options::WalkSpeed::Speed : 16.0f);
        }
        catch (...)
        {
        }
    }
}
