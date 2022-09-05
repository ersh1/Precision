#pragma once

#include "Havok/Blender.h"

enum class RagdollState : uint8_t
{
	kKeyframed,
	kBlendOut,
	kBlendIn,
	kRagdoll
};

struct ActiveRagdoll
{
	using KnockState = RE::KNOCK_STATE_ENUM;

	Blender blender{};
	std::vector<RE::hkQsTransform> animPose{};
	std::vector<RE::hkQsTransform> ragdollPose{};
	std::vector<float> stress{};
	RE::hkQsTransform hipBoneTransform{};
	float avgStress = 0.f;
	float deltaTime = 0.f;
	float impulseTime = 0.f;
	RE::hkRefPtr<RE::hkpEaseConstraintsAction> easeConstraintsAction = nullptr;
	double elapsedTime = 0.0;
	RagdollState state = RagdollState::kKeyframed;
	KnockState knockState = KnockState::kNormal;
	bool isOn = false;
	bool hasHipBoneTransform = false;
	bool shouldNullOutWorldWhenRemovingFromWorld = false;

	inline bool IsImpulseActive() const { return impulseTime > 0.f; }
};
