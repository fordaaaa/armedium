#pragma once
#include <string>

// ─── Single Source of Truth for Roblox Client Version ──────────────────────
// Include this header anywhere the version string is needed. When updating
// offsets from https://github.com/Offsetmanager/Roblox-Internal-External-Offsets,
// change ONLY this one line — all dependent files pick it up automatically.
#define ROBOX_CLIENT_VERSION "version-2366ba214ec740ca"

namespace OffsetVersion
{
    inline const std::string Client = ROBOX_CLIENT_VERSION;
}
