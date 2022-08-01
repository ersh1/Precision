#include "Utils.h"
#include "Havok/Havok.h"
#include "Offsets.h"
#include "PrecisionHandler.h"
#include "render/DrawHandler.h"

namespace Utils
{
	void PlayVisualEffect(RE::BGSReferenceEffect* a_visualEffect, RE::TESObjectREFR* a_target, float a_lifetime /*= -1.f*/, RE::TESObjectREFR* a_facingObjectRef /*= nullptr*/)
	{
		if (!a_visualEffect || !a_target) {
			return;
		}

		if (!a_target->GetParentCell() || !a_target->GetParentCell()->IsAttached()) {
			return;
		}

		if (!a_target->Get3D()) {
			return;
		}

		if (a_facingObjectRef && a_visualEffect->data.flags.none(RE::BGSReferenceEffect::Flag::kFaceTarget)) {
			a_facingObjectRef = nullptr;
		}

		auto artObject = a_visualEffect->data.artObject;
		auto effectShader = a_visualEffect->data.effectShader;

		if (artObject) {
			bool bAttachToCamera = a_visualEffect->data.flags.any(RE::BGSReferenceEffect::Flag::kAttachToCamera);
			bool bInheritRotation = a_visualEffect->data.flags.any(RE::BGSReferenceEffect::Flag::kInheritRotation);
			AddArtObject(a_target, artObject, a_lifetime, a_facingObjectRef, bAttachToCamera, bInheritRotation, nullptr, nullptr);
		}

		if (effectShader) {
			AddEffectShader(a_target, effectShader, a_lifetime, a_facingObjectRef, nullptr, nullptr, nullptr, nullptr);
		}
	}

	RE::hkQuaternion MultiplyQuaternions(const RE::hkQuaternion& a_first, const RE::hkQuaternion& a_second)
	{
		RE::hkQuaternion ret;
		ret.vec = a_first.vec.Cross(a_second.vec);
		ret.vec = ret.vec + (a_second.vec * a_first.vec.quad.m128_f32[3]);
		ret.vec = ret.vec + (a_first.vec * a_second.vec.quad.m128_f32[3]);

		RE::hkVector4 dot = a_first.vec.Dot3(a_second.vec);
		RE::hkVector4 w = a_first.vec * a_second.vec;

		w = w - dot;
		ret.vec = _mm_shuffle_ps(ret.vec.quad, _mm_unpackhi_ps(ret.vec.quad, w.quad), _MM_SHUFFLE(3, 0, 1, 0));
		return ret;
	}

	RE::hkQsTransform MultiplyTransforms(const RE::hkQsTransform& a_first, const RE::hkQsTransform& a_second)
	{
		RE::hkQsTransform ret;
		RE::hkVector4 vec;
		SetRotatedDir(vec, a_first.rotation, a_second.translation);
		ret.translation = a_first.translation + vec;
		ret.rotation = MultiplyQuaternions(a_first.rotation, a_second.rotation);
		ret.scale = a_first.scale * a_second.scale;
		return ret;
	}

	void NiMatrixToHkMatrix(const RE::NiMatrix3& a_niMat, RE::hkMatrix3& a_hkMat)
	{
		a_hkMat.col0 = { a_niMat.entry[0][0], a_niMat.entry[1][0], a_niMat.entry[2][0], 0.f };
		a_hkMat.col1 = { a_niMat.entry[0][1], a_niMat.entry[1][1], a_niMat.entry[2][1], 0.f };
		a_hkMat.col2 = { a_niMat.entry[0][2], a_niMat.entry[1][2], a_niMat.entry[2][2], 0.f };
	}

	void HkMatrixToNiMatrix(const RE::hkMatrix3& a_hkMat, RE::NiMatrix3& a_niMat)
	{
		a_niMat.entry[0][0] = a_hkMat.col0.quad.m128_f32[0];
		a_niMat.entry[1][0] = a_hkMat.col0.quad.m128_f32[1];
		a_niMat.entry[2][0] = a_hkMat.col0.quad.m128_f32[2];

		a_niMat.entry[0][1] = a_hkMat.col0.quad.m128_f32[0];
		a_niMat.entry[1][1] = a_hkMat.col0.quad.m128_f32[1];
		a_niMat.entry[2][1] = a_hkMat.col0.quad.m128_f32[2];

		a_niMat.entry[0][2] = a_hkMat.col0.quad.m128_f32[0];
		a_niMat.entry[1][2] = a_hkMat.col0.quad.m128_f32[1];
		a_niMat.entry[2][2] = a_hkMat.col0.quad.m128_f32[2];
	}

	RE::NiMatrix3 QuaternionToMatrix(const RE::NiQuaternion& a_quat)
	{
		float sqw = a_quat.w * a_quat.w;
		float sqx = a_quat.x * a_quat.x;
		float sqy = a_quat.y * a_quat.y;
		float sqz = a_quat.z * a_quat.z;

		RE::NiMatrix3 ret;

		// invs (inverse square length) is only required if quaternion is not already normalised
		float invs = 1.f / (sqx + sqy + sqz + sqw);
		ret.entry[0][0] = (sqx - sqy - sqz + sqw) * invs;  // since sqw + sqx + sqy + sqz =1/invs*invs
		ret.entry[1][1] = (-sqx + sqy - sqz + sqw) * invs;
		ret.entry[2][2] = (-sqx - sqy + sqz + sqw) * invs;

		float tmp1 = a_quat.x * a_quat.y;
		float tmp2 = a_quat.z * a_quat.w;
		ret.entry[1][0] = 2.f * (tmp1 + tmp2) * invs;
		ret.entry[0][1] = 2.f * (tmp1 - tmp2) * invs;

		tmp1 = a_quat.x * a_quat.z;
		tmp2 = a_quat.y * a_quat.w;
		ret.entry[2][0] = 2.f * (tmp1 - tmp2) * invs;
		ret.entry[0][2] = 2.f * (tmp1 + tmp2) * invs;
		tmp1 = a_quat.y * a_quat.z;
		tmp2 = a_quat.x * a_quat.w;
		ret.entry[2][1] = 2.f * (tmp1 + tmp2) * invs;
		ret.entry[1][2] = 2.f * (tmp1 - tmp2) * invs;

		return ret;
	}

	RE::NiMatrix3 MatrixFromAxisAngle(const RE::NiPoint3& axis, float theta)
	{
		RE::NiPoint3 a = axis;
		float cosTheta = cosf(theta);
		float sinTheta = sinf(theta);
		RE::NiMatrix3 result;

		result.entry[0][0] = cosTheta + a.x * a.x * (1 - cosTheta);
		result.entry[0][1] = a.x * a.y * (1 - cosTheta) - a.z * sinTheta;
		result.entry[0][2] = a.x * a.z * (1 - cosTheta) + a.y * sinTheta;

		result.entry[1][0] = a.y * a.x * (1 - cosTheta) + a.z * sinTheta;
		result.entry[1][1] = cosTheta + a.y * a.y * (1 - cosTheta);
		result.entry[1][2] = a.y * a.z * (1 - cosTheta) - a.x * sinTheta;

		result.entry[2][0] = a.z * a.x * (1 - cosTheta) - a.y * sinTheta;
		result.entry[2][1] = a.z * a.y * (1 - cosTheta) + a.x * sinTheta;
		result.entry[2][2] = cosTheta + a.z * a.z * (1 - cosTheta);

		return result;
	}

	void DrawCollider(RE::NiAVObject* a_node, [[maybe_unused]] float a_duration, [[maybe_unused]] glm::vec4 a_color)
	{
		Capsule capsule;
		if (GetCapsuleParams(a_node, capsule)) {
			RE::NiPoint3 center = a_node->world.translate;
			DrawHandler::DrawDebugCapsule(center, capsule.a, capsule.b, capsule.radius, a_node->world.rotate, a_duration, a_color, true);
		}
	}

	void DrawActorColliders(RE::Actor* a_actor, float a_duration, glm::vec4 a_color)
	{
		if (!a_actor) {
			return;
		}

		auto node = a_actor->Get3D();

		DrawColliders(node, a_duration, a_color);

		// wrong somewhat
		//if (auto controller = a_actor->GetCharController()) {
		//	for (auto& bhkShape : controller->shapes) {
		//		if (bhkShape && bhkShape->referencedObject) {
		//			auto listShape = static_cast<RE::hkpListShape*>(bhkShape->referencedObject.get());
		//			if (listShape && listShape->type == RE::hkpShapeType::kList) {
		//				for (auto& childInfo : listShape->childInfo) {
		//					if (childInfo.shape && childInfo.shape->type == RE::hkpShapeType::kCapsule) {
		//						//logger::debug("{}", childInfo.shape->type);

		//						auto capsule = static_cast<const RE::hkpCapsuleShape*>(childInfo.shape);
		//						float bhkInvWorldScale = *g_worldScaleInverse;

		//						float radius = capsule->radius * bhkInvWorldScale;
		//						RE::NiPoint3 a = Utils::HkVectorToNiPoint(capsule->vertexA) * bhkInvWorldScale;
		//						RE::NiPoint3 b = Utils::HkVectorToNiPoint(capsule->vertexB) * bhkInvWorldScale;

		//						RE::hkVector4 controllerPos;
		//						controller->GetPosition(controllerPos, true);
		//						RE::NiPoint3 center = Utils::HkVectorToNiPoint(controllerPos) * bhkInvWorldScale;

		//						constexpr RE::NiPoint3 upVector {0.f, 0.f, 1.f};
		//                              RE::NiMatrix3 matrix;
		//
		//                              RE::NiPoint3 forwardVec = Utils::HkVectorToNiPoint(controller->forwardVec);

		//                              Utils::SetRotationMatrix(matrix, -forwardVec.x, forwardVec.y, forwardVec.z);
		//                              DrawHandler::DrawDebugCapsule(center, a, b, radius,
		//                                                            matrix,
		//                                                            a_duration, glm::vec4{1.f, 1.f, 1.f, 1.f}, true);
		//					}
		//				}
		//			}
		//		}
		//	}
		//}
	}

	void DrawColliders(RE::NiAVObject* a_node, float a_duration, glm::vec4 a_color)
	{
		if (a_node) {
			DrawCollider(a_node, a_duration, a_color);

			auto node = a_node->AsNode();
			if (node) {
				for (auto& child : node->children) {
					DrawColliders(child.get(), a_duration, a_color);
				}
			}
		}
	}

	bool GetCapsuleParams(RE::NiAVObject* a_node, Capsule& a_outCapsule)
	{
		if (a_node && a_node->collisionObject) {
			auto collisionObject = static_cast<RE::bhkCollisionObject*>(a_node->collisionObject.get());
			auto rigidBody = collisionObject->GetRigidBody();

			if (rigidBody && rigidBody->referencedObject) {
				RE::hkpRigidBody* hkpRigidBody = static_cast<RE::hkpRigidBody*>(rigidBody->referencedObject.get());
				const RE::hkpShape* hkpShape = hkpRigidBody->collidable.shape;
				if (hkpShape->type == RE::hkpShapeType::kCapsule) {
					auto hkpCapsuleShape = static_cast<const RE::hkpCapsuleShape*>(hkpShape);
					float bhkInvWorldScale = *g_worldScaleInverse;

					a_outCapsule.radius = hkpCapsuleShape->radius * bhkInvWorldScale;
					a_outCapsule.a = Utils::HkVectorToNiPoint(hkpCapsuleShape->vertexA) * bhkInvWorldScale;
					a_outCapsule.b = Utils::HkVectorToNiPoint(hkpCapsuleShape->vertexB) * bhkInvWorldScale;

					return true;
				}
			}
		}

		return false;
	}

	RE::BGSBodyPartData* GetBodyPartData(RE::Actor* a_actor)
	{
		if (!a_actor) {
			return nullptr;
		}

		auto race = a_actor->GetRace();
		if (!race) {
			return nullptr;
		}

		auto bodyPartData = race->bodyPartData;
		if (!bodyPartData) {
			return nullptr;
		}

		return bodyPartData;
	}

	bool IsNodeOrChildOfNode(RE::NiAVObject* a_object, RE::NiNode* a_node)
	{
		if (a_object && a_node) {
			if (a_object == a_node) {
				return true;
			}

			if (a_object->parent) {
				return IsNodeOrChildOfNode(a_object->parent, a_node);
			}
		}

		return false;
	}

	bool IsNodeOrChildOfNode(RE::NiAVObject* a_object, RE::BSFixedString& a_nodeName)
	{
		if (a_object) {
			if (a_object->name == a_nodeName) {
				return true;
			}

			if (a_object->parent) {
				return IsNodeOrChildOfNode(a_object->parent, a_nodeName);
			}
		}

		return false;
	}

	RE::bhkRigidBody* GetFirstRigidBody(RE::NiAVObject* a_root)
	{
		auto rigidBody = GetRigidBody(a_root);
		if (rigidBody) {
			return rigidBody;
		}

		RE::NiNode* node = a_root->AsNode();
		if (node) {
			for (auto& child : node->children) {
				if (child) {
					return GetFirstRigidBody(child.get());
				}
			}
		}

		return nullptr;
	}

	bool FindRigidBody(RE::NiAVObject* a_root, RE::hkpRigidBody* a_query)
	{
		auto rigidBody = GetRigidBody(a_root);
		if (rigidBody && rigidBody->referencedObject && rigidBody->referencedObject.get() == a_query) {
			return true;
		}

		auto node = a_root->AsNode();
		if (node) {
			for (auto& child : node->children) {
				if (child) {
					if (FindRigidBody(child.get(), a_query)) {
						return true;
					}
				}
			}
		}

		return false;
	}

	RE::BShkbAnimationGraph* GetAnimationGraph(RE::hkbCharacter* a_character)
	{
		auto behaviorGraph = a_character->behaviorGraph;
		if (!behaviorGraph) {
			return nullptr;
		}

		RE::BShkbAnimationGraph* graph = (RE::BShkbAnimationGraph*)behaviorGraph->userData;
		return graph;
	}

	RE::Actor* GetActorFromCharacter(RE::hkbCharacter* a_character)
	{
		RE::BShkbAnimationGraph* graph = GetAnimationGraph(a_character);
		if (!graph) {
			return nullptr;
		}

		return graph->holder;
	}

	RE::Actor* GetActorFromRagdollDriver(RE::hkbRagdollDriver* a_driver)
	{
		RE::hkbCharacter* character = a_driver->character;
		if (!character)
			return nullptr;

		return GetActorFromCharacter(character);
	}

	int GetAnimBoneIndex(RE::hkbCharacter* a_character, const RE::BSFixedString& a_boneName)
	{
		RE::hkaSkeleton* animSkeleton = a_character->setup->animationSkeleton.get();
		for (int i = 0; i < animSkeleton->bones.size(); i++) {
			const RE::hkaBone& bone = animSkeleton->bones[i];
			if (bone.name.c_str() == a_boneName) {
				return i;
			}
		}
		return -1;
	}

	bool IsPlayerTeammateOrSummon(RE::Actor* a_actor)
	{
		if (a_actor) {
			if (bool bIsTeammate = a_actor->IsPlayerTeammate()) {
				return true;
			}

			if (a_actor->IsCommandedActor() && !a_actor->IsHostileToActor(RE::PlayerCharacter::GetSingleton())) {
				auto commandingActor = a_actor->GetCommandingActor();
				if (commandingActor && (commandingActor->IsPlayerRef() || commandingActor.get()->IsPlayerTeammate())) {
					return true;
				}
			}
		}

		return false;
	}

	void ForEachRagdollDriver(RE::TESObjectREFR* a_refr, std::function<void(RE::hkbRagdollDriver*)> a_func)
	{
		RE::BSAnimationGraphManagerPtr animGraphManager;
		if (a_refr->GetAnimationGraphManager(animGraphManager)) {
			RE::BSSpinLockGuard(animGraphManager->updateLock);
			for (auto& graph : animGraphManager->graphs) {
				auto& driver = graph.get()->characterInstance.ragdollDriver;
				if (driver) {
					a_func(driver.get());
				}
			}
		}
	}

	void ForEachAdjacentBody(RE::hkbRagdollDriver* a_driver, RE::hkpRigidBody* a_body, std::function<void(RE::hkpRigidBody*)> a_func)
	{
		if (!a_driver || !a_driver->ragdoll)
			return;

		for (RE::hkpConstraintInstance* constraint : a_driver->ragdoll->constraints) {
			if (constraint->GetRigidBodyA() == a_body) {
				a_func(constraint->GetRigidBodyB());
			} else if (constraint->GetRigidBodyB() == a_body) {
				a_func(constraint->GetRigidBodyA());
			}
		}
	}

	RE::hkQuaternion SetFromRotation(const RE::hkRotation& a_rotation)
	{
		float trace = a_rotation.col0.quad.m128_f32[0] + a_rotation.col1.quad.m128_f32[1] + a_rotation.col2.quad.m128_f32[2];
		RE::hkQuaternion ret;

		auto getVal = [a_rotation](int i, int j) {
			switch (i) {
			case 0:
				return a_rotation.col0.quad.m128_f32[j];
			case 1:
				return a_rotation.col1.quad.m128_f32[j];
			case 2:
				return a_rotation.col2.quad.m128_f32[j];
			default:
				return 0.f;
			}
		};

		if (trace > 0) {
			float s = sqrtf(trace + 1.f);
			float t = 0.5f / s;
			ret.vec.quad.m128_f32[0] = (getVal(2, 1) - getVal(1, 2)) * t;
			ret.vec.quad.m128_f32[1] = (getVal(0, 2) - getVal(2, 0)) * t;
			ret.vec.quad.m128_f32[2] = (getVal(1, 0) - getVal(0, 1)) * t;
			ret.vec.quad.m128_f32[3] = 0.5f * s;
		} else {
			const int next[] = { 1, 2, 0 };
			int i = 0;
			if (getVal(1, 1) > getVal(0, 0)) {
				i = 1;
			}
			if (getVal(2, 2) > getVal(i, i)) {
				i = 2;
			}

			int j = next[i];
			int k = next[j];

			float s = sqrtf(getVal(i, i) - (getVal(j, j) + getVal(k, k)) + 1.f);
			float t = 0.5f / s;

			ret.vec.quad.m128_f32[i] = 0.5f * s;
			ret.vec.quad.m128_f32[3] = (getVal(k, j) - getVal(j, k)) * t;
			ret.vec.quad.m128_f32[j] = (getVal(j, i) + getVal(i, j)) * t;
			ret.vec.quad.m128_f32[k] = (getVal(k, i) + getVal(i, k)) * t;
		}

		return ret;
	}

	void DrawCapsule(RE::bhkRigidBody* a_rigidBody)
	{
		auto hkpRigidBody = static_cast<RE::hkpRigidBody*>(a_rigidBody->referencedObject.get());
		auto capsuleShape = static_cast<const RE::hkpCapsuleShape*>(hkpRigidBody->GetShape());

		RE::NiPoint3 vertexA = Utils::HkVectorToNiPoint(capsuleShape->vertexA) * *g_worldScaleInverse;
		RE::NiPoint3 vertexB = Utils::HkVectorToNiPoint(capsuleShape->vertexB) * *g_worldScaleInverse;

		float radius = capsuleShape->radius * *g_worldScaleInverse;

		auto& transform = hkpRigidBody->motion.motionState.transform;
		RE::NiPoint3 origin = Utils::HkVectorToNiPoint(transform.translation * *g_worldScaleInverse);
		RE::NiQuaternion bodyRotation = Utils::HkQuatToNiQuat(Utils::SetFromRotation(transform.rotation));
		RE::NiMatrix3 rotNiMatrix = Utils::QuaternionToMatrix(bodyRotation);

		constexpr glm::vec4 weaponCapsuleColor{ 0.3, 0.25, 0.9, 1 };
		DrawHandler::DrawDebugCapsule(origin, vertexA, vertexB, radius, rotNiMatrix, 0.f, weaponCapsuleColor);
	}

	RE::NiTransform GetLocalTransform(RE::NiAVObject* a_node, const RE::NiTransform& a_worldTransform, bool a_bUseOldParentTransform /*= false*/)
	{
		RE::NiPointer<RE::NiNode> parent(a_node->parent);
		if (parent) {
			RE::NiTransform inverseParent = (a_bUseOldParentTransform ? parent->previousWorld : parent->world).Invert();
			return inverseParent * a_worldTransform;
		}
		return a_worldTransform;
	}

	void UpdateNodeTransformLocal(RE::NiAVObject* a_node, const RE::NiTransform& a_worldTransform)
	{
		// Given world transform, set the necessary local transform
		a_node->local = GetLocalTransform(a_node, a_worldTransform);
	}

	void UpdateBoneMatrices(RE::NiAVObject* a_obj)
	{
		RE::BSGeometry* geom = a_obj->AsGeometry();
		if (geom) {
			if (geom->skinInstance) {
				geom->skinInstance->frameID = static_cast<uint32_t>(-1);  // This is the frameID. UpdateBoneMatrices only updates the bone matrices if the frameID is not equal to the current frame.
				NiSkinInstance_UpdateBoneMatrices(geom->skinInstance.get(), a_obj->world);
			}
		}

		RE::NiNode* node = a_obj->AsNode();
		if (node) {
			for (auto& child : node->children) {
				if (child) {
					UpdateBoneMatrices(child.get());
				}
			}
		}
	}

	//http://www.iquilezles.org/www/articles/minispline/minispline.htm
	RE::NiPoint3 CatmullRom(const RE::NiPoint3& a_p0, const RE::NiPoint3& a_p1, const RE::NiPoint3& a_p2, const RE::NiPoint3& a_p3, float a_t)
	{
		RE::NiPoint3 a = a_p1 * 2.f;
		RE::NiPoint3 b = a_p2 - a_p0;
		RE::NiPoint3 c = a_p0 * 2.f - a_p1 * 5.f + a_p2 * 4.f - a_p3;
		RE::NiPoint3 d = -a_p0 + a_p1 * 3.f - a_p2 * 3.f + a_p3;

		RE::NiPoint3 ret = (a + (b * a_t) + (c * a_t * a_t) + (d * a_t * a_t * a_t)) * 0.5f;
		return ret;
	}

	std::vector<std::string_view> Tokenize(std::string_view a_string, const char a_delimiter)
	{
		size_t start = 0;
		size_t end = a_string.find_first_of(a_delimiter);

		std::vector<std::string_view> output;

		while (end <= std::string::npos) {
			auto substring = a_string.substr(start, end - start);

			if (substring.size() != 0) {  //if token has 0 size, skip it.
				output.emplace_back(substring);
			}

			if (end == std::string::npos)
				break;

			start = end + 1;
			end = a_string.find_first_of(a_delimiter, start);
		}

		return output;
	}
}
