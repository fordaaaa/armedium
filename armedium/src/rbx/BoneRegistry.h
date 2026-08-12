#pragma once
#include <cstdint>
#include <string>
#include <array>
#include "../SDK/SDK.h"

// ─── Universal Bone ID ─────────────────────────────────────────────────────
// Every aim/ESP feature references bones by BoneId instead of hardcoded struct
// fields. The registry maps each BoneId to its part-name for R6 and R15 (and
// any future rig type). Single source of truth — add a bone once, every
// feature picks it up immediately.
//
// To add a new bone:
//   1. Add to the BoneId enum before COUNT.
//   2. Add its registry entry in BONE_REGISTRY below (R6 name, R15 name,
//      which rig types it belongs to).
// That's it. All aiming, chams, ESP code will use it.

enum class BoneId : uint8_t
{
    Head = 0,
    HumanoidRootPart,
    Torso,
    UpperTorso,
    LowerTorso,
    LeftArm,
    RightArm,
    LeftHand,
    RightHand,
    LeftLeg,
    RightLeg,
    LeftFoot,
    RightFoot,
    // R15-only sub-parts
    LeftUpperArm,
    LeftLowerArm,
    RightUpperArm,
    RightLowerArm,
    LeftUpperLeg,
    LeftLowerLeg,
    RightUpperLeg,
    RightLowerLeg,
    COUNT
};

// ─── Bone Registry Entry ───────────────────────────────────────────────────
struct BoneRegistryEntry
{
    const char* r6Name;      // Part name for R6  (nullptr if N/A for R6)
    const char* r15Name;     // Part name for R15 (nullptr if N/A for R15)
    uint8_t     validRigs;   // Bitmask: 0b01 = R6, 0b10 = R15, 0b11 = both
    BoneId      id;          // Self-reference for lookup
};

// ─── Master Registry ───────────────────────────────────────────────────────
// constexpr array — zero runtime setup cost, lives in .rodata
inline constexpr std::array<BoneRegistryEntry, (size_t)BoneId::COUNT> BONE_REGISTRY = {{
    // id                       R6 name                R15 name               validRigs
    { BoneId::Head,             "Head",                "Head",                0b11 },
    { BoneId::HumanoidRootPart, "HumanoidRootPart",    "HumanoidRootPart",    0b11 },
    { BoneId::Torso,            "Torso",               nullptr,               0b01 },
    { BoneId::UpperTorso,       nullptr,               "UpperTorso",          0b10 },
    { BoneId::LowerTorso,       nullptr,               "LowerTorso",          0b10 },
    { BoneId::LeftArm,          "Left Arm",            nullptr,               0b01 },
    { BoneId::RightArm,         "Right Arm",           nullptr,               0b01 },
    { BoneId::LeftHand,         nullptr,               "LeftHand",            0b10 },
    { BoneId::RightHand,        nullptr,               "RightHand",           0b10 },
    { BoneId::LeftLeg,          "Left Leg",            nullptr,               0b01 },
    { BoneId::RightLeg,         "Right Leg",           nullptr,               0b01 },
    { BoneId::LeftFoot,         nullptr,               "LeftFoot",            0b10 },
    { BoneId::RightFoot,        nullptr,               "RightFoot",           0b10 },
    { BoneId::LeftUpperArm,     nullptr,               "LeftUpperArm",        0b10 },
    { BoneId::LeftLowerArm,     nullptr,               "LeftLowerArm",        0b10 },
    { BoneId::RightUpperArm,    nullptr,               "RightUpperArm",       0b10 },
    { BoneId::RightLowerArm,    nullptr,               "RightLowerArm",       0b10 },
    { BoneId::LeftUpperLeg,     nullptr,               "LeftUpperLeg",        0b10 },
    { BoneId::LeftLowerLeg,     nullptr,               "LeftLowerLeg",        0b10 },
    { BoneId::RightUpperLeg,    nullptr,               "RightUpperLeg",       0b10 },
    { BoneId::RightLowerLeg,    nullptr,               "RightLowerLeg",       0b10 },
}};

// ─── Helper Functions ──────────────────────────────────────────────────────

// Resolve a BoneId to its RobloxInstance for a given player.
// Returns RobloxInstance(0) if the bone doesn't exist for this player's rig.
inline RobloxInstance GetBone(const RobloxPlayer& player, BoneId id)
{
    return player.Bones[(uint8_t)id];
}

// Get the part name for a BoneId given a specific rig type (0=R6, 1=R15).
inline const char* BoneName(BoneId id, int rigType)
{
    auto& entry = BONE_REGISTRY[(uint8_t)id];
    if (rigType == 0)
        return entry.r6Name ? entry.r6Name : entry.r15Name;  // fallback to R15 name
    else
        return entry.r15Name ? entry.r15Name : entry.r6Name;  // fallback to R6 name
}

// Check if a BoneId is valid for a given rig type.
inline bool BoneValidForRig(BoneId id, int rigType)
{
    auto& entry = BONE_REGISTRY[(uint8_t)id];
    uint8_t mask = (rigType == 0) ? 0b01 : 0b10;
    return (entry.validRigs & mask) != 0;
}

// Get the default rig type for a BoneId (which rig it primarily belongs to).
// Used when a bone should map to the "best" instance regardless of actual rig.
inline int BoneDefaultRigMask(BoneId id)
{
    return BONE_REGISTRY[(uint8_t)id].validRigs;
}

// ─── Aim Target Bones (the bones user-facing selectors pick from) ──────────
// These are the 8 bones that show up in the aim target dropdown.
inline constexpr BoneId AIM_TARGET_BONES[] = {
    BoneId::Head,
    BoneId::HumanoidRootPart,
    BoneId::LeftArm,          // R6 Left Arm / R15 LeftHand
    BoneId::RightArm,         // R6 Right Arm / R15 RightHand
    BoneId::LeftLeg,          // R6 Left Leg / R15 LeftFoot
    BoneId::RightLeg,         // R6 Right Leg / R15 RightFoot
    BoneId::LowerTorso,       // R15 LowerTorso (falls back to HRP for R6)
    BoneId::UpperTorso,       // R15 UpperTorso (falls back to HRP for R6)
};
inline constexpr int AIM_TARGET_BONE_COUNT = 8;

// Get the RobloxInstance for an aim-target bone index (0-7).
// This replaces the old GetTargetBonePart() / SilentAim_GetTargetPart().
inline RobloxInstance GetAimTargetBone(const RobloxPlayer& player, int boneIdx)
{
    if (boneIdx < 0 || boneIdx >= AIM_TARGET_BONE_COUNT)
        return player.Bones[(uint8_t)BoneId::Head];

    BoneId id = AIM_TARGET_BONES[boneIdx];
    RobloxInstance part = player.Bones[(uint8_t)id];

    // For R15-only bones on R6, fall back to HumanoidRootPart
    if (!part.address && !BoneValidForRig(id, player.RigType))
        part = player.Bones[(uint8_t)BoneId::HumanoidRootPart];

    // If still nothing, fall back to Head
    if (!part.address)
        part = player.Bones[(uint8_t)BoneId::Head];

    return part;
}

// ─── Nearest-Aim Bone Groups ───────────────────────────────────────────────
// These are the bone groups used by the "nearest bone" aim mode.
// Head group
inline constexpr BoneId NEAREST_HEAD_BONES[] = { BoneId::Head };
inline constexpr int NEAREST_HEAD_COUNT = 1;

// Chest group (HRP + torso bones)
inline constexpr BoneId NEAREST_CHEST_BONES[] = {
    BoneId::HumanoidRootPart,
    BoneId::Torso,
    BoneId::UpperTorso,
    BoneId::LowerTorso,
};
inline constexpr int NEAREST_CHEST_COUNT = 4;

// Leg group
inline constexpr BoneId NEAREST_LEG_BONES_R6[] = {
    BoneId::LeftLeg,
    BoneId::RightLeg,
};
inline constexpr int NEAREST_LEG_R6_COUNT = 2;

inline constexpr BoneId NEAREST_LEG_BONES_R15[] = {
    BoneId::LeftFoot,
    BoneId::RightFoot,
    BoneId::LeftLowerLeg,
    BoneId::RightLowerLeg,
};
inline constexpr int NEAREST_LEG_R15_COUNT = 4;
