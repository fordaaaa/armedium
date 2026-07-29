#pragma once

#include <cstdint>
#include <string>
namespace Offsets {
    inline std::string ClientVersion = "version-5cf2272675e145f5";

    namespace AnimationTrack {
        inline constexpr uintptr_t Animation = 0xc0;
        inline constexpr uintptr_t Animator = 0x118;
        inline constexpr uintptr_t IsPlaying = 0x538;
        inline constexpr uintptr_t Looped = 0xf5;
        inline constexpr uintptr_t Speed = 0xe4;
    }

    namespace BasePart {
        inline constexpr uintptr_t AssemblyAngularVelocity = 0x104;
        inline constexpr uintptr_t AssemblyLinearVelocity = 0xf8;
        inline constexpr uintptr_t Color3 = 0x148;
        inline constexpr uintptr_t Material = 0x0;
        inline constexpr uintptr_t Position = 0xec;
        inline constexpr uintptr_t Primitive = 0x128;
        inline constexpr uintptr_t PrimitiveFlags = 0x1b6;
        inline constexpr uintptr_t PrimitiveOwner = 0x208;
        inline constexpr uintptr_t Rotation = 0xc8;
        inline constexpr uintptr_t Shape = 0x159;
        inline constexpr uintptr_t CanCollide = 0x8;
        inline constexpr uintptr_t CanQuery = 0x20;
        inline constexpr uintptr_t CanTouch = 0x10;
        inline constexpr uintptr_t Size = 0x1b8;
        inline constexpr uintptr_t Transparency = 0xd0;
        inline constexpr uintptr_t ValidatePrimitive = 0x6;
    }

    namespace ByteCode {
        inline constexpr uintptr_t Pointer = 0x10;
        inline constexpr uintptr_t Size = 0x20;
    }

    namespace Camera {
        inline constexpr uintptr_t CameraSubject = 0xc8;
        inline constexpr uintptr_t CameraType = 0x138;
        inline constexpr uintptr_t FieldOfView = 0x140;
        inline constexpr uintptr_t Position = 0xfc;
        inline constexpr uintptr_t Rotation = 0xd8;
        inline constexpr uintptr_t Viewport = 0x28c;
    }

    namespace ClickDetector {
        inline constexpr uintptr_t MaxActivationDistance = 0xe8;
        inline constexpr uintptr_t MouseIcon = 0xc8;
    }

    namespace DataModel {
        inline constexpr uintptr_t CreatorId = 0x180;
        inline constexpr uintptr_t GameId = 0x188;
        inline constexpr uintptr_t GameLoaded = 0x658;
        inline constexpr uintptr_t JobId = 0x120;
        inline constexpr uintptr_t PlaceId = 0x190;
        inline constexpr uintptr_t PlaceVersion = 0x1ac;
        inline constexpr uintptr_t PrimitiveCount = 0x488;
        inline constexpr uintptr_t ScriptContext = 0x440;
        inline constexpr uintptr_t ServerIP = 0x640;
        inline constexpr uintptr_t Workspace = 0x160;
    }

    namespace FFlags {
        inline constexpr uintptr_t DebugDisableTimeoutDisconnect = 0x682cf18;
        inline constexpr uintptr_t EnableLoadModule = 0x68227d8;
        inline constexpr uintptr_t PartyPlayerInactivityTimeoutInSeconds = 0x67eaa4c;
        inline constexpr uintptr_t TaskSchedulerTargetFps = 0x7542f70;
        inline constexpr uintptr_t WebSocketServiceEnableClientCreation = 0x6839c00;
    }

    namespace FakeDataModel {
        inline constexpr uintptr_t Pointer = 0x7c3d2e8;
        inline constexpr uintptr_t RealDataModel = 0x1d0;
    }

    namespace GuiObject {
        inline constexpr uintptr_t BackgroundColor3 = 0x540;
        inline constexpr uintptr_t BorderColor3 = 0x54c;
        inline constexpr uintptr_t Image = 0x988;
        inline constexpr uintptr_t LayoutOrder = 0x580;
        inline constexpr uintptr_t Position = 0x510;
        inline constexpr uintptr_t RichText = 0xb50;
        inline constexpr uintptr_t Rotation = 0x178;
        inline constexpr uintptr_t ScreenGui_Enabled = 0x4c4;
        inline constexpr uintptr_t Size = 0x530;
        inline constexpr uintptr_t Text = 0xda0;
        inline constexpr uintptr_t TextColor3 = 0xe50;
        inline constexpr uintptr_t Visible = 0x5ad;
    }

    namespace Humanoid {
        inline constexpr uintptr_t AutoRotate = 0x1d9;
        inline constexpr uintptr_t FloorMaterial = 0x184;
        inline constexpr uintptr_t Health = 0x188;
        inline constexpr uintptr_t HipHeight = 0x194;
        inline constexpr uintptr_t HumanoidState = 0x898;
        inline constexpr uintptr_t HumanoidStateID = 0x20;
        inline constexpr uintptr_t Jump = 0x1da;
        inline constexpr uintptr_t JumpHeight = 0x1a0;
        inline constexpr uintptr_t JumpPower = 0x1a4;
        inline constexpr uintptr_t MaxHealth = 0x1a8;
        inline constexpr uintptr_t MaxSlopeAngle = 0x1ac;
        inline constexpr uintptr_t MoveDirection = 0x140;
        inline constexpr uintptr_t RigType = 0x1c0;
        inline constexpr uintptr_t Walkspeed = 0x1d0;
        inline constexpr uintptr_t WalkspeedCheck = 0x3bc;
        inline constexpr uintptr_t AutoJumpEnabled = 0x1d5;
        inline constexpr uintptr_t PlatformStand = 0x1dc;
        inline constexpr uintptr_t Sit = 0x1dd;
        inline constexpr uintptr_t RequiresNeck = 0x1dd;
        inline constexpr uintptr_t UseJumpPower = 0x1e0;
        inline constexpr uintptr_t NameDisplayDistance = 0x1e4;
        inline constexpr uintptr_t HealthDisplayDistance = 0x1e8;
    }

    namespace Instance {
        inline constexpr uintptr_t AttributeContainer = 0x48;
        inline constexpr uintptr_t AttributeList = 0x18;
        inline constexpr uintptr_t AttributeToNext = 0x58;
        inline constexpr uintptr_t AttributeToValue = 0x18;
        inline constexpr uintptr_t ChildrenEnd = 0x8;
        inline constexpr uintptr_t ChildrenStart = 0x70;
        inline constexpr uintptr_t ClassBase = 0x230;
        inline constexpr uintptr_t ClassDescriptor = 0x18;
        inline constexpr uintptr_t ClassName = 0x8;
        inline constexpr uintptr_t Name = 0x98;
        inline constexpr uintptr_t Parent = 0x68;
    }

    namespace Lighting {
        inline constexpr uintptr_t Ambient = 0xc8;
        inline constexpr uintptr_t Brightness = 0x110;
        inline constexpr uintptr_t ClockTime = 0x1a8;
        inline constexpr uintptr_t ColorShift_Bottom = 0xe0;
        inline constexpr uintptr_t ColorShift_Top = 0xd4;
        inline constexpr uintptr_t ExposureCompensation = 0x11c;
        inline constexpr uintptr_t FogColor = 0xec;
        inline constexpr uintptr_t FogEnd = 0x124;
        inline constexpr uintptr_t FogStart = 0x128;
        inline constexpr uintptr_t GeographicLatitude = 0x180;
        inline constexpr uintptr_t OutdoorAmbient = 0xf8;
    }

    namespace LocalScript {
        inline constexpr uintptr_t ByteCode = 0x190;
    }

    namespace MeshPart {
        inline constexpr uintptr_t MeshId = 0x290;
        inline constexpr uintptr_t Texture = 0x2c0;
    }

    namespace Misc {
        inline constexpr uintptr_t Adornee = 0xf0;
        inline constexpr uintptr_t AnimationId = 0xc0;
        inline constexpr uintptr_t RequireLock = 0x0;
        inline constexpr uintptr_t StringLength = 0x10;
        inline constexpr uintptr_t Value = 0xb8;
    }

    namespace Model {
        inline constexpr uintptr_t PrimaryPart = 0x258;
        inline constexpr uintptr_t Scale = 0x144;
    }

    namespace ModuleScript {
        inline constexpr uintptr_t ByteCode = 0x138;
    }

    namespace MouseService {
        inline constexpr uintptr_t InputObject = 0x100;
        inline constexpr uintptr_t MousePosition = 0xd4;
        inline constexpr uintptr_t SensitivityPointer = 0x7dfd318;
    }

    namespace Player {
        inline constexpr uintptr_t CameraMode = 0x360;
        inline constexpr uintptr_t Country = 0x110;
        inline constexpr uintptr_t DisplayName = 0x138;
        inline constexpr uintptr_t Gender = 0xea0;
        inline constexpr uintptr_t LocalPlayer = 0x130;
        inline constexpr uintptr_t MaxZoomDistance = 0x358;
        inline constexpr uintptr_t MinZoomDistance = 0x35c;
        inline constexpr uintptr_t ModelInstance = 0x298;
        inline constexpr uintptr_t Mouse = 0x11c8;
        inline constexpr uintptr_t Team = 0x2c8;
        inline constexpr uintptr_t TeamColor = 0x39c;
        inline constexpr uintptr_t GroupId = 0x380;
        inline constexpr uintptr_t UserId = 0x2f0;
    }

    namespace PlayerConfigurer {
        inline constexpr uintptr_t OverrideDuration = 0x5894805;
        inline constexpr uintptr_t Pointer = 0x7fed958;
    }

    namespace PlayerMouse {
        inline constexpr uintptr_t Icon = 0xc8;
        inline constexpr uintptr_t Workspace = 0x150;
        inline constexpr uintptr_t Hit = 0x120;       // sCFrame (48 bytes)
        inline constexpr uintptr_t Target = 0x60;     // uintptr_t to BasePart under cursor
        inline constexpr uintptr_t UnitRay = 0xe0;    // origin(12) + direction(12)
    }

    namespace PrimitiveFlags {
        inline constexpr uintptr_t Anchored = 0x2;
        inline constexpr uintptr_t CanCollide = 0x8;
        inline constexpr uintptr_t CanTouch = 0x10;
    }

    namespace ProximityPrompt {
        inline constexpr uintptr_t ActionText = 0xb0;
        inline constexpr uintptr_t Enabled = 0x136;
        inline constexpr uintptr_t GamepadKeyCode = 0x11c;
        inline constexpr uintptr_t HoldDuration = 0x120;
        inline constexpr uintptr_t KeyCode = 0x124;
        inline constexpr uintptr_t MaxActivationDistance = 0x128;
        inline constexpr uintptr_t ObjectText = 0xd0;
        inline constexpr uintptr_t RequiresLineOfSight = 0x137;
    }

    namespace RenderView {
        inline constexpr uintptr_t DeviceD3D11 = 0x8;
        inline constexpr uintptr_t VisualEngine = 0x10;
    }

    namespace RunService {
        inline constexpr uintptr_t HeartbeatFPS = 0xbc;
        inline constexpr uintptr_t HeartbeatTask = 0x1b0;
    }

    namespace Sky {
        inline constexpr uintptr_t MoonAngularSize = 0x244;
        inline constexpr uintptr_t MoonTextureId = 0xc8;
        inline constexpr uintptr_t SkyboxBk = 0xf8;
        inline constexpr uintptr_t SkyboxDn = 0x128;
        inline constexpr uintptr_t SkyboxFt = 0x158;
        inline constexpr uintptr_t SkyboxLf = 0x188;
        inline constexpr uintptr_t SkyboxOrientation = 0x238;
        inline constexpr uintptr_t SkyboxRt = 0x1b8;
        inline constexpr uintptr_t SkyboxUp = 0x1e8;
        inline constexpr uintptr_t StarCount = 0x248;
        inline constexpr uintptr_t SunAngularSize = 0x23c;
        inline constexpr uintptr_t SunTextureId = 0x218;
    }

    namespace SpecialMesh {
        inline constexpr uintptr_t MeshId = 0xf8;
        inline constexpr uintptr_t Scale = 0xc4;
    }

    namespace StatsItem {
        inline constexpr uintptr_t Value = 0xc8;
    }

    namespace TaskScheduler {
        inline constexpr uintptr_t FakeDataModelToDataModel = 0x1d0;
        inline constexpr uintptr_t JobEnd = 0xd0;
        inline constexpr uintptr_t JobName = 0x18;
        inline constexpr uintptr_t JobStart = 0xc8;
        inline constexpr uintptr_t MaxFPS = 0xb0;
        inline constexpr uintptr_t Pointer = 0x81cc868;
        inline constexpr uintptr_t RenderJobToFakeDataModel = 0x38;
        inline constexpr uintptr_t RenderJobToRenderView = 0x1d0;
    }

    namespace Team {
        inline constexpr uintptr_t BrickColor = 0xd0;
        inline constexpr uintptr_t BrickColorName = 0x248;
    }

    namespace Textures {
        inline constexpr uintptr_t Decal_Texture = 0x180;
        inline constexpr uintptr_t Texture_Texture = 0x180;
    }

    namespace VisualEngine {
        inline constexpr uintptr_t Dimensions = 0xab0;
        inline constexpr uintptr_t Pointer = 0x835a548;
        inline constexpr uintptr_t ToDataModel1 = 0x700;
        inline constexpr uintptr_t ToDataModel2 = 0x1c8;
        inline constexpr uintptr_t ViewMatrix = 0x150;
    }

    namespace Workspace {
        inline constexpr uintptr_t CurrentCamera = 0x4a0;
        inline constexpr uintptr_t DistributedGameTime = 0x4c0;
        inline constexpr uintptr_t Gravity = 0x210;
        inline constexpr uintptr_t GravityContainer = 0x70;
        inline constexpr uintptr_t PrimitivesPointer1 = 0x3d8;
        inline constexpr uintptr_t PrimitivesPointer2 = 0x240;
        inline constexpr uintptr_t ReadOnlyGravity = 0x9b0;
    }

}
