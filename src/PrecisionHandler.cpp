#include "PrecisionHandler.h"
#include "Offsets.h"
#include "Settings.h"
#include "Utils.h"
#include "render/DrawHandler.h"

#include <ranges>

void PrecisionHandler::AddPlayerSink()
{
	auto playerCharacter = RE::PlayerCharacter::GetSingleton();
	RE::BSAnimationGraphManagerPtr graphManager;
	playerCharacter->GetAnimationGraphManager(graphManager);
	if (graphManager) {
		for (const auto& animationGraph : graphManager->graphs) {
			animationGraph->GetEventSource<RE::BSAnimationGraphEvent>()->AddEventSink(PrecisionHandler::GetSingleton());
		}
	}
	playerCharacter->NotifyAnimationGraph("Collision_WeightResetEnd"sv);
}

void PrecisionHandler::RemovePlayerSink()
{
	auto playerCharacter = RE::PlayerCharacter::GetSingleton();
	RE::BSAnimationGraphManagerPtr graphManager;
	playerCharacter->GetAnimationGraphManager(graphManager);
	if (graphManager) {
		for (const auto& animationGraph : graphManager->graphs) {
			animationGraph->GetEventSource<RE::BSAnimationGraphEvent>()->RemoveEventSink(PrecisionHandler::GetSingleton());
		}
	}
}

bool PrecisionHandler::AddActorSink(RE::Actor* a_actor)
{
	bool ret = a_actor->AddAnimationGraphEventSink(PrecisionHandler::GetSingleton());
	a_actor->NotifyAnimationGraph("Collision_WeightResetEnd"sv);
	return ret;
}

void PrecisionHandler::RemoveActorSink(RE::Actor* a_actor)
{
	a_actor->RemoveAnimationGraphEventSink(PrecisionHandler::GetSingleton());
}

constexpr char charToLower(const char c)
{
	return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

constexpr uint32_t hash(const char* data, size_t const size) noexcept
{
	uint32_t hash = 5381;

	for (const char* c = data; c < data + size; ++c)
		hash = ((hash << 5) + hash) + charToLower(*c);

	return hash;
}

constexpr uint32_t operator"" _h(const char* str, size_t size) noexcept
{
	return hash(str, size);
}

PrecisionHandler::EventResult PrecisionHandler::ProcessEvent(const RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource)
{
	if (!a_event || !a_event->holder || !Settings::bAttackCollisionsEnabled || Settings::bDisableMod) {
		return EventResult::kContinue;
	}

	auto actor = const_cast<RE::Actor*>(static_cast<const RE::Actor*>(a_event->holder));

	RE::BSAnimationGraphManagerPtr graphManager;
	actor->GetAnimationGraphManager(graphManager);
	if (!graphManager) {
		return EventResult::kContinue;
	}

	uint32_t activeGraphIdx = graphManager->GetRuntimeData().activeGraph;

	if (graphManager->graphs[activeGraphIdx] && graphManager->graphs[activeGraphIdx].get() != a_eventSource) {
		return EventResult::kContinue;
	}

	if (actor->IsInKillMove()) {
		return EventResult::kContinue;
	}

	std::string_view eventTag = a_event->tag.data();

	/*if (actor == RE::PlayerCharacter::GetSingleton()) {
		logger::debug("{}", a_event->tag);
	}*/

	switch (hash(eventTag.data(), eventTag.size())) {
	case "Collision_AttackStart"_h:
	case "Collision_Start"_h:
		{
			StartCollision(actor->GetHandle(), activeGraphIdx);
			break;
		}
	case "SoundPlay"_h:
		if (a_event->payload != "NPCHumanCombatShieldBashSD"sv) {  // crossbow bash
			break;
		}
	case "SoundPlay.WPNSwingUnarmed"_h:
	case "SoundPlay.NPCHumanCombatShieldBash"_h:
	case "SoundPlay.NPCHumanCombatShieldBashPower"_h:
	case "weaponSwing"_h:
	case "weaponLeftSwing"_h:
	case "preHitFrame"_h:
	case "CastOKStart"_h:
	case "CastOKStop"_h:
		{
			// only do this if we haven't received a Collision_Start event (vanilla)
			auto actorHandle = actor->GetHandle();
			if (!HasStartedPrecisionCollision(actorHandle, activeGraphIdx)) {
				bool bIsWPNSwingUnarmed = a_event->tag == "SoundPlay.WPNSwingUnarmed";
				AttackDefinition::SwingEvent swingEvent = AttackDefinition::SwingEvent::kWeaponSwing;
				if (a_event->tag == "preHitFrame") {
					swingEvent = AttackDefinition::SwingEvent::kPreHitFrame;
				} else if (a_event->tag == "CastOKStart") {
					swingEvent = AttackDefinition::SwingEvent::kCastOKStart;
				} else if (a_event->tag == "CastOKStop") {
					swingEvent = AttackDefinition::SwingEvent::kCastOKStop;
				}
				if ((bIsWPNSwingUnarmed && !HasStartedDefaultCollisionWithWeaponSwing(actorHandle)) || (!bIsWPNSwingUnarmed && !HasStartedDefaultCollisionWithWPNSwingUnarmed(actorHandle))) {
					AttackDefinition attackDefinition;

					std::optional<bool> bIsLeftSwing = std::nullopt;
					if (!bIsWPNSwingUnarmed) {
						bIsLeftSwing = a_event->tag == "weaponLeftSwing"sv;
					}
					if (GetAttackCollisionDefinition(actor, attackDefinition, bIsLeftSwing, swingEvent)) {
						if (bIsWPNSwingUnarmed) {
							SetStartedDefaultCollisionWithWPNSwingUnarmed(actorHandle);
						} else {
							SetStartedDefaultCollisionWithWeaponSwing(actorHandle);
						}

						AddAttack(actor->GetHandle(), attackDefinition);
					}
				}
			}
			break;
		}
	case "hitFrame"_h:
		{
			if ((Settings::bRecoilPlayer || Settings::bRecoilNPC) && Settings::bRemoveRecoilOnHitframe) {
				RemoveRecoilCollision(actor->GetHandle());
			}
			break;
		}
	case "Collision_Add"_h:
		{
			if (HasStartedPrecisionCollision(actor->GetHandle(), activeGraphIdx)) {
				CollisionDefinition collisionDefinition;
				if (ParseCollisionEvent(a_event, CollisionEventType::kAdd, collisionDefinition)) {
					AddAttackCollision(actor->GetHandle(), collisionDefinition);
				}
			}
			break;
		}
	case "Collision_Remove"_h:
		{
			if (HasStartedPrecisionCollision(actor->GetHandle(), activeGraphIdx)) {
				CollisionDefinition collisionDefinition;
				if (ParseCollisionEvent(a_event, CollisionEventType::kRemove, collisionDefinition)) {
					RemoveAttackCollision(actor->GetHandle(), collisionDefinition);
				}
			}
			break;
		}
	case "Collision_ClearTargets"_h:
		{
			auto actorHandle = actor->GetHandle();
			if (HasStartedPrecisionCollision(actorHandle, activeGraphIdx)) {
				CollisionDefinition collisionDefinition;
				if (ParseCollisionEvent(a_event, CollisionEventType::kClearTargets, collisionDefinition)) {
					if (collisionDefinition.ID) {
						ClearIDHitRefs(actorHandle, *collisionDefinition.ID);
					} else {
						auto attackCollision = GetAttackCollision(actorHandle, collisionDefinition.nodeName);
						if (attackCollision) {
							attackCollision->ClearHitRefs();
						}
					}
				}
			}
			break;
		}
	case "Collision_AttackEnd"_h:
	case "attackStop"_h:
	case "staggerStart"_h:
	case "Collision_Cancel"_h:
		{
			//ClearHitRefs(actor->GetHandle());
			//remove all collisions to be safe
			RemoveAllAttackCollisions(actor->GetHandle());
			break;
		}
	}

	return EventResult::kContinue;
}

void PrecisionHandler::Update(float a_deltaTime)
{
	/*if (!Settings::bAttackCollisionsEnabled || Settings::bDisableMod) {
		Clear();
	}*/

	if (RE::UI::GetSingleton()->GameIsPaused()) {
		return;
	}

	Settings::UpdateGlobals();

	ProcessMainUpdateJobs();
	ProcessDelayedJobs(a_deltaTime);

	// Update active actors
	{
		ReadLocker locker(activeActorsLock);
		for (auto& [handle, activeActor] : activeActors) {
			activeActor->Update(a_deltaTime);
		}
	}

	// do pending hits
	{
		ReadLocker locker(pendingHitsLock);
		for (auto& pendingHit : pendingHits) {
			pendingHit.Run();
		}
	}

	{
		WriteLocker locker(pendingHitsLock);
		pendingHits.clear();
	}

	// update trails
	for (auto it = _attackTrails.begin(); it != _attackTrails.end();) {
		if (!it->get()->Update(a_deltaTime)) {
			it = _attackTrails.erase(it);
		} else {
			++it;
		}
	}

	if (bCameraShakeActive) {
		if (currentCameraShakeTimer > 0.f) {
			currentCameraShake = sinf((currentCameraShakeLength - currentCameraShakeTimer) * currentCameraShakeFrequency) * currentCameraShakeStrength * fmax(currentCameraShakeLength - (currentCameraShakeLength - currentCameraShakeTimer), 0.f);
			currentCameraShakeTimer -= a_deltaTime;
		} else {
			bCameraShakeActive = false;
			currentCameraShake = 0.f;
			currentCameraShakeTimer = 0.f;
			currentCameraShakeStrength = 0.f;
			currentCameraShakeLength = 0.f;
			currentCameraShakeFrequency = 0.f;
		}
	}
}

void PrecisionHandler::ApplyHitImpulse(RE::ObjectRefHandle a_refHandle, RE::hkpRigidBody* a_rigidBody, const RE::NiPoint3& a_hitVelocity, const RE::hkVector4& a_hitPosition, float a_impulseMult, bool a_bIsActiveRagdoll, bool a_bAttackerIsPlayer, bool a_bIsDeferred /*= false */)
{
	if (!a_refHandle) {
		return;
	}

	auto refr = a_refHandle.get();

	if (!a_bIsDeferred) {
		if (auto actor = refr->As<RE::Actor>()) {
			auto actorHandle = actor->GetHandle();
			if (!PrecisionHandler::IsRagdollAdded(actorHandle)) {
				// Don't add ragdoll to the player if in first person
				if (actor->IsPlayerRef() && Utils::IsFirstPerson()) {
					return;
				}

				{
					WriteLocker locker(ragdollsToAddLock);
					ragdollsToAdd.emplace(actorHandle);
				}

				QueuePostHavokHitJob<DeferredImpulseJob>(a_refHandle, a_rigidBody, a_hitVelocity, a_hitPosition, a_impulseMult, a_bIsActiveRagdoll, a_bAttackerIsPlayer);

				return;
			}
		}
	}

	// Apply linear impulse at the center of mass to all bodies within 3 ragdoll constraints
	Utils::ForEachRagdollDriver(refr.get(), [=](RE::hkbRagdollDriver* driver) {
		auto ragdoll = GetActiveRagdollFromDriver(driver);
		if (!ragdoll) {
			return;
		}

		ragdoll->impulseTime = Settings::fRagdollImpulseTime;

		Utils::ForEachAdjacentBody(driver, a_rigidBody, [=](RE::hkpRigidBody* adjacentBody) {
			QueuePrePhysicsJob<LinearImpulseJob>(adjacentBody, a_refHandle, a_hitVelocity, a_impulseMult * Settings::fHitImpulseDecayMult1, a_bIsActiveRagdoll, a_bAttackerIsPlayer);
			Utils::ForEachAdjacentBody(driver, adjacentBody, [=](RE::hkpRigidBody* adjacentBody) {
				QueuePrePhysicsJob<LinearImpulseJob>(adjacentBody, a_refHandle, a_hitVelocity, a_impulseMult * Settings::fHitImpulseDecayMult2, a_bIsActiveRagdoll, a_bAttackerIsPlayer);
				Utils::ForEachAdjacentBody(driver, adjacentBody, [=](RE::hkpRigidBody* adjacentBody) {
					QueuePrePhysicsJob<LinearImpulseJob>(adjacentBody, a_refHandle, a_hitVelocity, a_impulseMult * Settings::fHitImpulseDecayMult3, a_bIsActiveRagdoll, a_bAttackerIsPlayer);
				});
			});
		});
	});

	// Apply a point impulse at the hit location to the body we actually hit
	QueuePrePhysicsJob<PointImpulseJob>(a_rigidBody, a_refHandle, a_hitVelocity, a_hitPosition, a_impulseMult, a_bIsActiveRagdoll, a_bAttackerIsPlayer);
}

void PrecisionHandler::AddHitstop(RE::ActorHandle a_actorHandle, float a_hitstopLength, bool a_bReceived)
{
	if (Settings::bEnableHitstop) {
		if (auto activeActor = GetActiveActor(a_actorHandle)) {
			activeActor->AddHitstop(a_hitstopLength, a_bReceived);
		}
	}
}

void PrecisionHandler::UpdateHitstop(RE::ActorHandle a_actorHandle, float a_deltaTime)
{
	if (Settings::bEnableHitstop) {
		if (auto activeActor = GetActiveActor(a_actorHandle)) {
			activeActor->UpdateHitstop(a_deltaTime);
		}
	}
}

float PrecisionHandler::GetHitstopMultiplier(RE::ActorHandle a_actorHandle, float a_deltaTime)
{
	if (Settings::bEnableHitstop) {
		if (auto activeActor = GetActiveActor(a_actorHandle)) {
			return activeActor->GetHitstopMultiplier(a_deltaTime);
		}
	}

	return 1.f;
}

float PrecisionHandler::GetDriveToPoseHitstopMultiplier(RE::ActorHandle a_actorHandle, float a_deltaTime)
{
	if (Settings::bEnableHitstop) {
		if (auto activeActor = GetActiveActor(a_actorHandle)) {
			return activeActor->GetDriveToPoseHitstopMultiplier(a_deltaTime);
		}
	}

	return 1.f;
}

void PrecisionHandler::ApplyCameraShake(float a_strength, float a_length, float a_frequency, float a_distanceSquared)
{
	// do a inverse-square falloff for shake strength
	if (a_distanceSquared > 0.f) {
		float strengthMult = 1.f;
		if (a_distanceSquared < Settings::cameraShakeRadiusSquared) {
			strengthMult = 1 - (a_distanceSquared / Settings::cameraShakeRadiusSquared);
		}
		a_strength *= strengthMult;
	}

	if (a_strength > 0.f) {
		if (currentCameraShakeStrength / a_strength < 0.8f) {
			bCameraShakeActive = true;
			currentCameraShakeStrength = a_strength;
			currentCameraShakeLength = a_length;
			currentCameraShakeFrequency = a_frequency;
			currentCameraShakeTimer = a_length;
		}
	}
}

bool PrecisionHandler::CalculateHitImpulse(RE::hkpRigidBody* a_rigidBody, const RE::NiPoint3& a_hitVelocity, float a_impulseMult, bool a_bIsActiveRagdoll, bool a_bAttackerIsPlayer, RE::NiPoint3& a_outImpulse)
{
	if (a_rigidBody->motion.type == RE::hkpMotion::MotionType::kKeyframed) {
		return false;
	}

	float massInv = a_rigidBody->motion.inertiaAndMassInv.quad.m128_f32[3];
	float mass = massInv <= 0.001f ? 99999.f : 1.f / massInv;

	float impulseStrength = std::clamp(
		Settings::fHitImpulseBaseStrength + Settings::fHitImpulseProportionalStrength * powf(mass, Settings::fHitImpulseMassExponent),
		Settings::fHitImpulseMinStrength, Settings::fHitImpulseMaxStrength);

	a_outImpulse = a_hitVelocity;
	float impulseSpeed = a_outImpulse.Unitize();

	if (a_bIsActiveRagdoll) {
		auto owner = RE::TESHavokUtilities::FindCollidableRef(a_rigidBody->collidable);
		if (owner && owner->formType == RE::FormType::ActorCharacter) {
			// scale back the impulse if the hit node is at ground level (don't apply strong impulse to feet for living actors)
			auto bhkRb = reinterpret_cast<RE::bhkRigidBody*>(a_rigidBody->userData);
			RE::hkAabb aabb;
			bhkRb->GetAabbWorldspace(aabb);

			auto niMin = Utils::HkVectorToNiPoint(aabb.min) * *g_worldScaleInverse;

			float feetPositionZ = owner->GetPosition().z;  // character position is at their feet

			float dist = niMin.z - feetPositionZ;
			if (dist < Settings::fHitImpulseFeetDistanceThreshold) {
				impulseSpeed *= std::fmax(0.f, Utils::Remap(dist, 0.f, Settings::fHitImpulseFeetDistanceThreshold, 0.f, 1.f));
			}
		}
	}

	float globalTimeMultiplier = *g_globalTimeMultiplier;

	if (a_bAttackerIsPlayer) {
		globalTimeMultiplier *= Utils::GetPlayerTimeMultiplier();
	}

	if (globalTimeMultiplier <= 0.f) {
		globalTimeMultiplier = 1.f;
	}

	impulseSpeed = fmin(impulseSpeed, (Settings::fHitImpulseMaxVelocity / globalTimeMultiplier));  // limit the imparted velocity to some reasonable value
	a_outImpulse *= impulseSpeed * *g_worldScale * mass;                                           // This impulse will give the object the exact velocity it is hit with
	a_outImpulse *= impulseStrength;                                                               // Scale the velocity as we see fit
	a_outImpulse *= a_impulseMult;

	if (a_outImpulse.z < 0) {
		// Impulse points downwards somewhat, scale back the downward component so we don't get things shooting into the ground.
		a_outImpulse.z *= Settings::fHitImpulseDownwardsMultiplier;
	}

	return true;
}

void PrecisionHandler::StartCollision(RE::ActorHandle a_actorHandle, uint32_t a_activeGraphIdx)
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		activeActor->attackCollisions.ignoreVanillaAttackEvents = a_activeGraphIdx;
	}
}

bool PrecisionHandler::AddAttack(RE::ActorHandle a_actorHandle, const AttackDefinition& a_attackDefinition)
{
	auto actor = a_actorHandle.get();
	if (!actor) {
		return false;
	}

	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		for (auto& collisionDef : a_attackDefinition.collisions) {
			if (collisionDef.delay) {
				float weaponSpeedMult = 1.f;
				actor->GetGraphVariableFloat("weaponSpeedMult"sv, weaponSpeedMult);
				if (weaponSpeedMult == 0.f) {
					weaponSpeedMult = 1.f;
				}
				float delay = *collisionDef.delay;
				delay /= weaponSpeedMult;

				QueueDelayedJob<DelayedAttackCollisionJob>(delay, a_actorHandle, collisionDef);
			} else {
				activeActor->attackCollisions.AddAttackCollision(a_actorHandle, collisionDef);
			}
		}

		return true;
	}

	return false;
}

void PrecisionHandler::AddAttackCollision(RE::ActorHandle a_actorHandle, const CollisionDefinition& a_collisionDefinition)
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->attackCollisions.AddAttackCollision(a_actorHandle, a_collisionDefinition);
	}
}

bool PrecisionHandler::RemoveRecoilCollision(RE::ActorHandle a_actorHandle)
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->attackCollisions.RemoveRecoilCollision();
	}

	return false;
}

bool PrecisionHandler::RemoveAttackCollision(RE::ActorHandle a_actorHandle, std::shared_ptr<AttackCollision> a_attackCollision)
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->attackCollisions.RemoveAttackCollision(a_attackCollision);
	}

	return false;
}

bool PrecisionHandler::RemoveAttackCollision(RE::ActorHandle a_actorHandle, const CollisionDefinition& a_collisionDefinition)
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->attackCollisions.RemoveAttackCollision(a_collisionDefinition);
	}

	return false;
}

bool PrecisionHandler::RemoveAllAttackCollisions(RE::ActorHandle a_actorHandle)
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->attackCollisions.RemoveAllAttackCollisions();
	}

	return false;
}

std::shared_ptr<AttackCollision> PrecisionHandler::GetAttackCollision(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_node) const
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->attackCollisions.GetAttackCollision(a_node);
	}

	return nullptr;
}

std::shared_ptr<AttackCollision> PrecisionHandler::GetAttackCollision(RE::ActorHandle a_actorHandle, std::string_view a_nodeName) const
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->attackCollisions.GetAttackCollision(a_nodeName);
	}

	return nullptr;
}

std::shared_ptr<AttackCollision> PrecisionHandler::GetAttackCollisionFromRecoilNode(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_node) const
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->attackCollisions.GetAttackCollisionFromRecoilNode(a_node);
	}

	return nullptr;
}

std::shared_ptr<ActiveActor> PrecisionHandler::GetActiveActor(RE::ActorHandle a_actorHandle)
{
	ReadLocker locker(activeActorsLock);

	auto search = PrecisionHandler::activeActors.find(a_actorHandle);
	if (search != PrecisionHandler::activeActors.end()) {
		return search->second;
	}

	return nullptr;
}

bool PrecisionHandler::ActorHasAttackCollision(RE::ActorHandle a_actorHandle)
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return !activeActor->attackCollisions.IsEmpty();
	}

	return false;
}

bool PrecisionHandler::HasStartedDefaultCollisionWithWeaponSwing(RE::ActorHandle a_actorHandle)
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->attackCollisions.bStartedWithWeaponSwing;
	}

	return false;
}

bool PrecisionHandler::HasStartedDefaultCollisionWithWPNSwingUnarmed(RE::ActorHandle a_actorHandle)
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->attackCollisions.bStartedWithWPNSwingUnarmed;
	}

	return false;
}

bool PrecisionHandler::HasStartedPrecisionCollision(RE::ActorHandle a_actorHandle, uint32_t a_activeGraphIdx)
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->attackCollisions.ignoreVanillaAttackEvents && activeActor->attackCollisions.ignoreVanillaAttackEvents == a_activeGraphIdx;
	}

	return false;
}

bool PrecisionHandler::HasHitstop(RE::ActorHandle a_actorHandle)
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->HasHitstop();
	}

	return false;
}

bool PrecisionHandler::HasActiveImpulse(RE::ActorHandle a_actorHandle)
{
	bool bHasImpulse = false;

	auto actor = a_actorHandle.get();
	if (actor) {
		Utils::ForEachRagdollDriver(actor.get(), [&](RE::hkbRagdollDriver* a_driver) {
			if (auto activeRagdoll = GetActiveRagdollFromDriver(a_driver)) {
				if (activeRagdoll->IsImpulseActive()) {
					bHasImpulse = true;
				}
			}
		});
	}

	return bHasImpulse;
}

void PrecisionHandler::SetStartedDefaultCollisionWithWeaponSwing(RE::ActorHandle a_actorHandle)
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		activeActor->attackCollisions.bStartedWithWeaponSwing = true;
	}
}

void PrecisionHandler::SetStartedDefaultCollisionWithWPNSwingUnarmed(RE::ActorHandle a_actorHandle)
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		activeActor->attackCollisions.bStartedWithWPNSwingUnarmed = true;
	}
}

bool PrecisionHandler::HasHitRef(RE::ActorHandle a_actorHandle, RE::ObjectRefHandle a_handle) const
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->attackCollisions.HasHitRef(a_handle);
	}

	return false;
}

uint32_t PrecisionHandler::GetHitCount(RE::ActorHandle a_actorHandle) const
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->attackCollisions.GetHitCount();
	}

	return 0;
}

uint32_t PrecisionHandler::GetHitNPCCount(RE::ActorHandle a_actorHandle) const
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->attackCollisions.GetHitNPCCount();
	}

	return 0;
}

bool PrecisionHandler::HasIDHitRef(RE::ActorHandle a_actorHandle, uint8_t a_ID, RE::ObjectRefHandle a_handle) const
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->attackCollisions.HasIDHitRef(a_ID, a_handle);
	}

	return false;
}

void PrecisionHandler::AddIDHitRef(RE::ActorHandle a_actorHandle, uint8_t a_ID, RE::ObjectRefHandle a_handle, float a_duration, bool a_bIsNPC)
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->attackCollisions.AddIDHitRef(a_ID, a_handle, a_duration, a_bIsNPC);
	}
}

void PrecisionHandler::ClearIDHitRefs(RE::ActorHandle a_actorHandle, uint8_t a_ID)
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		activeActor->attackCollisions.ClearIDHitRefs(a_ID);
	}
}

void PrecisionHandler::IncreaseIDDamagedCount(RE::ActorHandle a_actorHandle, uint8_t a_ID)
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		activeActor->attackCollisions.IncreaseIDDamagedCount(a_ID);
	}
}

uint32_t PrecisionHandler::GetIDHitCount(RE::ActorHandle a_actorHandle, uint8_t a_ID) const
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->attackCollisions.GetIDHitCount(a_ID);
	}

	return 0;
}

uint32_t PrecisionHandler::GetIDHitNPCCount(RE::ActorHandle a_actorHandle, uint8_t a_ID) const
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->attackCollisions.GetIDHitNPCCount(a_ID);
	}

	return 0;
}

uint32_t PrecisionHandler::GetIDDamagedCount(RE::ActorHandle a_actorHandle, uint8_t a_ID) const
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->attackCollisions.GetIDDamagedCount(a_ID);
	}

	return 0;
}

void PrecisionHandler::Initialize()
{
}

void PrecisionHandler::Clear()
{
	{
		WriteLocker locker(activeCollisionGroupsLock);
		activeCollisionGroups.clear();
	}

	{
		WriteLocker locker(activeActorsLock);
		activeActors.clear();
	}

	{
		WriteLocker locker(hittableCharControllerGroupsLock);
		hittableCharControllerGroups.clear();
	}

	{
		WriteLocker locker(disabledActorsLock);
		disabledActors.clear();
	}

	{
		WriteLocker locker(activeRagdollsLock);
		activeRagdolls.clear();
		activeActorsWithRagdolls.clear();
	}

	{
		WriteLocker locker(ragdollCollisionGroupsLock);
		ragdollCollisionGroups.clear();
	}

	{
		WriteLocker locker(pendingHitsLock);
		pendingHits.clear();
	}

	{
		WriteLocker locker(ragdollsToAddLock);
		WriteLocker locker2(ragdollsToRemoveLock);
		ragdollsToAdd.clear();
		ragdollsToRemove.clear();
	}

	_attackTrails.clear();

	PrecisionHandler::contactListener = ContactListener{};

	weaponMeshLengthMap.clear();
}

void PrecisionHandler::OnPreLoadGame()
{
	Clear();
}

void PrecisionHandler::OnPostLoadGame()
{
}

void PrecisionHandler::ProcessPrePhysicsStepJobs()
{
	WriteLocker locker(prePhysicsStepJobsLock);

	for (auto it = _prePhysicsStepJobs.begin(); it != _prePhysicsStepJobs.end();) {
		if (it->get()->Run()) {
			it = _prePhysicsStepJobs.erase(it);
		} else {
			++it;
		}
	}
}

void PrecisionHandler::ProcessMainUpdateJobs()
{
	WriteLocker locker(mainUpdateJobsLock);

	for (auto it = _mainUpdateJobs.begin(); it != _mainUpdateJobs.end();) {
		if (it->get()->Run()) {
			it = _mainUpdateJobs.erase(it);
		} else {
			++it;
		}
	}
}

void PrecisionHandler::ProcessPostHavokHitJobs()
{
	WriteLocker locker(postHavokHitJobsLock);

	for (auto it = _postHavokHitJobs.begin(); it != _postHavokHitJobs.end();) {
		if (it->get()->Run()) {
			it = _postHavokHitJobs.erase(it);
		} else {
			++it;
		}
	}
}

void PrecisionHandler::ProcessDelayedJobs(float a_deltaTime)
{
	WriteLocker locker(delayedJobsLock);

	for (auto it = _delayedJobs.begin(); it != _delayedJobs.end();) {
		if (it->get()->Run(a_deltaTime)) {
			it = _delayedJobs.erase(it);
		} else {
			++it;
		}
	}
}

bool PrecisionHandler::GetAttackCollisionDefinition(RE::Actor* a_actor, AttackDefinition& a_outAttackDefinition, std::optional<bool> a_bIsLeftSwing /*= std::nullopt*/, AttackDefinition::SwingEvent a_swingEvent /*= AttackDefinition::SwingEvent::kWeaponSwing*/) const
{
	using AttackAnimationDefinitionsMap = std::unordered_map<std::string, std::unordered_map<std::string, AttackDefinition>>;
	using AttackRaceDefinitionsMap = std::unordered_map<RE::BGSBodyPartData*, std::unordered_map<std::string, AttackDefinition>>;

	if (!a_actor) {
		return false;
	}

	// try finding a matching animation definition first
	RE::BSFixedString projectName;
	RE::hkStringPtr animName;
	float animTime;

	if (Utils::GetActiveAnim(a_actor, projectName, animName, animTime)) {
		std::string projectNameStr = projectName.data();
		std::transform(projectNameStr.begin(), projectNameStr.end(), projectNameStr.begin(), [](unsigned char c) { return (unsigned char)std::tolower(c); });

		AttackAnimationDefinitionsMap* definitionsMap = nullptr;
		switch (a_swingEvent) {
		case AttackDefinition::SwingEvent::kWeaponSwing:
			definitionsMap = &Settings::attackAnimationDefinitions;
			break;
		case AttackDefinition::SwingEvent::kPreHitFrame:
			definitionsMap = &Settings::attackAnimationDefinitionsPreHitFrame;
			break;
		case AttackDefinition::SwingEvent::kCastOKStart:
			definitionsMap = &Settings::attackAnimationDefinitionsCastOKStart;
			break;
		case AttackDefinition::SwingEvent::kCastOKStop:
			definitionsMap = &Settings::attackAnimationDefinitionsCastOKStop;
			break;
		}

		auto search = definitionsMap->find(projectNameStr);

		if (search != definitionsMap->end()) {
			auto& attackDefinitions = search->second;

			std::string animNameStr = animName.data();
			std::transform(animNameStr.begin(), animNameStr.end(), animNameStr.begin(), [](unsigned char c) { return (unsigned char)std::tolower(c); });
			std::replace(animNameStr.begin(), animNameStr.end(), '\\', '/');

			auto defIt = attackDefinitions.find(animNameStr);
			if (defIt != attackDefinitions.end()) {
				a_outAttackDefinition = defIt->second;
				return true;
			}
		}
	}

	// if not found, fall back to the usual attack event definitions
	auto race = a_actor->GetRace();
	if (!race) {
		return false;
	}

	auto bodyPartData = Utils::GetBodyPartData(a_actor);
	if (!bodyPartData) {
		return false;
	}

	AttackRaceDefinitionsMap* definitionsMap = nullptr;
	switch (a_swingEvent) {
	case AttackDefinition::SwingEvent::kWeaponSwing:
		definitionsMap = &Settings::attackRaceDefinitions;
		break;
	case AttackDefinition::SwingEvent::kPreHitFrame:
		definitionsMap = &Settings::attackRaceDefinitionsPreHitFrame;
		break;
	case AttackDefinition::SwingEvent::kCastOKStart:
		definitionsMap = &Settings::attackRaceDefinitionsCastOKStart;
		break;
	case AttackDefinition::SwingEvent::kCastOKStop:
		definitionsMap = &Settings::attackRaceDefinitionsCastOKStop;
		break;
	}

	auto search = definitionsMap->find(bodyPartData);

	if (search == definitionsMap->end()) {
		return false;
	}

	auto& attackDefinitions = search->second;

	auto& attackData = Actor_GetAttackData(a_actor);

	if (a_bIsLeftSwing.has_value()) {
		if (attackData && attackData->IsLeftAttack() != a_bIsLeftSwing) {
			attackData = GetOppositeAttackEvent(attackData, race->attackDataMap.get());
		}
	}

	std::string_view attackEvent;
	if (attackData) {
		attackEvent = attackData->event;
	} else {
		attackEvent = "DEFAULT_UNARMED"sv;
		auto currentProcess = a_actor->GetActorRuntimeData().currentProcess;
		if (currentProcess && currentProcess->middleHigh) {
			auto equipment = a_bIsLeftSwing ? currentProcess->middleHigh->leftHand : currentProcess->middleHigh->rightHand;
			if (equipment && equipment->object) {
				if (auto weapon = equipment->object->As<RE::TESObjectWEAP>()) {
					if (weapon && weapon->weaponData.animationType != RE::WEAPON_TYPE::kHandToHandMelee) {
						attackEvent = "DEFAULT"sv;
					}
				}
			}
		}
	}

	auto defIt = attackDefinitions.find(attackEvent.data());
	if (defIt == attackDefinitions.end()) {
		return false;
	}

	a_outAttackDefinition = defIt->second;
	return true;
}

bool PrecisionHandler::ParseCollisionEvent(const RE::BSAnimationGraphEvent* a_event, [[maybe_unused]] CollisionEventType a_eventType, CollisionDefinition& a_outCollisionDefinition) const
{
	auto& payload = a_event->payload;

	auto parameters = Utils::Tokenize(payload.c_str(), '|');

	if (parameters.empty()) {
		logger::error("Invalid collision event: {}", a_event->tag);
		return false;
	}

	const auto parseStringParameter = [](std::string_view& a_parameter, std::string_view& a_outStringView) {
		auto start = a_parameter.find('(') + 1;
		auto end = a_parameter.find(')');
		a_outStringView = a_parameter.substr(start, end - start);
		return a_outStringView.length() > 0;
	};

	const auto parseIntParameter = [](std::string_view& a_parameter, uint8_t& a_outInt) {
		auto start = a_parameter.find('(');
		auto end = a_parameter.find(')');
		auto intString = a_parameter.substr(start + 1, end);
		auto result = std::from_chars(intString.data(), intString.data() + intString.size(), a_outInt);
		return result.ec == std::errc();
	};

	const auto parseFloatParameter = [](std::string_view& a_parameter, float& a_outFloat) {
		auto start = a_parameter.find('(');
		auto end = a_parameter.find(')');
		auto floatString = a_parameter.substr(start + 1, end);
		auto result = std::from_chars(floatString.data(), floatString.data() + floatString.size(), a_outFloat);
		return result.ec == std::errc();
	};

	const auto parseNiPoint3Parameter = [](std::string_view& a_parameter, RE::NiPoint3& a_outNiPoint3) {
		auto start = a_parameter.find('(') + 1;
		auto xsplit = a_parameter.find(',', start) + 1;
		auto ysplit = a_parameter.find(',', xsplit) + 1;
		auto end = a_parameter.find(')', ysplit) + 1;
		auto xString = a_parameter.substr(start, xsplit - start - 1);
		xString.remove_prefix(std::min(xString.find_first_not_of(" "), xString.size()));
		auto yString = a_parameter.substr(xsplit, ysplit - xsplit - 1);
		yString.remove_prefix(std::min(yString.find_first_not_of(" "), yString.size()));
		auto zString = a_parameter.substr(ysplit, end - ysplit - 1);
		zString.remove_prefix(std::min(zString.find_first_not_of(" "), zString.size()));

		auto result = std::from_chars(xString.data(), xString.data() + xString.size(), a_outNiPoint3.x);
		if (result.ec != std::errc()) {
			return false;
		}
		result = std::from_chars(yString.data(), yString.data() + yString.size(), a_outNiPoint3.y);
		if (result.ec != std::errc()) {
			return false;
		}
		result = std::from_chars(zString.data(), zString.data() + zString.size(), a_outNiPoint3.z);
		return result.ec == std::errc();
	};

	const auto parseNiColorAParameter = [](std::string_view& a_parameter, RE::NiColorA& a_outNiColorA) {
		auto start = a_parameter.find('(') + 1;
		auto rsplit = a_parameter.find(',', start) + 1;
		auto gsplit = a_parameter.find(',', rsplit) + 1;
		auto bsplit = a_parameter.find(',', gsplit) + 1;
		auto end = a_parameter.find(')', bsplit) + 1;
		auto rString = a_parameter.substr(start, rsplit - start - 1);
		rString.remove_prefix(std::min(rString.find_first_not_of(" "), rString.size()));
		auto gString = a_parameter.substr(rsplit, gsplit - rsplit - 1);
		gString.remove_prefix(std::min(gString.find_first_not_of(" "), gString.size()));
		auto bString = a_parameter.substr(gsplit, bsplit - gsplit - 1);
		bString.remove_prefix(std::min(bString.find_first_not_of(" "), bString.size()));
		auto aString = a_parameter.substr(bsplit, end - bsplit - 1);
		aString.remove_prefix(std::min(aString.find_first_not_of(" "), aString.size()));

		auto result = std::from_chars(rString.data(), rString.data() + rString.size(), a_outNiColorA.red);
		if (result.ec != std::errc()) {
			return false;
		}
		result = std::from_chars(gString.data(), gString.data() + gString.size(), a_outNiColorA.green);
		if (result.ec != std::errc()) {
			return false;
		}
		result = std::from_chars(bString.data(), bString.data() + bString.size(), a_outNiColorA.blue);
		if (result.ec != std::errc()) {
			return false;
		}
		result = std::from_chars(aString.data(), aString.data() + aString.size(), a_outNiColorA.alpha);
		return result.ec == std::errc();
	};

	// parse optional parameters
	for (auto& parameter : parameters) {
		auto nameEnd = parameter.find('(');
		auto parameterName = parameter.substr(0, nameEnd);
		switch (hash(parameterName.data(), parameterName.size())) {
		case "node"_h:
			{
				std::string_view parsedString;
				if (parseStringParameter(parameter, parsedString)) {
					a_outCollisionDefinition.nodeName = parsedString;
				} else {
					logger::error("Invalid {} event payload - node name could not be parsed - {}.{}", a_event->tag, a_event->tag, a_event->payload);
					return false;
				}
				break;
			}
		case "id"_h:
			{
				uint8_t parsedInt;
				if (parseIntParameter(parameter, parsedInt)) {
					a_outCollisionDefinition.ID = parsedInt;
				} else {
					logger::error("Invalid {} event payload - ID could not be parsed - {}.{}", a_event->tag, a_event->tag, a_event->payload);
					return false;
				}
				break;
			}
		case "noRecoil"_h:
			{
				a_outCollisionDefinition.bNoRecoil = true;
				break;
			}
		case "noTrail"_h:
			{
				a_outCollisionDefinition.bNoTrail = true;
				break;
			}
		case "trailUseTrueLength"_h:
			{
				a_outCollisionDefinition.bTrailUseTrueLength = true;
				break;
			}
		case "weaponTip"_h:
			{
				a_outCollisionDefinition.bWeaponTip = true;
				break;
			}
		case "damageMult"_h:
			{
				float parsedFloat;
				if (parseFloatParameter(parameter, parsedFloat)) {
					a_outCollisionDefinition.damageMult = parsedFloat;
				} else {
					logger::error("Invalid {} event payload - damageMult could not be parsed - {}.{}", a_event->tag, a_event->tag, a_event->payload);
					return false;
				}
				break;
			}
		case "duration"_h:
			{
				float parsedFloat;
				if (parseFloatParameter(parameter, parsedFloat)) {
					a_outCollisionDefinition.duration = parsedFloat;
				} else {
					logger::error("Invalid {} event payload - duration could not be parsed - {}.{}", a_event->tag, a_event->tag, a_event->payload);
					return false;
				}
				break;
			}
		case "radius"_h:
			{
				float parsedFloat;
				if (parseFloatParameter(parameter, parsedFloat)) {
					a_outCollisionDefinition.capsuleRadius = parsedFloat;
				} else {
					logger::error("Invalid {} event payload - radius could not be parsed - {}.{}", a_event->tag, a_event->tag, a_event->payload);
					return false;
				}
				break;
			}
		case "radiusMult"_h:
			{
				float parsedFloat;
				if (parseFloatParameter(parameter, parsedFloat)) {
					a_outCollisionDefinition.radiusMult = parsedFloat;
				} else {
					logger::error("Invalid {} event payload - radius mult could not be parsed - {}.{}", a_event->tag, a_event->tag, a_event->payload);
					return false;
				}
				break;
			}
		case "length"_h:
			{
				float parsedFloat;
				if (parseFloatParameter(parameter, parsedFloat)) {
					a_outCollisionDefinition.capsuleLength = parsedFloat;
				} else {
					logger::error("Invalid {} event payload - length could not be parsed - {}.{}", a_event->tag, a_event->tag, a_event->payload);
					return false;
				}
				break;
			}
		case "lengthMult"_h:
			{
				float parsedFloat;
				if (parseFloatParameter(parameter, parsedFloat)) {
					a_outCollisionDefinition.lengthMult = parsedFloat;
				} else {
					logger::error("Invalid {} event payload - length mult could not be parsed - {}.{}", a_event->tag, a_event->tag, a_event->payload);
					return false;
				}
				break;
			}
		case "rotation"_h:
			{
				RE::NiPoint3 parsedNiPoint3;
				if (parseNiPoint3Parameter(parameter, parsedNiPoint3)) {
					parsedNiPoint3.x = Utils::DegreeToRadian(parsedNiPoint3.x);
					parsedNiPoint3.y = Utils::DegreeToRadian(parsedNiPoint3.y);
					parsedNiPoint3.z = Utils::DegreeToRadian(parsedNiPoint3.z);
					if (!a_outCollisionDefinition.transform) {
						a_outCollisionDefinition.transform = RE::NiTransform();
					}
					a_outCollisionDefinition.transform->rotate.SetEulerAnglesXYZ(parsedNiPoint3);
				} else {
					logger::error("Invalid {} event payload - rotation could not be parsed - {}.{}", a_event->tag, a_event->tag, a_event->payload);
					return false;
				}
				break;
			}
		case "translation"_h:
			{
				RE::NiPoint3 parsedNiPoint3;
				if (parseNiPoint3Parameter(parameter, parsedNiPoint3)) {
					if (!a_outCollisionDefinition.transform) {
						a_outCollisionDefinition.transform = RE::NiTransform();
					}
					a_outCollisionDefinition.transform->translate = parsedNiPoint3;
				} else {
					logger::error("Invalid {} event payload - translation could not be parsed - {}.{}", a_event->tag, a_event->tag, a_event->payload);
					return false;
				}
				break;
			}
		case "scale"_h:
			{
				float parsedFloat;
				if (parseFloatParameter(parameter, parsedFloat)) {
					if (!a_outCollisionDefinition.transform) {
						a_outCollisionDefinition.transform = RE::NiTransform();
					}
					a_outCollisionDefinition.transform->scale = parsedFloat;
				} else {
					logger::error("Invalid {} event payload - scale could not be parsed - {}.{}", a_event->tag, a_event->tag, a_event->payload);
					return false;
				}
				break;
			}
		case "groundShake"_h:
			{
				RE::NiPoint3 parsedNiPoint3;
				if (parseNiPoint3Parameter(parameter, parsedNiPoint3)) {
					a_outCollisionDefinition.groundShake = parsedNiPoint3;
				} else {
					logger::error("Invalid {} event payload - ground shake could not be parsed - {}.{}", a_event->tag, a_event->tag, a_event->payload);
					return false;
				}
				break;
			}
		case "trailLifetimeMult"_h:
			{
				float parsedFloat;
				if (parseFloatParameter(parameter, parsedFloat)) {
					if (!a_outCollisionDefinition.trailOverride) {
						a_outCollisionDefinition.trailOverride = TrailOverride();
					}
					a_outCollisionDefinition.trailOverride->lifetimeMult = parsedFloat;
				} else {
					logger::error("Invalid {} event payload - trail lifetime mult could not be parsed - {}.{}", a_event->tag, a_event->tag, a_event->payload);
					return false;
				}
				break;
			}
		case "trailBaseColorOverride"_h:
			{
				RE::NiColorA parsedNiColorA;
				if (parseNiColorAParameter(parameter, parsedNiColorA)) {
					if (!a_outCollisionDefinition.trailOverride) {
						a_outCollisionDefinition.trailOverride = TrailOverride();
					}
					a_outCollisionDefinition.trailOverride->baseColorOverride = parsedNiColorA;
				} else {
					logger::error("Invalid {} event payload - trail base color override could not be parsed - {}.{}", a_event->tag, a_event->tag, a_event->payload);
					return false;
				}
				break;
				break;
			}
		case "trailBaseColorScaleMult"_h:
			{
				float parsedFloat;
				if (parseFloatParameter(parameter, parsedFloat)) {
					if (!a_outCollisionDefinition.trailOverride) {
						a_outCollisionDefinition.trailOverride = TrailOverride();
					}
					a_outCollisionDefinition.trailOverride->baseColorScaleMult = parsedFloat;
				} else {
					logger::error("Invalid {} event payload - trail base color scale mult could not be parsed - {}.{}", a_event->tag, a_event->tag, a_event->payload);
					return false;
				}
				break;
			}
		case "trailMeshOverride"_h:
			{
				std::string_view parsedString;
				if (parseStringParameter(parameter, parsedString)) {
					if (!a_outCollisionDefinition.trailOverride) {
						a_outCollisionDefinition.trailOverride = TrailOverride();
					}
					a_outCollisionDefinition.trailOverride->meshOverride = parsedString;
				} else {
					logger::error("Invalid {} event payload - trail mesh override could not be parsed - {}.{}", a_event->tag, a_event->tag, a_event->payload);
					return false;
				}
				break;
			}
		}
	}

	switch (a_eventType) {
	case CollisionEventType::kAdd:
		if (a_outCollisionDefinition.nodeName == ""sv) {
			logger::error("Invalid Collision_Add event payload: node name is missing - {}.{}", a_event->tag, a_event->payload);
			return false;
		}
		break;
	case CollisionEventType::kRemove:
		if (a_outCollisionDefinition.nodeName == ""sv && !a_outCollisionDefinition.ID) {
			logger::error("Invalid Collision_Remove event payload: node name and ID are missing - {}.{}", a_event->tag, a_event->payload);
			return false;
		}
		break;
	case CollisionEventType::kClearTargets:
		if (a_outCollisionDefinition.nodeName == "sv" && !a_outCollisionDefinition.ID) {
			logger::error("Invalid Collision_ClearTargets event payload: node name and ID are missing - {}.{}", a_event->tag, a_event->payload);
			return false;
		}
		break;
	}

	return true;
}

bool PrecisionHandler::CheckActorInCombat(RE::ActorHandle a_actorHandle)
{
	if (auto activeActor = GetActiveActor(a_actorHandle)) {
		return activeActor->CheckInCombat();
	}

	if (auto actor = a_actorHandle.get()) {
		return actor->IsInCombat();
	}

	return false;
}

bool PrecisionHandler::HasJumpIframes(RE::Actor* a_actor)
{
	if (Settings::bEnableJumpIframes && a_actor && !a_actor->IsInRagdollState() && Utils::GetBodyPartData(a_actor) == Settings::defaultBodyPartData) {  // do this only for humanoids
		if (auto charController = a_actor->GetCharController()) {
			if (charController->context.currentState == RE::hkpCharacterStateTypes::kJumping || charController->context.currentState == RE::hkpCharacterStateTypes::kInAir) {
				return true;
			}
		}
	}

	return false;
}

float PrecisionHandler::GetWeaponMeshLength(RE::NiAVObject* a_weaponNode)
{
	if (a_weaponNode) {
		RE::NiBound modelBound = Utils::GetModelBounds(a_weaponNode);
		float offset = fabs(modelBound.center.y);
		return (modelBound.radius + offset);
	}

	return 0.f;
}

float PrecisionHandler::GetWeaponAttackLength(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_weaponNode, RE::TESObjectWEAP* a_weapon, std::optional<float> a_overrideLength /*= std::nullopt*/, float a_lengthMult /*= 1.f*/)
{
	auto actor = a_actorHandle.get();
	if (!actor) {
		return Settings::fMinWeaponLength * GetAttackLengthMult(nullptr) * a_lengthMult;
	}

	float length = 0.f;
	if (a_overrideLength) {
		length = *a_overrideLength;
		length *= actor->GetScale();  // apply actor scale
	} else {
		if (!TryGetCachedWeaponMeshReach(actor.get(), a_weapon, length)) {  // this applies actor scale already
			length = GetWeaponMeshLength(a_weaponNode);
		}
	}

	if (length > 0.f) {
		if (actor->IsPlayerRef() && Utils::IsFirstPerson()) {
			length = fmax(length + Settings::fFirstPersonAttackLengthOffset, 0.f);
		}

		float mult = GetAttackLengthMult(actor.get()) * a_lengthMult;

		return fmax(length * mult, Settings::fMinWeaponLength * mult);
	}

	return Settings::fMinWeaponLength * GetAttackLengthMult(actor.get()) * a_lengthMult;
}

float PrecisionHandler::GetWeaponAttackRadius(RE::ActorHandle a_actorHandle, RE::TESObjectWEAP* a_weapon, std::optional<float> a_overrideRadius /*= std::nullopt*/, float a_radiusMult /*= 1.f*/)
{
	float radius = Settings::fWeaponCapsuleRadius;

	auto actor = a_actorHandle.get();
	if (!actor) {
		return radius;
	}

	if (a_overrideRadius) {
		radius = *a_overrideRadius;
	} else {
		// search overrides
		auto search = Settings::weaponRadiusOverrides.find(a_weapon);
		if (search != Settings::weaponRadiusOverrides.end()) {
			radius = search->second;
		}
	}

	// apply actor scale
	radius *= actor->GetScale();

	float mult = GetAttackRadiusMult(actor.get()) * a_radiusMult;

	return fmax(radius * mult, Settings::fMinWeaponRadius);
}

const RE::hkpCapsuleShape* PrecisionHandler::GetNodeCapsuleShape(RE::NiAVObject* a_node)
{
	if (a_node->collisionObject) {
		auto collisionObject = static_cast<RE::bhkCollisionObject*>(a_node->collisionObject.get());
		auto rigidBody = collisionObject->GetRigidBody();

		if (rigidBody && rigidBody->referencedObject) {
			RE::hkpRigidBody* hkpRigidBody = static_cast<RE::hkpRigidBody*>(rigidBody->referencedObject.get());
			const RE::hkpShape* hkpShape = hkpRigidBody->collidable.shape;
			if (hkpShape->type == RE::hkpShapeType::kCapsule) {
				auto hkpCapsuleShape = static_cast<const RE::hkpCapsuleShape*>(hkpShape);

				return hkpCapsuleShape;
			}
		}
	}

	return nullptr;
}

bool PrecisionHandler::GetNodeAttackDimensions(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_node, std::optional<float> a_overrideLength, float a_lengthMult, std::optional<float> a_overrideRadius, float a_radiusMult, RE::hkVector4& a_outVertexA, RE::hkVector4& a_outVertexB, float& a_outRadius)
{
	auto actor = a_actorHandle.get();
	if (!actor) {
		return false;
	}

	float actorScale = actor->GetScale();

	float length = 0.f;
	float radius = 0.f;

	if (auto capsuleShape = GetNodeCapsuleShape(a_node)) {
		a_outVertexA = capsuleShape->vertexA;
		a_outVertexB = capsuleShape->vertexB;
		a_outRadius = capsuleShape->radius;
	}

	if (a_overrideRadius) {
		radius = *a_overrideRadius * *g_worldScale;
	} else {
		radius = a_outRadius;
	}

	float originalLength = a_outVertexA.GetDistance3(a_outVertexB);
	originalLength = fmax(radius * 2.f, length);
	length = originalLength;

	float finalRadiusMult = a_radiusMult;
	float finalLengthMult = a_lengthMult;

	if (a_overrideLength) {
		length = *a_overrideLength * *g_worldScale;
	}

	if (actor->IsPlayerRef() && Utils::IsFirstPerson()) {
		length += Settings::fFirstPersonAttackLengthOffset * *g_worldScale;
		length = fmax(length, 0.f);
	}

	if (length > 0.f && originalLength > 0.f) {
		finalLengthMult *= length / originalLength;
	}

	finalLengthMult *= GetAttackLengthMult(actor.get());
	finalRadiusMult *= GetAttackRadiusMult(actor.get());

	a_outVertexA = a_outVertexA * actorScale * finalLengthMult;
	a_outVertexB = a_outVertexB * actorScale;
	a_outRadius = a_outRadius * actorScale * finalRadiusMult;

	return true;
}

float PrecisionHandler::GetAttackLengthMult(RE::Actor* a_actor)
{
	float mult = Settings::fWeaponLengthMult;

	if (a_actor) {
		if (a_actor->IsPlayerRef()) {
			mult *= Settings::fPlayerAttackLengthMult;
		}

		if (a_actor->IsOnMount()) {
			mult *= Settings::fMountedAttackLengthMult;
		}
	}

	return mult;
}

float PrecisionHandler::GetAttackRadiusMult(RE::Actor* a_actor)
{
	float mult = 1.f;

	if (a_actor) {
		if (a_actor->IsPlayerRef()) {
			mult *= Settings::fPlayerAttackRadiusMult;
		}

		if (a_actor->IsOnMount()) {
			mult *= Settings::fMountedAttackRadiusMult;
		}
	}

	return mult;
}

bool PrecisionHandler::GetInventoryWeaponReach(RE::Actor* a_actor, RE::TESBoundObject* a_object, float& a_outReach)
{
	if (a_actor) {
		if (auto& currentProcess = a_actor->GetActorRuntimeData().currentProcess) {
			if (currentProcess->cachedValues) {
				float forwardOffset = currentProcess->cachedValues->cachedForwardLength;
				float scale = a_actor->GetScale();
				if (!a_object || a_object == g_unarmedWeapon) {
					float lengthMult = GetAttackLengthMult(a_actor) * scale;
					a_outReach = (Settings::fMinWeaponLength + Settings::fWeaponLengthUnarmedOffset + Settings::fAIWeaponReachOffset) * lengthMult + forwardOffset;
					return true;
				} else if (auto weapon = a_object->As<RE::TESObjectWEAP>()) {
					if (weapon->IsMelee()) {
						if (PrecisionHandler::TryGetCachedWeaponMeshReach(a_actor, weapon, a_outReach)) {
							float lengthMult = GetAttackLengthMult(a_actor) * scale;
							a_outReach = (fmax(a_outReach, Settings::fMinWeaponLength) + Settings::fAIWeaponReachOffset) * lengthMult + forwardOffset;
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

std::shared_ptr<ActiveRagdoll> PrecisionHandler::GetActiveRagdollFromDriver(RE::hkbRagdollDriver* a_driver)
{
	ReadLocker locker(activeRagdollsLock);

	auto search = activeRagdolls.find(a_driver);
	if (search != activeRagdolls.end()) {
		return search->second;
	}

	return nullptr;
}

void PrecisionHandler::CacheWeaponMeshReach(RE::TESObjectWEAP* a_weapon, RE::NiAVObject* a_object)
{
	WriteLocker locker(PrecisionHandler::weaponMeshLengthLock);

	if (!weaponMeshLengthMap.contains(a_weapon)) {
		weaponMeshLengthMap.try_emplace(a_weapon, GetWeaponMeshLength(a_object));
	}
}

bool PrecisionHandler::GetCachedWeaponMeshReach(RE::TESObjectWEAP* a_weapon, float& a_outReach)
{
	ReadLocker locker(PrecisionHandler::weaponMeshLengthLock);

	auto search = weaponMeshLengthMap.find(a_weapon);
	if (search != weaponMeshLengthMap.end()) {
		a_outReach = search->second;
		return true;
	}

	return false;
}

bool PrecisionHandler::TryGetCachedWeaponMeshReach(RE::Actor* a_actor, RE::TESObjectWEAP* a_weapon, float& a_outReach)
{
	if (!a_weapon || !a_actor) {
		return false;
	}

	{
		ReadLocker locker(PrecisionHandler::weaponMeshLengthLock);

		auto search = weaponMeshLengthMap.find(a_weapon);
		if (search != weaponMeshLengthMap.end()) {
			a_outReach = search->second * a_actor->GetScale();

			return true;
		}
	}

	// search overrides first
	auto search = Settings::weaponLengthOverrides.find(a_weapon);
	if (search != Settings::weaponLengthOverrides.end()) {
		float reach = search->second;

		{
			WriteLocker locker(PrecisionHandler::weaponMeshLengthLock);
			weaponMeshLengthMap.emplace(a_weapon, reach);
		}

		a_outReach = reach * a_actor->GetScale();

		return true;
	}

	if (auto bipedAnim = a_actor->GetBiped(false)) {
		auto& bipObject = bipedAnim->objects[g_weaponTypeToBipedObject[a_weapon->weaponData.animationType.underlying()]];

		if (bipObject.part == a_weapon->As<RE::TESModelTextureSwap>() || (a_weapon->firstPersonModelObject && bipObject.part == a_weapon->firstPersonModelObject->As<RE::TESModelTextureSwap>())) {
			float reach = GetWeaponMeshLength(bipObject.partClone.get());
			if (reach > 0.f) {
				{
					WriteLocker locker(PrecisionHandler::weaponMeshLengthLock);
					weaponMeshLengthMap.emplace(a_weapon, reach);
				}

				a_outReach = reach * a_actor->GetScale();
				return true;
			}
		}
	}

	return false;
}

RE::NiPointer<RE::BGSAttackData>& PrecisionHandler::GetOppositeAttackEvent(RE::NiPointer<RE::BGSAttackData>& a_attackData, RE::BGSAttackDataMap* attackDataMap)
{
	if (!attackDataMap || !a_attackData) {
		return a_attackData;
	}

	for (auto& entry : Settings::attackEventPairs) {
		std::string_view oppositeEvent;

		bool bIsRightEvent = entry.first == a_attackData->event.data();
		if (bIsRightEvent) {
			oppositeEvent = entry.second;
		}
		bool bIsLeftEvent = entry.second == a_attackData->event.data();
		if (bIsLeftEvent) {
			oppositeEvent = entry.first;
		}

		auto attackDataSearch = attackDataMap->attackDataMap.find(oppositeEvent);
		if (attackDataSearch != attackDataMap->attackDataMap.end()) {
			return attackDataSearch->second;
		}
	}

	return a_attackData;
}

bool PrecisionHandler::AddPreHitCallback(SKSE::PluginHandle a_pluginHandle, PreHitCallback a_preHitCallback)
{
	WriteLocker locker(callbacksLock);

	if (preHitCallbacks.contains(a_pluginHandle)) {
		return false;
	}

	preHitCallbacks.emplace(a_pluginHandle, a_preHitCallback);
	return true;
}

bool PrecisionHandler::AddPostHitCallback(SKSE::PluginHandle a_pluginHandle, PostHitCallback a_postHitCallback)
{
	WriteLocker locker(callbacksLock);

	if (postHitCallbacks.contains(a_pluginHandle)) {
		return false;
	}

	postHitCallbacks.emplace(a_pluginHandle, a_postHitCallback);
	return true;
}

bool PrecisionHandler::AddPrePhysicsStepCallback(SKSE::PluginHandle a_pluginHandle, PrePhysicsStepCallback a_prePhysicsHitCallback)
{
	WriteLocker locker(callbacksLock);

	if (prePhysicsStepCallbacks.contains(a_pluginHandle)) {
		return false;
	}

	prePhysicsStepCallbacks.emplace(a_pluginHandle, a_prePhysicsHitCallback);
	return true;
}

bool PrecisionHandler::AddCollisionFilterComparisonCallback(SKSE::PluginHandle a_pluginHandle, CollisionFilterComparisonCallback a_collisionFilterComparisonCallback)
{
	WriteLocker locker(callbacksLock);

	if (collisionFilterComparisonCallbacks.contains(a_pluginHandle)) {
		return false;
	}

	collisionFilterComparisonCallbacks.emplace(a_pluginHandle, a_collisionFilterComparisonCallback);
	return true;
}

bool PrecisionHandler::AddWeaponWeaponCollisionCallback(SKSE::PluginHandle a_pluginHandle, WeaponCollisionCallback a_weaponCollisionCallback)
{
	WriteLocker locker(callbacksLock);

	if (weaponWeaponCollisionCallbacks.contains(a_pluginHandle)) {
		return false;
	}

	weaponWeaponCollisionCallbacks.emplace(a_pluginHandle, a_weaponCollisionCallback);
	return true;
}

bool PrecisionHandler::AddWeaponProjectileCollisionCallback(SKSE::PluginHandle a_pluginHandle, WeaponCollisionCallback a_weaponCollisionCallback)
{
	WriteLocker locker(callbacksLock);

	if (weaponProjectileCollisionCallbacks.contains(a_pluginHandle)) {
		return false;
	}

	weaponProjectileCollisionCallbacks.emplace(a_pluginHandle, a_weaponCollisionCallback);
	return true;
}

bool PrecisionHandler::AddCollisionFilterSetupCallback(SKSE::PluginHandle a_pluginHandle, CollisionFilterSetupCallback a_collisionFilterSetupCallback)
{
	WriteLocker locker(callbacksLock);

	if (collisionFilterSetupCallbacks.contains(a_pluginHandle)) {
		return false;
	}

	collisionFilterSetupCallbacks.emplace(a_pluginHandle, a_collisionFilterSetupCallback);
	return true;
}

bool PrecisionHandler::AddContactListenerCallback(SKSE::PluginHandle a_pluginHandle, ContactListenerCallback a_contactListenerCallback)
{
	WriteLocker locker(callbacksLock);

	if (contactListenerCallbacks.contains(a_pluginHandle)) {
		return false;
	}

	contactListenerCallbacks.emplace(a_pluginHandle, a_contactListenerCallback);
	return true;
}

bool PrecisionHandler::AddPrecisionLayerSetupCallback(SKSE::PluginHandle a_pluginHandle, PrecisionLayerSetupCallback a_precisionLayerSetupCallback)
{
	WriteLocker locker(callbacksLock);

	if (precisionLayerSetupCallbacks.contains(a_pluginHandle)) {
		return false;
	}

	precisionLayerSetupCallbacks.emplace(a_pluginHandle, a_precisionLayerSetupCallback);
	return true;
}

bool PrecisionHandler::RemovePreHitCallback(SKSE::PluginHandle a_pluginHandle)
{
	WriteLocker locker(callbacksLock);

	return preHitCallbacks.erase(a_pluginHandle);
}

bool PrecisionHandler::RemovePostHitCallback(SKSE::PluginHandle a_pluginHandle)
{
	WriteLocker locker(callbacksLock);

	return postHitCallbacks.erase(a_pluginHandle);
}

bool PrecisionHandler::RemovePrePhysicsStepCallback(SKSE::PluginHandle a_pluginHandle)
{
	WriteLocker locker(callbacksLock);

	return prePhysicsStepCallbacks.erase(a_pluginHandle);
}

bool PrecisionHandler::RemoveCollisionFilterComparisonCallback(SKSE::PluginHandle a_pluginHandle)
{
	WriteLocker locker(callbacksLock);

	return collisionFilterComparisonCallbacks.erase(a_pluginHandle);
}

bool PrecisionHandler::RemoveWeaponWeaponCollisionCallback(SKSE::PluginHandle a_pluginHandle)
{
	WriteLocker locker(callbacksLock);

	return weaponWeaponCollisionCallbacks.erase(a_pluginHandle);
}

bool PrecisionHandler::RemoveWeaponProjectileCollisionCallback(SKSE::PluginHandle a_pluginHandle)
{
	WriteLocker locker(callbacksLock);

	return weaponProjectileCollisionCallbacks.erase(a_pluginHandle);
}

bool PrecisionHandler::RemoveCollisionFilterSetupCallback(SKSE::PluginHandle a_pluginHandle)
{
	WriteLocker locker(callbacksLock);

	return collisionFilterSetupCallbacks.erase(a_pluginHandle);
}

bool PrecisionHandler::RemoveContactListenerCallback(SKSE::PluginHandle a_pluginHandle)
{
	WriteLocker locker(callbacksLock);

	return contactListenerCallbacks.erase(a_pluginHandle);
}

bool PrecisionHandler::RemovePrecisionLayerSetupCallback(SKSE::PluginHandle a_pluginHandle)
{
	WriteLocker locker(callbacksLock);

	return precisionLayerSetupCallbacks.erase(a_pluginHandle);
}

float PrecisionHandler::GetAttackCollisionReach(RE::ActorHandle a_actorHandle, RequestedAttackCollisionType a_collisionType /*= RequestedAttackCollisionType::Default*/) const
{
	float length = 0.f;

	if (a_actorHandle) {
		auto actor = a_actorHandle.get();
		if (actor) {
			if (Settings::bDisableMod) {
				return Actor_GetReach(actor.get());
			}

			if (auto activeActor = GetActiveActor(a_actorHandle)) {
				if (!activeActor->attackCollisions.IsEmpty()) {  // actor has at least one active collision
					activeActor->attackCollisions.ForEachAttackCollision([&](std::shared_ptr<AttackCollision> attackCollision) {
						if (a_collisionType == RequestedAttackCollisionType::LeftWeapon && attackCollision->nodeName != "SHIELD"sv) {
							return;
						} else if (a_collisionType == RequestedAttackCollisionType::RightWeapon && attackCollision->nodeName != "WEAPON"sv) {
							return;
						}

						if (attackCollision->capsuleLength > length) {
							length = attackCollision->capsuleLength;
						}
					});

					return length;
				}
			}

			if (a_collisionType == RequestedAttackCollisionType::Current) {
				// actor has no active collisions, so return 0
				return 0.f;
			}

			auto currentProcess = actor->GetActorRuntimeData().currentProcess;
			if (currentProcess && currentProcess->middleHigh) {
				RE::InventoryEntryData* equipment = nullptr;

				bool bIsLeftWeapon = a_collisionType == RequestedAttackCollisionType::LeftWeapon;

				if (bIsLeftWeapon) {
					equipment = currentProcess->middleHigh->leftHand;
				} else {
					equipment = currentProcess->middleHigh->rightHand;
				}

				RE::NiAVObject* weaponNode = nullptr;
				RE::TESObjectWEAP* weapon = nullptr;

				if (equipment && equipment->object) {
					weapon = equipment->object->As<RE::TESObjectWEAP>();
				}

				if (auto niAVObject = actor->GetNodeByName(bIsLeftWeapon ? "WEAPON"sv : "SHIELD"sv)) {
					auto node = niAVObject->AsNode();
					if (node && node->children.size() > 0 && node->children[0]) {
						weaponNode = node->children[0].get();
					}
				}

				length = GetWeaponAttackLength(a_actorHandle, weaponNode, weapon);
			}
		}

		if (length == 0.f) {
			length = Actor_GetReach(actor.get());
		}
	}

	return length;
}

bool PrecisionHandler::IsActorActive(RE::ActorHandle a_actorHandle)
{
	ReadLocker locker(activeActorsLock);

	return PrecisionHandler::activeActors.size() > 0 && PrecisionHandler::activeActors.contains(a_actorHandle);
}

bool PrecisionHandler::IsActorActiveCollisionGroup(uint16_t a_collisionGroup)
{
	ReadLocker locker(activeCollisionGroupsLock);

	return PrecisionHandler::activeCollisionGroups.size() > 0 && PrecisionHandler ::activeCollisionGroups.contains(a_collisionGroup);
}

bool PrecisionHandler::IsActorCharacterControllerHittable(RE::ActorHandle a_actorHandle)
{
	if (!IsActorActive(a_actorHandle)) {
		return true;
	}

	if (auto actor = a_actorHandle.get()) {
		if (auto controller = actor->GetCharController()) {
			return IsCharacterControllerHittable(controller);
		}
	}

	return false;
}

bool PrecisionHandler::IsCharacterControllerHittable(RE::bhkCharacterController* a_controller)
{
	if (a_controller) {
		uint32_t filterInfo;
		a_controller->GetCollisionFilterInfo(filterInfo);

		return IsCharacterControllerHittableCollisionGroup(filterInfo >> 16);
	}

	return false;
}

bool PrecisionHandler::IsCharacterControllerHittableCollisionGroup(uint16_t a_collisionGroup)
{
	ReadLocker locker(PrecisionHandler::hittableCharControllerGroupsLock);
	return PrecisionHandler::hittableCharControllerGroups.size() > 0 && PrecisionHandler::hittableCharControllerGroups.contains(a_collisionGroup);
}

bool PrecisionHandler::IsRagdollCollsionGroup(uint16_t a_collisionGroup)
{
	ReadLocker locker(PrecisionHandler::ragdollCollisionGroupsLock);
	return PrecisionHandler::ragdollCollisionGroups.size() > 0 && PrecisionHandler::ragdollCollisionGroups.contains(a_collisionGroup);
}

bool PrecisionHandler::IsActorDisabled(RE::ActorHandle a_actorHandle)
{
	ReadLocker locker(PrecisionHandler::disabledActorsLock);
	return PrecisionHandler::disabledActors.contains(a_actorHandle);
}

bool PrecisionHandler::ToggleDisableActor(RE::ActorHandle a_actorHandle, bool a_bDisable)
{
	WriteLocker locker(disabledActorsLock);

	if (a_bDisable) {
		auto [iter, bSuccess] = disabledActors.emplace(a_actorHandle);
		return bSuccess;
	} else {
		return disabledActors.erase(a_actorHandle);
	}
}

bool PrecisionHandler::IsRagdollAdded(RE::ActorHandle a_actorHandle)
{
	ReadLocker locker(activeRagdollsLock);

	return PrecisionHandler::activeActorsWithRagdolls.size() > 0 && PrecisionHandler::activeActorsWithRagdolls.contains(a_actorHandle);
}

RE::NiAVObject* PrecisionHandler::GetOriginalFromClone(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_clone)
{
	ReadLocker locker(activeActorsLock);

	auto search = PrecisionHandler::activeActors.find(a_actorHandle);
	if (search != PrecisionHandler::activeActors.end()) {
		auto& clonedSkeleton = search->second;
		return clonedSkeleton->GetOriginalFromClone(a_clone);
	}

	return nullptr;
}

RE::hkpRigidBody* PrecisionHandler::GetOriginalFromClone(RE::ActorHandle a_actorHandle, RE::hkpRigidBody* a_clone)
{
	ReadLocker locker(activeActorsLock);

	auto search = PrecisionHandler::activeActors.find(a_actorHandle);
	if (search != PrecisionHandler::activeActors.end()) {
		auto& clonedSkeleton = search->second;
		return clonedSkeleton->GetOriginalFromClone(a_clone);
	}

	return nullptr;
}

std::vector<PRECISION_API::PreHitCallbackReturn> PrecisionHandler::RunPreHitCallbacks(const PrecisionHitData& a_precisionHitData)
{
	ReadLocker locker(callbacksLock);

	std::vector<PreHitCallbackReturn> ret;
	for (auto& entry : preHitCallbacks) {
		ret.emplace_back(entry.second(a_precisionHitData));
	}
	return ret;
}

void PrecisionHandler::RunPostHitCallbacks(const PrecisionHitData& a_precisionHitData, const RE::HitData& a_hitData)
{
	ReadLocker locker(callbacksLock);

	for (auto& entry : postHitCallbacks) {
		entry.second(a_precisionHitData, a_hitData);
	}
}

void PrecisionHandler::RunPrePhysicsStepCallbacks(RE::bhkWorld* a_world)
{
	{
		ReadLocker locker(callbacksLock);

		for (auto& entry : prePhysicsStepCallbacks) {
			entry.second(a_world);
		}
	}

	ProcessPrePhysicsStepJobs();
}

PRECISION_API::CollisionFilterComparisonResult PrecisionHandler::RunCollisionFilterComparisonCallbacks(RE::bhkCollisionFilter* a_collisionFilter, uint32_t a_filterInfoA, uint32_t a_filterInfoB)
{
	ReadLocker locker(callbacksLock);

	for (auto& entry : collisionFilterComparisonCallbacks) {
		CollisionFilterComparisonResult result = entry.second(a_collisionFilter, a_filterInfoA, a_filterInfoB);
		if (result != CollisionFilterComparisonResult::Continue) {
			return result;
		}
	}

	return PRECISION_API::CollisionFilterComparisonResult::Continue;
}

std::vector<PrecisionHandler::WeaponCollisionCallbackReturn> PrecisionHandler::RunWeaponWeaponCollisionCallbacks(const PrecisionHitData& a_precisionHitData)
{
	ReadLocker locker(callbacksLock);

	std::vector<WeaponCollisionCallbackReturn> ret;
	for (auto& entry : weaponWeaponCollisionCallbacks) {
		ret.emplace_back(entry.second(a_precisionHitData));
	}
	return ret;
}

std::vector<PrecisionHandler::WeaponCollisionCallbackReturn> PrecisionHandler::RunWeaponProjectileCollisionCallbacks(const PrecisionHitData& a_precisionHitData)
{
	ReadLocker locker(callbacksLock);

	std::vector<WeaponCollisionCallbackReturn> ret;
	for (auto& entry : weaponProjectileCollisionCallbacks) {
		ret.emplace_back(entry.second(a_precisionHitData));
	}
	return ret;
}

void PrecisionHandler::RunCollisionFilterSetupCallbacks(RE::bhkCollisionFilter* a_collisionFilter)
{
	ReadLocker locker(callbacksLock);

	for (auto& entry : collisionFilterSetupCallbacks) {
		entry.second(a_collisionFilter);
	}
}

void PrecisionHandler::RunContactListenerCallbacks(const RE::hkpContactPointEvent& a_event)
{
	ReadLocker locker(callbacksLock);

	for (auto& entry : contactListenerCallbacks) {
		entry.second(a_event);
	}
}

void PrecisionHandler::RunPrecisionLayerSetupCallbacks()
{
	ReadLocker locker(callbacksLock);

	if (precisionLayerSetupCallbacks.empty()) {
		return;
	}

	uint64_t attackLayersToAdd = 0;
	uint64_t attackLayersToRemove = 0;
	uint64_t bodyLayersToAdd = 0;
	uint64_t bodyLayersToRemove = 0;

	for (auto& entry : precisionLayerSetupCallbacks) {
		auto result = entry.second();

		switch (result.precisionLayerType) {
		case PRECISION_API::PrecisionLayerType::Attack:
			attackLayersToAdd |= result.layersToAdd;
			attackLayersToRemove |= result.layersToRemove;
			break;
		case PRECISION_API::PrecisionLayerType::Body:
			bodyLayersToAdd |= result.layersToAdd;
			bodyLayersToRemove |= result.layersToRemove;
			break;
		}
	}

	Settings::iPrecisionAttackLayerBitfield |= attackLayersToAdd;
	Settings::iPrecisionAttackLayerBitfield &= ~attackLayersToRemove;
	Settings::iPrecisionBodyLayerBitfield |= bodyLayersToAdd;
	Settings::iPrecisionBodyLayerBitfield &= ~bodyLayersToRemove;
}
