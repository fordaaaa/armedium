#pragma once
#include "../rbx/globals/options.h"
#include "../rbx/globals/globals.h"
#include <map>
#include <thread>

inline void RunHitboxExpander()
{
    struct ExpandedPart
    {
        uintptr_t primitive;
        Vectors::Vector3 originalSize;
        float originalTransparency;
        Vectors::Vector3 writtenSize;
        float writtenTransparency;
    };
    static std::map<uintptr_t, ExpandedPart> expandedParts; // key = HRP instance address
    static bool wasEnabled = false;

    auto restorePart = [](const std::pair<const uintptr_t, ExpandedPart>& entry) {
        const uintptr_t hrpAddr = entry.first;
        const ExpandedPart& data = entry.second;

        Vectors::Vector3 currentSize = Memory->read<Vectors::Vector3>(data.primitive + Offsets::BasePart::Size);
        if ((currentSize - data.writtenSize).Magnitude() < 0.01f)
            Memory->write<Vectors::Vector3>(data.primitive + Offsets::BasePart::Size, data.originalSize);

        float currentTransparency = Memory->read<float>(hrpAddr + Offsets::BasePart::Transparency);
        if (fabsf(currentTransparency - data.writtenTransparency) < 0.01f)
            Memory->write<float>(hrpAddr + Offsets::BasePart::Transparency, data.originalTransparency);
    };

    while (true)
    {
        try
        {
            if (Options::HitboxExpander::Enabled != wasEnabled)
            {
                wasEnabled = Options::HitboxExpander::Enabled;
                if (!wasEnabled)
                {
                    for (const auto& entry : expandedParts)
                        restorePart(entry);
                    expandedParts.clear();
                }
            }

            if (!Options::HitboxExpander::Enabled)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            if (!Globals::Roblox::Players.address)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            auto players = Globals::Roblox::Players.GetChildren();
            for (auto player : players)
            {
                if (!Options::HitboxExpander::Enabled)
                    break;

                if (!player.address || player.address == Globals::Roblox::LocalPlayer.address)
                    continue;

                auto character = player.Character();
                if (!character.address) 
                    continue;

                auto hrp = character.FindFirstChild("HumanoidRootPart");
                if (!hrp.address) 
                    continue;

                uintptr_t primitive = Memory->read<uintptr_t>(hrp.address + Offsets::BasePart::Primitive);
                if (!primitive) 
                    continue;

                // Set hitbox size (horizontal for X/Z, vertical for Y)
                Vectors::Vector3 currentSize = Memory->read<Vectors::Vector3>(primitive + Offsets::BasePart::Size);
                Vectors::Vector3 newSize = { 
                    Options::HitboxExpander::HorizontalSize, 
                    Options::HitboxExpander::VerticalSize, 
                    Options::HitboxExpander::HorizontalSize 
                };
                float newTransparency = Options::HitboxExpander::ShowHitbox
                    ? Options::HitboxExpander::HitboxTransparency
                    : 1.0f;

                // Remember the original state the first time we touch this part
                auto it = expandedParts.find(hrp.address);
                if (it == expandedParts.end())
                {
                    if (currentSize.Magnitude() > 0.01f)
                    {
                        float currentTransparency = Memory->read<float>(hrp.address + Offsets::BasePart::Transparency);
                        expandedParts[hrp.address] = { primitive, currentSize, currentTransparency, newSize, newTransparency };
                    }
                }
                else
                {
                    it->second.writtenSize = newSize;
                    it->second.writtenTransparency = newTransparency;
                }

                Memory->write<Vectors::Vector3>(primitive + Offsets::BasePart::Size, newSize);

                // Collision flags live in the primitive flags byte
                uint8_t flags = Memory->read<uint8_t>(primitive + Offsets::Primitive::Flags);
                if (Options::HitboxExpander::WalkThrough)
                    flags &= ~Offsets::PrimitiveFlags::CanCollide;
                else
                    flags |= Offsets::PrimitiveFlags::CanCollide;
                Memory->write<uint8_t>(primitive + Offsets::Primitive::Flags, flags);

                Memory->write<float>(hrp.address + Offsets::BasePart::Transparency, newTransparency);
            }

            // Prune entries whose part no longer exists (stale memory reads back as zero size)
            for (auto it = expandedParts.begin(); it != expandedParts.end(); )
            {
                Vectors::Vector3 currentSize = Memory->read<Vectors::Vector3>(it->second.primitive + Offsets::BasePart::Size);
                if (currentSize.Magnitude() <= 0.01f)
                    it = expandedParts.erase(it);
                else
                    ++it;
            }
        }
        catch (...)
        {
            // Silently catch any exceptions to prevent crashes
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
