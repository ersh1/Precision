#include "PendingHit.h"

#include "PrecisionHandler.h"
#include "Settings.h"
#include "Utils.h"
#include "render/DrawHandler.h"

void PendingHit::Run()
{
	if (bOnlyFX) {
		RunFXOnly();
		return;
	} else if (attackCollision && attackCollision->bIsRecoiling) {
		// Don't do anything, a hit with caused by a recoil is already queued
		return;
	}

	auto precisionHandler = PrecisionHandler::GetSingleton();

	RE::NiPoint3 niHitPos = Utils::HkVectorToNiPoint(contactPoint.position) * *g_worldScaleInverse;
	RE::NiPoint3 niSeparatingNormal = Utils::HkVectorToNiPoint(contactPoint.separatingNormal);
	RE::NiPoint3 niHitVelocity = Utils::HkVectorToNiPoint(hitPointVelocity) * *g_worldScaleInverse;
	//float hitSpeed = hitVelocity.Length();

	constexpr float additiveBias = 0.f;
	constexpr float multiplicativeBias = 1.f;

	float damageAdditive = additiveBias;
	float damageMultiplicative = multiplicativeBias;
	float staggerAdditive = additiveBias;
	float staggerMultiplicative = multiplicativeBias;

	RE::hkpRigidBody* hitRigidBody = static_cast<RE::hkpRigidBody*>(hitRigidBodyWrapper->referencedObject.get());
	RE::hkpRigidBody* hittingRigidBody = static_cast<RE::hkpRigidBody*>(hittingRigidBodyWrapper->referencedObject.get());

	auto originalHitRigidBody = hitRigidBody;

	RE::Actor* targetActor = target ? target->As<RE::Actor>() : nullptr;
	if (targetActor) {
		auto targetActorHandle = targetActor->GetHandle();
		// try to get real rigid body first in case it's a clone
		if (auto original = PrecisionHandler::GetOriginalFromClone(targetActorHandle, hitRigidBody)) {
			originalHitRigidBody = original;
		}
	}

	PRECISION_API::PrecisionHitData precisionHitData(attacker.get(), target.get(), originalHitRigidBody, hittingRigidBody, niHitPos, niSeparatingNormal, niHitVelocity, hitBodyShapeKey, hittingBodyShapeKey);

	// run pre hit callbacks
	if (precisionHandler->preHitCallbacks.size() > 0) {
		auto callbackReturns = precisionHandler->RunPreHitCallbacks(precisionHitData);

		for (auto& entry : callbackReturns) {
			if (entry.bIgnoreHit) {
				// abort hit
				return;
			}

			for (auto& modifier : entry.modifiers) {
				if (modifier.modifierOperation == PRECISION_API::PreHitModifier::ModifierOperation::Additive) {
					if (modifier.modifierType == PRECISION_API::PreHitModifier::ModifierType::Damage) {
						damageAdditive += modifier.modifierValue - additiveBias;
					} else {  // Stagger
						staggerAdditive += modifier.modifierValue - additiveBias;
					}
				} else {  // Multiplicative
					if (modifier.modifierType == PRECISION_API::PreHitModifier::ModifierType::Damage) {
						damageMultiplicative += modifier.modifierValue - multiplicativeBias;
					} else {  // Stagger
						staggerMultiplicative += modifier.modifierValue - multiplicativeBias;
					}
				}
			}
		}
	}

	// add modifiers from the attack collision
	damageMultiplicative += attackCollision->damageMult - multiplicativeBias;
	staggerMultiplicative += attackCollision->staggerMult - multiplicativeBias;

	// create an artificial hkpCdPoint to insert into the point collector
	RE::hkpCdPoint cdPoint;
	cdPoint.contact = contactPoint;
	RE::hkpCdBody cdBodyA{};
	RE::hkpCdBody cdBodyB{};

	cdBodyA.parent = &hittingRigidBody->collidable;
	cdBodyA.shapeKey = hittingBodyShapeKey;

	cdBodyB.parent = &hitRigidBody->collidable;
	cdBodyB.shapeKey = hitBodyShapeKey;

	cdPoint.cdBodyA = &cdBodyA;
	cdPoint.cdBodyB = &cdBodyB;

	RE::hkpAllCdPointCollector* allCdPointCollector = GetAllCdPointCollector(false, true);
	allCdPointCollector->Reset();
	allCdPointCollector->AddCdPoint(cdPoint);

	// calc final multipliers
	float damageMult = 1.f;
	float staggerMult = 1.f;

	damageMult = (damageMult + damageAdditive) * damageMultiplicative;
	staggerMult = (staggerMult + staggerAdditive) * staggerMultiplicative;

	auto attackerHandle = attacker->GetHandle();

	// sweep attack diminishing returns
	if (Settings::uSweepAttackMode == SweepAttackMode::kDiminishingReturns && !Utils::IsSweepAttackActive(attackerHandle)) {
		auto damagedCount = attackCollision->GetDamagedCount();
		float diminishingReturnsMultiplier = pow(Settings::fSweepAttackDiminishingReturnsFactor, damagedCount);
		damageMult *= diminishingReturnsMultiplier;
	}

	// Cache the precise vectors etc so they are used in the hooked functions instead of vanilla ones
	PrecisionHandler::cachedAttackData.SetPreciseHitVectors(niHitPos, niHitVelocity);
	PrecisionHandler::cachedAttackData.SetDamageMult(damageMult);
	PrecisionHandler::cachedAttackData.SetStaggerMult(staggerMult);
	PrecisionHandler::cachedAttackData.SetAttackingActorHandle(attackerHandle);
	PrecisionHandler::cachedAttackData.SetHittingNode(GetNiObjectFromCollidable(&hittingRigidBody->collidable));

	CalculateCurrentHitTargetForWeaponSwing(attacker.get());  // call this for vanilla stuff, skipping the actual collision check thanks to the hook (the collision is active)

	if (Settings::bDebug && Settings::bDisplayHitLocations) {
		constexpr glm::vec4 red{ 1.0, 0.0, 0.0, 1.0 };
		DrawHandler::AddPoint(niHitPos, 1.f, red);
	}

	// do hitstop and hitstop camera shake
	if (Settings::bEnableHitstop || (Settings::bEnableHitstopCameraShake && attacker->IsPlayerRef())) {
		//uint32_t hitCount = targetActor ? attackCollision->GetHitNPCCount() : attackCollision->GetHitCount();
		uint32_t hitCount = targetActor ? precisionHandler->GetHitNPCCount(attackerHandle) : precisionHandler->GetHitCount(attackerHandle);

		bool bIsActorAlive = targetActor ? !targetActor->IsDead() : false;
		bool bIsPowerAttack = false;
		bool bIsTwoHanded = false;

		auto& attackData = attacker->GetActorRuntimeData().currentProcess->high->attackData;
		if (attackData && attackData->data.flags.any(RE::AttackData::AttackFlag::kPowerAttack)) {
			bIsPowerAttack = true;
		}

		if (auto attackingObject = attacker->GetAttackingWeapon()) {
			if (auto attackingWeapon = attackingObject->object->As<RE::TESObjectWEAP>()) {
				if (attackingWeapon->GetWeaponType() == RE::WEAPON_TYPE::kTwoHandAxe || attackingWeapon->GetWeaponType() == RE::WEAPON_TYPE::kTwoHandSword) {
					bIsTwoHanded = true;
				}
			}
		}

		float feetPosition = attacker->GetPositionZ();

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

				PrecisionHandler::AddHitstop(attackerHandle, hitstopLength, false);
				if (Settings::bApplyHitstopToTarget && targetActor) {
					// don't apply to the player in first person because it feels weird
					bool bIsPlayer = targetActor->IsPlayerRef();
					bool bIsFirstPerson = bIsPlayer && Utils::IsFirstPerson();

					if (!bIsFirstPerson) {
						PrecisionHandler::AddHitstop(targetActor->GetHandle(), hitstopLength, true);
					}
				}
			}
		}

		if (Settings::bEnableHitstopCameraShake && attacker->IsPlayerRef()) {
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

	if (targetActor) {
		bool bIsLeftHand = attackCollision->nodeName == "SHIELD"sv;

		RE::HitData hitData;
		HitData_ctor(&hitData);

		auto attackerProcess = attacker->GetActorRuntimeData().currentProcess;

		if (precisionHandler->postHitCallbacks.size() > 0) {
			//auto currentWeapon = attacker->GetAttackingWeapon();
			if (attackerProcess) {
				auto currentWeapon = AIProcess_GetCurrentlyEquippedWeapon(attackerProcess, bIsLeftHand);
				HitData_Populate(&hitData, attacker.get(), targetActor, currentWeapon, bIsLeftHand);
				hitData.hitPosition = niHitPos;
				hitData.hitDirection = niHitVelocity;
			}
		}

		bool bIsDead = targetActor->IsDead();

		PrecisionHandler::cachedAttackData.SetLastHitNode(GetNiObjectFromCollidable(&hitRigidBody->collidable));

		DealDamage(attacker.get(), targetActor, nullptr, bIsLeftHand);

		attackCollision->IncreaseDamagedCount();

		// send hit events
		{
			float targetAngle = target->data.angle.z;
			constexpr RE::NiPoint3 forwardVector{ 0.f, 1.f, 0.f };
			constexpr RE::NiPoint3 upVector{ 0.f, 0.f, 1.f };
			RE::NiPoint3 targetForwardDirection = Utils::RotateAngleAxis(forwardVector, -targetAngle, upVector);
			RE::NiPoint3 targetRightDirection = targetForwardDirection.Cross(upVector);

			RE::NiPoint3 hitDirection = niHitVelocity;
			hitDirection.z = 0.f;
			hitDirection.Unitize();
			float forwardDot = targetForwardDirection.Dot(hitDirection);

			if (forwardDot > 0.707f) {
				// same dir as target, so hit from the back
				target->NotifyAnimationGraph("Precision_HitB");
				//logger::debug("Precision_HitB");
			} else if (forwardDot < -0.707f) {
				// opposite dir as target, so hit from the front
				target->NotifyAnimationGraph("Precision_HitF");
				//logger::debug("Precision_HitF");
			} else {
				// hit from the sides
				float rightDot = targetRightDirection.Dot(hitDirection);
				if (rightDot > 0.f) {
					// hit from the left
					target->NotifyAnimationGraph("Precision_HitL");
					//logger::debug("Precision_HitL");
				} else {
					// hit from the right
					target->NotifyAnimationGraph("Precision_HitR");
					//logger::debug("Precision_HitR");
				}
			}
		}

		bool bJustKilled = !bIsDead && targetActor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth) <= 0.f;

		if (Settings::bApplyImpulseOnHit && attacker && attackerProcess && attackerProcess->high) {
			if (!bJustKilled || Settings::bApplyImpulseOnKill) {
				float impulseMult = Settings::fHitImpulseBaseMult;
				auto& attackData = attackerProcess->high->attackData;
				if (attackData && attackData->data.flags.any(RE::AttackData::AttackFlag::kPowerAttack)) {
					impulseMult *= Settings::fHitImpulsePowerAttackMult;
				}

				bool bIsInRagdollState = targetActor->IsInRagdollState();
				bool bIsBlocking = targetActor->IsBlocking();

				if (bIsInRagdollState) {
					impulseMult *= Settings::fHitImpulseRagdollMult;
				}

				if (bJustKilled) {
					impulseMult *= Settings::fHitImpulseKillMult;
				}

				if (bIsBlocking) {
					impulseMult *= Settings::fHitImpulseBlockMult;
				}

				if (!bIsInRagdollState || bIsDead) {  // don't apply impulse to ragdolled alive targets because they won't be able to get up when regularly hit
					bool bIsActiveRagdoll = !bIsDead && !bJustKilled;

					auto targetHandle = targetActor->GetHandle();

					bool bAttackerIsPlayer = attacker->IsPlayerRef();
					precisionHandler->ApplyHitImpulse(targetHandle, originalHitRigidBody, niHitVelocity, contactPoint.position, impulseMult, bIsActiveRagdoll, bAttackerIsPlayer, false);
				}
			}
		}

		if (Settings::bDebug && Settings::bDisplayHitNodeCollisions) {
			constexpr glm::vec4 green{ 0.0, 1.0, 0.0, 1.0 };

			auto node = RE::NiPointer<RE::NiAVObject>(GetNiObjectFromCollidable(hitRigidBody->GetCollidable()));
			DrawHandler::AddCollider(node, 1.f, green);
		}

		// run post hit callbacks
		if (precisionHandler->postHitCallbacks.size() > 0) {
			precisionHandler->RunPostHitCallbacks(precisionHitData, hitData);
		}
	}

	PrecisionHandler::cachedAttackData.Clear();

	RemoveMagicEffectsDueToAction(attacker.get(), -1);
}

void PendingHit::RunFXOnly()
{
	RE::NiPoint3 niHitPos = Utils::HkVectorToNiPoint(contactPoint.position) * *g_worldScaleInverse;
	RE::NiPoint3 niHitVelocity = Utils::HkVectorToNiPoint(hitPointVelocity) * *g_worldScaleInverse;

	RE::hkpRigidBody* hitRigidBody = static_cast<RE::hkpRigidBody*>(hitRigidBodyWrapper->referencedObject.get());
	RE::hkpRigidBody* hittingRigidBody = static_cast<RE::hkpRigidBody*>(hittingRigidBodyWrapper->referencedObject.get());

	// create an artificial hkpCdPoint to insert into the point collector
	RE::hkpCdPoint cdPoint;
	cdPoint.contact = contactPoint;
	RE::hkpCdBody cdBodyA{};
	RE::hkpCdBody cdBodyB{};

	cdBodyA.parent = &hittingRigidBody->collidable;
	cdBodyA.shapeKey = hittingBodyShapeKey;

	cdBodyB.parent = &hitRigidBody->collidable;
	cdBodyB.shapeKey = hitBodyShapeKey;

	cdPoint.cdBodyA = &cdBodyA;
	cdPoint.cdBodyB = &cdBodyB;

	RE::hkpAllCdPointCollector* allCdPointCollector = GetAllCdPointCollector(false, true);
	allCdPointCollector->Reset();
	allCdPointCollector->AddCdPoint(cdPoint);

	// Cache the precise vectors etc so they are used in the hooked functions instead of vanilla ones
	PrecisionHandler::cachedAttackData.SetPreciseHitVectors(niHitPos, niHitVelocity);
	PrecisionHandler::cachedAttackData.SetAttackingActorHandle(attacker->GetHandle());

	CalculateCurrentHitTargetForWeaponSwing(attacker.get());  // call this for vanilla stuff, skipping the actual collision check thanks to the hook (the collision is active)

	if (Settings::bDebug && Settings::bDisplayRecoilCollisions) {
		constexpr glm::vec4 recoilColor{ 1, 0, 1, 1 };
		DrawHandler::AddPoint(niHitPos, 2.f, recoilColor, true);
	}

	PrecisionHandler::cachedAttackData.Clear();

	RemoveMagicEffectsDueToAction(attacker.get(), -1);
}
