#pragma once
#include <algorithm>
#include <cmath>
#include "../overlay/utils/W2S.h"
#include "../overlay/imgui/imgui.h"
#include "../rbx/globals/options.h"
#include "../rbx/globals/globals.h"
#include "../overlay/imgui/KeyBind.h"
#include "wallcheck.h"

inline Vectors::Vector3 GetVelocity(const RobloxInstance& part)
{
	if (!part.address)
		return Vectors::Vector3{ 0.f, 0.f, 0.f };
	
	uintptr_t primitiveAddr = Memory->read<uintptr_t>(part.address + Offsets::BasePart::Primitive);
	if (!primitiveAddr)
		return Vectors::Vector3{ 0.f, 0.f, 0.f };
	
	return Memory->read<Vectors::Vector3>(primitiveAddr + Offsets::BasePart::AssemblyLinearVelocity);
}

inline Vectors::Vector3 GetNearestBonePart(const RobloxPlayer& player, RobloxInstance& outPart)
{
    POINT p;
    GetCursorPos(&p);
    float cursorX = static_cast<float>(p.x);
    float cursorY = static_cast<float>(p.y);

    float bestDist = FLT_MAX;
    Vectors::Vector3 bestPos = player.Head.Position();
    RobloxInstance bestPart(0);

    Vectors::Vector3 camPos{};
    bool needCam = Options::Aimbot::VisibleOnly && Options::WallCheck::Enabled;
    if (needCam) camPos = WallCheck_GetCameraPosition();

    auto tryBone = [&](const RobloxInstance& part) {
        if (!part.address) return;
        Vectors::Vector3 pos3D = part.Position();
        if (needCam && !IsPointVisible(camPos, pos3D, nullptr, part.address))
            return;
        Vectors::Vector2 pos2D = WorldToScreen(pos3D);
        if (pos2D.x == -1 && pos2D.y == -1) return;
        float dx = pos2D.x - cursorX;
        float dy = pos2D.y - cursorY;
        float dist = dx * dx + dy * dy;
        if (dist < bestDist) {
            bestDist = dist;
            bestPos = pos3D;
            bestPart = part;
        }
    };

    if (Options::Aimbot::NearestHead)
        tryBone(player.Head);

    if (Options::Aimbot::NearestChest) {
        tryBone(player.HumanoidRootPart);
        if (player.RigType == 1) {
            tryBone(player.Upper_Torso);
            tryBone(player.Lower_Torso);
        }
    }

    if (Options::Aimbot::NearestLegs) {
        if (player.RigType == 0) {
            tryBone(player.Left_Leg);
            tryBone(player.Right_Leg);
        } else {
            tryBone(player.Left_Foot);
            tryBone(player.Right_Foot);
            tryBone(player.Left_Lower_Leg);
            tryBone(player.Right_Lower_Leg);
        }
    }

    outPart = bestPart;

    if (Options::Aimbot::Prediction && bestPart.address != 0) {
        Vectors::Vector3 velocity = GetVelocity(bestPart);
        return Vectors::Vector3{
            bestPos.x + velocity.x / Options::Aimbot::PredictionX,
            bestPos.y + velocity.y / Options::Aimbot::PredictionY,
            bestPos.z + velocity.z / Options::Aimbot::PredictionX
        };
    }

    return bestPos;
}

inline Vectors::Vector3 GetNearestBonePosition(const RobloxPlayer& player)
{
    RobloxInstance part(0);
    return GetNearestBonePart(player, part);
}

inline RobloxInstance GetTargetBonePart(const RobloxPlayer& player, int boneIdx)
{
    switch (boneIdx)
    {
        case 0: return player.Head;
        case 1: return player.HumanoidRootPart;
        case 2: return (player.RigType == 0) ? player.Left_Arm : player.Left_Hand;
        case 3: return (player.RigType == 0) ? player.Right_Arm : player.Right_Hand;
        case 4: return (player.RigType == 0) ? player.Left_Leg : player.Left_Foot;
        case 5: return (player.RigType == 0) ? player.Right_Leg : player.Right_Foot;
        case 6: return (player.RigType == 1) ? player.Lower_Torso : player.HumanoidRootPart;
        case 7: return (player.RigType == 1) ? player.Upper_Torso : player.HumanoidRootPart;
        default: return player.Head;
    }
}

// Part randomizer roll: headChance% of the time keep the requested bone,
// otherwise pick a random body part (torso/arms/legs). Tries not to repeat
// the same part twice in a row for a more natural mix.
inline int RollRandomAimPart(int requestedBone, float headChance)
{
    static int lastRolled = -1;
    static unsigned int seed = 0x9e3779b9u;

    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    unsigned int rnd = seed;

    if (headChance <= 0.0f || (rnd % 100u) < static_cast<unsigned int>(headChance))
        return requestedBone;

    static constexpr int partPool[] = { 1, 2, 3, 4, 5 }; // torso, arms, legs
    int pick = partPool[rnd % 5u];
    if (pick == lastRolled)
    {
        pick = partPool[(rnd / 7u) % 5u];
        if (pick == lastRolled)
            pick = 1;
    }
    lastRolled = pick;
    return pick;
}

inline void GetTargetBoneAndPosition(const RobloxPlayer& player, RobloxInstance& outPart, Vectors::Vector3& outPos)
{
    if (Options::Aimbot::NearestAim)
    {
        outPos = GetNearestBonePart(player, outPart);
        return;
    }

    Vectors::Vector3 velocity = GetVelocity(player.HumanoidRootPart);
    bool isInAir = (velocity.y > 1.0f || velocity.y < -1.0f);

    int boneToUse = isInAir ? Options::Aimbot::AirTargetBone : Options::Aimbot::TargetBone;

    if (Options::Aimbot::PartRandomizer)
        boneToUse = RollRandomAimPart(boneToUse, Options::Aimbot::HeadChance);

    outPart = GetTargetBonePart(player, boneToUse);
    if (!outPart.address)
        outPart = player.Head;

    outPos = outPart.Position();

    if (Options::Aimbot::Prediction && outPart.address != 0)
    {
        Vectors::Vector3 v = GetVelocity(outPart);
        outPos.x += v.x / Options::Aimbot::PredictionX;
        outPos.y += v.y / Options::Aimbot::PredictionY;
        outPos.z += v.z / Options::Aimbot::PredictionX;
    }
}

inline Vectors::Vector3 GetTargetPosition(const RobloxPlayer& player)
{
    RobloxInstance part(0);
    Vectors::Vector3 pos;
    GetTargetBoneAndPosition(player, part, pos);
    return pos;
}

inline RobloxPlayer GetClosestPlayer()
{
    RobloxPlayer target;
    auto maxDistance = FLT_MAX;
    auto localTeam = Globals::Roblox::LocalPlayer.Team();
    std::string localTeamColor;
    if (localTeam.address != 0)
    {
        localTeamColor = Memory->readString(Memory->read<uintptr_t>(localTeam.address + Offsets::Team::BrickColorName));
    }
    auto localCharacter = Globals::Roblox::LocalPlayer.Character();
    auto localHRP = localCharacter.FindFirstChild("HumanoidRootPart");
    if (!localHRP.address)
        return target; // dead/spectating - no 3D distance to compute

    POINT p;
    GetCursorPos(&p);

    Vectors::Vector3 camPos{};
    bool needCam = Options::Aimbot::VisibleOnly && Options::WallCheck::Enabled;
    if (needCam) camPos = WallCheck_GetCameraPosition();

    auto players = SnapshotCachedPlayerObjects();
    for (auto& player : players)
    {
        auto HRP = player.HumanoidRootPart;
        if (!HRP.address)
            continue;

        if (player.address == Globals::Roblox::LocalPlayer.address)
            continue;

        if (Options::Aimbot::TeamCheck && !player.TeamColor.empty() && !localTeamColor.empty() &&
            player.TeamColor == localTeamColor)
            continue;

        if (player.Health == 0)
            continue;

        // Skip knocked/downed players if check is enabled (health at or below 5)
        if (player.Health > 0 && player.Health <= 5.0f && Options::Aimbot::DownedCheck)
            continue;

        RobloxInstance targetPart(0);
        Vectors::Vector3 targetPos;
        if (Options::Aimbot::NearestAim)
            targetPos = GetNearestBonePart(player, targetPart);
        else
            GetTargetBoneAndPosition(player, targetPart, targetPos);

        if (needCam && !IsPointVisible(camPos, targetPos, nullptr, targetPart.address))
            continue;

        auto targetPos2D = WorldToScreen(targetPos);

        if (targetPos2D.x == -1 && targetPos2D.y == -1)
            continue;

        Vectors::Vector3 diff = localHRP.Position() - targetPos;
        float distance3D = diff.Magnitude();
        
        if (distance3D > Options::Aimbot::Range)
            continue;

        // Viewport mode shifts the whole render - it engages from any angle the
        // target is on screen, so the pixel-FOV gate (which made it feel dead)
        // is skipped. Range still applies (3D distance).
        float maxDist = (Options::Aimbot::AimingType == 2) ? FLT_MAX : Options::Aimbot::FOV;

        auto distance = targetPos2D.Distance({ static_cast<float>(p.x), static_cast<float>(p.y) });

        if (distance < maxDistance && distance <= maxDist)
        {
            maxDistance = distance;
            target = player;
        }
    }
    return target;
}

inline float ApplySmoothnessCurve(float smoothness, int curveType)
{
    // Apply curve transformation based on selected type
    // Use exponential scaling for more balanced control across the range
    float t;
    switch (curveType)
    {
        case 0: // Linear - exponential scaling for better balance
        {
            // Map 0.0-1.0 smoothness to exponential speed curve
            // Lower values = faster, higher values = much slower
            float exponent = 1.0f + (smoothness * 4.0f); // 1.0 to 5.0
            t = pow(1.0f - smoothness, exponent);
            break;
        }
        case 1: // Ease In (starts slow, ends fast)
        {
            float exponent = 1.5f + (smoothness * 3.0f);
            t = pow(1.0f - smoothness, exponent);
            break;
        }
        case 2: // Ease Out (starts fast, ends slow)
        {
            float exponent = 2.0f + (smoothness * 2.5f);
            t = pow(1.0f - smoothness, exponent);
            break;
        }
        case 3: // Ease In-Out (smooth on both ends)
        {
            float exponent = 1.8f + (smoothness * 3.5f);
            t = pow(1.0f - smoothness, exponent);
            break;
        }
        case 4: // Custom Bezier Curve
        {
            if (Options::Aimbot::CustomCurveEnabled)
            {
                // Cubic Bezier curve with control points
                float p0 = 0.0f;
                float p1 = Options::Aimbot::CustomCurveP1[1];
                float p2 = Options::Aimbot::CustomCurveP2[1];
                float p3 = 1.0f;
                
                float u = 1.0f - smoothness;
                float tt = smoothness * smoothness;
                float ttt = tt * smoothness;
                float uu = u * u;
                float uuu = uu * u;
                
                // Bezier formula
                float curveValue = uuu * p0 + 3 * uu * smoothness * p1 + 3 * u * tt * p2 + ttt * p3;
                t = 1.0f - curveValue;
            }
            else
            {
                // Fallback to linear if custom not enabled
                float exponent = 1.0f + (smoothness * 4.0f);
                t = pow(1.0f - smoothness, exponent);
            }
            break;
        }
        default:
        {
            float exponent = 1.0f + (smoothness * 4.0f);
            t = pow(1.0f - smoothness, exponent);
            break;
        }
    }
    return std::clamp<float>(t, 0.001f, 1.0f);
}

inline void CameraRotation(const RobloxPlayer& target)
{
    Matrixes::Matrix3x3 currentRotation = Memory->read<Matrixes::Matrix3x3>(Globals::Roblox::Camera.address + Offsets::Camera::Rotation);

    sCFrame cameraCFrame = Globals::Roblox::Camera.CFrame();
    Vectors::Vector3 camPos = Memory->read<Vectors::Vector3>(Globals::Roblox::Camera.address + Offsets::Camera::Position);

    Vectors::Vector3 targetPos = GetTargetPosition(target);
    
    // Add shake if enabled
    if (Options::Aimbot::Shake && Options::Aimbot::ShakeIntensity > 0.0f)
    {
        static float shakeTime = 0.0f;
        shakeTime += 0.1f;
        
        float shakeX = sin(shakeTime * 10.0f) * Options::Aimbot::ShakeIntensity * 0.1f;
        float shakeY = cos(shakeTime * 8.0f) * Options::Aimbot::ShakeIntensity * 0.1f;
        float shakeZ = sin(shakeTime * 12.0f) * Options::Aimbot::ShakeIntensity * 0.1f;
        
        targetPos.x += shakeX;
        targetPos.y += shakeY;
        targetPos.z += shakeZ;
    }

    sCFrame lookAtCFrame = LookAt(camPos, targetPos);

    Vectors::Vector3 rightVec = lookAtCFrame.GetRightVector();
    Vectors::Vector3 upVec = lookAtCFrame.GetUpVector();
    Vectors::Vector3 lookVec = lookAtCFrame.GetLookVector();

    Matrixes::Matrix3x3 rotationMatrix
    {
        rightVec.x, upVec.x, lookVec.x,
        rightVec.y, upVec.y, lookVec.y,
        rightVec.z, upVec.z, lookVec.z
    };

    Vectors::Vector4 currentQuat = Vectors::Vector4::FromMatrix(currentRotation);
    Vectors::Vector4 targetQuat = Vectors::Vector4::FromMatrix(rotationMatrix);

    // Apply smoothness curve
    float t = ApplySmoothnessCurve(Options::Aimbot::Smoothness, Options::Aimbot::SmoothnessCurve);

    Vectors::Vector4 smoothedQuat = Vectors::Vector4::Slerp(currentQuat, targetQuat, t);
    Matrixes::Matrix3x3 smoothedMatrix = smoothedQuat.ToMatrix();

    Memory->write<Matrixes::Matrix3x3>(Globals::Roblox::Camera.address + Offsets::Camera::Rotation, smoothedMatrix);
}

inline void Mouse(const Vectors::Vector2& targetPos, const POINT& p)
{
    static float accumulatedX = 0.0f;
    static float accumulatedY = 0.0f;

    float dx = static_cast<float>(targetPos.x - p.x);
    float dy = static_cast<float>(targetPos.y - p.y);

    // Add shake if enabled
    if (Options::Aimbot::Shake && Options::Aimbot::ShakeIntensity > 0.0f)
    {
        static float shakeTime = 0.0f;
        shakeTime += 0.1f;
        
        float shakeX = sin(shakeTime * 10.0f) * Options::Aimbot::ShakeIntensity * 0.5f;
        float shakeY = cos(shakeTime * 8.0f) * Options::Aimbot::ShakeIntensity * 0.5f;
        
        dx += shakeX;
        dy += shakeY;
    }

    // Apply smoothness curve
    float t = ApplySmoothnessCurve(Options::Aimbot::Smoothness, Options::Aimbot::SmoothnessCurve);
    
    // Scale for mouse movement (higher = faster)
    float speedScale = 50.0f;
    t = t * speedScale;

    float moveX = dx * t;
    float moveY = dy * t;

    accumulatedX += moveX;
    accumulatedY += moveY;

    int intMoveX = static_cast<int>(accumulatedX);
    int intMoveY = static_cast<int>(accumulatedY);

    accumulatedX -= intMoveX;
    accumulatedY -= intMoveY;

    if (intMoveX != 0 || intMoveY != 0)
    {
        SetCursorPos(p.x + intMoveX, p.y + intMoveY);
    }
}

inline void MouseSendInput(const Vectors::Vector2& targetPos, const POINT& currentPos, float sensitivity)
{
    if (currentPos.x == targetPos.x && currentPos.y == targetPos.y)
        return;

    static float accumulatedX = 0.0f;
    static float accumulatedY = 0.0f;

    float dx = static_cast<float>(targetPos.x - currentPos.x);
    float dy = static_cast<float>(targetPos.y - currentPos.y);

    // Add shake if enabled
    if (Options::Aimbot::Shake && Options::Aimbot::ShakeIntensity > 0.0f)
    {
        static float shakeTime = 0.0f;
        shakeTime += 0.1f;
        
        float shakeX = sin(shakeTime * 10.0f) * Options::Aimbot::ShakeIntensity * 0.5f;
        float shakeY = cos(shakeTime * 8.0f) * Options::Aimbot::ShakeIntensity * 0.5f;
        
        dx += shakeX;
        dy += shakeY;
    }

    // Apply smoothness curve
    float t = ApplySmoothnessCurve(Options::Aimbot::Smoothness, Options::Aimbot::SmoothnessCurve);

    // Compensate for the game's in-game sensitivity: higher game sensitivity
    // -> smaller mickeys sent. Only capped at 2x (for sensitivities near 0) -
    // a floor here would break the compensation at high sensitivity (the
    // crosshair overshoots) and deaden the top of the Mouse Sens slider.
    float sensitivityScale = std::clamp(1.0f / (sensitivity + 0.2f), 0.0f, 2.0f);
    float speedScale = 1.5f;

    float moveX = dx * t * sensitivityScale * speedScale;
    float moveY = dy * t * sensitivityScale * speedScale;

    accumulatedX += moveX;
    accumulatedY += moveY;

    int intMoveX = static_cast<int>(accumulatedX);
    int intMoveY = static_cast<int>(accumulatedY);

    if (std::abs(dx) < 1.0f && std::abs(dy) < 1.0f)
    {
        accumulatedX = 0.0f;
        accumulatedY = 0.0f;
        return;
    }

    accumulatedX -= intMoveX;
    accumulatedY -= intMoveY;

    if (intMoveX != 0 || intMoveY != 0)
    {
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dx = intMoveX;
        input.mi.dy = intMoveY;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;

        SendInput(1, &input, sizeof(INPUT));
    }
}

struct ViewportOffset
{
    short x;
    short y;
};

// Viewport shift aim: works in FPS games (e.g. Rivals) where the camera is
// scripted every frame and the cursor is locked to center (no raw input / mouse
// movement possible). Shifting Camera.Viewport slides the rendered image so the
// target lands under the crosshair without moving the camera or the cursor.
//
// IMPORTANT: the engine re-derives Camera.Viewport every frame, so the shift
// must be written EVERY frame while active (a dirty-check "write on change"
// loses to the engine's reset). The reset path is guarded so it never fights
// an active aim (shared by aimbot + silent aim).
inline void ApplyViewportAim(bool active, const Vectors::Vector2& targetPos)
{
    static DWORD lastActiveWrite = 0;

    try
    {
        uintptr_t cameraAddr = Globals::Roblox::Camera.address;
        uintptr_t engine = Globals::Roblox::VisualEngine;
        if (!cameraAddr || !engine)
            return;

        auto res = Memory->read<Vectors::Vector2>(engine + Offsets::VisualEngine::Dimensions);

        DWORD now = GetTickCount64();

        if (active)
        {
            // A stale view matrix (typical mid-teleport) can produce garbage
            // screen coords. float->short is UB for NaN/out-of-range values,
            // so bail on non-finite input and clamp to the short range.
            if (!std::isfinite(targetPos.x) || !std::isfinite(targetPos.y))
                return;

            ViewportOffset vp;
            vp.x = static_cast<short>(std::clamp((res.x - targetPos.x) * 2.0f, -32767.0f, 32767.0f));
            vp.y = static_cast<short>(std::clamp((res.y - targetPos.y) * 2.0f, -32767.0f, 32767.0f));

            Memory->write<ViewportOffset>(cameraAddr + Offsets::Camera::Viewport, vp);
            lastActiveWrite = now;
        }
        else if ((now - lastActiveWrite) >= 50)
        {
            ViewportOffset vp;
            vp.x = static_cast<short>(res.x);
            vp.y = static_cast<short>(res.y);

            Memory->write<ViewportOffset>(cameraAddr + Offsets::Camera::Viewport, vp);
        }
    }
    catch (...)
    {
    }
}

inline void RunAimbot(ImDrawList* drawList)
{
    if (!Options::Aimbot::Aimbot)
        return;

    try
    {
    auto localCharacter = Globals::Roblox::LocalPlayer.Character();
    auto localHRP = localCharacter.FindFirstChild("HumanoidRootPart");
    auto Dimensions = Memory->read<Vectors::Vector2>(Globals::Roblox::VisualEngine + Offsets::VisualEngine::Dimensions);

    if (SnapshotCachedPlayerObjects().empty())
        return;

    POINT p;
    GetCursorPos(&p);

    // NOTE: p stays in RAW screen coords - WorldToScreen already returns screen
    // coords, so converting to client coords would misalign everything in
    // windowed mode.

    int CombatType;

    // Tolerance-based locked-cursor detection: in FPS games (Rivals) the cursor
    // is snapped to center, but never EXACTLY (float compare + window offsets),
    // so a strict == misdetects third-person mode and kills the aim method.
    bool xAxisCheck = std::fabs(p.x - Dimensions.x / 2.0f) <= 3.0f;
    bool yAxisCheck = std::fabs(p.y - Dimensions.y / 2.0f) <= 25.0f;

    if (xAxisCheck && yAxisCheck)
    {
        CombatType = 0;
    }
    else
    {
        CombatType = 1;
    }

    ImColor FOVColor = IM_COL32(
        static_cast<int>(Options::Aimbot::FOVColor[0] * 255.f),
        static_cast<int>(Options::Aimbot::FOVColor[1] * 255.f),
        static_cast<int>(Options::Aimbot::FOVColor[2] * 255.f),
        255);

    ImColor FOVFillColor = IM_COL32(
        static_cast<int>(Options::Aimbot::FOVFillColor[0] * 255.f),
        static_cast<int>(Options::Aimbot::FOVFillColor[1] * 255.f),
        static_cast<int>(Options::Aimbot::FOVFillColor[2] * 255.f),
        static_cast<int>(Options::Aimbot::FOVFillColor[3] * 255.f));

    if (Options::Aimbot::FOV && Options::Aimbot::ShowFOV)
    {
        drawList->AddCircle(ImVec2(p.x, p.y), Options::Aimbot::FOV, FOVColor, 0, Options::Aimbot::FOVThickness);
        if (Options::Aimbot::ShowFOVFill)
        {
            drawList->AddCircleFilled(ImVec2(p.x, p.y), Options::Aimbot::FOV, FOVFillColor, 0);
        }
    }

    // Toggle mode: detect key press edge (only trigger once per press)
    // No keybind set (0) = always active, same as silent aim.
    static bool wasKeyPressed = false;
    bool isKeyPressed = (Options::Aimbot::AimbotKey != 0) && KeyBind::IsPressed(Options::Aimbot::AimbotKey);

    if (Options::Aimbot::AimbotKey != 0)
    {
    if (Options::Aimbot::ToggleType == 1)
    {
        // Toggle mode: only toggle on key press edge (not while held)
        if (isKeyPressed && !wasKeyPressed)
        {
            Options::Aimbot::Toggled = !Options::Aimbot::Toggled;
        }
        wasKeyPressed = isKeyPressed;
        
        // In toggle mode, check if toggled state is active
        if (!Options::Aimbot::Toggled)
        {
            Options::Aimbot::CurrentTarget = RobloxPlayer(0);
            return;
        }
    }
    else
    {
        // Hold mode: check if key is currently pressed
        if (!isKeyPressed)
        {
            Options::Aimbot::CurrentTarget = RobloxPlayer(0);
            Options::Aimbot::Toggled = false; // Reset toggle state when in hold mode
            return;
        }
    }
    } // no keybind set -> always active

    // Dead/spectating: no character to measure range against. Checked after
    // the key handling so toggling still works while dead (an earlier return
    // swallowed key presses and left the toggle stale for the next respawn).
    // Dropping the sticky target here prevents it from surviving a respawn.
    if (!localHRP.address)
    {
        Options::Aimbot::CurrentTarget = RobloxPlayer(0);
        return;
    }

    auto localTeam = Globals::Roblox::LocalPlayer.Team();
    std::string localTeamColor;
    if (localTeam.address != 0)
    {
        localTeamColor = Memory->readString(Memory->read<uintptr_t>(localTeam.address + Offsets::Team::BrickColorName));
    }

    // Stutter logic: skip aiming every X ticks
    static int stutterTickCounter = 0;
    if (Options::Aimbot::Stutter && Options::Aimbot::StutterTicks > 0)
    {
        stutterTickCounter++;
        if (stutterTickCounter >= Options::Aimbot::StutterTicks)
        {
            stutterTickCounter = 0;
            return; // Skip this tick
        }
    }
    else
    {
        stutterTickCounter = 0;
    }

    RobloxPlayer target;
    if (Options::Aimbot::StickyAim)
    {
        if (Options::Aimbot::CurrentTarget.address == 0 ||
            Options::Aimbot::CurrentTarget.Health == 0 ||
            (Options::Aimbot::CurrentTarget.Health <= 5 && Options::Aimbot::DownedCheck) ||
            (Options::Aimbot::TeamCheck && !localTeamColor.empty() && !Options::Aimbot::CurrentTarget.TeamColor.empty() &&
             Options::Aimbot::CurrentTarget.TeamColor == localTeamColor))
        {
            Options::Aimbot::CurrentTarget = GetClosestPlayer();
        }
        else
        {
            // The target may have left the game: its cached part addresses can
            // point at reused/other objects and still read as "alive". Only
            // keep sticking if the player is still cached WITH a character -
            // a destroyed character makes Position() read (0,0,0), which could
            // lock the aim onto world origin.
            bool stillCached = false;
            auto cachedPlayers = SnapshotCachedPlayerObjects();
            for (auto& cp : cachedPlayers)
            {
                if (cp.address == Options::Aimbot::CurrentTarget.address)
                {
                    stillCached = (cp.HumanoidRootPart.address != 0);
                    break;
                }
            }

            bool keep = stillCached;
            if (keep)
            {
                // Only spend the position reads below on a still-valid target
                RobloxInstance curPart(0);
                Vectors::Vector3 curPos;
                GetTargetBoneAndPosition(Options::Aimbot::CurrentTarget, curPart, curPos);
                Vectors::Vector3 diff = curPos - localHRP.Position();
                float distance3D = diff.Magnitude();

                keep = distance3D <= Options::Aimbot::Range;
                if (keep && Options::Aimbot::VisibleOnly && Options::WallCheck::Enabled)
                {
                    Vectors::Vector3 camPos = WallCheck_GetCameraPosition();
                    keep = IsPointVisible(camPos, curPos, nullptr, curPart.address);
                }
            }

            if (!keep)
            {
                Options::Aimbot::CurrentTarget = GetClosestPlayer();
            }
        }

        target = Options::Aimbot::CurrentTarget;
    }
    else
    {
        target = GetClosestPlayer();
    }

    // SensitivityPointer is a version-specific absolute address; if it's stale
    // the read can be garbage/zero. Fall back to 1.0 so the mouse method
    // still produces sane movement instead of imperceptibly tiny moves.
    float sensitivity = 1.0f;
    try
    {
        float sensRaw = Memory->read<float>(Memory->getBaseAddress() + Offsets::MouseService::SensitivityPointer);
        if (std::isfinite(sensRaw) && sensRaw > 0.01f && sensRaw < 100.0f)
            sensitivity = sensRaw;
    }
    catch (...)
    {
    }

    // Manual override: lets the user tune the mouse method per game (the
    // pointer read is often stale on mismatched client versions)
    if (Options::Aimbot::MouseSensitivity > 0.0f)
        sensitivity = Options::Aimbot::MouseSensitivity;

    if (target.address != 0)
    {
        auto targetPos = WorldToScreen(GetTargetPosition(target));

        if (targetPos.x != -1 && targetPos.y != -1)
        {
            // Viewport shift works in any camera mode (FPS, third person, scripted
            // cameras) - never gate it behind the locked-cursor CombatType check,
            // which is exactly what broke it in Rivals.
            if (Options::Aimbot::AimingType == 2)
            {
                ApplyViewportAim(true, targetPos);
            }
            else
            {
                switch (CombatType)
                {
                case 0:
                {
                    // FPS (locked cursor): raw-input games (Rivals) ignore
                    // relative SendInput moves and script the camera every
                    // frame, so Camera/Mouse are dead there. Fall back to the
                    // viewport shift, which slides the rendered image instead
                    // of fighting the camera - the only thing that works.
                    if (Options::Aimbot::ViewportFallbackFPS)
                    {
                        ApplyViewportAim(true, targetPos);
                        break;
                    }

                    switch(Options::Aimbot::AimingType)
                    {
                        case 0: // Camera
                        {
                            CameraRotation(target);
                            break;
                        }
                        case 1: // Mouse
                        {
                            MouseSendInput(targetPos, p, sensitivity);
                            break;
                        }
                    }
                    break;
                }

                case 1:
                {
                    Mouse(targetPos, p);
                    break;
                }

                default:
                    break;
            }
            }
        }
    }
    }
    catch (...)
    {
    }
}



