#pragma once
// ─── World gravity helpers ─────────────────────────────────────────────────
// Workspace::Gravity (0x22c) is stale — live on version-2366ba214ec740ca it
// reads 0.0. Real gravity lives in World (World+0x22c == 196.2) plus the
// Workspace ReadOnlyGravity mirror (0x9f0). Always go through these.
#include "../rbx/globals/globals.h"
#include "../rbx/globals/options.h"

namespace Gravity
{
    inline uintptr_t ResolveWorld()
    {
        if (!Globals::Roblox::Workspace.address)
            return 0;
        return Memory->read<uintptr_t>(Globals::Roblox::Workspace.address + Offsets::Workspace::World);
    }

    inline float Read()
    {
        uintptr_t world = ResolveWorld();
        return world ? Memory->read<float>(world + Offsets::World::Gravity) : 196.2f;
    }

    inline void Write(float value)
    {
        uintptr_t world = ResolveWorld();
        if (world)
            Memory->write<float>(world + Offsets::World::Gravity, value);
    }

    inline float ReadRO()
    {
        if (!Globals::Roblox::Workspace.address)
            return 196.2f;
        return Memory->read<float>(Globals::Roblox::Workspace.address + Offsets::Workspace::ReadOnlyGravity);
    }

    inline void WriteRO(float value)
    {
        if (!Globals::Roblox::Workspace.address)
            return;
        Memory->write<float>(Globals::Roblox::Workspace.address + Offsets::Workspace::ReadOnlyGravity, value);
    }
}
