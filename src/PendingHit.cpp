#include "PendingHit.h"

#include "PrecisionHandler.h"
#include "Settings.h"
#include "Utils.h"
#include "render/DrawHandler.h"

void PendingHit::DoHit()
{
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

	PRECISION_API::PrecisionHitData precisionHitData(attacker.get(), target.get(), hitRigidBody.get(), hittingRigidBody.get(), niHitPos, niSeparatingNormal, niHitVelocity, hitBodyShapeKey, hittingBodyShapeKey);

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

	// sweep attack diminishing returns
	if (Settings::uSweepAttackMode == SweepAttackMode::kDiminishingReturns && !Utils::IsSweepAttackActive(attacker->GetHandle())) {
		auto damagedCount = attackCollision->GetDamagedCount();
		float diminishingReturnsMultiplier = pow(Settings::fSweepAttackDiminishingReturnsFactor, damagedCount);
		damageMult *= diminishingReturnsMultiplier;
	}

	// Cache the precise vectors etc so they are used in the hooked functions instead of vanilla ones
	PrecisionHandler::cachedAttackData.SetPreciseHitVectors(niHitPos, niHitVelocity);
	PrecisionHandler::cachedAttackData.SetDamageMult(damageMult);
	PrecisionHandler::cachedAttackData.SetStaggerMult(staggerMult);
	PrecisionHandler::cachedAttackData.SetAttackingActorHandle(attacker->GetHandle());
	PrecisionHandler::cachedAttackData.SetHittingNode(GetNiObjectFromCollidable(&hittingRigidBody->collidable));

	CalculateCurrentHitTargetForWeaponSwing(attacker.get());  // call this for vanilla stuff, skipping the actual collision check thanks to the hook (the collision is active)

	RE::Actor* targetActor = target ? target->As<RE::Actor>() : nullptr;
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
					precisionHandler->ApplyHitImpulse(target->GetHandle(), hitRigidBody.get(), niHitVelocity, contactPoint.position, impulseMult, bIsActiveRagdoll);
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
