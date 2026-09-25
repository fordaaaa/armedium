#pragma once
// ─── armedium menu (rewritten) ─────────────────────────────────────────────
// Fresh sidebar-style menu. Binds the same Options::* fields as before, so
// existing configs keep loading. Overlay plumbing (device, sync, hotkeys)
// stays in renderer.cpp; everything visual for the menu lives here.
#include <string>
#include <vector>
#include <cctype>
#include <filesystem>
#include "imgui/imgui.h"
#include "imgui/KeyBind.h"
#include "../rbx/globals/options.h"
#include "../rbx/globals/globals.h"
#include "../rbx/configs/configs.h"
#include "../rbx/Webhook.h"

extern ImVec4 main_color;
extern ImFont* InterMedium;
extern ImFont* IconFont;
extern HWND g_overlayHwnd;

namespace Menu
{
    inline int Tab = 0;
    inline int Sub[4] = { 0, 0, 0, 0 };
    inline char Filter[64] = {};
    inline float MenuAlpha = 0.0f;
    inline float BgAlpha = 0.0f;

    // ── search filter ──────────────────────────────────────────────────────
    inline bool Match(const char* label)
    {
        if (!Filter[0]) return true;
        std::string h(label), n(Filter);
        for (auto& c : h) c = (char)tolower((unsigned char)c);
        for (auto& c : n) c = (char)tolower((unsigned char)c);
        return h.find(n) != std::string::npos;
    }

    inline ImVec4 Accent()
    {
        return ImVec4(Options::Misc::MenuAccentColor[0], Options::Misc::MenuAccentColor[1], Options::Misc::MenuAccentColor[2], 1.0f);
    }

    // ── filter-aware widgets (hidden when they don't match the search) ─────
    inline bool Toggle(const char* label, bool* v)
    {
        if (!Match(label)) return false;
        ImGui::PushStyleColor(ImGuiCol_CheckMark, Accent());
        bool r = ImGui::Checkbox(label, v);
        ImGui::PopStyleColor();
        return r;
    }
    inline bool Slider(const char* label, float* v, float lo, float hi, const char* fmt = "%.1f")
    {
        if (!Match(label)) return false;
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, Accent());
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, Accent());
        bool r = ImGui::SliderFloat(label, v, lo, hi, fmt);
        ImGui::PopStyleColor(2);
        return r;
    }
    inline bool SliderI(const char* label, int* v, int lo, int hi)
    {
        if (!Match(label)) return false;
        return ImGui::SliderInt(label, v, lo, hi);
    }
    inline bool Drop(const char* label, int* v, const char* const items[], int count)
    {
        if (!Match(label)) return false;
        return ImGui::Combo(label, v, items, count);
    }
    inline bool Color(const char* label, float c[3])
    {
        if (!Match(label)) return false;
        return ImGui::ColorEdit3(label, c, ImGuiColorEditFlags_NoInputs);
    }
    inline void Key(const char* label, int* k)
    {
        if (!Match(label)) return;
        KeybindSelector(label, k);
    }
    inline void Section(const char* title)
    {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, Accent());
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();
    }

    // Player dropdown bound to a snapshot index (fling target / teleport).
    inline void PlayerCombo(const char* id, const char* label, int* selected)
    {
        ImGui::TextUnformatted(label);
        auto players = SnapshotCachedPlayerObjects();
        static std::vector<std::string> names;
        static std::vector<int> toPlayer;
        names.clear();
        toPlayer.clear();
        int shown = 0;
        for (int i = 0; i < (int)players.size(); i++)
        {
            if (IsLocalPlayerEntry(players[i])) continue;
            if (i == *selected) shown = (int)names.size();
            names.push_back(players[i].Name);
            toPlayer.push_back(i);
        }
        if (names.empty()) { names.push_back("No players"); toPlayer.push_back(-1); }
        ImGui::Combo(id, &shown, [](void* d, int idx, const char** out) -> bool {
            auto& v = *(std::vector<std::string>*)d;
            if (idx < 0 || idx >= (int)v.size()) return false;
            *out = v[idx].c_str();
            return true;
        }, &names, (int)names.size());
        if (shown >= 0 && shown < (int)toPlayer.size())
            *selected = toPlayer[shown];
    }

    // ── pages ──────────────────────────────────────────────────────────────
    inline void PageAimbot()
    {
        Section("Aimbot");
        Toggle("Enabled", &Options::Aimbot::Aimbot);
        Key("Aim Key", &Options::Aimbot::AimbotKey);
        static const char* toggleTypes[] = { "Hold", "Toggle" };
        Drop("Toggle Type", &Options::Aimbot::ToggleType, toggleTypes, 2);
        static const char* methods[] = { "Camera", "Mouse", "Viewport" };
        Drop("Method", &Options::Aimbot::AimingType, methods, 3);
        if (Options::Aimbot::AimingType != 2)
            Toggle("FPS Viewport Fallback", &Options::Aimbot::ViewportFallbackFPS);

        Section("Target");
        static const char* parts[] = { "Head", "Torso", "Left Arm", "Right Arm", "Left Leg", "Right Leg" };
        Drop("Hit Part", &Options::Aimbot::TargetBone, parts, 6);
        Drop("Air Hit Part", &Options::Aimbot::AirTargetBone, parts, 6);
        Toggle("Team Check", &Options::Aimbot::TeamCheck);
        Toggle("Downed Check", &Options::Aimbot::DownedCheck);
        Toggle("Visible Only", &Options::Aimbot::VisibleOnly);
        Toggle("Sticky Aim", &Options::Aimbot::StickyAim);
        Slider("Range", &Options::Aimbot::Range, 10.f, 2000.f, "%.0f");

        Section("Field of View");
        Toggle("Show FOV", &Options::Aimbot::ShowFOV);
        Toggle("FOV Fill", &Options::Aimbot::ShowFOVFill);
        Slider("FOV Size", &Options::Aimbot::FOV, 10.f, 800.f, "%.0f");
        Slider("FOV Thickness", &Options::Aimbot::FOVThickness, 1.f, 10.f);
        Color("FOV Color", Options::Aimbot::FOVColor);

        Section("Smoothing");
        Slider("Smoothness", &Options::Aimbot::Smoothness, 0.f, 50.f);
        static const char* curves[] = { "Linear", "Ease In", "Ease Out", "Ease In-Out", "Custom" };
        Drop("Curve", &Options::Aimbot::SmoothnessCurve, curves, 5);
        if (Options::Aimbot::SmoothnessCurve == 4)
        {
            Toggle("Custom Curve", &Options::Aimbot::CustomCurveEnabled);
            Slider("Curve P1 X", &Options::Aimbot::CustomCurveP1[0], 0.f, 1.f);
            Slider("Curve P1 Y", &Options::Aimbot::CustomCurveP1[1], 0.f, 1.f);
            Slider("Curve P2 X", &Options::Aimbot::CustomCurveP2[0], 0.f, 1.f);
            Slider("Curve P2 Y", &Options::Aimbot::CustomCurveP2[1], 0.f, 1.f);
        }
        Slider("Mouse Sensitivity", &Options::Aimbot::MouseSensitivity, 0.f, 10.f);

        Section("Humanizer");
        Toggle("Prediction", &Options::Aimbot::Prediction);
        Slider("Prediction X", &Options::Aimbot::PredictionX, 0.f, 5.f);
        Slider("Prediction Y", &Options::Aimbot::PredictionY, 0.f, 5.f);
        Toggle("Shake", &Options::Aimbot::Shake);
        Slider("Shake Intensity", &Options::Aimbot::ShakeIntensity, 0.f, 10.f);
        Toggle("Stutter", &Options::Aimbot::Stutter);
        SliderI("Stutter Ticks", &Options::Aimbot::StutterTicks, 1, 30);
        Toggle("Part Randomizer", &Options::Aimbot::PartRandomizer);
        Slider("Head Chance %", &Options::Aimbot::HeadChance, 0.f, 100.f, "%.0f");
        Toggle("Nearest Aim", &Options::Aimbot::NearestAim);
        Toggle("Nearest: Head", &Options::Aimbot::NearestHead);
        Toggle("Nearest: Chest", &Options::Aimbot::NearestChest);
        Toggle("Nearest: Legs", &Options::Aimbot::NearestLegs);

        Section("Wall Check");
        Toggle("Wall Check (Visibility)", &Options::WallCheck::Enabled);
        if (Options::WallCheck::Enabled)
            SliderI("Refresh (ms)", &Options::WallCheck::RefreshMs, 100, 3000);
    }

    inline void PageTriggerbot()
    {
        Section("Triggerbot");
        Toggle("Enabled", &Options::Triggerbot::Enabled);
        Key("Trigger Key", &Options::Triggerbot::TriggerbotKey);
        static const char* toggleTypes[] = { "Hold", "Toggle" };
        Drop("Toggle Type", &Options::Triggerbot::ToggleType, toggleTypes, 2);
        Toggle("Team Check", &Options::Triggerbot::TeamCheck);
        Toggle("Downed Check", &Options::Triggerbot::DownedCheck);
        Toggle("Visible Only", &Options::Triggerbot::VisibleOnly);
        Slider("Radius", &Options::Triggerbot::Radius, 1.f, 200.f, "%.0f");
        Slider("Range", &Options::Triggerbot::Range, 10.f, 2000.f, "%.0f");
        SliderI("Delay (ms)", &Options::Triggerbot::Delay, 0, 1000);

        Section("Per-Part FOV");
        Toggle("Advanced FOV", &Options::Triggerbot::AdvancedFOV);
        Toggle("Show Advanced FOV", &Options::Triggerbot::ShowAdvancedFOV);
        if (Options::Triggerbot::AdvancedFOV)
        {
            struct FovRow { const char* name; float* x; float* y; };
            const FovRow rows[] = {
                { "Head", &Options::Triggerbot::HeadFOV_X, &Options::Triggerbot::HeadFOV_Y },
                { "Torso", &Options::Triggerbot::TorsoFOV_X, &Options::Triggerbot::TorsoFOV_Y },
                { "Upper Torso", &Options::Triggerbot::UpperTorsoFOV_X, &Options::Triggerbot::UpperTorsoFOV_Y },
                { "Lower Torso", &Options::Triggerbot::LowerTorsoFOV_X, &Options::Triggerbot::LowerTorsoFOV_Y },
                { "Left Upper Arm", &Options::Triggerbot::LeftUpperArmFOV_X, &Options::Triggerbot::LeftUpperArmFOV_Y },
                { "Left Lower Arm", &Options::Triggerbot::LeftLowerArmFOV_X, &Options::Triggerbot::LeftLowerArmFOV_Y },
                { "Left Hand", &Options::Triggerbot::LeftHandFOV_X, &Options::Triggerbot::LeftHandFOV_Y },
                { "Right Upper Arm", &Options::Triggerbot::RightUpperArmFOV_X, &Options::Triggerbot::RightUpperArmFOV_Y },
                { "Right Lower Arm", &Options::Triggerbot::RightLowerArmFOV_X, &Options::Triggerbot::RightLowerArmFOV_Y },
                { "Right Hand", &Options::Triggerbot::RightHandFOV_X, &Options::Triggerbot::RightHandFOV_Y },
                { "Left Upper Leg", &Options::Triggerbot::LeftUpperLegFOV_X, &Options::Triggerbot::LeftUpperLegFOV_Y },
                { "Left Lower Leg", &Options::Triggerbot::LeftLowerLegFOV_X, &Options::Triggerbot::LeftLowerLegFOV_Y },
                { "Left Foot", &Options::Triggerbot::LeftFootFOV_X, &Options::Triggerbot::LeftFootFOV_Y },
                { "Right Upper Leg", &Options::Triggerbot::RightUpperLegFOV_X, &Options::Triggerbot::RightUpperLegFOV_Y },
                { "Right Lower Leg", &Options::Triggerbot::RightLowerLegFOV_X, &Options::Triggerbot::RightLowerLegFOV_Y },
                { "Right Foot", &Options::Triggerbot::RightFootFOV_X, &Options::Triggerbot::RightFootFOV_Y },
            };
            for (auto& r : rows)
            {
                char bx[64], by[64];
                snprintf(bx, sizeof(bx), "%s X", r.name);
                snprintf(by, sizeof(by), "%s Y", r.name);
                Slider(bx, r.x, 0.f, 200.f, "%.0f");
                Slider(by, r.y, 0.f, 200.f, "%.0f");
            }
        }
    }

    inline void PageHitbox()
    {
        Section("Hitbox Expander");
        Toggle("Enabled", &Options::HitboxExpander::Enabled);
        Slider("Horizontal Size", &Options::HitboxExpander::HorizontalSize, 1.f, 30.f, "%.1f");
        Slider("Vertical Size", &Options::HitboxExpander::VerticalSize, 1.f, 30.f, "%.1f");
        Toggle("Show Hitbox", &Options::HitboxExpander::ShowHitbox);
        Slider("Hitbox Transparency", &Options::HitboxExpander::HitboxTransparency, 0.f, 1.f);
        Toggle("Walk Through", &Options::HitboxExpander::WalkThrough);
    }

    inline void PageSilent()
    {
        Section("Silent Aim");
        Toggle("Enabled", &Options::SilentAim::Enabled);
        Key("Silent Key", &Options::SilentAim::Key);
        static const char* toggleTypes[] = { "Hold", "Toggle" };
        Drop("Toggle Type", &Options::SilentAim::ToggleType, toggleTypes, 2);
        static const char* methods[] = { "Mouse Hit", "Unit Ray", "Viewport" };
        Drop("Method", &Options::SilentAim::Method, methods, 3);
        Toggle("Team Check", &Options::SilentAim::TeamCheck);
        Toggle("Downed Check", &Options::SilentAim::DownedCheck);
        Toggle("Visible Only", &Options::SilentAim::VisibleOnly);
        static const char* parts[] = { "Head", "Torso", "Left Arm", "Right Arm", "Left Leg", "Right Leg" };
        Drop("Target Bone", &Options::SilentAim::TargetBone, parts, 6);
        static const char* prio[] = { "Closest to Crosshair", "Lowest Health" };
        Drop("Target Priority", &Options::SilentAim::TargetPriority, prio, 2);
        Slider("FOV", &Options::SilentAim::FOV, 10.f, 800.f, "%.0f");
        Toggle("Show FOV", &Options::SilentAim::ShowFOV);
        Slider("Range", &Options::SilentAim::Range, 10.f, 3000.f, "%.0f");

        Section("Humanizer");
        Toggle("Prediction", &Options::SilentAim::Prediction);
        Slider("Prediction X", &Options::SilentAim::PredictionX, 0.f, 5.f);
        Slider("Prediction Y", &Options::SilentAim::PredictionY, 0.f, 5.f);
        Toggle("Part Randomizer", &Options::SilentAim::PartRandomizer);
        Slider("Head Chance %", &Options::SilentAim::HeadChance, 0.f, 100.f, "%.0f");

        Section("Hitbox On Fire");
        Toggle("Hitbox On Fire", &Options::SilentAim::HitboxOnFire);
        Slider("Hitbox Multiplier", &Options::SilentAim::HitboxMult, 1.f, 20.f, "%.1f");
        SliderI("Hitbox Frames", &Options::SilentAim::HitboxFrames, 1, 10);
    }

    inline void PageESP()
    {
        Section("ESP");
        Toggle("Team Check", &Options::ESP::TeamCheck);
        static const char* boxes[] = { "None", "Normal Box", "3D Box" };
        Drop("Box Style", &Options::ESP::BoxType, boxes, 3);
        Slider("Box Thickness", &Options::ESP::BoxThickness, 1.f, 10.f);
        Toggle("Corner ESP", &Options::ESP::CornerESP);
        Toggle("Tracers", &Options::ESP::Tracers);
        static const char* starts[] = { "Bottom", "Top", "Mouse" };
        Drop("Tracer Origin", &Options::ESP::TracersStart, starts, 3);
        Slider("Tracer Thickness", &Options::ESP::TracerThickness, 1.f, 10.f);
        Toggle("Skeleton", &Options::ESP::Skeleton);
        Slider("Skeleton Thickness", &Options::ESP::SkeletonThickness, 1.f, 10.f);
        Toggle("Name", &Options::ESP::Name);
        Toggle("Distance", &Options::ESP::Distance);
        Toggle("Health", &Options::ESP::Health);
        Toggle("Head Circle", &Options::ESP::HeadCircle);
        Slider("Head Circle Thickness", &Options::ESP::HeadCircleThickness, 1.f, 10.f);
        Slider("Head Circle Max Scale", &Options::ESP::HeadCircleMaxScale, 1.f, 10.f);
        Toggle("Headless", &Options::ESP::Headless);
        Toggle("Weapon", &Options::ESP::Weapon);
        Toggle("Facing Arrow", &Options::ESP::FacingArrow);
        Toggle("Offscreen Arrows", &Options::ESP::OffscreenArrows);

        Section("Outlines");
        Toggle("Outlines Enabled", &Options::ESP::OutlineEnabled);
        Toggle("Box Outline", &Options::ESP::BoxOutline);
        Toggle("Corner Outline", &Options::ESP::CornerOutline);
        Toggle("Tracer Outline", &Options::ESP::TracerOutline);
        Toggle("Skeleton Outline", &Options::ESP::SkeletonOutline);
        Toggle("Head Circle Outline", &Options::ESP::HeadCircleOutline);
        Toggle("Name Outline", &Options::ESP::NameOutline);
        Toggle("Distance Outline", &Options::ESP::DistanceOutline);
        Toggle("Remove Borders", &Options::ESP::RemoveBorders);
        Slider("3D ESP Thickness", &Options::ESP::ESP3DThickness, 1.f, 10.f);
    }

    inline void PageColors()
    {
        Section("Colors");
        Color("Name", Options::ESP::Color);
        Color("Box", Options::ESP::BoxColor);
        Color("Corner", Options::ESP::CornerColor);
        Color("Skeleton", Options::ESP::SkeletonColor);
        Color("Distance", Options::ESP::DistanceColor);
        Color("Tracer", Options::ESP::TracerColor);
        Color("3D ESP", Options::ESP::ESP3DColor);
        Color("Head Circle", Options::ESP::HeadCircleColor);
        Color("Chams", Options::ESP::ChamsColor);
        Color("Weapon", Options::ESP::WeaponColor);
        Color("Facing Arrow", Options::ESP::FacingArrowColor);
        Color("Offscreen Arrow", Options::ESP::OffscreenArrowColor);
        Color("Menu Accent", Options::Misc::MenuAccentColor);
    }

    inline void PagePartChams()
    {
        Section("Part Chams");
        Toggle("Enabled", &Options::PartChams::Enabled);
        Toggle("Only When Menu Closed", &Options::PartChams::OnlyWhenMenuClosed);
        Toggle("Team Check", &Options::PartChams::TeamCheck);
        Toggle("Filled", &Options::PartChams::Filled);
        Slider("Alpha", &Options::PartChams::Alpha, 0.f, 1.f);
        Slider("Thickness", &Options::PartChams::Thickness, 1.f, 10.f);
        static const char* sel[] = { "Head", "Torso", "Arms", "Legs", "All" };
        Drop("Parts", &Options::PartChams::PartSelect, sel, 5);
        Color("Color", Options::PartChams::Color);
    }

    inline void PageHitboxChams()
    {
        Section("Hitbox Chams");
        Toggle("Enabled", &Options::HitboxChams::Enabled);
        Toggle("Only When Menu Closed", &Options::HitboxChams::OnlyWhenMenuClosed);
        Toggle("Team Check", &Options::HitboxChams::TeamCheck);
        Toggle("Filled", &Options::HitboxChams::Filled);
        Toggle("Highlight Target", &Options::HitboxChams::HighlightTarget);
        Slider("Alpha", &Options::HitboxChams::Alpha, 0.f, 1.f);
        Slider("Thickness", &Options::HitboxChams::Thickness, 1.f, 10.f);
        static const char* bones[] = { "Head", "Torso", "Left Arm", "Right Arm", "Left Leg", "Right Leg", "Lower Torso", "Upper Torso" };
        Drop("Part", &Options::HitboxChams::PartSelect, bones, 8);
        Color("Color", Options::HitboxChams::Color);
    }

    inline void PageFly()
    {
        Section("Fly");
        Toggle("Enabled", &Options::Fly::Enabled);
        Key("Fly Key", &Options::Fly::FlyKey);
        static const char* toggleTypes[] = { "Hold", "Toggle" };
        Drop("Toggle Type", &Options::Fly::ToggleType, toggleTypes, 2);
        Slider("Fly Speed", &Options::Fly::Speed, 10.f, 1000.f, "%.0f");
    }

    inline void PageSpeed()
    {
        Section("WalkSpeed");
        Toggle("Enabled", &Options::WalkSpeed::Enabled);
        Key("Speed Key", &Options::WalkSpeed::WalkSpeedKey);
        static const char* toggleTypes[] = { "Hold", "Toggle" };
        Drop("Toggle Type", &Options::WalkSpeed::ToggleType, toggleTypes, 2);
        Slider("Speed", &Options::WalkSpeed::Speed, 8.f, 500.f, "%.0f");
    }

    inline void PageFling()
    {
        Section("Fling");
        Toggle("Enabled", &Options::Fling::Enabled);
        Key("Fling Key", &Options::Fling::FlingKey);
        static const char* toggleTypes[] = { "Hold", "Toggle" };
        Drop("Toggle Type", &Options::Fling::ToggleType, toggleTypes, 2);
        Slider("Fling Speed", &Options::Fling::Speed, 100.f, 20000.f, "%.0f");
        Toggle("Target Fling", &Options::Fling::TargetFling);
        if (Options::Fling::TargetFling)
            PlayerCombo("##flingTarget", "Target", &Options::Fling::TargetPlayerIndex);

        Section("Anti-Fling");
        Toggle("Enabled", &Options::AntiFling::Enabled);
        static const char* modes[] = { "Disable Collision", "Stop Velocity" };
        Drop("Mode", &Options::AntiFling::Mode, modes, 2);
    }

    inline void PageTeleport()
    {
        Section("Teleport");
        Key("Teleport Key", &Options::Teleport::TPKey);
        static const char* toggleTypes[] = { "Hold", "Toggle" };
        Drop("Toggle Type", &Options::Teleport::ToggleType, toggleTypes, 2);
        Toggle("Ctrl+Click Teleport", &Options::Teleport::CtrlClickTP);
        Toggle("Teleport To Players", &Options::Teleport::TPToPlayers);
        if (Options::Teleport::TPToPlayers)
            PlayerCombo("##tpTarget", "Player", &Options::Teleport::SelectedPlayer);
    }

    inline void PagePlayer()
    {
        Section("Character");
        Toggle("Infinite Jump", &Options::InfiniteJump::Enabled);
        Toggle("Auto Jump", &Options::AutoJump::Enabled);
        Toggle("Noclip", &Options::Noclip::Enabled);
        Key("Noclip Key", &Options::Noclip::NoclipKey);
        Toggle("Platform Stand", &Options::PlatformStand::Enabled);
        Toggle("Auto Rotate", &Options::AutoRotate::Enabled);

        Section("Physics");
        Toggle("Gravity Mod", &Options::GravityMod::Enabled);
        Slider("Gravity", &Options::GravityMod::Value, 0.f, 500.f, "%.0f");
        Toggle("Jump Power Mod", &Options::JumpPowerMod::Enabled);
        Slider("Jump Power", &Options::JumpPowerMod::Value, 0.f, 500.f, "%.0f");

        Section("Nameplates");
        Toggle("Name Occlusion", &Options::NameOcclusion::Enabled);
        Toggle("Hide Nameplates", &Options::NameOcclusion::HideNameplates);
        Slider("Name Display Distance", &Options::NameOcclusion::NameDisplayDistance, 0.f, 1000.f, "%.0f");
        Slider("Health Display Distance", &Options::NameOcclusion::HealthDisplayDistance, 0.f, 1000.f, "%.0f");
    }

    inline void PageSettings()
    {
        Section("Camera");
        Toggle("FOV Changer", &Options::Misc::FOVEnabled);
        Slider("Camera FOV", &Options::Misc::FOV, 10.f, 120.f, "%.0f");

        Section("Overlay");
        Toggle("Keybind List", &Options::Misc::KeybindList);
        Slider("Keybind X", &Options::Misc::KeybindListX, 0.f, 3000.f, "%.0f");
        Slider("Keybind Y", &Options::Misc::KeybindListY, 0.f, 2000.f, "%.0f");
        Toggle("FPS Counter", &Options::Misc::FPSCounterEnabled);
        Slider("FPS X", &Options::Misc::FPSCounterX, 0.f, 3000.f, "%.0f");
        Slider("FPS Y", &Options::Misc::FPSCounterY, 0.f, 2000.f, "%.0f");
        Toggle("Performance Metrics", &Options::Misc::ShowPerformanceMetrics);
        Toggle("Dim Background", &Options::Misc::DimBackground);
        Toggle("Stream Proof", &Options::Misc::StreamProof);
        Toggle("Show Console", &Options::Misc::ShowConsole);
        Toggle("Cache NPCs", &Options::Misc::CacheNPCs);
        Color("Menu Accent", Options::Misc::MenuAccentColor);

        Section("Notifications");
        Toggle("Notifications", &Options::Misc::NotificationsEnabled);
        SliderI("Max Queue", &Options::Misc::NotificationMaxQueue, 1, 20);
        SliderI("Duration (ms)", &Options::Misc::NotificationDuration, 500, 10000);

        Section("Crosshair");
        Toggle("Crosshair", &Options::Crosshair::Enabled);
        static const char* styles[] = { "Static", "Pulse" };
        Drop("Style", &Options::Crosshair::Style, styles, 2);
        Slider("Size", &Options::Crosshair::Size, 1.f, 50.f, "%.0f");
        Slider("Gap", &Options::Crosshair::Gap, 0.f, 30.f, "%.0f");
        Slider("Thickness", &Options::Crosshair::Thickness, 1.f, 10.f);
        Slider("Spin Speed", &Options::Crosshair::SpinSpeed, 0.f, 500.f, "%.0f");

        Section("Macro");
        Toggle("Macro", &Options::Macro::Enabled);
        Key("Macro Key", &Options::Macro::MacroKey);
        static const char* toggleTypes[] = { "Hold", "Toggle" };
        Drop("Toggle Type", &Options::Macro::ToggleType, toggleTypes, 2);
        SliderI("Delay (ms)", &Options::Macro::Delay, 1, 2000);
    }

    inline void PageConfig()
    {
        Section("Configs");
        static char name[64] = "default";
        static std::vector<std::string> list;
        static int selected = -1;
        static float lastScan = 0;
        if (ImGui::GetTime() - lastScan > 2.0f)
        {
            list.clear();
            try {
                for (const auto& e : std::filesystem::directory_iterator(Globals::configsPath))
                    if (e.path().extension() == ".json")
                        list.push_back(e.path().filename().string());
            } catch (...) {}
            lastScan = (float)ImGui::GetTime();
        }
        ImGui::InputText("##cfgname", name, sizeof(name));
        if (ImGui::Button("Save Config", ImVec2(-1, 28)))
        {
            std::string f(name);
            if (!f.empty())
            {
                if (f.find(".json") == std::string::npos) f += ".json";
                CreateConfig(f);
            }
        }
        for (int i = 0; i < (int)list.size(); i++)
            if (ImGui::Selectable(list[i].c_str(), selected == i))
                selected = i;
        if (selected >= 0 && selected < (int)list.size())
        {
            if (ImGui::Button("Load Selected", ImVec2(-1, 28)))
                LoadConfig(list[selected]);
            if (ImGui::Button("Overwrite Selected", ImVec2(-1, 24)))
                CreateConfig(list[selected]);
        }

        Section("Discord Webhook");
        Toggle("Webhook Enabled", &Options::Misc::WebhookEnabled);
        static char url[512] = "";
        if (!url[0] && !Options::Misc::WebhookURL.empty())
            strcpy_s(url, Options::Misc::WebhookURL.c_str());
        ImGui::InputText("##webhookurl", url, sizeof(url));
        if (ImGui::Button("Set Webhook", ImVec2(-1, 24)))
            Options::Misc::WebhookURL = url;
        if (ImGui::Button("Test Webhook", ImVec2(-1, 24)))
            SendWebhookAsync("Webhook test");
    }

    // ── shell ──────────────────────────────────────────────────────────────
    inline void Render(HWND hwnd, ImGuiIO& io, ImFont* font, bool menuOpen)
    {
        // per-frame housekeeping (moved out of renderer.cpp)
        static bool lastStreamProof = Options::Misc::StreamProof;
        if (lastStreamProof != Options::Misc::StreamProof)
        {
            SetWindowDisplayAffinity(hwnd, Options::Misc::StreamProof ? 0x00000011 : 0x00000000);
            lastStreamProof = Options::Misc::StreamProof;
        }
        static bool lastConsole = Options::Misc::ShowConsole;
        if (lastConsole != Options::Misc::ShowConsole)
        {
            ShowWindow(GetConsoleWindow(), Options::Misc::ShowConsole ? SW_SHOW : SW_HIDE);
            lastConsole = Options::Misc::ShowConsole;
        }
        main_color = Accent();

        float speed = 0.08f;
        if (menuOpen)
        {
            if (MenuAlpha < 1.f) MenuAlpha += speed; if (MenuAlpha > 1.f) MenuAlpha = 1.f;
            if (BgAlpha < 0.7f) BgAlpha += speed; if (BgAlpha > 0.7f) BgAlpha = 0.7f;
        }
        else
        {
            if (MenuAlpha > 0.f) MenuAlpha -= speed; if (MenuAlpha < 0.f) MenuAlpha = 0.f;
            if (BgAlpha > 0.f) BgAlpha -= speed; if (BgAlpha < 0.f) BgAlpha = 0.f;
        }
        if (!menuOpen && MenuAlpha <= 0.f) return;

        if (Options::Misc::DimBackground && BgAlpha > 0.f)
            ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0),
                ImVec2(io.DisplaySize.x, io.DisplaySize.y), IM_COL32(0, 0, 0, (int)(BgAlpha * 180)));

        const ImVec2 size(700, 470);
        ImGui::SetNextWindowSize(size);
        ImGui::SetNextWindowBgAlpha(MenuAlpha);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, MenuAlpha);
        ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - size.x) * .5f, (io.DisplaySize.y - size.y) * .5f), ImGuiCond_Once);
        ImGui::Begin("##armedium", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
        {
            ImGui::PushFont(font);
            ImVec2 pos = ImGui::GetWindowPos();
            ImVec2 win = ImGui::GetWindowSize();
            ImDrawList* d = ImGui::GetWindowDrawList();
            ImVec4 ac = Accent();
            ImU32 accent = IM_COL32((int)(ac.x * 255), (int)(ac.y * 255), (int)(ac.z * 255), 255);

            // frame
            d->AddRectFilled(pos, ImVec2(pos.x + win.x, pos.y + win.y), IM_COL32(13, 13, 16, 250), 10.f);
            d->AddRect(ImVec2(pos.x + 1, pos.y + 1), ImVec2(pos.x + win.x - 1, pos.y + win.y - 1), IM_COL32(34, 34, 42, 255), 10.f);
            d->AddLine(ImVec2(pos.x + 14, pos.y + 1), ImVec2(pos.x + win.x - 14, pos.y + 1), accent, 2.f);

            const float pad = 12.f, sideW = 148.f;
            const float topH = 44.f;

            // header
            ImGui::SetCursorPos(ImVec2(pad + 4, 10));
            ImGui::TextUnformatted("armedium");
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(.55f, .55f, .62f, 1.f));
            ImGui::TextUnformatted("v2");
            ImGui::PopStyleColor();
            ImGui::SameLine(win.x - pad - 214);
            ImGui::PushItemWidth(210);
            ImGui::InputTextWithHint("##search", "Search settings...", Filter, sizeof(Filter));
            ImGui::PopItemWidth();
            ImGui::SameLine();
            char fps[32];
            snprintf(fps, sizeof(fps), "%d FPS", (int)io.Framerate);
            ImGui::TextUnformatted(fps);

            d->AddLine(ImVec2(pos.x + pad, pos.y + topH), ImVec2(pos.x + win.x - pad, pos.y + topH), IM_COL32(30, 30, 38, 255));

            // sidebar
            const char* tabs[] = { "Aim", "Visuals", "Movement", "Misc" };
            ImGui::SetCursorPos(ImVec2(pad, topH + 10));
            ImGui::BeginChild("##nav", ImVec2(sideW, win.y - topH - 24), false, ImGuiWindowFlags_NoBackground);
            {
                for (int i = 0; i < 4; i++)
                {
                    bool active = Tab == i;
                    if (active)
                    {
                        ImVec2 mp = ImGui::GetCursorScreenPos();
                        d->AddRectFilled(mp, ImVec2(mp.x + sideW - 8, mp.y + 34), IM_COL32(28, 28, 36, 255), 6.f);
                        d->AddLine(ImVec2(mp.x, mp.y + 6), ImVec2(mp.x, mp.y + 28), accent, 3.f);
                    }
                    ImGui::PushStyleColor(ImGuiCol_Text, active ? ImVec4(1, 1, 1, 1) : ImVec4(.55f, .55f, .62f, 1.f));
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
                    if (ImGui::Button(tabs[i], ImVec2(sideW - 8, 34))) { Tab = i; Filter[0] = '\0'; }
                    ImGui::PopStyleColor(4);
                }
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(.4f, .4f, .46f, 1.f));
                ImGui::TextWrapped("Insert toggles");
                ImGui::PopStyleColor();
            }
            ImGui::EndChild();

            // content
            ImGui::SameLine();
            ImGui::BeginChild("##body", ImVec2(win.x - sideW - pad * 2 - 6, win.y - topH - 24), false, ImGuiWindowFlags_NoBackground);
            {
                const char* subs[4][4] = {
                    { "Aimbot", "Triggerbot", "Hitbox", "Silent" },
                    { "ESP", "Colors", "Part Chams", "Hitbox Chams" },
                    { nullptr, nullptr, nullptr, nullptr }, // Movement handled below (5 pills)
                    { "Settings", "Config", nullptr, nullptr },
                };
                int subCount[4] = { 4, 4, 0, 2 };
                if (Tab == 2)
                {
                    static const char* msubs[] = { "Fly", "Speed", "Fling", "Teleport", "Player" };
                    for (int i = 0; i < 5; i++)
                    {
                        bool a = Sub[2] == i;
                        if (a) { ImGui::PushStyleColor(ImGuiCol_Button, Accent()); ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1)); }
                        if (ImGui::Button(msubs[i], ImVec2(86, 24))) Sub[2] = i;
                        if (a) ImGui::PopStyleColor(2);
                        ImGui::SameLine();
                    }
                    ImGui::NewLine();
                }
                else
                {
                    for (int i = 0; i < subCount[Tab]; i++)
                    {
                        bool a = Sub[Tab] == i;
                        if (a) { ImGui::PushStyleColor(ImGuiCol_Button, Accent()); ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1)); }
                        if (ImGui::Button(subs[Tab][i], ImVec2(86, 24))) Sub[Tab] = i;
                        if (a) ImGui::PopStyleColor(2);
                        ImGui::SameLine();
                    }
                    ImGui::NewLine();
                }
                ImGui::Separator();
                ImGui::BeginChild("##page", ImVec2(0, 0), false);
                {
                    if (Tab == 0)
                    {
                        if (Sub[0] == 0) PageAimbot();
                        else if (Sub[0] == 1) PageTriggerbot();
                        else if (Sub[0] == 2) PageHitbox();
                        else PageSilent();
                    }
                    else if (Tab == 1)
                    {
                        if (Sub[1] == 0) PageESP();
                        else if (Sub[1] == 1) PageColors();
                        else if (Sub[1] == 2) PagePartChams();
                        else PageHitboxChams();
                    }
                    else if (Tab == 2)
                    {
                        if (Sub[2] == 0) PageFly();
                        else if (Sub[2] == 1) PageSpeed();
                        else if (Sub[2] == 2) PageFling();
                        else if (Sub[2] == 3) PageTeleport();
                        else PagePlayer();
                    }
                    else
                    {
                        if (Sub[3] == 0) PageSettings();
                        else PageConfig();
                    }
                }
                ImGui::EndChild();
            }
            ImGui::EndChild();

            ImGui::PopFont();
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
}
