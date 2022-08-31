#include "AttackCollision.h"

#include "Offsets.h"
#include "PrecisionHandler.h"
#include "Settings.h"

AttackCollision::AttackCollision(RE::ActorHandle a_actorHandle, const CollisionDefinition& a_collisionDefinition) :
	actorHandle(a_actorHandle), nodeName(a_collisionDefinition.nodeName)
{
	lastUpdate = *g_durationOfApplicationRunTimeMS;

	auto actor = a_actorHandle.get();
	if (actor && actor->currentProcess) {
		auto cell = actor->GetParentCell();
		if (cell) {
			if (RE::NiPointer<RE::NiNode> newNode = RE::NiPointer<RE::NiNode>(AddCollision(actorHandle, a_collisionDefinition))) {
				bool bIsWeapon = a_collisionDefinition.nodeName == "WEAPON"sv || a_collisionDefinition.nodeName == "SHIELD"sv;
				if (bIsWeapon) {
					RE::InventoryEntryData* weaponItem = nullptr;
					RE::TESForm* equipment = nullptr;
					RE::TESObjectWEAP* equippedWeapon = nullptr;

					bool bIsBashing = false;

					bool bIsLeftHand = a_collisionDefinition.nodeName != "WEAPON"sv;

					if (bIsLeftHand) {
						weaponItem = actor->currentProcess->middleHigh->leftHand;
					} else {
						weaponItem = actor->currentProcess->middleHigh->rightHand;
					}

					if (weaponItem) {
						equipment = weaponItem->object;
					}

					if (!equipment) {
						bIsWeapon = false;
					}

					bIsBashing = actor->GetAttackState() == RE::ATTACK_STATE_ENUM::kBash;

					bool bShowTrail = Settings::bDisplayTrails && !a_collisionDefinition.bNoTrail && !bIsBashing;

					if (bShowTrail) {
						if (equipment) {
							equippedWeapon = equipment->As<RE::TESObjectWEAP>();
						}

						if (!equippedWeapon || (equippedWeapon && equippedWeapon->weaponData.animationType == RE::WEAPON_TYPE::kHandToHandMelee)) {
							bShowTrail = false;
						}
					}					

					// get visual weapon length
					if (newNode->parent && newNode->parent->children.size() > 0) {
						auto& weaponNode = newNode->parent->children[0];
						if (weaponNode && weaponNode != newNode) {
							visualWeaponLength = PrecisionHandler::GetWeaponMeshLength(weaponNode.get());
						}
					}

					// add trail
					if (bShowTrail) {
						PrecisionHandler::GetSingleton()->_attackTrails.emplace_back(std::make_shared<AttackTrail>(newNode.get(), actorHandle, cell, weaponItem, bIsLeftHand, a_collisionDefinition.bTrailUseTrueLength, a_collisionDefinition.trailOverride));
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
			groundShake = a_collisionDefinition.groundShake;

			lifetime = a_collisionDefinition.duration;
			if (a_collisionDefinition.duration) {
				if (lifetime == 0) {
					lifetime = Settings::fDefaultCollisionLifetime;
					bool bPowerAttack = false;
					if (auto& attackData = Actor_GetAttackData(actor.get())) {
						bPowerAttack = attackData->data.flags.any(RE::AttackData::AttackFlag::kPowerAttack);
					}
					if (bPowerAttack) {
						*lifetime *= Settings::fDefaultCollisionLifetimePowerAttackMult;
					}
				}

				float weaponSpeedMult = 1.f;
				actor->GetGraphVariableFloat("weaponSpeedMult"sv, weaponSpeedMult);
				if (weaponSpeedMult == 0.f) {
					weaponSpeedMult = 1.f;
				}
				*lifetime *= (1.f / weaponSpeedMult);

				// really hacky fix for a weird issue
				if (*g_deltaTime > 0.011f) {
					*lifetime += *g_deltaTime;
				}

				if (a_collisionDefinition.durationMult) {
					*lifetime *= *a_collisionDefinition.durationMult;
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

	float length = 0.f;
	float radius = 0.f;
	RE::hkVector4 vertexA{};
	RE::hkVector4 vertexB{};

	float havokWorldScale = *g_worldScale;
	float havokInvWorldScale = *g_worldScaleInverse;

	bool bIsWeapon = nodeName == "WEAPON"sv || nodeName == "SHIELD"sv;

	RE::NiPoint3 tipOffset;

	bool bIsShieldBashing = false;
	bool bIsBowBashing = false;
	
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

			bool bIsBashingWithLeftHand = leftHandEquipment && leftHandEquipment != rightHandEquipment;  // has something else in the left hand so use the left hand node
			if (!bIsBashingWithLeftHand && rightHandEquipment) {  // check if it's a bow, bows are held in the left hand even if they're technically right hand
				if (auto rightHandWeapon = rightHandEquipment->As<RE::TESObjectWEAP>()) {
					if (rightHandWeapon->weaponData.animationType == RE::WEAPON_TYPE::kBow) {
						bIsBashingWithLeftHand = true;
						bIsBowBashing = true;
					}
				}
			}
			
			if (bIsBashingWithLeftHand) {
				bIsShieldBashing = leftHandEquipment && leftHandEquipment->IsArmor();

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

		RE::NiAVObject* weaponNode = nullptr;
		if (node->children.size() > 0 && node->children[0]) {
			weaponNode = node->children[0].get();
		}

		// sum up mults
		float lengthMult = 1.f;
		float radiusMult = 1.f;

		if (a_collisionDefinition.lengthMult) {
			lengthMult *= *a_collisionDefinition.lengthMult;
		}

		if (a_collisionDefinition.radiusMult) {
			radiusMult *= *a_collisionDefinition.radiusMult;
		}
		
		if (a_collisionDefinition.transform) {
			lengthMult *= a_collisionDefinition.transform->scale;
			radiusMult *= a_collisionDefinition.transform->scale;
		}
		
		// calc length and radius
		length = PrecisionHandler::GetWeaponAttackLength(a_actorHandle, weaponNode, a_collisionDefinition.capsuleLength, lengthMult) * havokWorldScale;
		radius = PrecisionHandler::GetWeaponAttackRadius(a_actorHandle, a_collisionDefinition.capsuleRadius, radiusMult) * havokWorldScale;
		
		// special cases
		if (bIsShieldBashing || bIsBowBashing) {
			float halfLength = length * 0.5f;
			vertexA.quad.m128_f32[0] = halfLength;
			vertexB.quad.m128_f32[0] = -halfLength;
			if (bIsShieldBashing) {
				radius = length;
			}
		} else {
			vertexA.quad.m128_f32[0] = length;
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

		// sum up mults
		float lengthMult = 1.f;
		float radiusMult = 1.f;

		if (a_collisionDefinition.lengthMult) {
			lengthMult *= *a_collisionDefinition.lengthMult;
		}

		if (a_collisionDefinition.radiusMult) {
			radiusMult *= *a_collisionDefinition.radiusMult;
		}

		if (a_collisionDefinition.transform) {
			radiusMult *= a_collisionDefinition.transform->scale;
			lengthMult *= a_collisionDefinition.transform->scale;
		}

		// calc attack dimensions
		if (!PrecisionHandler::GetNodeAttackDimensions(a_actorHandle, node, a_collisionDefinition.capsuleLength, lengthMult, a_collisionDefinition.capsuleRadius, radiusMult, vertexA, vertexB, radius)) {
			return nullptr;
		}		
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

	a_deltaTime = PrecisionHandler::GetSingleton()->GetHitstop(actorHandle, a_deltaTime, false);

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

				if (dot2 != 0.f) {
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
	{
		WriteLocker locker(lock);

		for (auto it = _attackCollisions.begin(); it != _attackCollisions.end();) {
			auto& collision = *it;
			if (!collision->Update(a_deltaTime)) {
				it = _attackCollisions.erase(it);
			} else {
				++it;
			}
			
		}
	}	

	// update and remove expired entries
	for (auto it = _IDHitRefs.begin(); it != _IDHitRefs.end();) {
		it->second.Update(a_deltaTime);
		if (it->second.IsEmpty()) {
			it = _IDHitRefs.erase(it);
		} else {
			++it;
		}
	}
}

bool AttackCollisions::IsEmpty() const
{
	ReadLocker locker(lock);
	
	return _attackCollisions.empty();
}

std::shared_ptr<AttackCollision> AttackCollisions::GetAttackCollision(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_node) const
{
	ReadLocker locker(lock);
	
	auto it = std::find_if(_attackCollisions.begin(), _attackCollisions.end(), [a_node](auto& attackCollision) { return attackCollision->collisionNode.get() == a_node; });
	if (it != _attackCollisions.end()) {
		return *it;
	}

	return nullptr;
}

std::shared_ptr<AttackCollision> AttackCollisions::GetAttackCollision(RE::ActorHandle a_actorHandle, std::string_view a_nodeName) const
{
	ReadLocker locker(lock);
	
	auto it = std::find_if(_attackCollisions.begin(), _attackCollisions.end(), [a_nodeName](auto& attackCollision) { return attackCollision->nodeName == a_nodeName; });
	if (it != _attackCollisions.end()) {
		return *it;
	}

	return nullptr;
}

void AttackCollisions::AddAttackCollision(RE::ActorHandle a_actorHandle, const CollisionDefinition& a_collisionDefinition)
{
	WriteLocker locker(lock);
	
	_attackCollisions.emplace_back(std::make_shared<AttackCollision>(a_actorHandle, a_collisionDefinition));
}

bool AttackCollisions::RemoveAttackCollision(RE::ActorHandle a_actorHandle, const CollisionDefinition& a_collisionDefinition)
{
	WriteLocker locker(lock);
	
	auto prevSize = _attackCollisions.size();

	if (!_attackCollisions.empty()) {
		if (a_collisionDefinition.ID) {  // remove all matching ID
			_attackCollisions.erase(std::remove_if(_attackCollisions.begin(), _attackCollisions.end(), [a_collisionDefinition](auto& attackCollision) { return !attackCollision || attackCollision->ID == a_collisionDefinition.ID; }));
		} else {  // remove the first matching the node name
			auto search = std::find_if(_attackCollisions.begin(), _attackCollisions.end(), [a_collisionDefinition](auto& attackCollision) { return !attackCollision || attackCollision->nodeName == a_collisionDefinition.nodeName; });
			if (search != _attackCollisions.end()) {
				_attackCollisions.erase(search);
			} else {
				logger::error("Could not find an attack collision to remove: {}", a_collisionDefinition.nodeName);
			}
		}
	}

	return prevSize != _attackCollisions.size();
}

bool AttackCollisions::RemoveAttackCollision(RE::ActorHandle a_actorHandle, std::shared_ptr<AttackCollision> a_attackCollision)
{
	WriteLocker locker(lock);
	
	auto prevSize = _attackCollisions.size();

	if (!_attackCollisions.empty()) {
		_attackCollisions.erase(std::remove_if(_attackCollisions.begin(), _attackCollisions.end(), [a_attackCollision](auto& attackCollision) { return attackCollision == a_attackCollision; }));
	}

	return prevSize != _attackCollisions.size();
}

bool AttackCollisions::RemoveAllAttackCollisions(RE::ActorHandle a_actorHandle)
{
	WriteLocker locker(lock);

	auto prevSize = _attackCollisions.size();
	
	_attackCollisions.clear();

	return prevSize != _attackCollisions.size();
}

void AttackCollisions::ForEachAttackCollision(std::function<void(std::shared_ptr<AttackCollision>)> a_func) const
{
	ReadLocker locker(lock);

	for (auto& attackCollision : _attackCollisions) {
		a_func(attackCollision);
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
