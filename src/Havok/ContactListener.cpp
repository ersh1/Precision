#include "Havok/ContactListener.h"
#include "Havok.h"
#include "PrecisionHandler.h"
#include "Settings.h"
#include "Utils.h"
#include "render/DrawHandler.h"

RE::hkVector4 GetParentNodePointVelocity(RE::NiAVObject* a_node, const RE::hkVector4& a_hkHitPos)
{
	if (a_node && a_node->parent) {
		if (a_node->parent->collisionObject) {
			auto collisionObject = static_cast<RE::bhkCollisionObject*>(a_node->parent->collisionObject.get());
			auto rigidBody = collisionObject->GetRigidBody();
			if (rigidBody && rigidBody->referencedObject) {
				auto hkpRigidBody = static_cast<RE::hkpRigidBody*>(rigidBody->referencedObject.get());
				if (hkpRigidBody) {
					return hkpRigidBody->motion.GetPointVelocity(a_hkHitPos);
				}
			}
		} else {
			return GetParentNodePointVelocity(a_node->parent, a_hkHitPos);
		}
	}
	return RE::hkVector4();
}

void ContactListener::ContactPointCallback(const RE::hkpContactPointEvent& a_event)
{
	if (a_event.contactPointProperties->flags & RE::hkContactPointMaterial::FlagEnum::kIsDisabled ||
		!a_event.contactPointProperties->flags & RE::hkContactPointMaterial::FlagEnum::kIsNew) {
		return;
	}

	// run callbacks and re-check flag
	PrecisionHandler::GetSingleton()->RunContactListenerCallbacks(a_event);
	if (a_event.contactPointProperties->flags & RE::hkContactPointMaterial::FlagEnum::kIsDisabled) {
		return;
	}

	RE::hkpRigidBody* rigidBodyA = a_event.bodies[0];
	RE::hkpRigidBody* rigidBodyB = a_event.bodies[1];

	CollisionLayer layerA = static_cast<CollisionLayer>(rigidBodyA->collidable.broadPhaseHandle.collisionFilterInfo & 0x7f);
	CollisionLayer layerB = static_cast<CollisionLayer>(rigidBodyB->collidable.broadPhaseHandle.collisionFilterInfo & 0x7f);

	if (layerA != CollisionLayer::kPrecision && layerB != CollisionLayer::kPrecision) {
		return;  // Every collision we care about involves the Precision layer
	}

	auto hitRigidBody = layerA == CollisionLayer::kPrecision ? rigidBodyB : rigidBodyA;
	auto hittingRigidBody = hitRigidBody == rigidBodyA ? rigidBodyB : rigidBodyA;

	int hitBodyIdx = rigidBodyA == hitRigidBody ? 0 : 1;

	RE::bhkRigidBody* hittingRigidBodyWrapper = reinterpret_cast<RE::bhkRigidBody*>(hittingRigidBody->userData);
	if (!hittingRigidBodyWrapper) {
		if (hittingRigidBody->collidable.broadPhaseHandle.objectQualityType == RE::hkpCollidableQualityType::kKeyframedReporting && !Utils::IsMoveableEntity(hitRigidBody)) {
			// It's not a hit, so disable contact for keyframed/fixed objects in this case
			a_event.contactPointProperties->flags |= RE::hkpContactPointProperties::kIsDisabled;
		}
		return;
	}

	RE::TESObjectREFR* attacker = hittingRigidBody->GetUserData();
	RE::TESObjectREFR* target = hitRigidBody->GetUserData();

	CollisionLayer hitLayer = hitRigidBody == rigidBodyA ? layerA : layerB;

	if (!attacker || attacker->formType != RE::FormType::ActorCharacter) {
		a_event.contactPointProperties->flags |= RE::hkpContactPointProperties::kIsDisabled;
		return;
	}

	if (!target && hitLayer != CollisionLayer::kGround) {
		if (hittingRigidBody->collidable.broadPhaseHandle.objectQualityType == RE::hkpCollidableQualityType::kKeyframedReporting && !Utils::IsMoveableEntity(hitRigidBody)) {
			// It's not a hit, so disable contact for keyframed/fixed objects in this case
			a_event.contactPointProperties->flags |= RE::hkpContactPointProperties::kIsDisabled;
		}
		return;
	}

	auto precisionHandler = PrecisionHandler::GetSingleton();

	auto attackerActor = attacker->As<RE::Actor>();
	auto attackerHandle = attackerActor->GetHandle();
	auto node = GetNiObjectFromCollidable(hittingRigidBody->GetCollidable());
	auto attackCollision = precisionHandler->GetAttackCollision(attackerHandle, node);
	RE::hkVector4 hkHitPos = a_event.contactPoint->position;

	RE::Actor* targetActor = target ? target->As<RE::Actor>() : nullptr;

	if (!attackCollision) {
		a_event.contactPointProperties->flags |= RE::hkpContactPointProperties::kIsDisabled;
		return;
	}

	RE::NiPoint3 niHitPos = Utils::HkVectorToNiPoint(hkHitPos) * *g_worldScaleInverse;

	RE::hkVector4 pointVelocity = hittingRigidBody->motion.GetPointVelocity(hkHitPos);

	if (pointVelocity.IsEqual(RE::hkVector4())) {  // point velocity is zero
		auto hittingNode = GetNiObjectFromCollidable(&hittingRigidBody->collidable);
		pointVelocity = GetParentNodePointVelocity(hittingNode, hkHitPos);
	}

	if (pointVelocity.IsEqual(RE::hkVector4())) {  // still zero, skip this collision
		a_event.contactPointProperties->flags |= RE::hkpContactPointProperties::kIsDisabled;
		return;
	}

	if (layerA == CollisionLayer::kPrecision && layerB == CollisionLayer::kPrecision) {
		if (precisionHandler->weaponWeaponCollisionCallbacks.size() > 0) {
			RE::hkpShapeKey* hittingBodyShapeKeysPtr = a_event.GetShapeKeys(hitBodyIdx ? 0 : 1);
			RE::hkpShapeKey* hitBodyShapeKeysPtr = a_event.GetShapeKeys(hitBodyIdx);
			RE::hkpShapeKey hittingBodyShapeKey = hittingBodyShapeKeysPtr ? *hittingBodyShapeKeysPtr : RE::HK_INVALID_SHAPE_KEY;
			RE::hkpShapeKey hitBodyShapeKey = hitBodyShapeKeysPtr ? *hitBodyShapeKeysPtr : RE::HK_INVALID_SHAPE_KEY;
			
			auto niSeparatingNormal = Utils::HkVectorToNiPoint(a_event.contactPoint->separatingNormal);
			RE::NiPoint3 niHitVelocity = Utils::HkVectorToNiPoint(pointVelocity) * *g_worldScaleInverse;
			PRECISION_API::PrecisionHitData precisionHitData(attackerActor, target, hitRigidBody, hittingRigidBody, niHitPos, niSeparatingNormal, niHitVelocity, hitBodyShapeKey, hittingBodyShapeKey);
			auto callbackReturns = precisionHandler->RunWeaponWeaponCollisionCallbacks(precisionHitData);

			for (auto& entry : callbackReturns) {
				if (entry.bIgnoreHit) {
					// abort hit
					a_event.contactPointProperties->flags |= RE::hkpContactPointProperties::kIsDisabled;
					return;
				}
			}
		} else {
			a_event.contactPointProperties->flags |= RE::hkpContactPointProperties::kIsDisabled;
			return;  // Disable weapon-weapon collisions
		}
	}

	if (hitLayer == CollisionLayer::kProjectile) {
		RE::hkVector4 projectileVelocity = hitRigidBody->motion.GetPointVelocity(hkHitPos);
		if (!projectileVelocity.IsEqual(RE::hkVector4())) {  // projectile velocity is not zero
			if (precisionHandler->weaponProjectileCollisionCallbacks.size() > 0) {			
				RE::hkpShapeKey* hittingBodyShapeKeysPtr = a_event.GetShapeKeys(hitBodyIdx ? 0 : 1);
				RE::hkpShapeKey* hitBodyShapeKeysPtr = a_event.GetShapeKeys(hitBodyIdx);
				RE::hkpShapeKey hittingBodyShapeKey = hittingBodyShapeKeysPtr ? *hittingBodyShapeKeysPtr : RE::HK_INVALID_SHAPE_KEY;
				RE::hkpShapeKey hitBodyShapeKey = hitBodyShapeKeysPtr ? *hitBodyShapeKeysPtr : RE::HK_INVALID_SHAPE_KEY;

				auto niSeparatingNormal = Utils::HkVectorToNiPoint(a_event.contactPoint->separatingNormal);
				RE::NiPoint3 niHitVelocity = Utils::HkVectorToNiPoint(pointVelocity) * *g_worldScaleInverse;
				PRECISION_API::PrecisionHitData precisionHitData(attackerActor, target, hitRigidBody, hittingRigidBody, niHitPos, niSeparatingNormal, niHitVelocity, hitBodyShapeKey, hittingBodyShapeKey);
				auto callbackReturns = precisionHandler->RunWeaponProjectileCollisionCallbacks(precisionHitData);

				for (auto& entry : callbackReturns) {
					if (entry.bIgnoreHit) {
						// abort hit
						a_event.contactPointProperties->flags |= RE::hkpContactPointProperties::kIsDisabled;
						return;
					}
				}
			} else {
				a_event.contactPointProperties->flags |= RE::hkpContactPointProperties::kIsDisabled;
				return;  // Disable weapon-moving projectile collisions
			}
		} 
	}
	
	auto feetPosition = attackerActor->GetPositionZ();

	if (!targetActor) {
		// if not actor, check actual weapon length if relevant
		// don't do this for player in first person
		bool bIsPlayer = attackerActor->IsPlayerRef();
		bool bIsFirstPerson = bIsPlayer && RE::PlayerCamera::GetSingleton()->IsInFirstPerson();

		auto hittingNode = GetNiObjectFromCollidable(&hittingRigidBody->collidable);

		float hitDistanceFromWeaponRoot = hittingNode->world.translate.GetDistance(niHitPos);

		if (!bIsFirstPerson) {
			float visualWeaponLength = attackCollision->GetVisualWeaponLength();
			if (hittingNode && visualWeaponLength > 0.f) {
				if (hitDistanceFromWeaponRoot > visualWeaponLength) {
					// skip collision if the contact point is farther away than visual weapon length
					a_event.contactPointProperties->flags |= RE::hkpContactPointProperties::kIsDisabled;
					return;
				}
			}
		}

		// check recoil
		if ((bIsPlayer ? Settings::bRecoilPlayer : Settings::bRecoilNPC) && !attackCollision->bNoRecoil && !Utils::IsMoveableEntity(hitRigidBody) && (!target || !target->GetBaseObject() || !target->GetBaseObject()->As<RE::BGSDestructibleObjectForm>())) {
			if (bIsPlayer || (attackerActor && attackerActor->IsInCombat())) {  // don't do recoils for NPCs out of combat because of them hitting things like training dummies and recoiling
				if (attackerActor && !attackerActor->IsInKillMove()) {          // don't do recoils while in killmove, or bad things happen
					int32_t rightWeaponType = 0;
					attackerActor->GetGraphVariableInt("iRightHandType", rightWeaponType);
					if (rightWeaponType > 0 && rightWeaponType < 7) {  // 1h & 2h melee
						bool bIsCloseEnough = bIsFirstPerson ? RE::PlayerCamera::GetSingleton()->cameraRoot->world.translate.GetDistance(niHitPos) <= Settings::fRecoilFirstPersonDistanceThreshold : hitDistanceFromWeaponRoot <= Settings::fRecoilThirdPersonDistanceThreshold;

						bool bIsBashing = attackerActor->GetAttackState() == RE::ATTACK_STATE_ENUM::kBash;

						bool bDoRecoil = bIsCloseEnough && !bIsBashing;

						if (bDoRecoil) {
							// skip recoil if contact point is close to feet level
							if (fabs(feetPosition - niHitPos.z) < Settings::fGroundFeetDistanceThreshold) {
								bDoRecoil = false;
							}
						}

						if (bDoRecoil) {
							auto& attackData = attackerActor->currentProcess->high->attackData;
							if (attackData) {
								if (!Settings::bRecoilPowerAttack && attackData->data.flags.any(RE::AttackData::AttackFlag::kPowerAttack)) {  // skip recoil if the attack is a power attack
									bDoRecoil = false;
								}
							}
						}

						if (bDoRecoil) {
							if (auto shape = hitRigidBody->GetShape()) {
								auto bhkShape = reinterpret_cast<RE::bhkShape*>(shape->userData);
								RE::MATERIAL_ID materialID = bhkShape->materialID;

								RE::hkpShapeKey* hitShapeKeys = a_event.GetShapeKeys(hitBodyIdx);
								if (hitShapeKeys && *hitShapeKeys != RE::HK_INVALID_SHAPE_KEY) {
									typedef RE::bhkCompressedMeshShape* (__thiscall RE::bhkShape::*Func34)() const;
									auto compressedMeshShape = (bhkShape->*reinterpret_cast<Func34>(&RE::bhkShape::Unk_34))();
									if (compressedMeshShape) {
										typedef RE::MATERIAL_ID (__thiscall RE::bhkCompressedMeshShape::*Func36)(
											RE::hkpShapeKey a_shapeKey) const;
										materialID =
											(compressedMeshShape->*reinterpret_cast<Func36>(&RE::bhkCompressedMeshShape::Unk_36))(*hitShapeKeys);
									}
								}

								auto materialType = GetBGSMaterialType(materialID);
								if (materialType && Settings::recoilMaterials.contains(materialType)) {
									bool bUseVanillaEvent = Settings::bUseVanillaRecoil || bIsFirstPerson;

									attackerActor->NotifyAnimationGraph(bIsFirstPerson ? Settings::firstPersonRecoilEvent : bUseVanillaEvent ? Settings::vanillaRecoilEvent :
                                                                                                                                               Settings::recoilEvent);

									if (Settings::bEnableRecoilCameraShake && attackerActor->IsPlayerRef()) {
										precisionHandler->ApplyCameraShake(
											Settings::fRecoilCameraShakeStrength, Settings::fRecoilCameraShakeDuration,
											Settings::fRecoilCameraShakeFrequency, 0.f);
									}

									if (Settings::bDebug && Settings::bDisplayRecoilCollisions) {
										constexpr glm::vec4 recoilColor{ 1, 0, 1, 1 };
										DrawHandler::AddPoint(niHitPos, 2.f, recoilColor, true);
									}

									precisionHandler->RemoveAttackCollision(attackerHandle, attackCollision);
								}
							}
						}
					}
				}
			}
		}

		// check ground shake
		if (attackCollision->groundShake && fabs(feetPosition - niHitPos.z) < Settings::fGroundFeetDistanceThreshold) {
			auto cameraPos = RE::PlayerCamera::GetSingleton()->cameraRoot->world.translate;		
			auto distanceSquared = cameraPos.GetSquaredDistance(niHitPos);

			precisionHandler->ApplyCameraShake(
				attackCollision->groundShake->x, attackCollision->groundShake->y,
				attackCollision->groundShake->z, distanceSquared);
		}
	}

	// filter out self for whatever reason
	if (targetActor == attackerActor) {
		a_event.contactPointProperties->flags |= RE::hkpContactPointProperties::kIsDisabled;
		return;
	}

	if (attackCollision->HasHitRef(target ? target->GetHandle() : RE::ObjectRefHandle())) {
		// refr has already been recently hit, so disable the contact point and gtfo
		a_event.contactPointProperties->flags |= RE::hkpContactPointProperties::kIsDisabled;
		return;
	}

	if (hitLayer == CollisionLayer::kCharController && target && target->formType == RE::FormType::ActorCharacter) {
		if (targetActor) {
			auto charController = targetActor->GetCharController();
			if (charController) {	
				if (!PrecisionHandler::IsCharacterControllerHittable(charController)) {
					a_event.contactPointProperties->flags |= RE::hkpContactPointProperties::kIsDisabled;
					return;
				}
			}
		}
	}

	// while in combat, filter out actors like teammates etc.
	if (targetActor && attackerActor && precisionHandler->CheckActorInCombat(attackerHandle)) {
		// don't let player hit their teammates or summons
		bool bAttackerIsPlayer = attackerActor->IsPlayerRef();
		bool bTargetIsPlayer = targetActor->IsPlayerRef();
		bool bAttackerIsTeammate = false;
		bool bTargetIsTeammate = false;
		if (!bAttackerIsPlayer) {
			bAttackerIsTeammate = Utils::IsPlayerTeammateOrSummon(attackerActor);
		}
		if (!bTargetIsPlayer) {
			bTargetIsTeammate = Utils::IsPlayerTeammateOrSummon(targetActor);
		}
		if (Settings::bNoPlayerTeammateAttackCollision && bAttackerIsPlayer && bTargetIsTeammate) {
			if (targetActor->currentCombatTarget != attackerActor->GetHandle()) {
				a_event.contactPointProperties->flags |= RE::hkpContactPointProperties::kIsDisabled;
				return;
			}
		}
		// don't let the player's teammates or summons hit the player
		if (Settings::bNoPlayerTeammateAttackCollision && bAttackerIsTeammate && bTargetIsPlayer) {
			if (attackerActor->currentCombatTarget != targetActor->GetHandle()) {
				a_event.contactPointProperties->flags |= RE::hkpContactPointProperties::kIsDisabled;
				return;
			}
		}
		// don't let the player's teammates hit each other
		if (Settings::bNoPlayerTeammateAttackCollision && bAttackerIsTeammate && bTargetIsTeammate) {
			if (attackerActor->currentCombatTarget != targetActor->GetHandle()) {
				a_event.contactPointProperties->flags |= RE::hkpContactPointProperties::kIsDisabled;
				return;
			}
		}

		// don't hit actors that aren't hostile and are in combat already
		if (Settings::bNoNonHostileAttackCollision && precisionHandler->CheckActorInCombat(targetActor->GetHandle()) && !targetActor->IsHostileToActor(attackerActor)) {
			a_event.contactPointProperties->flags |= RE::hkpContactPointProperties::kIsDisabled;
			return;
		}
	}

	// jump iframes
	if (Settings::bEnableJumpIframes && targetActor && Utils::GetBodyPartData(targetActor) == Settings::defaultBodyPartData) {  // do this only for humanoids
		if (auto charController = targetActor->GetCharController()) {
			if (charController->context.currentState == RE::hkpCharacterStateTypes::kJumping || charController->context.currentState == RE::hkpCharacterStateTypes::kInAir) {
				if (auto hitNode = GetNiObjectFromCollidable(hitRigidBody->GetCollidable())) {
					if (!Utils::IsNodeOrChildOfNode(hitNode, Settings::jumpIframeNode)) {
						a_event.contactPointProperties->flags |= RE::hkpContactPointProperties::kIsDisabled;

						if (Settings::bDebug && Settings::bDisplayIframeHits) {
							constexpr glm::vec4 blue{ 0.2, 0.2, 1.0, 1.0 };
							DrawHandler::AddPoint(niHitPos, 1.f, blue);
						}

						return;
					}
				}
			}
		}
	}

	// disable physical collision with actor
	if (targetActor || Settings::bDisablePhysicalCollisionOnHit) {
		a_event.contactPointProperties->flags |= RE::hkpContactPointProperties::kIsDisabled;
	}	

	// add to already hit refs so we don't hit the target again within the same attack
	attackCollision->AddHitRef(target ? target->GetHandle() : RE::ObjectRefHandle(), targetActor ? FLT_MAX : Settings::fHitSameRefCooldown, targetActor ? true : false);

	// sweep attack check
	if (Settings::uSweepAttackMode == SweepAttackMode::kMaxTargets) {
		bool bHasSweepPerk = Utils::IsSweepAttackActive(attackerActor->GetHandle());
		uint32_t maxTargets = bHasSweepPerk ? Settings::uMaxTargetsSweepAttack : Settings::uMaxTargetsNoSweepAttack;
		if (maxTargets > 0 && attackCollision->GetHitNPCCount() > maxTargets) {
			return;
		}
	}

	// do hitstop and camera shake
	if (Settings::bEnableHitstop || (Settings::bEnableHitstopCameraShake && attackerActor->IsPlayerRef())) {
		//uint32_t hitCount = targetActor ? attackCollision->GetHitNPCCount() : attackCollision->GetHitCount();
		uint32_t hitCount = targetActor ? precisionHandler->GetHitNPCCount(attackerHandle) : precisionHandler->GetHitCount(attackerHandle);

		bool bIsActorAlive = targetActor ? !targetActor->IsDead() : false;
		bool bIsPowerAttack = false;
		bool bIsTwoHanded = false;

		auto& attackData = attackerActor->currentProcess->high->attackData;
		if (attackData && attackData->data.flags.any(RE::AttackData::AttackFlag::kPowerAttack)) {
			bIsPowerAttack = true;
		}

		if (auto attackingObject = attackerActor->GetAttackingWeapon()) {
			if (auto attackingWeapon = attackingObject->object->As<RE::TESObjectWEAP>()) {
				if (attackingWeapon->GetWeaponType() == RE::WEAPON_TYPE::kTwoHandAxe || attackingWeapon->GetWeaponType() == RE::WEAPON_TYPE::kTwoHandSword) {
					bIsTwoHanded = true;
				}
			}
		}

		if (Settings::bEnableHitstop) {
			// skip hitstop if not hitting an actor and contact point is close to feet level
			if (targetActor || fabs(feetPosition - niHitPos.z) >= Settings::fGroundFeetDistanceThreshold) {
				float diminishingReturnsMultiplier = pow(Settings::fHitstopDurationDiminishingReturnsFactor, hitCount - 1);

				float hitstopLength = (bIsActorAlive ? Settings::fHitstopDurationNPC : Settings::fHitstopDurationOther) * diminishingReturnsMultiplier;

				if (bIsPowerAttack) {
					hitstopLength *= Settings::fHitstopDurationPowerAttackMultiplier;
				}

				if (bIsTwoHanded) {
					hitstopLength *= Settings::fHitstopDurationTwoHandedMultiplier;
				}

				precisionHandler->AddHitstop(attackerHandle, hitstopLength);
				if (Settings::bApplyHitstopToTarget && targetActor) {
					precisionHandler->AddHitstop(targetActor->GetHandle(), hitstopLength);
				}
			}
		}

		if (Settings::bEnableHitstopCameraShake && attackerActor->IsPlayerRef()) {
			/*RE::NiPoint3 niHitPos = Utils::HkVectorToNiPoint(hkHitPos) * *g_worldScaleInverse;
			ApplyCameraShake(targetActor ? Settings::fHitstopCameraShakeStrengthNPC : Settings::fHitstopCameraShakeStrengthOther, niHitPos, Settings::fHitstopCameraShakeLength);*/

			//*g_currentCameraShakeStrength = targetActor ? Settings::fHitstopCameraShakeStrengthNPC : Settings::fHitstopCameraShakeStrengthOther;

			// skip camera shake if not hitting an actor and contact point is close to feet level
			if (targetActor || fabs(feetPosition - niHitPos.z) >= Settings::fGroundFeetDistanceThreshold) {
				float diminishingReturnsMultiplier = pow(Settings::fHitstopCameraShakeDurationDiminishingReturnsFactor, hitCount - 1);

				float cameraShakeLength = (bIsActorAlive ? Settings::fHitstopCameraShakeDurationNPC : Settings::fHitstopCameraShakeDurationOther) * diminishingReturnsMultiplier;
				float cameraShakeStrength = (bIsActorAlive ? Settings::fHitstopCameraShakeStrengthNPC : Settings::fHitstopCameraShakeStrengthOther) * diminishingReturnsMultiplier;

				if (bIsPowerAttack) {
					cameraShakeStrength *= Settings::fHitstopCameraShakePowerAttackMultiplier;
					cameraShakeLength *= Settings::fHitstopCameraShakePowerAttackMultiplier;
				}

				if (bIsTwoHanded) {
					cameraShakeStrength *= Settings::fHitstopCameraShakeTwoHandedMultiplier;
					cameraShakeLength *= Settings::fHitstopCameraShakeTwoHandedMultiplier;
				}

				precisionHandler->ApplyCameraShake(cameraShakeStrength, cameraShakeLength, Settings::fHitstopCameraShakeFrequency, 0.f);
			}
		}
	}

	// do not damage actors in a killmove
	if (targetActor && targetActor->IsInKillMove()) {
		return;
	}

	if (Settings::bDebug && Settings::bDisplayHitLocations) {
		constexpr glm::vec4 red{ 1.0, 0.0, 0.0, 1.0 };
		DrawHandler::AddPoint(niHitPos, 1.f, red);
	}

	PrecisionHandler::pendingHits.emplace_back(attackerActor, target, hitRigidBody, hittingRigidBody, a_event, pointVelocity, hitBodyIdx, attackCollision);
}

void ContactListener::CollisionAddedCallback(const RE::hkpCollisionEvent& a_event)
{
	a_event;
	//throw std::logic_error("The method or operation is not implemented.");
}

void ContactListener::CollisionRemovedCallback(const RE::hkpCollisionEvent& a_event)
{
	a_event;
	//throw std::logic_error("The method or operation is not implemented.");
}
