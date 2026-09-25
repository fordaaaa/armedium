#pragma once
// ─── World visuals: fullbright / no-fog / daytime ──────────────────────────
// Client-side Lighting tweaks. Offsets batch-verified live on 2366ba
// (Ambient/Brightness/ClockTime/FogStart/FogEnd all read sane values).
// Originals are saved on enable and restored on disable.
#include "../rbx/globals/globals.h"
#include "../rbx/globals/options.h"
#include <thread>
#include <chrono>

namespace WorldVisualsState
{
    inline uintptr_t Lighting = 0;
    inline int ResolveTick = 0;
    inline bool WasActive = false;
    inline float SavedAmbient[3] = { 0.5f, 0.5f, 0.5f };
    inline float SavedOutdoor[3] = { 0.5f, 0.5f, 0.5f };
    inline float SavedBrightness = 2.f;
    inline float SavedFogStart = 0.f;
    inline float SavedFogEnd = 100000.f;
    inline float SavedClock = 12.f;
}

inline uintptr_t WorldVisuals_ResolveLighting()
{
    if (!Globals::Roblox::DataModel.address)
        return 0;
    auto kids = Globals::Roblox::DataModel.GetChildren();
    for (auto& k : kids)
        if (k.Class() == "Lighting")
            return k.address;
    return 0;
}

inline void WorldVisuals_Save(uintptr_t light)
{
    for (int i = 0; i < 3; i++)
    {
        WorldVisualsState::SavedAmbient[i] = Memory->read<float>(light + Offsets::Lighting::Ambient + i * 4);
        WorldVisualsState::SavedOutdoor[i] = Memory->read<float>(light + Offsets::Lighting::OutdoorAmbient + i * 4);
    }
    WorldVisualsState::SavedBrightness = Memory->read<float>(light + Offsets::Lighting::Brightness);
    WorldVisualsState::SavedFogStart = Memory->read<float>(light + Offsets::Lighting::FogStart);
    WorldVisualsState::SavedFogEnd = Memory->read<float>(light + Offsets::Lighting::FogEnd);
    WorldVisualsState::SavedClock = Memory->read<float>(light + Offsets::Lighting::ClockTime);
}

inline void WorldVisuals_Restore(uintptr_t light)
{
    for (int i = 0; i < 3; i++)
    {
        Memory->write<float>(light + Offsets::Lighting::Ambient + i * 4, WorldVisualsState::SavedAmbient[i]);
        Memory->write<float>(light + Offsets::Lighting::OutdoorAmbient + i * 4, WorldVisualsState::SavedOutdoor[i]);
    }
    Memory->write<float>(light + Offsets::Lighting::Brightness, WorldVisualsState::SavedBrightness);
    Memory->write<float>(light + Offsets::Lighting::FogStart, WorldVisualsState::SavedFogStart);
    Memory->write<float>(light + Offsets::Lighting::FogEnd, WorldVisualsState::SavedFogEnd);
    Memory->write<float>(light + Offsets::Lighting::ClockTime, WorldVisualsState::SavedClock);
}

void RunWorldVisuals()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        bool active = Options::WorldVisuals::Fullbright || Options::WorldVisuals::NoFog || Options::WorldVisuals::DayTime;

        try
        {
            if (!active)
            {
                if (WorldVisualsState::WasActive && WorldVisualsState::Lighting)
                    WorldVisuals_Restore(WorldVisualsState::Lighting);
                WorldVisualsState::WasActive = false;
                continue;
            }

            if (!WorldVisualsState::Lighting || (++WorldVisualsState::ResolveTick % 50 == 0))
            {
                uintptr_t found = WorldVisuals_ResolveLighting();
                if (found && found != WorldVisualsState::Lighting)
                {
                    if (WorldVisualsState::WasActive && WorldVisualsState::Lighting)
                        WorldVisuals_Restore(WorldVisualsState::Lighting);
                    WorldVisualsState::Lighting = found;
                    WorldVisualsState::WasActive = false;
                }
                else if (found)
                {
                    WorldVisualsState::Lighting = found;
                }
            }
            uintptr_t light = WorldVisualsState::Lighting;
            if (!light) continue;

            if (!WorldVisualsState::WasActive)
            {
                WorldVisuals_Save(light);
                WorldVisualsState::WasActive = true;
            }

            if (Options::WorldVisuals::Fullbright)
            {
                for (int i = 0; i < 3; i++)
                {
                    Memory->write<float>(light + Offsets::Lighting::Ambient + i * 4, 1.f);
                    Memory->write<float>(light + Offsets::Lighting::OutdoorAmbient + i * 4, 1.f);
                }
                Memory->write<float>(light + Offsets::Lighting::Brightness, 5.f);
            }
            if (Options::WorldVisuals::NoFog)
            {
                Memory->write<float>(light + Offsets::Lighting::FogStart, 100000.f);
                Memory->write<float>(light + Offsets::Lighting::FogEnd, 1000000.f);
            }
            if (Options::WorldVisuals::DayTime)
            {
                Memory->write<float>(light + Offsets::Lighting::ClockTime, Options::WorldVisuals::Clock);
            }
        }
        catch (...)
        {
        }
    }
}
