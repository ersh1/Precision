#include "Havok/Blender.h"

#include "Havok/ActiveRagdoll.h"
#include "Offsets.h"

Blender::Curve::Curve(float a_duration) :
	duration(a_duration)
{}

float Blender::Curve::GetBlendValueAtTime(float a_time)
{
	// Linear by default
	// y = (1/d)*t
	return std::clamp(a_time / duration, 0.f, 1.f);
}

Blender::PowerCurve::PowerCurve(float a_duration, float a_power) :
	a(pow(a_power, -duration)),
	b(a_power),
	Curve(a_duration)
{}

float Blender::PowerCurve::GetBlendValueAtTime(float a_time)
{
	// y = a * t^b
	return std::clamp(a * pow(a_time, b), 0.f, 1.f);
}

void Blender::StartBlend(BlendType a_blendType, const Curve& a_blendCurve)
{
	if (bIsActive) {
		// We were already blending before, so set the initial pose to the current blend pose
		initialPose = currentPose;
		bIsFirstBlendFrame = false;
	} else {
		bIsFirstBlendFrame = true;
	}

	elapsedTime = 0.f;
	type = a_blendType;
	curve = a_blendCurve;
	bIsActive = true;
}

bool Blender::Update(const struct ActiveRagdoll& a_ragdoll, [[maybe_unused]] const RE::hkbRagdollDriver& a_driver, RE::hkbGeneratorOutput& a_inOut, float a_deltaTime)
{
	using TrackHeader = RE::hkbGeneratorOutput::TrackHeader;

	elapsedTime += a_deltaTime;
	float lerpAmount = curve.GetBlendValueAtTime(elapsedTime);

	//int32_t numTracks = a_inOut.tracks->masterHeader.numTracks;

	TrackHeader* poseHeader = GetTrackHeader(a_inOut, RE::hkbGeneratorOutput::StandardTracks::TRACK_POSE);
	if (poseHeader && poseHeader->onFraction > 0.f) {
		int numPoses = poseHeader->numData;
		RE::hkQsTransform* poseOut = (RE::hkQsTransform*)Track_getData(a_inOut, *poseHeader);

		// Save initial pose if necessary
		if ((type == BlendType::kAnimToRagdoll || type == BlendType::kRagdollToAnim || type == BlendType::kRagdollToCurrentRagdoll) && bIsFirstBlendFrame) {
			if (type == BlendType::kAnimToRagdoll)
				initialPose = a_ragdoll.animPose;
			else if (type == BlendType::kRagdollToAnim)
				initialPose = a_ragdoll.ragdollPose;
			else if (type == BlendType::kRagdollToCurrentRagdoll)
				initialPose = a_ragdoll.ragdollPose;
		}
		bIsFirstBlendFrame = false;

		// Blend poses
		if (type == BlendType::kAnimToRagdoll) {
			hkbBlendPoses(numPoses, initialPose.data(), a_ragdoll.ragdollPose.data(), lerpAmount, poseOut);
		} else if (type == BlendType::kRagdollToAnim) {
			hkbBlendPoses(numPoses, initialPose.data(), a_ragdoll.animPose.data(), lerpAmount, poseOut);
		} else if (type == BlendType::kCurrentAnimToRagdoll) {
			hkbBlendPoses(numPoses, a_ragdoll.animPose.data(), a_ragdoll.ragdollPose.data(), lerpAmount, poseOut);
		} else if (type == BlendType::kCurrentRagdollToAnim) {
			hkbBlendPoses(numPoses, a_ragdoll.ragdollPose.data(), a_ragdoll.animPose.data(), lerpAmount, poseOut);
		} else if (type == BlendType::kRagdollToCurrentRagdoll) {
			hkbBlendPoses(numPoses, initialPose.data(), a_ragdoll.ragdollPose.data(), lerpAmount, poseOut);
		}

		currentPose.assign(poseOut, poseOut + numPoses);  // save the blended pose in case we need to blend out from here
	}

	if (elapsedTime >= curve.duration) {
		bIsActive = false;
		return true;
	}

	return false;
}