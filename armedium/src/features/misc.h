#pragma once

#include "../rbx/globals/options.h"
#include "../rbx/globals/globals.h"

#include <thread>

inline void DisablePartCollision(const RobloxInstance& part)
{
	if (!part.address) return;
	uintptr_t primitive = Memory->read<uintptr_t>(part.address + Offsets::BasePart::Primitive);
	if (!primitive) return;
	uint8_t flags = Memory->read<uint8_t>(primitive + Offsets::Primitive::Flags);
	flags &= ~(Offsets::PrimitiveFlags::CanCollide | Offsets::PrimitiveFlags::CanQuery | Offsets::PrimitiveFlags::CanTouch);
	Memory->write<uint8_t>(primitive + Offsets::Primitive::Flags, flags);
}

inline void RestorePartCollision(const RobloxInstance& part)
{
	if (!part.address) return;
	uintptr_t primitive = Memory->read<uintptr_t>(part.address + Offsets::BasePart::Primitive);
	if (!primitive) return;
	uint8_t flags = Memory->read<uint8_t>(primitive + Offsets::Primitive::Flags);
	flags |= (Offsets::PrimitiveFlags::CanCollide | Offsets::PrimitiveFlags::CanQuery | Offsets::PrimitiveFlags::CanTouch);
	Memory->write<uint8_t>(primitive + Offsets::Primitive::Flags, flags);
}

inline void MiscLoop()
{
	static bool wasNoclipActive = false;
	static bool wasNoclipKeyPressed = false;

	static bool wasGravityEnabled = false;
	static float originalGravity = 196.0f;
	static float originalReadOnlyGravity = 196.0f;

	static bool wasJumpPowerEnabled = false;
	static float originalJumpPower = 50.0f;

	static bool wasNameOcclusionEnabled = false;
	static float originalNameDisplayDistance = 100.0f;
	static float originalHealthDisplayDistance = 100.0f;

	static bool wasHeadlessEnabled = false;
	static bool wasAutoJumpEnabled = false;
	static bool wasPlatformStandEnabled = false;

	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		try
		{
			auto localPlayer = Globals::Roblox::LocalPlayer;
			if (!localPlayer.address)
				continue;

			auto character = localPlayer.Character();
			if (!character.address)
				continue;

			auto humanoid = character.FindFirstChildWhichIsA("Humanoid");

			// Camera FOV
			if (Options::Misc::FOVEnabled && Globals::Roblox::Camera.address != 0)
			{
				Globals::Roblox::Camera.SetFOV(Options::Misc::FOV);
			}

			// Headless feature
			if (Options::ESP::Headless)
			{
				auto head = character.FindFirstChild("Head");
				if (head.address != 0)
					Memory->write<float>(head.address + Offsets::BasePart::Transparency, 1.0f);
				wasHeadlessEnabled = true;
			}
			else if (wasHeadlessEnabled)
			{
				auto head = character.FindFirstChild("Head");
				if (head.address != 0)
					Memory->write<float>(head.address + Offsets::BasePart::Transparency, 0.0f);
				wasHeadlessEnabled = false;
			}

			// Infinite Jump
			if (Options::InfiniteJump::Enabled && humanoid.address != 0)
			{
				if (GetAsyncKeyState(VK_SPACE) & 0x8000)
				{
					Memory->write<bool>(humanoid.address + Offsets::Humanoid::Jump, true);
				}
			}

			// AutoJump
			if (Options::AutoJump::Enabled && humanoid.address != 0)
			{
				Memory->write<bool>(humanoid.address + Offsets::Humanoid::AutoJumpEnabled, true);
				wasAutoJumpEnabled = true;
			}
			else if (wasAutoJumpEnabled && humanoid.address != 0)
			{
				Memory->write<bool>(humanoid.address + Offsets::Humanoid::AutoJumpEnabled, false);
				wasAutoJumpEnabled = false;
			}

			// PlatformStand
			if (Options::PlatformStand::Enabled && humanoid.address != 0)
			{
				Memory->write<bool>(humanoid.address + Offsets::Humanoid::PlatformStand, true);
				wasPlatformStandEnabled = true;
			}
			else if (wasPlatformStandEnabled && humanoid.address != 0)
			{
				Memory->write<bool>(humanoid.address + Offsets::Humanoid::PlatformStand, false);
				wasPlatformStandEnabled = false;
			}

			// Noclip
			if (Options::Noclip::Enabled)
			{
				if (Options::Noclip::NoclipKey != 0)
				{
					bool isKeyPressed = (GetAsyncKeyState(Options::Noclip::NoclipKey) & 0x8000) != 0;
					if (Options::Noclip::ToggleType == 1)
					{
						if (isKeyPressed && !wasNoclipKeyPressed)
							Options::Noclip::Toggled = !Options::Noclip::Toggled;
						wasNoclipKeyPressed = isKeyPressed;
					}
					else
					{
						Options::Noclip::Toggled = isKeyPressed;
					}
				}
				else
				{
					Options::Noclip::Toggled = true;
				}
			}

			bool noclipActive = Options::Noclip::Enabled && Options::Noclip::Toggled;
			if (!noclipActive && wasNoclipActive)
			{
				auto parts = character.GetChildren();
				for (auto& part : parts)
					RestorePartCollision(part);
			}
			wasNoclipActive = noclipActive;

			if (noclipActive)
			{
				auto parts = character.GetChildren();
				for (auto& part : parts)
					DisablePartCollision(part);
			}

			// Gravity Modifier
			if (Options::GravityMod::Enabled && Globals::Roblox::Workspace.address != 0)
			{
				if (!wasGravityEnabled)
				{
					originalGravity = Memory->read<float>(Globals::Roblox::Workspace.address + Offsets::Workspace::Gravity);
					originalReadOnlyGravity = Memory->read<float>(Globals::Roblox::Workspace.address + Offsets::Workspace::ReadOnlyGravity);
					wasGravityEnabled = true;
				}
				Memory->write<float>(Globals::Roblox::Workspace.address + Offsets::Workspace::Gravity, Options::GravityMod::Value);
				Memory->write<float>(Globals::Roblox::Workspace.address + Offsets::Workspace::ReadOnlyGravity, Options::GravityMod::Value);
			}
			else if (wasGravityEnabled && Globals::Roblox::Workspace.address != 0)
			{
				Memory->write<float>(Globals::Roblox::Workspace.address + Offsets::Workspace::Gravity, originalGravity);
				Memory->write<float>(Globals::Roblox::Workspace.address + Offsets::Workspace::ReadOnlyGravity, originalReadOnlyGravity);
				wasGravityEnabled = false;
			}

			// Jump Power Modifier
			if (Options::JumpPowerMod::Enabled && humanoid.address != 0)
			{
				if (!wasJumpPowerEnabled)
				{
					originalJumpPower = Memory->read<float>(humanoid.address + Offsets::Humanoid::JumpPower);
					wasJumpPowerEnabled = true;
				}
				Memory->write<float>(humanoid.address + Offsets::Humanoid::JumpPower, Options::JumpPowerMod::Value);
			}
			else if (wasJumpPowerEnabled && humanoid.address != 0)
			{
				Memory->write<float>(humanoid.address + Offsets::Humanoid::JumpPower, originalJumpPower);
				wasJumpPowerEnabled = false;
			}

			// NameOcclusion / Nameplate hiding
			if (Options::NameOcclusion::Enabled && humanoid.address != 0)
			{
				if (!wasNameOcclusionEnabled)
				{
					originalNameDisplayDistance = Memory->read<float>(humanoid.address + Offsets::Humanoid::NameDisplayDistance);
					originalHealthDisplayDistance = Memory->read<float>(humanoid.address + Offsets::Humanoid::HealthDisplayDistance);
					wasNameOcclusionEnabled = true;
				}

				if (Options::NameOcclusion::HideNameplates)
				{
					Memory->write<float>(humanoid.address + Offsets::Humanoid::NameDisplayDistance, -1.0f);
					Memory->write<float>(humanoid.address + Offsets::Humanoid::HealthDisplayDistance, -1.0f);
				}
				else if (Options::NameOcclusion::NameDisplayDistance > 0.0f)
				{
					Memory->write<float>(humanoid.address + Offsets::Humanoid::NameDisplayDistance, Options::NameOcclusion::NameDisplayDistance);
					Memory->write<float>(humanoid.address + Offsets::Humanoid::HealthDisplayDistance, Options::NameOcclusion::HealthDisplayDistance);
				}
			}
			else if (wasNameOcclusionEnabled && humanoid.address != 0)
			{
				Memory->write<float>(humanoid.address + Offsets::Humanoid::NameDisplayDistance, originalNameDisplayDistance);
				Memory->write<float>(humanoid.address + Offsets::Humanoid::HealthDisplayDistance, originalHealthDisplayDistance);
				wasNameOcclusionEnabled = false;
			}

			// AutoRotate
			if (Options::AutoRotate::Enabled && humanoid.address != 0)
			{
				Memory->write<bool>(humanoid.address + Offsets::Humanoid::AutoRotate, true);
			}
		}
		catch (...)
		{
		}
	}
}
