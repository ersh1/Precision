#pragma once

#include "Havok/Havok.h"

struct Blender
{
	enum class BlendType : uint8_t
	{
		kAnimToRagdoll,
		kCurrentAnimToRagdoll,
		kRagdollToAnim,
		kCurrentRagdollToAnim,
		kRagdollToCurrentRagdoll
	};

	struct Curve
	{
		Curve(float a_duration);
		virtual float GetBlendValueAtTime(float a_time);

		float duration = 1.0;
	};

	struct PowerCurve : Curve
	{
		PowerCurve(float a_duration, float a_power);
		virtual float GetBlendValueAtTime(float a_time) override;

		float a;
		float b;
	};

	void StartBlend(BlendType a_blendType, const Curve& a_blendCurve);

	inline void StopBlend() { bIsActive = false; }

	bool Update(const struct ActiveRagdoll& a_ragdoll, const RE::hkbRagdollDriver& a_driver, RE::hkbGeneratorOutput& a_inOut, float a_deltaTime);

	std::vector<RE::hkQsTransform> initialPose{};
	std::vector<RE::hkQsTransform> currentPose{};
	std::vector<RE::hkQsTransform> scratchPose{};
	float elapsedTime = 0.f;
	BlendType type = BlendType::kAnimToRagdoll;
	Curve curve{ 1.0 };
	bool bIsFirstBlendFrame = false;
	bool bIsActive = false;
};
