#include "AttackCollision.h"

#include "Offsets.h"
#include "PrecisionHandler.h"
#include "Settings.h"

AttackCollision::AttackCollision(RE::ActorHandle a_actorHandle, const CollisionDefinition& a_collisionDefinition) :
	actorHandle(a_actorHandle), nodeName(a_collisionDefinition.nodeName)
{
	lastUpdate = *g_durationOfApplicationRunTimeMS;

	auto actor = a_actorHandle.get();
	if (actor) {
		auto cell = actor->GetParentCell();
		if (cell) {
			if (RE::NiPointer<RE::NiNode> newNode = RE::NiPointer<RE::NiNode>(AddCollision(actorHandle, a_collisionDefinition))) {
				bool bIsWeapon = a_collisionDefinition.nodeName == "WEAPON"sv || a_collisionDefinition.nodeName == "SHIELD"sv;
				if (bIsWeapon) {
					RE::TESForm* equipment = nullptr;
					RE::TESObjectWEAP* equippedWeapon = nullptr;

					bool bIsBashing = false;

					if (a_collisionDefinition.nodeName == "WEAPON"sv) {
						equipment = actor->currentProcess->GetEquippedRightHand();
					} else {
						equipment = actor->currentProcess->GetEquippedLeftHand();
					}

					if (!equipment) {
						bIsWeapon = false;
					}

					bIsBashing = actor->GetAttackState() == RE::ATTACK_STATE_ENUM::kBash;

					if (equipment) {
						equippedWeapon = equipment->As<RE::TESObjectWEAP>();
					}

					// get visual weapon length
					if (newNode->parent && newNode->parent->children.size() > 0) {
						auto& weaponNode = newNode->parent->children[0];
						if (weaponNode && weaponNode != newNode) {
							visualWeaponLength = PrecisionHandler::GetWeaponAttackReach(actorHandle, weaponNode.get(), equippedWeapon, true, false);
						}
					}

					// add trail
					if (Settings::bDisplayTrails && !a_collisionDefinition.bNoTrail && !bIsBashing) {
						PrecisionHandler::GetSingleton()->_attackTrails.emplace_back(std::make_shared<AttackTrail>(newNode.get(), actorHandle, cell, equippedWeapon));
					}
				}

				Utils::Capsule capsule;
				Utils::GetCapsuleParams(newNode.get(), capsule);
				capsuleLength = fmax(capsule.a.GetDistance(capsule.b), capsule.radius * 2.f);

				collisionNode = newNode;
			}

			ID = a_collisionDefinition.ID;
			bNoRecoil = a_collisionDefinition.bNoRecoil;
			damageMult = a_collisionDefinition.damageMult;

			lifetime = a_collisionDefinition.duration;
			if (a_collisionDefinition.duration) {
				if (lifetime == 0) {
					lifetime = Settings::fDefaultCollisionLifetime;
					if (auto& attackData = Actor_GetAttackData(actor.get())) {
						bool bPowerAttack = attackData->data.flags.any(RE::AttackData::AttackFlag::kPowerAttack);
						if (bPowerAttack) {
							*lifetime *= Settings::fDefaultCollisionLifetimePowerAttackMult;
						}
					}
				}

				float weaponSpeedMult = 1.f;
				actor->GetGraphVariableFloat("weaponSpeedMult"sv, weaponSpeedMult);
				*lifetime *= (1.f / weaponSpeedMult);

				// really hacky fix for a weird issue
				if (*g_deltaTime > 0.011f) {
					*lifetime += *g_deltaTime;
				}
			}
		}
	}
}

AttackCollision::~AttackCollision()
{
	RemoveCollision(actorHandle);
}

RE::NiNode* AttackCollision::AddCollision(RE::ActorHandle a_actorHandle, const CollisionDefinition& a_collisionDefinition)
{
	auto actor = a_actorHandle.get().get();
	if (!actor) {
		return nullptr;
	}

	auto cell = actor->GetParentCell();
	if (!cell) {
		return nullptr;
	}

	auto world = cell->GetbhkWorld();
	if (!world) {
		return nullptr;
	}

	auto root = actor->Get3D();
	if (!root) {
		return nullptr;
	}

	auto bone = root->GetObjectByName(nodeName);
	if (!bone) {
		return nullptr;
	}

	auto node = bone->AsNode();
	if (!node) {
		return nullptr;
	}

	float radius = 0.f;
	RE::hkVector4 vertexA{};
	RE::hkVector4 vertexB{};

	float havokWorldScale = *g_worldScale;
	float havokInvWorldScale = *g_worldScaleInverse;

	bool bIsFirstPerson = false;
	bool bIsPlayer = a_actorHandle.native_handle() == 0x100000;
	if (bIsPlayer) {
		RE::BSAnimationGraphManagerPtr graphManager;
		actor->GetAnimationGraphManager(graphManager);
		if (graphManager) {
			bIsFirstPerson = graphManager->activeGraph != 0;
		}
	}

	bool bIsWeapon = nodeName == "WEAPON"sv || nodeName == "SHIELD"sv;

	float actorScale = actor->GetScale();

	RE::NiPoint3 tipOffset;

	bool bIsShieldBashing = false;
	
	if (bIsWeapon) {
		bool bIsBashing = actor->GetAttackState() == RE::ATTACK_STATE_ENUM::kBash;
		if (!bIsBashing) {
			if (auto& attackData = Actor_GetAttackData(actor)) {
				bIsBashing = attackData->event == "bashStart"sv;
			}
		}

		if (bIsBashing) {
			auto rightHandEquipment = actor->currentProcess->GetEquippedRightHand();
			auto leftHandEquipment = actor->currentProcess->GetEquippedLeftHand();
			
			if (leftHandEquipment && leftHandEquipment != rightHandEquipment) {  // has something else in the left hand so use the left hand node
				bIsShieldBashing = leftHandEquipment->IsArmor();

				nodeName = "SHIELD"sv;

				bone = root->GetObjectByName(nodeName);
				if (!bone) {
					return nullptr;
				}
				node = bone->AsNode();
				if (!node) {
					return nullptr;
				}
			}
		}

		RE::TESForm* equipment = nullptr;

		if (nodeName == "WEAPON"sv) {
			equipment = actor->currentProcess->GetEquippedRightHand();
		} else {
			equipment = actor->currentProcess->GetEquippedLeftHand();
		}

		RE::TESObjectWEAP* equippedWeapon = nullptr;
		if (equipment) {
			equippedWeapon = equipment->As<RE::TESObjectWEAP>();
		}

		RE::NiAVObject* weaponNode = nullptr;
		if (node->children.size() > 0 && node->children[0]) {
			weaponNode = node->children[0].get();
		}

		float length = PrecisionHandler::GetWeaponAttackReach(a_actorHandle, weaponNode, equippedWeapon, !Settings::bUseWeaponReach, true) * havokWorldScale;

		if (a_collisionDefinition.capsuleLength) {
			length = *a_collisionDefinition.capsuleLength * havokWorldScale;
		}

		if (a_collisionDefinition.capsuleRadius) {
			radius = *a_collisionDefinition.capsuleRadius * havokWorldScale;
		}

		if (a_collisionDefinition.transform) {
			radius *= a_collisionDefinition.transform->scale;
			length *= a_collisionDefinition.transform->scale;
		}

		radius = Settings::fWeaponCapsuleRadius * havokWorldScale;



		if (bIsPlayer) {
			length *= bIsFirstPerson ? Settings::fFirstPersonPlayerWeaponReachMult : Settings::fThirdPersonPlayerWeaponReachMult;
			radius *= bIsFirstPerson ? Settings::fFirstPersonPlayerCapsuleRadiusMult : Settings::fThirdPersonPlayerCapsuleRadiusMult;
		}

		if (actor->IsOnMount()) {
			length *= Settings::fMountedWeaponReachMult;
			radius *= Settings::fMountedCapsuleRadiusMult;
		}

		/*length *= actorScale;
		radius *= actorScale;*/

		if (bIsShieldBashing) {
			if ( bIsFirstPerson) {  // workaround for unfortunate shield bash FPP animation not really hitting stuff in front of you
				length *= 2.f;
			}
			float halfLength = length * 0.5f;
			vertexA.quad.m128_f32[0] = -halfLength;
			vertexB.quad.m128_f32[0] = halfLength;
			radius = length;
		} else {
			vertexB.quad.m128_f32[0] = length;
		}

		if (a_collisionDefinition.bWeaponTip) {
			float offset = (length - radius) * havokInvWorldScale;
			tipOffset = { 0.f, offset, 0.f };
		}
	} else if (node) {
		// Set default fallback values based on hand collision's capsule
		radius = 0.068565f;
		vertexA = { 0.002939f, 0.003175f, 0.054681f, 0.f };
		vertexB = { 0.001875f, 0.003175f, 0.054681f, 0.f };

		if (node->collisionObject) {
			auto collisionObject = static_cast<RE::bhkCollisionObject*>(node->collisionObject.get());
			auto rigidBody = collisionObject->GetRigidBody();

			if (rigidBody && rigidBody->referencedObject) {
				RE::hkpRigidBody* hkpRigidBody = static_cast<RE::hkpRigidBody*>(rigidBody->referencedObject.get());
				const RE::hkpShape* hkpShape = hkpRigidBody->collidable.shape;
				if (hkpShape->type == RE::hkpShapeType::kCapsule) {
					auto hkpCapsuleShape = static_cast<const RE::hkpCapsuleShape*>(hkpShape);

					radius = hkpCapsuleShape->radius;
					vertexA = hkpCapsuleShape->vertexA;
					vertexB = hkpCapsuleShape->vertexB;
				}
			}
		}

		if (a_collisionDefinition.capsuleLength) {
			float length = std::fmax(vertexA.GetDistance3(vertexB), radius);
			float forcedLength = *a_collisionDefinition.capsuleLength * *g_worldScale;
			float mult = forcedLength / length;

			vertexB = vertexB * mult;
		}

		if (a_collisionDefinition.capsuleRadius) {
			radius = *a_collisionDefinition.capsuleRadius * havokWorldScale;
		}

		if (a_collisionDefinition.transform) {
			radius *= a_collisionDefinition.transform->scale;
			vertexA = vertexA * a_collisionDefinition.transform->scale;
			vertexB = vertexB * a_collisionDefinition.transform->scale;
		}

		radius *= actorScale;
		vertexA = vertexA * actorScale;
		vertexB = vertexB * actorScale;
	} else {
		return nullptr;
	}

	RE::BSWriteLockGuard lock(world->worldLock);

	RE::bhkCapsuleShape* weaponShape = reinterpret_cast<RE::bhkCapsuleShape*>(RE::MemoryManager::GetSingleton()->Allocate(sizeof(RE::bhkCapsuleShape), 0, false));
	if (!weaponShape) {
		return nullptr;
	}

	bhkCapsuleShape_ctor(weaponShape);
	bhkCapsuleShape_SetSize(weaponShape, vertexA, vertexB, radius);

	auto capsuleShape = static_cast<RE::hkpCapsuleShape*>(weaponShape->referencedObject.get());

	RE::bhkRigidBodyCinfo cInfo;
	bhkRigidBodyCinfo_ctor(&cInfo);

	uint32_t collisionFilterInfo = 0;

#pragma warning(suppress: 4834)
	actor->GetCollisionFilterInfo(collisionFilterInfo);

	if (auto rb = Utils::GetRigidBody(actor->Get3D())) {
		if (auto hkpRigidBody = static_cast<RE::hkpRigidBody*>(rb->referencedObject.get())) {
			collisionFilterInfo = hkpRigidBody->collidable.broadPhaseHandle.collisionFilterInfo;
		}
	}

	uint16_t collisionGroup = collisionFilterInfo >> 16;

	cInfo.collisionFilterInfo = (uint32_t)collisionGroup << 16 | static_cast<uint32_t>(CollisionLayer::kPrecision);
	cInfo.hkCinfo.collisionFilterInfo = (uint32_t)collisionGroup << 16 | static_cast<uint32_t>(CollisionLayer::kPrecision);
	cInfo.shape = capsuleShape;
	cInfo.hkCinfo.shape = capsuleShape;
	cInfo.hkCinfo.motionType = RE::hkpMotion::MotionType::kKeyframed;
	cInfo.hkCinfo.enableDeactivation = false;
	cInfo.hkCinfo.solverDeactivation = RE::hkpRigidBodyCinfo::SolverDeactivation::kOff;
	cInfo.hkCinfo.qualityType = RE::hkpCollidableQualityType::kKeyframed;

	// set transform
	RE::hkTransform transform = Utils::GetHkTransformOfNode(bone);
	cInfo.hkCinfo.position = transform.translation;
	cInfo.hkCinfo.rotation = Utils::SetFromRotation(transform.rotation);

	RE::bhkRigidBody* rigidBody = reinterpret_cast<RE::bhkRigidBody*>(RE::MemoryManager::GetSingleton()->Allocate(sizeof(RE::bhkRigidBody), 0, false));
	if (!rigidBody) {
		return nullptr;
	}
	std::memset(rigidBody, 0, sizeof(RE::bhkRigidBody));

	bhkRigidBody_ctor(rigidBody);
	bhkRigidBody_ApplyCinfo(rigidBody, &cInfo);

	RE::hkpRigidBody* hkpRb = static_cast<RE::hkpRigidBody*>(rigidBody->referencedObject.get());
	hkpRb->collidable.broadPhaseHandle.objectQualityType = static_cast<int8_t>(RE::hkpCollidableQualityType::kKeyframedReporting);

	auto newNode = RE::NiNode::Create(0);
	node->AttachChild(newNode, true);

	if (a_collisionDefinition.transform) {
		newNode->local = *a_collisionDefinition.transform;

		if (bIsShieldBashing) {  // swap the axis when shield bashing
			RE::NiPoint3 vec = newNode->local.translate;
			newNode->local.translate.z = vec.x;
			newNode->local.translate.x = vec.z;
		}
	}

	if (a_collisionDefinition.bWeaponTip) {
		newNode->local.translate += tipOffset;
	}

	if (bIsWeapon) {
		RE::NiMatrix3 weaponRotation(0.f, RE::NI_HALF_PI, -RE::NI_HALF_PI);
		RE::NiMatrix3 newRotation = weaponRotation * newNode->local.rotate;
		newNode->local.rotate = newRotation;
	}

	RE::bhkCollisionObject* bhkCollisionObject = reinterpret_cast<RE::bhkCollisionObject*>(RE::MemoryManager::GetSingleton()->Allocate(sizeof(RE::bhkCollisionObject), 0, false));
	if (!bhkCollisionObject) {
		return nullptr;
	}
	std::memset(bhkCollisionObject, 0, sizeof(RE::bhkCollisionObject));
	bhkNiCollisionObject_NiNode_ctor(bhkCollisionObject, newNode);
	reinterpret_cast<std::uintptr_t*>(bhkCollisionObject)[0] = RE::VTABLE_bhkCollisionObject[0].address();

	bhkNiCollisionObject_setWorldObject(bhkCollisionObject, rigidBody);

	bhkRigidBody_setActivated(rigidBody, true);
	hkpWorld_AddEntity(static_cast<RE::ahkpWorld*>(world->referencedObject.get()), static_cast<RE::hkpRigidBody*>(rigidBody->referencedObject.get()), RE::hkpEntityActivation::kDoActivate);

	return newNode;
}

bool AttackCollision::RemoveCollision(RE::ActorHandle a_actorHandle)
{
	if (!collisionNode) {
		return false;
	}

	auto actor = a_actorHandle.get().get();
	if (!actor) {
		return false;
	}

	auto cell = actor->GetParentCell();
	if (!cell) {
		return false;
	}

	auto world = cell->GetbhkWorld();
	if (!world) {
		return false;
	}

	auto root = actor->Get3D();
	if (!root) {
		return false;
	}

	RE::BSWriteLockGuard lock(world->worldLock);

	bool bRemoved = false;

	if (collisionNode->collisionObject) {
		auto niCollisionObject = collisionNode->collisionObject;
		if (niCollisionObject) {
			auto collisionObject = RE::NiPointer<RE::bhkCollisionObject>(static_cast<RE::bhkCollisionObject*>(niCollisionObject.get()));
			if (auto rb = collisionObject->GetRigidBody()) {
				auto rigidBody = RE::NiPointer<RE::bhkRigidBody>(rb);
				if (rigidBody->referencedObject) {
					auto hkpRigidBody = RE::hkRefPtr<RE::hkpRigidBody>(static_cast<RE::hkpRigidBody*>(rigidBody->referencedObject.get()));
					if (hkpRigidBody->world) {
						hkpWorld_RemoveEntity(static_cast<RE::ahkpWorld*>(world->referencedObject.get()), &bRemoved, hkpRigidBody.get());
					}
				}
			}
		}
	}

	auto parentNode = RE::NiPointer<RE::NiNode>(collisionNode->parent);

	if (parentNode) {
		parentNode->DetachChild(collisionNode.get());
	}

	return true;
}

float AttackCollision::GetVisualWeaponLength() const
{
	return visualWeaponLength;
}

bool AttackCollision::HasHitRef(RE::ObjectRefHandle a_handle) const
{
	if (!ID) {
		return _hitRefs.HasHitRef(a_handle);
	} else {
		return PrecisionHandler::GetSingleton()->HasIDHitRef(actorHandle, *ID, a_handle);
	}
}

void AttackCollision::AddHitRef(RE::ObjectRefHandle a_handle, float a_duration, bool a_bIsNPC)
{
	if (!ID) {
		_hitRefs.AddHitRef(a_handle, a_duration, a_bIsNPC);
	} else {
		PrecisionHandler::GetSingleton()->AddIDHitRef(actorHandle, *ID, a_handle, a_duration, a_bIsNPC);
	}
	PrecisionHandler::GetSingleton()->AddHitRef(actorHandle, a_handle, a_duration, a_bIsNPC);
}

void AttackCollision::ClearHitRefs()
{
	if (!ID) {
		_hitRefs.ClearHitRefs();
	} else {
		PrecisionHandler::GetSingleton()->ClearIDHitRefs(actorHandle, *ID);
	}
}

void AttackCollision::IncreaseDamagedCount()
{
	if (!ID) {
		_hitRefs.IncreaseDamagedCount();
	} else {
		PrecisionHandler::GetSingleton()->IncreaseIDDamagedCount(actorHandle, *ID);
	}
}

uint32_t AttackCollision::GetHitCount() const
{
	if (!ID) {
		return _hitRefs.GetHitCount();
	} else {
		return PrecisionHandler::GetSingleton()->GetIDHitCount(actorHandle, *ID);
	}
}

uint32_t AttackCollision::GetHitNPCCount() const
{
	if (!ID) {
		return _hitRefs.GetHitNPCCount();
	} else {
		return PrecisionHandler::GetSingleton()->GetIDHitNPCCount(actorHandle, *ID);
	}
}

uint32_t AttackCollision::GetDamagedCount() const
{
	if (!ID) {
		return _hitRefs.GetDamagedCount();
	} else {
		return PrecisionHandler::GetSingleton()->GetIDDamagedCount(actorHandle, *ID);
	}
}

bool AttackCollision::Update(float a_deltaTime)
{
	if (Settings::bDebug && Settings::bDisplayWeaponCapsule) {
		constexpr glm::vec4 color{ 1, 0, 0, 1 };
		Utils::DrawCollider(collisionNode.get(), 0.f, color);
	}

	if (lastUpdate == *g_durationOfApplicationRunTimeMS) {
		return true;
	}

	lastUpdate = *g_durationOfApplicationRunTimeMS;

	a_deltaTime = PrecisionHandler::GetHitstop(actorHandle, a_deltaTime, false);

	_hitRefs.Update(a_deltaTime);

	auto actor = actorHandle.get();
	if (!actor) {
		return false;
	}

	auto cell = actor->GetParentCell();
	if (!cell) {
		return false;
	}

	// water splash
	if (Settings::bEnableWaterSplashes && lastUpdate - lastSplashUpdate > Settings::iWaterSplashCooldownMs)
	{  
		RE::NiPoint3& currentPos = collisionNode->world.translate;

		float waterHeight;
		if (cell->GetWaterHeight(currentPos, waterHeight)) {
			Utils::Capsule capsule;
			GetCapsuleParams(collisionNode.get(), capsule);
			capsule.a = (collisionNode->world.rotate * capsule.a) + currentPos;
			capsule.b = (collisionNode->world.rotate * capsule.b) + currentPos;

			if ((capsule.a.z > waterHeight && capsule.b.z < waterHeight) || (capsule.a.z < waterHeight && capsule.b.z > waterHeight)) {
				constexpr RE::NiPoint3 upVector{ 0.f, 0.f, 1.f };

				RE::NiPoint3 line = capsule.b - capsule.a;

				// calc line plane intersection
				float dot = upVector.Dot(capsule.a);
				float dot2 = upVector.Dot(line);

				RE::NiPoint3 intersection = capsule.a + (line * ((waterHeight - dot) / dot2));

				//RE::BSSoundHandle soundHandle{};
				//auto audioManager = RE::BSAudioManager::GetSingleton();
				//audioManager->BuildSoundDataFromEditorID(soundHandle, "CWaterSmall", 17);
				//if (soundHandle.IsValid()) {
				//	BSSoundHandle_SetPosition(&soundHandle, intersection.x, intersection.y, intersection.z);
				//	//soundHandle.SetPosition(intersection);
				//	soundHandle.Play();
				//}
				//RE::BSTempEffectParticle::Spawn(cell, 1.0, "Effects/waterSplash.NIF", RE::NiMatrix3(), intersection, 0.75f, 7, 0);

				PlayWaterImpact(21.f, intersection);  // medium impact
				lastSplashUpdate = lastUpdate;
			}
		}
	}

	if (lifetime) {
		if (*lifetime < 0.f) {
			return false;
		}
		*lifetime -= a_deltaTime;
	}

	return true;
}

void AttackCollisions::Update(float a_deltaTime)
{
	// update and remove expired entries
	auto it = _IDHitRefs.begin();
	while (it != _IDHitRefs.end()) {
		it->second.Update(a_deltaTime);
		if (it->second.IsEmpty()) {
			it = _IDHitRefs.erase(it);
		} else {
			++it;
		}
	}
}

bool AttackCollisions::HasHitRef(RE::ObjectRefHandle a_handle) const
{
	return _hitRefs.HasHitRef(a_handle);
}

void AttackCollisions::AddHitRef(RE::ObjectRefHandle a_handle, float a_duration, bool a_bIsNPC)
{
	_hitRefs.AddHitRef(a_handle, a_duration, a_bIsNPC);
}

void AttackCollisions::ClearHitRefs()
{
	_hitRefs.ClearHitRefs();
}

uint32_t AttackCollisions::GetHitCount() const
{
	return _hitRefs.GetHitCount();
}

uint32_t AttackCollisions::GetHitNPCCount() const
{
	return _hitRefs.GetHitNPCCount();
}

bool AttackCollisions::HasIDHitRef(uint8_t a_ID, RE::ObjectRefHandle a_handle) const
{
	auto search = _IDHitRefs.find(a_ID);
	if (search != _IDHitRefs.end()) {
		return search->second.HasHitRef(a_handle);
	}

	return false;
}

void AttackCollisions::AddIDHitRef(uint8_t a_ID, RE::ObjectRefHandle a_handle, float a_duration, bool a_bIsNPC)
{
	_IDHitRefs[a_ID].AddHitRef(a_handle, a_duration, a_bIsNPC);
}

void AttackCollisions::ClearIDHitRefs(uint8_t a_ID)
{
	auto search = _IDHitRefs.find(a_ID);
	if (search != _IDHitRefs.end()) {
		return search->second.ClearHitRefs();
	}
}

void AttackCollisions::IncreaseIDDamagedCount(uint8_t a_ID)
{
	auto search = _IDHitRefs.find(a_ID);
	if (search != _IDHitRefs.end()) {
		return search->second.IncreaseDamagedCount();
	}
}

uint32_t AttackCollisions::GetIDHitCount(uint8_t a_ID) const
{
	auto search = _IDHitRefs.find(a_ID);
	if (search != _IDHitRefs.end()) {
		return search->second.GetHitCount();
	}

	return 0;
}

uint32_t AttackCollisions::GetIDHitNPCCount(uint8_t a_ID) const
{
	auto search = _IDHitRefs.find(a_ID);
	if (search != _IDHitRefs.end()) {
		return search->second.GetHitNPCCount();
	}

	return 0;
}

uint32_t AttackCollisions::GetIDDamagedCount(uint8_t a_ID) const
{
	auto search = _IDHitRefs.find(a_ID);
	if (search != _IDHitRefs.end()) {
		return search->second.GetDamagedCount();
	}

	return 0;
}
