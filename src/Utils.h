#pragma once

#include "Offsets.h"

namespace Utils
{
	void PlayVisualEffect(RE::BGSReferenceEffect* a_visualEffect, RE::TESObjectREFR* a_target, float a_lifetime = -1.f, RE::TESObjectREFR* a_facingObjectRef = nullptr);

	[[nodiscard]] constexpr inline float DegreeToRadian(float a_angle)
	{
		return a_angle * 0.017453292f;
	}

	[[nodiscard]] constexpr inline float RadianToDegree(float a_radian)
	{
		return a_radian * 57.295779513f;
	}

	[[nodiscard]] inline float Clamp(float value, float min, float max)
	{
		return value < min ? min : value < max ? value :
		                                         max;
	}

	[[nodiscard]] inline float AngleDifference(float a_angle1, float a_angle2)
	{
		float diff = fmodf(a_angle2 - a_angle1 + 180.f, 360.f) - 180.f;
		return diff < -180.f ? diff + 360.f : diff;
	}

	struct Capsule
	{
		RE::NiPoint3 a;
		RE::NiPoint3 b;
		float radius;
	};

	[[nodiscard]] inline RE::NiQuaternion MatrixToQuaternion(const RE::NiMatrix3& m)
	{
		RE::NiQuaternion q;
		NiMatrixToNiQuaternion(q, m);
		return q;
	}
	[[nodiscard]] inline RE::NiPoint3 HkVectorToNiPoint(const RE::hkVector4& vec) { return { vec.quad.m128_f32[0], vec.quad.m128_f32[1], vec.quad.m128_f32[2] }; }
	[[nodiscard]] inline RE::hkVector4 NiPointToHkVector(const RE::NiPoint3& pt) { return { pt.x, pt.y, pt.z, 0 }; };
	[[nodiscard]] inline RE::NiQuaternion HkQuatToNiQuat(const RE::hkQuaternion& quat) { return { quat.vec.quad.m128_f32[3], quat.vec.quad.m128_f32[0], quat.vec.quad.m128_f32[1], quat.vec.quad.m128_f32[2] }; };
	[[nodiscard]] inline RE::hkQuaternion NiQuatToHkQuat(const RE::NiQuaternion& quat)
	{
		RE::hkQuaternion ret;
		ret.vec.quad = _mm_setr_ps(quat.x, quat.y, quat.z, quat.w);
		return ret;
	};

	[[nodiscard]] RE::hkQuaternion MultiplyQuaternions(const RE::hkQuaternion& a_first, const RE::hkQuaternion& a_second);
	[[nodiscard]] RE::hkQsTransform MultiplyTransforms(const RE::hkQsTransform& a_first, const RE::hkQsTransform& a_second);

	void NiMatrixToHkMatrix(const RE::NiMatrix3& a_niMat, RE::hkMatrix3& a_hkMat);
	void HkMatrixToNiMatrix(const RE::hkMatrix3& a_hkMat, RE::NiMatrix3& a_niMat);

	[[nodiscard]] RE::hkTransform NiTransformToHkTransform(const RE::NiTransform& a_niTransform);
	[[nodiscard]] RE::NiTransform HkTransformToNiTransform(const RE::hkTransform& a_hkTransform);

	[[nodiscard]] RE::hkQsTransform NiTransformToHkQsTransform(const RE::NiTransform& a_niTransform);
	[[nodiscard]] RE::NiTransform HkQsTransformToNiTransform(const RE::hkQsTransform& a_hkQsTransform);

	[[nodiscard]] RE::NiQuaternion slerp(const RE::NiQuaternion& a_quatA, const RE::NiQuaternion& a_quatB, double a_t);
	[[nodiscard]] inline RE::NiPoint3 lerp(const RE::NiPoint3& a_A, const RE::NiPoint3& a_B, float a_t) { return a_A * (1.f - a_t) + a_B * a_t; }
	[[nodiscard]] inline float lerp(float a_A, float a_B, float a_t) { return a_A * (1.f - a_t) + a_B * a_t; }
	[[nodiscard]] RE::hkQsTransform lerphkQsTransform(RE::hkQsTransform& a_transfA, RE::hkQsTransform& a_transfB, float a_t);

	RE::NiMatrix3 QuaternionToMatrix(const RE::NiQuaternion& a_quat);

	inline RE::NiPoint3 ForwardVectorFromNiMatrix3(const RE::NiMatrix3& a_matrix)
	{
		return { a_matrix.entry[0][1], a_matrix.entry[1][1], a_matrix.entry[2][1] };
	}

	inline void SetRotatedDir(RE::hkVector4& vec, const RE::hkQuaternion& quat, const RE::hkVector4& direction)
	{
		float qreal = quat.vec.quad.m128_f32[3];
		float q2minus1 = qreal * qreal - 0.5f;

		float imagDotDir = (quat.vec.quad.m128_f32[0] * direction.quad.m128_f32[0]) + (quat.vec.quad.m128_f32[1] * direction.quad.m128_f32[1]) + (quat.vec.quad.m128_f32[2] * direction.quad.m128_f32[2]);

		vec.quad.m128_f32[0] = (direction.quad.m128_f32[0] * q2minus1) + (quat.vec.quad.m128_f32[0] * imagDotDir);
		vec.quad.m128_f32[1] = (direction.quad.m128_f32[1] * q2minus1) + (quat.vec.quad.m128_f32[1] * imagDotDir);
		vec.quad.m128_f32[2] = (direction.quad.m128_f32[2] * q2minus1) + (quat.vec.quad.m128_f32[2] * imagDotDir);
		vec.quad.m128_f32[3] = (direction.quad.m128_f32[3] * q2minus1) + (quat.vec.quad.m128_f32[3] * imagDotDir);

		RE::hkVector4 imagCrossDir;
		imagCrossDir.quad.m128_f32[0] = (quat.vec.quad.m128_f32[1] * direction.quad.m128_f32[2]) - (quat.vec.quad.m128_f32[2] * direction.quad.m128_f32[1]);
		imagCrossDir.quad.m128_f32[1] = (quat.vec.quad.m128_f32[2] * direction.quad.m128_f32[0]) - (quat.vec.quad.m128_f32[0] * direction.quad.m128_f32[2]);
		imagCrossDir.quad.m128_f32[2] = (quat.vec.quad.m128_f32[0] * direction.quad.m128_f32[1]) - (quat.vec.quad.m128_f32[1] * direction.quad.m128_f32[0]);
		imagCrossDir.quad.m128_f32[3] = 0.f;

		vec.quad.m128_f32[0] += imagCrossDir.quad.m128_f32[0] * qreal;
		vec.quad.m128_f32[1] += imagCrossDir.quad.m128_f32[1] * qreal;
		vec.quad.m128_f32[2] += imagCrossDir.quad.m128_f32[2] * qreal;
		vec.quad.m128_f32[3] += imagCrossDir.quad.m128_f32[3] * qreal;

		vec.quad.m128_f32[0] += vec.quad.m128_f32[0];
		vec.quad.m128_f32[1] += vec.quad.m128_f32[1];
		vec.quad.m128_f32[2] += vec.quad.m128_f32[2];
		vec.quad.m128_f32[3] += vec.quad.m128_f32[3];
	}

	inline void NormalizeHkVector4(RE::hkVector4& vec)
	{
		float x = vec.quad.m128_f32[0];
		float y = vec.quad.m128_f32[1];
		float z = vec.quad.m128_f32[2];
		float w = vec.quad.m128_f32[3];

		float lengthSq = (x * x) + (y * y) + (z * z) + (w * w);
		float length = sqrtf(lengthSq);

		if (length == 1.f) {
			return;
		} else if (length > FLT_EPSILON) {
			x *= 1.f / length;
			y *= 1.f / length;
			z *= 1.f / length;
			w *= 1.f / length;
		} else {
			x = 0.f;
			y = 0.f;
			z = 0.f;
			w = 0.f;
		}

		vec.quad = _mm_setr_ps(x, y, z, w);
	}

	inline void NormalizeHkQuat(RE::hkQuaternion& quat)
	{
		NormalizeHkVector4(quat.vec);
	}

	inline float DotProduct(const RE::NiQuaternion& a_quatA, const RE::NiQuaternion& a_quatB) { return a_quatA.w * a_quatB.w + a_quatA.x * a_quatB.x + a_quatA.y * a_quatB.y + a_quatA.z * a_quatB.z; }

	inline float NiQuaternionLength(const RE::NiQuaternion& a_quat) { return sqrtf(DotProduct(a_quat, a_quat)); }

	RE::NiQuaternion NiQuaternionMultiply(const RE::NiQuaternion& a_quatA, const RE::NiQuaternion& a_quatB);
	RE::NiQuaternion NiQuaternionMultiply(const RE::NiQuaternion& a_quat, float a_multiplier);

	RE::NiQuaternion NiQuaternionIdentity();
	RE::NiQuaternion NormalizeNiQuat(const RE::NiQuaternion& a_quat);

	[[nodiscard]] inline RE::NiPoint3 RotateAngleAxis(const RE::NiPoint3& vec, const float angle, const RE::NiPoint3& axis)
	{
		float S = sin(angle);
		float C = cos(angle);

		const float XX = axis.x * axis.x;
		const float YY = axis.y * axis.y;
		const float ZZ = axis.z * axis.z;

		const float XY = axis.x * axis.y;
		const float YZ = axis.y * axis.z;
		const float ZX = axis.z * axis.x;

		const float XS = axis.x * S;
		const float YS = axis.y * S;
		const float ZS = axis.z * S;

		const float OMC = 1.f - C;

		return RE::NiPoint3((OMC * XX + C) * vec.x + (OMC * XY - ZS) * vec.y + (OMC * ZX + YS) * vec.z,
			(OMC * XY + ZS) * vec.x + (OMC * YY + C) * vec.y + (OMC * YZ - XS) * vec.z,
			(OMC * ZX - YS) * vec.x + (OMC * YZ + XS) * vec.y + (OMC * ZZ + C) * vec.z);
	}

	[[nodiscard]] RE::NiMatrix3 MatrixFromAxisAngle(const RE::NiPoint3& axis, float theta);

	[[nodiscard]] inline RE::NiPoint3 TransformVectorByMatrix(const RE::NiPoint3& a_vector, const RE::NiMatrix3& a_matrix)
	{
		return RE::NiPoint3(a_matrix.entry[0][0] * a_vector.x + a_matrix.entry[0][1] * a_vector.y + a_matrix.entry[0][2] * a_vector.z,
			a_matrix.entry[1][0] * a_vector.x + a_matrix.entry[1][1] * a_vector.y + a_matrix.entry[1][2] * a_vector.z,
			a_matrix.entry[2][0] * a_vector.x + a_matrix.entry[2][1] * a_vector.y + a_matrix.entry[2][2] * a_vector.z);
	}

	[[nodiscard]] inline RE::NiPoint3 InverseTransformVectorByMatrix(const RE::NiPoint3& a_vector, const RE::NiMatrix3& a_matrix)
	{
		return RE::NiPoint3(a_matrix.entry[0][0] * a_vector.x + a_matrix.entry[1][0] * a_vector.y + a_matrix.entry[2][0] * a_vector.z,
			a_matrix.entry[0][1] * a_vector.x + a_matrix.entry[1][1] * a_vector.y + a_matrix.entry[2][1] * a_vector.z,
			a_matrix.entry[0][2] * a_vector.x + a_matrix.entry[1][2] * a_vector.y + a_matrix.entry[2][2] * a_vector.z);
	}

	[[nodiscard]] inline float Remap(const float a_oldValue, const float a_oldMin, const float a_oldMax, const float a_newMin, const float a_newMax)
	{
		return (((a_oldValue - a_oldMin) * (a_newMax - a_newMin)) / (a_oldMax - a_oldMin)) + a_newMin;
	}

	inline void SetRotationMatrix(RE::NiMatrix3& a_matrix, float sacb, float cacb, float sb)
	{
		float cb = std::sqrtf(1 - sb * sb);
		float ca = cacb / cb;
		float sa = sacb / cb;
		a_matrix.entry[0][0] = ca;
		a_matrix.entry[0][1] = -sacb;
		a_matrix.entry[0][2] = sa * sb;
		a_matrix.entry[1][0] = sa;
		a_matrix.entry[1][1] = cacb;
		a_matrix.entry[1][2] = -ca * sb;
		a_matrix.entry[2][0] = 0.0;
		a_matrix.entry[2][1] = sb;
		a_matrix.entry[2][2] = cb;
	}

	void DrawCollider(RE::NiAVObject* a_node, float a_duration, glm::vec4 a_color);
	void DrawActorColliders(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_root, float a_duration, glm::vec4 a_color);
	void DrawColliders(RE::NiAVObject* a_node, float a_duration, glm::vec4 a_color, bool a_bCheckJumpIframes = false);

	bool GetCapsuleParams(RE::NiAVObject* a_node, Capsule& a_outCapsule);

	[[nodiscard]] inline RE::bhkRigidBody* GetRigidBody(RE::NiAVObject* a_object)
	{
		auto collisionObject = a_object->GetCollisionObject();
		if (collisionObject) {
			return collisionObject->GetRigidBody();
		}
		return nullptr;
	}

	[[nodiscard]] bool GetActorCollisionFilterInfo(RE::Actor* a_actor, uint32_t& a_outCollisionFilterInfo);

	RE::BGSBodyPartData* GetBodyPartData(RE::Actor* a_actor);

	bool IsNodeOrChildOfNode(RE::NiAVObject* a_object, RE::NiNode* a_node);

	bool IsNodeOrChildOfNode(RE::NiAVObject* a_object, RE::BSFixedString& a_nodeName);

	[[nodiscard]] RE::NiPointer<RE::bhkRigidBody> GetFirstRigidBody(RE::NiAVObject* a_root);

	bool FindRigidBody(RE::NiAVObject* a_root, RE::hkpRigidBody* a_outRigidBody);

	[[nodiscard]] RE::BShkbAnimationGraph* GetAnimationGraph(RE::hkbCharacter* a_character);
	[[nodiscard]] RE::Actor* GetActorFromCharacter(RE::hkbCharacter* a_character);
	[[nodiscard]] RE::Actor* GetActorFromRagdollDriver(RE::hkbRagdollDriver* a_driver);

	[[nodiscard]] int GetAnimBoneIndex(RE::hkbCharacter* a_character, const RE::BSFixedString& a_boneName);
	[[nodiscard]] int GetAnimBoneIndexFromRagdollBoneIndex(const RE::hkbRagdollDriver& a_driver, int a_ragdollBoneIndex);

	[[nodiscard]] inline bool IsMotionTypeMoveable(uint8_t a_motionType)
	{
		return (
			a_motionType == static_cast<uint8_t>(RE::hkpMotion::MotionType::kDynamic) ||
			a_motionType == static_cast<uint8_t>(RE::hkpMotion::MotionType::kSphereInertia) ||
			a_motionType == static_cast<uint8_t>(RE::hkpMotion::MotionType::kBoxInertia) ||
			a_motionType == static_cast<uint8_t>(RE::hkpMotion::MotionType::kThinBoxInertia));
	}

	[[nodiscard]] inline bool IsMoveableEntity(RE::hkpEntity* entity) { return IsMotionTypeMoveable(entity->motion.type.underlying()); }

	[[nodiscard]] inline bool IsActorGettingUp(RE::Actor* a_actor) { return a_actor->AsActorState()->GetKnockState() == RE::KNOCK_STATE_ENUM::kGetUp; }

	// returns true if actor is player's teammate, summon, or teammate's summon
	[[nodiscard]] bool IsPlayerTeammateOrSummon(RE::Actor* a_actor);

	[[nodiscard]] bool IsSweepAttackActive(RE::ActorHandle a_actorHandle, bool a_bIsLeftHand = false);

	void ForEachRagdollDriver(RE::TESObjectREFR* a_refr, std::function<void(RE::hkbRagdollDriver*)> a_func);
	void ForEachAdjacentBody(RE::hkbRagdollDriver* a_driver, RE::hkpRigidBody* a_body, std::function<void(RE::hkpRigidBody*)> a_func);

	[[nodiscard]] RE::hkQuaternion SetFromRotation(const RE::hkRotation& a_rotation);

	inline RE::hkTransform GetHkTransformOfNode(RE::NiAVObject* a_node)
	{
		RE::hkTransform ret;

		float havokWorldScale = *g_worldScale;
		ret.translation = NiPointToHkVector(a_node->world.translate * havokWorldScale);
		NiMatrixToHkMatrix(a_node->world.rotate, ret.rotation);

		return ret;
	}

	[[nodiscard]] bool GetTorsoPos(RE::Actor* a_actor, RE::NiPoint3& point);

	RE::NiTransform GetLocalTransform(RE::NiAVObject* a_node, const RE::NiTransform& a_worldTransform, bool a_bUseOldParentTransform = false);
	void UpdateNodeTransformLocal(RE::NiAVObject* a_node, const RE::NiTransform& a_worldTransform);
	void UpdateBoneMatrices(RE::NiAVObject* a_obj);

	RE::NiBound GetModelBounds(RE::NiAVObject* a_obj);
	//float GetTopVertex(RE::NiAVObject* a_obj);

	bool GetActiveAnim(RE::Actor* a_actor, RE::BSFixedString& a_outProjectName, RE::hkStringPtr& a_outAnimationName, float& a_outAnimationTime);

	RE::NiPoint3 CatmullRom(const RE::NiPoint3& a_p0, const RE::NiPoint3& a_p1, const RE::NiPoint3& a_p2, const RE::NiPoint3& a_p3, float a_t);

	[[nodiscard]] inline RE::NiPoint3 ToOrientationRotation(const RE::NiPoint3& a_vector)
	{
		RE::NiPoint3 ret;

		// Pitch
		ret.x = atan2(a_vector.z, std::sqrtf(a_vector.x * a_vector.x + a_vector.y * a_vector.y));

		// Roll
		ret.y = 0;

		// Yaw
		ret.z = atan2(a_vector.y, a_vector.x);

		return ret;
	}

	// thanks dTRY
	[[nodiscard]] std::vector<std::string_view> Tokenize(std::string_view a_string, const char a_delimiter);

	void SetBonesKeyframed(RE::hkbRagdollDriver* a_driver);

	void SetBehaviorGraphWorld(RE::Actor* a_actor, RE::bhkWorld* a_world);

	void FillCloningProcess(RE::NiCloningProcess& a_cloningProcess, const RE::NiPoint3& a_scale);

	[[nodiscard]] inline float GetUnarmedReach(RE::Actor* a_actor)
	{
		if (auto race = a_actor->GetRace()) {
			return race->data.unarmedReach;
		}
		return 0.f;
	}

	RE::MATERIAL_ID GetHitMaterialID(RE::hkpRigidBody* a_hitRigidBody, const RE::hkpContactPointEvent& a_event, int a_hitBodyIdx);

	template <class T>
	T* Clone(T* a_object, const RE::NiPoint3& a_scale)
	{
		if (!a_object) {
			return nullptr;
		}

		RE::NiCloningProcess cloningProcess{};
		FillCloningProcess(cloningProcess, a_scale);

		auto clone = reinterpret_cast<T*>(NiObject_Clone(a_object, cloningProcess));

		/*CleanupProcessMap(cloningProcess.processMap);
		CleanupCloneMap(cloningProcess.cloneMap);*/

		return clone;
	}

	// The one in commonlib doesn't traverse the children of a node with a collision object
	RE::BSVisit::BSVisitControl TraverseAllScenegraphCollisions(RE::NiAVObject* a_object, std::function<RE::BSVisit::BSVisitControl(RE::bhkNiCollisionObject*)> a_func);

	float GetPlayerTimeMultiplier();

	bool IsFirstPerson();
}
