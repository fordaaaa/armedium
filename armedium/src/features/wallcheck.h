#pragma once
#include <windows.h>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>
#include <unordered_set>
#include <string>
#include <cmath>
#include <algorithm>
#include <cfloat>
#include "../rbx/math/math.h"
#include "../rbx/offsets.h"
#include "../rbx/globals/options.h"
#include "../rbx/globals/globals.h"

struct WallCheckPart
{
    uintptr_t address = 0;
    Vectors::Vector3 position{};
    Vectors::Vector3 size{};
    Matrixes::Matrix3x3 rotation{};
    bool isPlayerPart = false;
    bool isLocalPart = false;
    bool canOcclude = true;
};

namespace WallCheckCache
{
    inline std::mutex Mutex;
    inline std::vector<WallCheckPart> Parts;
}

inline Vectors::Vector3 WallCheck_GetCameraPosition()
{
    return Memory->read<Vectors::Vector3>(Globals::Roblox::Camera.address + Offsets::Camera::Position);
}

inline bool WallCheck_IsBasePartClass(const std::string& cls)
{
    static const char* partClasses[] = {
        "Part", "WedgePart", "CornerWedgePart", "Cylinder", "Ball",
        "MeshPart", "TrussPart", "PartOperation", "UnionOperation",
        "SpawnLocation", "Seat", "VehicleSeat", "ControllerPart"
    };
    for (auto* name : partClasses)
        if (cls == name) return true;
    return false;
}

inline bool RayOBB(const Vectors::Vector3& origin, const Vectors::Vector3& dir,
                   const Vectors::Vector3& center, const Matrixes::Matrix3x3& rot,
                   const Vectors::Vector3& half, float& outT)
{
    float ox = origin.x - center.x;
    float oy = origin.y - center.y;
    float oz = origin.z - center.z;

    float lox = rot.r00 * ox + rot.r10 * oy + rot.r20 * oz;
    float loy = rot.r01 * ox + rot.r11 * oy + rot.r21 * oz;
    float loz = rot.r02 * ox + rot.r12 * oy + rot.r22 * oz;

    float ldx = rot.r00 * dir.x + rot.r10 * dir.y + rot.r20 * dir.z;
    float ldy = rot.r01 * dir.x + rot.r11 * dir.y + rot.r21 * dir.z;
    float ldz = rot.r02 * dir.x + rot.r12 * dir.y + rot.r22 * dir.z;

    float tmin = -FLT_MAX;
    float tmax = FLT_MAX;

    if (std::abs(ldx) < 1e-8f)
    {
        if (lox < -half.x || lox > half.x) return false;
    }
    else
    {
        float t1 = (-half.x - lox) / ldx;
        float t2 = (half.x - lox) / ldx;
        if (t1 > t2) std::swap(t1, t2);
        tmin = (std::max)(tmin, t1);
        tmax = (std::min)(tmax, t2);
        if (tmin > tmax) return false;
    }

    if (std::abs(ldy) < 1e-8f)
    {
        if (loy < -half.y || loy > half.y) return false;
    }
    else
    {
        float t1 = (-half.y - loy) / ldy;
        float t2 = (half.y - loy) / ldy;
        if (t1 > t2) std::swap(t1, t2);
        tmin = (std::max)(tmin, t1);
        tmax = (std::min)(tmax, t2);
        if (tmin > tmax) return false;
    }

    if (std::abs(ldz) < 1e-8f)
    {
        if (loz < -half.z || loz > half.z) return false;
    }
    else
    {
        float t1 = (-half.z - loz) / ldz;
        float t2 = (half.z - loz) / ldz;
        if (t1 > t2) std::swap(t1, t2);
        tmin = (std::max)(tmin, t1);
        tmax = (std::min)(tmax, t2);
        if (tmin > tmax) return false;
    }

    if (tmax < 0.0f) return false;
    outT = (tmin > 0.0f) ? tmin : 0.0f;
    return true;
}

inline bool IsPointVisible(const Vectors::Vector3& from, const Vectors::Vector3& to,
                           float* outHitDist = nullptr, uintptr_t skipPartAddr = 0)
{
    std::vector<WallCheckPart> snapshot;
    {
        std::lock_guard<std::mutex> lock(WallCheckCache::Mutex);
        snapshot = WallCheckCache::Parts;
    }

    if (snapshot.empty())
    {
        if (outHitDist) *outHitDist = (to - from).Magnitude();
        return true;
    }

    Vectors::Vector3 dir = to - from;
    float targetDist = dir.Magnitude();
    if (targetDist < 0.001f)
    {
        if (outHitDist) *outHitDist = 0.0f;
        return true;
    }
    dir = dir * (1.0f / targetDist);

    bool hit = false;
    float nearest = targetDist;

    for (const auto& p : snapshot)
    {
        if (p.address == skipPartAddr) continue;
        if (!p.canOcclude) continue;
        if (p.isPlayerPart)
        {
            if (p.isLocalPart && !Options::WallCheck::SelfOcclusion) continue;
            if (!p.isLocalPart && !Options::WallCheck::PlayerOcclusion) continue;
        }

        Vectors::Vector3 half{ p.size.x * 0.5f, p.size.y * 0.5f, p.size.z * 0.5f };
        float t = 0.0f;
        if (RayOBB(from, dir, p.position, p.rotation, half, t) && t < nearest)
        {
            nearest = t;
            hit = true;
        }
    }

    if (outHitDist) *outHitDist = hit ? nearest : targetDist;
    return !hit;
}

inline bool IsPartVisible(const RobloxInstance& part, float* outHitDist = nullptr)
{
    Vectors::Vector3 from = WallCheck_GetCameraPosition();
    Vectors::Vector3 to = part.Position();
    return IsPointVisible(from, to, outHitDist, part.address);
}

inline void WallCheck_CollectPlayerCharacters(std::unordered_set<uintptr_t>& outChars)
{
    uintptr_t localChar = Globals::Roblox::LocalPlayer.Character().address;
    if (localChar) outChars.insert(localChar);
    for (const auto& pl : Globals::Caches::CachedPlayerObjects)
        if (pl.Character.address) outChars.insert(pl.Character.address);
}

inline void WallCheck_ScanInstance(const RobloxInstance& inst, int depth,
                                   std::vector<WallCheckPart>& out,
                                   const std::unordered_set<uintptr_t>& playerChars,
                                   uintptr_t localChar)
{
    if (depth > Options::WallCheck::MaxDepth) return;

    std::vector<RobloxInstance> children;
    try { children = inst.GetChildren(); } catch (...) { return; }

    for (auto& child : children)
    {
        if (!child.address) continue;
        if ((int)out.size() >= Options::WallCheck::MaxParts) return;

        std::string cls;
        try { cls = child.Class(); } catch (...) { continue; }

        if (cls == "Terrain") continue;

        if (WallCheck_IsBasePartClass(cls))
        {
            try
            {
                uintptr_t prim = Memory->read<uintptr_t>(child.address + Offsets::BasePart::Primitive);
                if (!prim) continue;

                sCFrame cf = Memory->read<sCFrame>(prim + Offsets::BasePart::Rotation);
                Vectors::Vector3 size = Memory->read<Vectors::Vector3>(prim + Offsets::BasePart::Size);
                uint8_t flags = Memory->read<uint8_t>(prim + Offsets::Primitive::Flags);

                if (!std::isfinite(cf.x) || !std::isfinite(cf.y) || !std::isfinite(cf.z)) continue;
                if (!std::isfinite(size.x) || !std::isfinite(size.y) || !std::isfinite(size.z)) continue;
                if (size.x < 0.01f || size.x > 5000.f || size.y < 0.01f || size.y > 5000.f ||
                    size.z < 0.01f || size.z > 5000.f) continue;

                WallCheckPart p;
                p.address = child.address;
                p.position = cf.Position();
                p.rotation = { cf.r00, cf.r01, cf.r02, cf.r10, cf.r11, cf.r12, cf.r20, cf.r21, cf.r22 };
                p.size = size;
                p.canOcclude = (flags & Offsets::PrimitiveFlags::CanQuery) != 0;

                uintptr_t parent = Memory->read<uintptr_t>(child.address + Offsets::Instance::Parent);
                for (int i = 0; i < 8 && parent; i++)
                {
                    if (parent == localChar) { p.isPlayerPart = true; p.isLocalPart = true; break; }
                    if (playerChars.count(parent)) { p.isPlayerPart = true; break; }
                    parent = Memory->read<uintptr_t>(parent + Offsets::Instance::Parent);
                }

                out.push_back(p);
            }
            catch (...) { continue; }
        }
        else
        {
            WallCheck_ScanInstance(child, depth + 1, out, playerChars, localChar);
        }
    }
}

inline void WallCheck_BuildBuffer()
{
    std::vector<WallCheckPart> built;
    std::unordered_set<uintptr_t> playerChars;
    WallCheck_CollectPlayerCharacters(playerChars);
    uintptr_t localChar = Globals::Roblox::LocalPlayer.Character().address;
    WallCheck_ScanInstance(Globals::Roblox::Workspace, 0, built, playerChars, localChar);

    std::lock_guard<std::mutex> lock(WallCheckCache::Mutex);
    WallCheckCache::Parts.swap(built);
}

inline void RunWallCheckCache()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(Options::WallCheck::RefreshMs));
        if (!Options::WallCheck::Enabled) continue;
        if (!Globals::Roblox::Workspace.address) continue;
        try { WallCheck_BuildBuffer(); } catch (...) {}
    }
}
