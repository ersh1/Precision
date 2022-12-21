#pragma once

#include "Havok/Blender.h"

enum class RagdollState : uint8_t
{
	kIdle,
	kBlendIn,
	kBlendOut
};

struct ActiveRagdoll
{
	using KnockState = RE::KNOCK_STATE_ENUM;

	Blender blender{};
	std::vector<RE::hkQsTransform> animPose{};
	std::vector<RE::hkQsTransform> ragdollPose{};
	RE::hkQsTransform rootBoneTransform{};
	RE::hkQsTransform worldFromModel{};
	RE::hkQsTransform worldFromModelPostPhysics{};
	RE::hkQsTransform stickyWorldFromModel{};
	RE::NiPoint3 rootOffset{};    // meters
	float rootOffsetAngle = 0.f;  // radians
	float deltaTime = 0.f;
	float impulseTime = 0.f;
	RE::hkRefPtr<RE::hkpEaseConstraintsAction> easeConstraintsAction = nullptr;
	std::unordered_map<RE::hkpConstraintInstance*, std::pair<RE::hkVector4, RE::hkVector4>> originalConstraintPivots{};
	double elapsedTime = 0.0;
	RagdollState state = RagdollState::kIdle;
	KnockState knockState = KnockState::kNormal;
	bool bWasRigidBodyOn = true;
	bool bWasComputingWorldFromModel = false;
	bool bFadeInWorldFromModel = false;
	bool bFadeOutWorldFromModel = false;
	double worldFromModelFadeInTime = 0.0;
	double worldFromModelFadeOutTime = 0.0;
	bool isOn = false;
	bool shouldNullOutWorldWhenRemovingFromWorld = false;
	bool bHasRootBoneTransform = false;

	bool bReadyToRemove = false;

	inline bool IsImpulseActive() const { return impulseTime > 0.f; }
};
