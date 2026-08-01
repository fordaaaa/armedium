#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>

#include "../offsets.h"
#include "../../rbx/math/math.h"
#include "../../Memory/MemoryManager.h"

class RobloxInstance
{
public:
	uintptr_t address;

	RobloxInstance(uintptr_t addy)
	{
		address = addy;
	}

	operator bool() const
	{
		return address != 0;
	}

	inline std::string Name() const
	{
		return Memory->readString(Memory->read<uintptr_t>(address + Offsets::Instance::Name));
	}

	inline std::string Class() const
	{
		return Memory->readString(Memory->read<uintptr_t>(Memory->read<uintptr_t>(address + Offsets::Instance::ClassDescriptor) + Offsets::Instance::ClassName));
	}

	inline bool IsA(std::string className) const
	{
		if (Class() == className)
		{
			return true;
		}
		return false;
	}

	inline std::vector<RobloxInstance> GetChildren() const
	{
		uintptr_t childrenStart = Memory->read<uintptr_t>(address + Offsets::Instance::ChildrenStart);
		if (!childrenStart)
			return {};

		// childrenStart points at a small struct holding the first and last
		// element-slot addresses (slots are 0x10 apart). Only those two VALUES
		// form a meaningful range - the struct pointer itself is unrelated to
		// where the slot array lives in the heap.
		uintptr_t child = Memory->read<uintptr_t>(childrenStart);
		uintptr_t childrenEnd = Memory->read<uintptr_t>(childrenStart + Offsets::Instance::ChildrenEnd);

		if (!child || childrenEnd <= child)
			return {};

		// Sanity cap (~1MB stride = ~65k children): a stale childrenStart/End
		// pair after a server transition can otherwise make this loop allocate
		// unbounded memory and crash the process via std::bad_alloc.
		if (childrenEnd - child > 0x100000)
			return {};

		std::vector<RobloxInstance> returnVector;

		for (; child < childrenEnd; child += 0x10)
		{
			returnVector.emplace_back(RobloxInstance(Memory->read<uintptr_t>(child)));
		}

		return returnVector;
	}

	inline RobloxInstance FindFirstChild(std::string name = "") const
	{
		for (auto& child : this->GetChildren())
		{
			if (name == "")
				return child;

			if (child.Name() == name)
				return child;
		}
		return 0;
	}

	inline RobloxInstance FindFirstChildWhichIsA(std::string className = "") const
	{
		for (auto& child : this->GetChildren())
		{
			if (child.Class() == className)
				return child;
		}
		return 0;
	}

	inline Vectors::Vector3 Position() const
	{
		return Memory->read<Vectors::Vector3>(Memory->read<uintptr_t>(address + Offsets::BasePart::Primitive) + Offsets::BasePart::Position); // offsets::Primitive) + offsets::Position
	}

	inline Vectors::Vector3 Size() const
	{
		return Memory->read<Vectors::Vector3>(Memory->read<uintptr_t>(address + Offsets::BasePart::Primitive) + Offsets::BasePart::Size); // offsets::Primitive) + offsets::PartSize
	}

	inline sCFrame CFrame() const
	{
		if (Class() == "Camera")
		{
			auto rotation = Memory->read<Matrixes::Matrix3x3>(address + Offsets::Camera::Rotation);
			auto position = Memory->read<Vectors::Vector3>(address + Offsets::Camera::Position);

			sCFrame newCFrame
			{
				rotation.r00, rotation.r01, rotation.r02,
				rotation.r10, rotation.r11, rotation.r12,
				rotation.r20, rotation.r21, rotation.r22,
				position.x, position.y, position.z
			};
			return newCFrame;
		}
		else
		{
			uintptr_t primitiveAddr = Memory->read<uintptr_t>(address + Offsets::BasePart::Primitive);
			return Memory->read<sCFrame>(primitiveAddr + Offsets::BasePart::Rotation);
		}
	}

	inline RobloxInstance Character() const
	{
		return RobloxInstance(Memory->read<uintptr_t>(address + Offsets::Player::ModelInstance));
	}

	inline float Health() const
	{
		auto character = Character();
		auto humanoid = character.FindFirstChildWhichIsA("Humanoid");

		return Memory->read<float>(humanoid.address + Offsets::Humanoid::Health);
	}

	inline float MaxHealth() const
	{
		auto character = Character();
		auto humanoid = character.FindFirstChildWhichIsA("Humanoid");

		return Memory->read<float>(humanoid.address + Offsets::Humanoid::MaxHealth);
	}

	inline RobloxInstance Team() const
	{
		return RobloxInstance(Memory->read<uintptr_t>(address + Offsets::Player::Team));
	}

	inline int RigType() const
	{
		return Memory->read<int>(address + Offsets::Humanoid::RigType);
	}

	inline void SetWalkspeed(float value)
	{
		value = std::round(value);
		Memory->write(address + Offsets::Humanoid::WalkspeedCheck, value);
		Memory->write(address + Offsets::Humanoid::Walkspeed, value);
	}

	inline void SetJumpPower(float value)
	{
		value = std::round(value);
		Memory->write(address + Offsets::Humanoid::JumpPower, value);
	}

	inline float GetWalkspeed()
	{
		return std::round(Memory->read<float>(address + Offsets::Humanoid::Walkspeed));
	}

	inline float GetJumpPower()
	{
		return (float)std::round(Memory->read<float>(address + Offsets::Humanoid::JumpPower));
	}

	inline float GetFOV()
	{
		auto radiantsFOV = Memory->read<float>(address + Offsets::Camera::FieldOfView);

		auto degreesFOV = radiantsFOV * 180.0f / 3.1415926535f;

		return (float)std::round(degreesFOV);

	}

	inline void SetFOV(float value)
	{
		value = (float)std::round(value);

		auto radiantsValue = value * 3.1415926535f / 180.0f;

		Memory->write<float>(address + Offsets::Camera::FieldOfView, radiantsValue);
	}

};