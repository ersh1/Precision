#include "PrecisionHandler.h"
#include "Offsets.h"
#include "Settings.h"
#include "Utils.h"

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

	uint32_t activeGraphIdx = graphManager->activeGraph;

	if (graphManager->graphs[activeGraphIdx] && graphManager->graphs[activeGraphIdx].get() != a_eventSource) {
		return EventResult::kContinue;
	}

	if (actor->IsInKillMove()) {
		return EventResult::kContinue;
	}

	std::string_view eventTag = a_event->tag.data();

	if (actor == RE::PlayerCharacter::GetSingleton()) {
		logger::debug("{}", a_event->tag);
	}

	switch (hash(eventTag.data(), eventTag.size())) {
	case "Collision_AttackStart"_h:
	case "Collision_Start"_h:
		{
			ClearHitRefs(actor->GetHandle());
			StartCollision(actor->GetHandle(), activeGraphIdx);
			break;
		}
	case "SoundPlay"_h:
		if (a_event->payload != "NPCHumanCombatShieldBashSD"sv) {  // crossbow bash
			break;
		}
	case "SoundPlay.WPNSwingUnarmed"_h:
	case "SoundPlay.NPCHumanCombatShieldBash"_h:
	case "weaponSwing"_h:
	case "weaponLeftSwing"_h:
		{
			// only do this if we haven't received a Collision_Start event (vanilla)
			auto actorHandle = actor->GetHandle();
			if (!HasStartedPrecisionCollision(actorHandle, activeGraphIdx)) {
				bool bIsWPNSwingUnarmed = a_event->tag == "SoundPlay.WPNSwingUnarmed";
				if ((bIsWPNSwingUnarmed && !HasStartedDefaultCollisionWithWeaponSwing(actorHandle)) || (!bIsWPNSwingUnarmed && !HasStartedDefaultCollisionWithWPNSwingUnarmed(actorHandle))) {
					AttackDefinition attackDefinition;
					if (bIsWPNSwingUnarmed) {
						SetStartedDefaultCollisionWithWPNSwingUnarmed(actorHandle);
					} else {
						SetStartedDefaultCollisionWithWeaponSwing(actorHandle);
					}

					std::optional<bool> bIsLeftSwing = std::nullopt;
					if (!bIsWPNSwingUnarmed) {
						bIsLeftSwing = a_event->tag == "weaponLeftSwing"sv;
					}
					if (GetAttackCollisionDefinition(actor, attackDefinition, bIsLeftSwing)) {
						AddAttack(actor->GetHandle(), attackDefinition);
					}
				}
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
	if (!Settings::bAttackCollisionsEnabled || Settings::bDisableMod) {
		Clear();
	}

	if (RE::UI::GetSingleton()->GameIsPaused()) {
		return;
	}

	Settings::UpdateGlobals();

	ProcessMainUpdateJobs();
	ProcessDelayedJobs(a_deltaTime);

	if (Settings::bDebug && Settings::bDisplaySkeletonColliders) {
		auto& actorHandles = RE::ProcessLists::GetSingleton()->highActorHandles;
		if (actorHandles.size() > 0) {
			auto playerCamera = RE::PlayerCamera::GetSingleton();
			if (playerCamera) {
				auto cameraPos = playerCamera->pos;
				for (auto& actorHandle : actorHandles) {
					if (actorHandle) {
						auto actorPtr = actorHandle.get();
						if (actorPtr) {
							auto distance = cameraPos.GetDistance(actorPtr->GetPosition());
							if (distance < 2000.f) {
								glm::vec4 color{ 1.f, 0.5f, 0.f, 1.f };
								Utils::DrawActorColliders(actorPtr.get(), 0.f, color);
							}
						}
					}
				}
			}
		}

		auto playerCharacter = RE::PlayerCharacter::GetSingleton();
		if (playerCharacter) {
			glm::vec4 color{ 1.f, 0.5f, 0.f, 1.f };
			Utils::DrawActorColliders(playerCharacter, 0.f, color);
		}
	}

	for (auto& pendingHit : pendingHits) {
		pendingHit.DoHit();
	}

	pendingHits.clear();

	{
		WriteLocker locker(attackCollisionsLock);

		for (auto it = _actorsWithAttackCollisions.begin(); it != _actorsWithAttackCollisions.end();) {
			auto actorHandle = it->first;
			if (!actorHandle) {
				//++it;
				//RemoveActor_Impl(actorHandle);
				it = _actorsWithAttackCollisions.erase(it);
			} else {
				auto actor = it->first.get();
				if (!actor || !actor->parentCell || !actor->parentCell->GetbhkWorld() || !actor->Get3D() || (!it->second.ignoreVanillaAttackEvents && it->second.IsEmpty())) {
					//++it;
					//RemoveActor_Impl(actorHandle);
					it = _actorsWithAttackCollisions.erase(it);
				} else {
					it->second.Update(a_deltaTime);
					++it;
				}
			}
		}
	}

	{
		WriteLocker locker(actorsInCombatLock);
		for (auto it = _actorsInLingeringCombat.begin(); it != _actorsInLingeringCombat.end();) {

			it->second -= a_deltaTime;
			if (it->second < 0.f) {
				it = _actorsInLingeringCombat.erase(it);
			} else {
				++it;
			}			
		}
	}
	
	for (auto& trail : _attackTrails) {
		trail->Update(a_deltaTime);
	}

	std::erase_if(_attackTrails, [](std::shared_ptr<AttackTrail>& trail) { return !trail->bActive; });

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
			currentCameraShakeAxis = {};
		}
	}
}

void PrecisionHandler::ApplyHitImpulse(RE::ObjectRefHandle a_refHandle, RE::hkpRigidBody* a_rigidBody, const RE::NiPoint3& a_hitVelocity, const RE::hkVector4& a_hitPosition, float a_impulseMult, bool a_bIsActiveRagdoll)
{
	if (!a_refHandle) {
		return;
	}

	auto refr = a_refHandle.get().get();
	// Apply linear impulse at the center of mass to all bodies within 3 ragdoll constraints
	Utils::ForEachRagdollDriver(refr, [=](RE::hkbRagdollDriver* driver) {
		auto ragdoll = GetActiveRagdollFromDriver(driver);
		if (!ragdoll) {
			return;
		}
		
		ragdoll->impulseTime = Settings::fRagdollImpulseTime;

		Utils::ForEachAdjacentBody(driver, a_rigidBody, [=](RE::hkpRigidBody* adjacentBody) {
			QueuePrePhysicsJob<LinearImpulseJob>(adjacentBody, a_refHandle, a_hitVelocity, a_impulseMult * Settings::fHitImpulseDecayMult1, a_bIsActiveRagdoll);
			Utils::ForEachAdjacentBody(driver, adjacentBody, [=](RE::hkpRigidBody* adjacentBody) {
				QueuePrePhysicsJob<LinearImpulseJob>(adjacentBody, a_refHandle, a_hitVelocity, a_impulseMult * Settings::fHitImpulseDecayMult2, a_bIsActiveRagdoll);
				Utils::ForEachAdjacentBody(driver, adjacentBody, [=](RE::hkpRigidBody* adjacentBody) {
					QueuePrePhysicsJob<LinearImpulseJob>(adjacentBody, a_refHandle, a_hitVelocity, a_impulseMult * Settings::fHitImpulseDecayMult3, a_bIsActiveRagdoll);
				});
			});
		});
	});

	// Apply a point impulse at the hit location to the body we actually hit
	QueuePrePhysicsJob<PointImpulseJob>(a_rigidBody, a_refHandle, a_hitVelocity, a_hitPosition, a_impulseMult, a_bIsActiveRagdoll);
}

void PrecisionHandler::AddHitstop(RE::ActorHandle a_refHandle, float a_hitstopLength)
{
	if (Settings::bEnableHitstop) {
		WriteLocker locker(activeHitstopsLock);

		auto& hitstop = activeHitstops[a_refHandle];
		hitstop += a_hitstopLength;
	}
}

float PrecisionHandler::GetHitstop(RE::ActorHandle a_actorHandle, float a_deltaTime, bool a_bUpdate)
{
	if (Settings::bEnableHitstop) {
		if (a_deltaTime > 0.f) {
			WriteLocker locker(activeHitstopsLock);
			
			auto hitstop = activeHitstops.find(a_actorHandle);
			if (hitstop != activeHitstops.end()) {
				float newHitstopLength = hitstop->second - a_deltaTime;
				if (a_bUpdate) {
					hitstop->second = newHitstopLength;
				}

				float mult = 1.f;
				if (newHitstopLength <= 0.f) {
					mult = (a_deltaTime + newHitstopLength) / a_deltaTime;
					if (a_bUpdate) {
						activeHitstops.erase(hitstop);
					}
				}

				a_deltaTime *= Settings::fHitstopSlowdownTimeMultiplier + ((1.f - Settings::fHitstopSlowdownTimeMultiplier) * (1.f - mult));
			}
		}
	}

	return a_deltaTime;
}

void PrecisionHandler::ApplyCameraShake(float a_strength, float a_length, float a_frequency, const RE::NiPoint3& a_axis)
{
	if (a_strength > currentCameraShakeStrength || a_length > currentCameraShakeTimer) {
		bCameraShakeActive = true;
		currentCameraShakeStrength = a_strength;
		currentCameraShakeLength = a_length;
		currentCameraShakeFrequency = a_frequency;
		currentCameraShakeTimer = a_length;
		currentCameraShakeAxis = a_axis;
	}
}

PrecisionHandler::PrecisionHandler() :
	attackCollisionsLock(), callbacksLock()
{}

void PrecisionHandler::StartCollision_Impl(RE::ActorHandle a_actorHandle, uint32_t a_activeGraphIdx)
{
	// create the entry in the map and set the bool
	auto& attackCollisions = _actorsWithAttackCollisions[a_actorHandle];
	attackCollisions.ignoreVanillaAttackEvents = a_activeGraphIdx;
}

RE::NiPoint3 PrecisionHandler::CalculateHitImpulse(RE::hkpRigidBody* a_rigidBody, const RE::NiPoint3& a_hitVelocity, float a_impulseMult, bool a_bIsActiveRagdoll)
{
	if (a_rigidBody->motion.type == RE::hkpMotion::MotionType::kKeyframed) {
		return RE::NiPoint3();
	}
	
	float massInv = a_rigidBody->motion.inertiaAndMassInv.quad.m128_f32[3];
	float mass = massInv <= 0.001f ? 99999.f : 1.f / massInv;

	float impulseStrength = std::clamp(
		Settings::fHitImpulseBaseStrength + Settings::fHitImpulseProportionalStrength * powf(mass, Settings::fHitImpulseMassExponent),
		Settings::fHitImpulseMinStrength, Settings::fHitImpulseMaxStrength);

	RE::NiPoint3 impulse = a_hitVelocity;
	float impulseSpeed = impulse.Unitize();

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
	
	if (globalTimeMultiplier <= 0.f) {
		globalTimeMultiplier = 1.f;
	}
	
	impulseSpeed = fmin(impulseSpeed, (Settings::fHitImpulseMaxVelocity / *g_globalTimeMultiplier));  // limit the imparted velocity to some reasonable value
	impulse *= impulseSpeed * *g_worldScale * mass;                                                   // This impulse will give the object the exact velocity it is hit with
	impulse *= impulseStrength;                                                                       // Scale the velocity as we see fit
	impulse *= a_impulseMult;

	if (impulse.z < 0) {
		// Impulse points downwards somewhat, scale back the downward component so we don't get things shooting into the ground.
		impulse.z *= Settings::fHitImpulseDownwardsMultiplier;
	}

	return impulse;
}

void PrecisionHandler::RemoveActor(RE::ActorHandle a_actorHandle)
{
	WriteLocker locker(attackCollisionsLock);

	_actorsWithAttackCollisions.erase(a_actorHandle);
}

void PrecisionHandler::StartCollision(RE::ActorHandle a_actorHandle, uint32_t a_activeGraphIdx)
{
	WriteLocker locker(attackCollisionsLock);

	StartCollision_Impl(a_actorHandle, a_activeGraphIdx);
}

bool PrecisionHandler::AddAttack(RE::ActorHandle a_actorHandle, const AttackDefinition& a_attackDefinition)
{
	WriteLocker locker(attackCollisionsLock);

	auto actor = a_actorHandle.get();
	if (!actor) {
		return false;
	}
	
	auto& activeActor = _actorsWithAttackCollisions[a_actorHandle];

	for (auto& collisionDef : a_attackDefinition.collisions) {
		if (collisionDef.delay) {
			QueueDelayedJob<DelayedAttackCollisionJob>(*collisionDef.delay, a_actorHandle, collisionDef);
		} else {
			activeActor.AddAttackCollision(a_actorHandle, collisionDef);
		}
	}

	return true;
}

void PrecisionHandler::AddAttackCollision(RE::ActorHandle a_actorHandle, const CollisionDefinition& a_collisionDefinition)
{
	WriteLocker locker(attackCollisionsLock);

	auto& activeActor = _actorsWithAttackCollisions[a_actorHandle];

	activeActor.AddAttackCollision(a_actorHandle, a_collisionDefinition);
}

bool PrecisionHandler::RemoveAttackCollision(RE::ActorHandle a_actorHandle, std::shared_ptr<AttackCollision> a_attackCollision)
{
	WriteLocker locker(attackCollisionsLock);

	if (!_actorsWithAttackCollisions.contains(a_actorHandle)) {
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

	auto& activeActor = _actorsWithAttackCollisions[a_actorHandle];

	return activeActor.RemoveAttackCollision(a_actorHandle, a_attackCollision);
}

bool PrecisionHandler::RemoveAttackCollision(RE::ActorHandle a_actorHandle, const CollisionDefinition& a_collisionDefinition)
{
	WriteLocker locker(attackCollisionsLock);

	if (!_actorsWithAttackCollisions.contains(a_actorHandle)) {
		return false;
	}

	auto& activeActor = _actorsWithAttackCollisions[a_actorHandle];

	return activeActor.RemoveAttackCollision(a_actorHandle, a_collisionDefinition);
}

bool PrecisionHandler::RemoveAllAttackCollisions(RE::ActorHandle a_actorHandle)
{
	WriteLocker locker(attackCollisionsLock);

	if (!_actorsWithAttackCollisions.contains(a_actorHandle)) {
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

	auto& activeActor = _actorsWithAttackCollisions[a_actorHandle];
	activeActor.RemoveAllAttackCollisions(a_actorHandle);

	return _actorsWithAttackCollisions.erase(a_actorHandle) > 0;
}

std::shared_ptr<AttackCollision> PrecisionHandler::GetAttackCollision(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_node) const
{
	ReadLocker locker(attackCollisionsLock);

	if (!a_actorHandle || !a_node) {
		return nullptr;
	}

	auto search = _actorsWithAttackCollisions.find(a_actorHandle);
	if (search != _actorsWithAttackCollisions.end()) {
		return search->second.GetAttackCollision(a_actorHandle, a_node);
	}

	return nullptr;
}

std::shared_ptr<AttackCollision> PrecisionHandler::GetAttackCollision(RE::ActorHandle a_actorHandle, std::string_view a_nodeName) const
{
	ReadLocker locker(attackCollisionsLock);

	if (!a_actorHandle) {
		return nullptr;
	}

	auto search = _actorsWithAttackCollisions.find(a_actorHandle);
	if (search != _actorsWithAttackCollisions.end()) {
		return search->second.GetAttackCollision(a_actorHandle, a_nodeName);
	}

	return nullptr;
}

bool PrecisionHandler::HasActor(RE::ActorHandle a_actorHandle) const
{
	ReadLocker locker(attackCollisionsLock);

	return _actorsWithAttackCollisions.contains(a_actorHandle);
}

bool PrecisionHandler::HasStartedDefaultCollisionWithWeaponSwing(RE::ActorHandle a_actorHandle) const
{
	ReadLocker locker(attackCollisionsLock);

	auto search = _actorsWithAttackCollisions.find(a_actorHandle);
	if (search != _actorsWithAttackCollisions.end()) {
		return search->second.bStartedWithWeaponSwing;
	}

	return false;
}

bool PrecisionHandler::HasStartedDefaultCollisionWithWPNSwingUnarmed(RE::ActorHandle a_actorHandle) const
{
	ReadLocker locker(attackCollisionsLock);

	auto search = _actorsWithAttackCollisions.find(a_actorHandle);
	if (search != _actorsWithAttackCollisions.end()) {
		return search->second.bStartedWithWPNSwingUnarmed;
	}

	return false;
}

bool PrecisionHandler::HasStartedPrecisionCollision(RE::ActorHandle a_actorHandle, uint32_t a_activeGraphIdx) const
{
	ReadLocker locker(attackCollisionsLock);

	auto search = _actorsWithAttackCollisions.find(a_actorHandle);
	if (search != _actorsWithAttackCollisions.end()) {
		return search->second.ignoreVanillaAttackEvents && search->second.ignoreVanillaAttackEvents == a_activeGraphIdx;
	}

	return false;
}

bool PrecisionHandler::HasHitstop(RE::ActorHandle a_actorHandle) const
{
	ReadLocker locker(activeHitstopsLock);

	return activeHitstops.contains(a_actorHandle);
}

void PrecisionHandler::SetStartedDefaultCollisionWithWeaponSwing(RE::ActorHandle a_actorHandle)
{
	WriteLocker locker(attackCollisionsLock);
	
	auto& attackCollisions = _actorsWithAttackCollisions[a_actorHandle];
	attackCollisions.bStartedWithWeaponSwing = true;
}

void PrecisionHandler::SetStartedDefaultCollisionWithWPNSwingUnarmed(RE::ActorHandle a_actorHandle)
{
	WriteLocker locker(attackCollisionsLock);

	auto& attackCollisions = _actorsWithAttackCollisions[a_actorHandle];
	attackCollisions.bStartedWithWPNSwingUnarmed = true;
}

bool PrecisionHandler::HasHitRef(RE::ActorHandle a_actorHandle, RE::ObjectRefHandle a_handle) const
{
	ReadLocker locker(attackCollisionsLock);

	auto search = _actorsWithAttackCollisions.find(a_actorHandle);
	if (search != _actorsWithAttackCollisions.end()) {
		return search->second.HasHitRef(a_handle);
	}

	return false;
}

void PrecisionHandler::AddHitRef(RE::ActorHandle a_actorHandle, RE::ObjectRefHandle a_handle, float a_duration, bool a_bIsNPC)
{
	ReadLocker locker(attackCollisionsLock);

	auto search = _actorsWithAttackCollisions.find(a_actorHandle);
	if (search != _actorsWithAttackCollisions.end()) {
		search->second.AddHitRef(a_handle, a_duration, a_bIsNPC);
	}
}

void PrecisionHandler::ClearHitRefs(RE::ActorHandle a_actorHandle)
{
	ReadLocker locker(attackCollisionsLock);

	auto search = _actorsWithAttackCollisions.find(a_actorHandle);
	if (search != _actorsWithAttackCollisions.end()) {
		search->second.ClearHitRefs();
	}
}

uint32_t PrecisionHandler::GetHitCount(RE::ActorHandle a_actorHandle) const
{
	ReadLocker locker(attackCollisionsLock);

	auto search = _actorsWithAttackCollisions.find(a_actorHandle);
	if (search != _actorsWithAttackCollisions.end()) {
		return search->second.GetHitCount();
	}

	return 0;
}

uint32_t PrecisionHandler::GetHitNPCCount(RE::ActorHandle a_actorHandle) const
{
	ReadLocker locker(attackCollisionsLock);

	auto search = _actorsWithAttackCollisions.find(a_actorHandle);
	if (search != _actorsWithAttackCollisions.end()) {
		return search->second.GetHitNPCCount();
	}

	return 0;
}

bool PrecisionHandler::HasIDHitRef(RE::ActorHandle a_actorHandle, uint8_t a_ID, RE::ObjectRefHandle a_handle) const
{
	ReadLocker locker(attackCollisionsLock);

	auto search = _actorsWithAttackCollisions.find(a_actorHandle);
	if (search != _actorsWithAttackCollisions.end()) {
		return search->second.HasIDHitRef(a_ID, a_handle);
	}

	return false;
}

void PrecisionHandler::AddIDHitRef(RE::ActorHandle a_actorHandle, uint8_t a_ID, RE::ObjectRefHandle a_handle, float a_duration, bool a_bIsNPC)
{
	ReadLocker locker(attackCollisionsLock);

	auto search = _actorsWithAttackCollisions.find(a_actorHandle);
	if (search != _actorsWithAttackCollisions.end()) {
		search->second.AddIDHitRef(a_ID, a_handle, a_duration, a_bIsNPC);
	}
}

void PrecisionHandler::ClearIDHitRefs(RE::ActorHandle a_actorHandle, uint8_t a_ID)
{
	ReadLocker locker(attackCollisionsLock);

	auto search = _actorsWithAttackCollisions.find(a_actorHandle);
	if (search != _actorsWithAttackCollisions.end()) {
		search->second.ClearIDHitRefs(a_ID);
	}
}

void PrecisionHandler::IncreaseIDDamagedCount(RE::ActorHandle a_actorHandle, uint8_t a_ID)
{
	ReadLocker locker(attackCollisionsLock);

	auto search = _actorsWithAttackCollisions.find(a_actorHandle);
	if (search != _actorsWithAttackCollisions.end()) {
		search->second.IncreaseIDDamagedCount(a_ID);
	}
}

uint32_t PrecisionHandler::GetIDHitCount(RE::ActorHandle a_actorHandle, uint8_t a_ID) const
{
	ReadLocker locker(attackCollisionsLock);

	auto search = _actorsWithAttackCollisions.find(a_actorHandle);
	if (search != _actorsWithAttackCollisions.end()) {
		return search->second.GetIDHitCount(a_ID);
	}

	return 0;
}

uint32_t PrecisionHandler::GetIDHitNPCCount(RE::ActorHandle a_actorHandle, uint8_t a_ID) const
{
	ReadLocker locker(attackCollisionsLock);

	auto search = _actorsWithAttackCollisions.find(a_actorHandle);
	if (search != _actorsWithAttackCollisions.end()) {
		return search->second.GetIDHitNPCCount(a_ID);
	}

	return 0;
}

uint32_t PrecisionHandler::GetIDDamagedCount(RE::ActorHandle a_actorHandle, uint8_t a_ID) const
{
	ReadLocker locker(attackCollisionsLock);

	auto search = _actorsWithAttackCollisions.find(a_actorHandle);
	if (search != _actorsWithAttackCollisions.end()) {
		return search->second.GetIDDamagedCount(a_ID);
	}

	return 0;
}

void PrecisionHandler::Initialize()
{
}

void PrecisionHandler::Clear()
{
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
	WriteLocker locker(jobsLock);

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
	WriteLocker locker(jobsLock);

	for (auto it = _mainUpdateJobs.begin(); it != _mainUpdateJobs.end();) {
		if (it->get()->Run()) {
			it = _mainUpdateJobs.erase(it);
		} else {
			++it;
		}
	}
}

void PrecisionHandler::ProcessDelayedJobs(float a_deltaTime)
{
	WriteLocker locker(jobsLock);

	for (auto it = _delayedJobs.begin(); it != _delayedJobs.end();) {
		if (it->get()->Run(a_deltaTime)) {
			it = _delayedJobs.erase(it);
		} else {
			++it;
		}
	}
}

bool PrecisionHandler::GetAttackCollisionDefinition(RE::Actor* a_actor, AttackDefinition& a_outAttackDefinition, std::optional<bool> bIsLeftSwing /*= std::nullopt*/) const
{
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
		auto search = Settings::attackAnimationDefinitions.find(projectNameStr);
		if (search != Settings::attackAnimationDefinitions.end()) {
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

	auto search = Settings::attackRaceDefinitions.find(bodyPartData);
	if (search == Settings::attackRaceDefinitions.end()) {
		return false;
	}

	auto& attackDefinitions = search->second;

	auto& attackData = Actor_GetAttackData(a_actor);

	if (bIsLeftSwing.has_value()) {
		if (attackData && attackData->IsLeftAttack() != bIsLeftSwing) {
			attackData = GetOppositeAttackEvent(attackData, race->attackDataMap.get());
		}
	}

	std::string_view attackEvent;
	if (attackData) {
		attackEvent = attackData->event;
	} else {
		attackEvent = "DEFAULT_UNARMED"sv;
		if (a_actor->currentProcess && a_actor->currentProcess->middleHigh) {
			auto equipment = bIsLeftSwing ? a_actor->currentProcess->middleHigh->leftHand : a_actor->currentProcess->middleHigh->rightHand;
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
	auto actor = a_actorHandle.get();
	if (actor) {
		bool bIsInCombat = actor->IsInCombat();

		WriteLocker locker(actorsInCombatLock);

		auto search = _actorsInLingeringCombat.find(a_actorHandle);
		if (search != _actorsInLingeringCombat.end()) {
			if (bIsInCombat) {
				// refresh combat linger time because we're still in combat
				search->second = Settings::fCombatStateLingerTime;
			} else {
				return true;  // found, no longer in combat but still in lingering combat state
			}
		} else if (bIsInCombat) {
			// add actor to combat map
			_actorsInLingeringCombat.emplace(a_actorHandle, Settings::fCombatStateLingerTime);
		}

		return bIsInCombat;
	}
	
	return false;
}

float PrecisionHandler::GetWeaponMeshLength(RE::NiAVObject* a_weaponNode)
{
	if (a_weaponNode) {
		RE::NiBound modelBound = Utils::GetModelBounds(a_weaponNode);
		float offset = modelBound.center.y;
		return modelBound.radius + offset;
	}

	return 0.f;
}

float PrecisionHandler::GetWeaponAttackLength(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_weaponNode, std::optional<float> a_overrideLength /*= std::nullopt*/, float a_lengthMult /*= 1.f*/)
{
	auto actor = a_actorHandle.get();
	if (!actor) {
		return Settings::fMinWeaponLength;
	}

	float length = 0.f;
	if (a_overrideLength) {
		length = *a_overrideLength;
		length *= actor->GetScale();  // apply actor scale
	} else {
		length = GetWeaponMeshLength(a_weaponNode);
	}

	if (length > 0.f) {
		bool bIsPlayer = actor->IsPlayerRef();

		if (bIsPlayer) {
			bool bIsFirstPerson = RE::PlayerCamera::GetSingleton()->IsInFirstPerson();
			if (bIsFirstPerson) {
				length += Settings::fFirstPersonAttackLengthOffset;
				length = fmax(length, 0.f);
			}
			length *= Settings::fPlayerAttackLengthMult;
		}

		if (actor->IsOnMount()) {
			length *= Settings::fMountedAttackLengthMult;
		}
		
		return fmax(length * Settings::fWeaponLengthMult * a_lengthMult, Settings::fMinWeaponLength);
	}
	
	return Settings::fMinWeaponLength;
}

float PrecisionHandler::GetWeaponAttackRadius(RE::ActorHandle a_actorHandle, std::optional<float> a_overrideRadius /*= std::nullopt*/, float a_radiusMult /*= 1.f*/)
{
	float radius = Settings::fWeaponCapsuleRadius;
	
	auto actor = a_actorHandle.get();
	if (!actor) {
		return radius;
	}

	if (a_overrideRadius) {
		radius = *a_overrideRadius;
	}
	
	radius *= actor->GetScale();
	radius *= a_radiusMult;

	bool bIsPlayer = actor->IsPlayerRef();
	if (bIsPlayer) {
	}

	if (bIsPlayer) {
		radius *= Settings::fPlayerAttackRadiusMult;
	}

	if (actor->IsOnMount()) {
		radius *= Settings::fMountedAttackRadiusMult;
	}

	return radius;
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

	bool bIsPlayer = actor->IsPlayerRef();
	if (bIsPlayer) {
		bool bIsFirstPerson = RE::PlayerCamera::GetSingleton()->IsInFirstPerson();
		if (bIsFirstPerson) {
			length += Settings::fFirstPersonAttackLengthOffset * *g_worldScale;
			length = fmax(length, 0.f);
		}
	}

	if (length > 0.f) {
		finalLengthMult *= length / originalLength;
	}

	if (bIsPlayer) {
		finalLengthMult *= Settings::fPlayerAttackLengthMult;
		finalRadiusMult *= Settings::fPlayerAttackRadiusMult;
	}

	if (actor->IsOnMount()) {
		finalLengthMult *= Settings::fMountedAttackLengthMult;
		finalRadiusMult *= Settings::fMountedAttackRadiusMult;
	}

	a_outVertexA = a_outVertexA * actorScale * finalLengthMult;
	a_outVertexB = a_outVertexB * actorScale;
	a_outRadius = a_outRadius * actorScale * finalRadiusMult;

	return true;
}

float PrecisionHandler::GetNodeAttackLength(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_node, std::optional<float> a_overrideLength /*= std::nullopt*/, float a_lengthMult /*= 1.f*/)
{
	auto actor = a_actorHandle.get();
	if (!actor) {
		return 0.f;
	}

	float actorScale = actor->GetScale();

	float length = 0.f;

	if (a_overrideLength) {
		length = *a_overrideLength;
	} else {
		if (auto capsuleShape = GetNodeCapsuleShape(a_node)) {
			length = capsuleShape->vertexA.GetDistance3(capsuleShape->vertexB);
			length = fmax(capsuleShape->radius * 2.f, length);
		}
	}

	length *= actorScale;
	length *= *g_worldScaleInverse;
	length *= a_lengthMult;

	bool bIsPlayer = actor->IsPlayerRef();
	if (bIsPlayer) {
		bool bIsFirstPerson = RE::PlayerCamera::GetSingleton()->IsInFirstPerson();
		if (bIsFirstPerson) {
			length += Settings::fFirstPersonAttackLengthOffset * *g_worldScale;
			length = fmax(length, 0.f);
		}

		length *= Settings::fPlayerAttackLengthMult;
	}

	if (actor->IsOnMount()) {
		length *= Settings::fMountedAttackLengthMult;
	}
	
	return length;
}

float PrecisionHandler::GetNodeAttackRadius(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_node, std::optional<float> a_overrideRadius /*= std::nullopt*/, float a_radiusMult /*= 1.f*/)
{
	auto actor = a_actorHandle.get();
	if (!actor) {
		return 0.f;
	}

	float actorScale = actor->GetScale();

	float radius = 0.f;

	if (a_overrideRadius) {
		radius = *a_overrideRadius;
	} else {
		if (auto capsuleShape = GetNodeCapsuleShape(a_node)) {
			radius = capsuleShape->radius;
		}
	}

	radius *= actorScale;
	radius *= *g_worldScaleInverse;
	radius *= a_radiusMult;

	bool bIsPlayer = actor->IsPlayerRef();
	if (bIsPlayer) {
		radius *= Settings::fPlayerAttackRadiusMult;
	}

	if (actor->IsOnMount()) {
		radius *= Settings::fMountedAttackRadiusMult;
	}

	return radius;	
}

std::shared_ptr<ActiveRagdoll> PrecisionHandler::GetActiveRagdollFromDriver(RE::hkbRagdollDriver* a_driver)
{
	auto search = activeRagdolls.find(a_driver);
	if (search != activeRagdolls.end()) {
		return search->second;
	}
	
	return nullptr;
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

float PrecisionHandler::GetAttackCollisionReach(RE::ActorHandle a_actorHandle, RequestedAttackCollisionType a_collisionType /*= RequestedAttackCollisionType::Default*/) const
{
	float length = 0.f;

	if (a_actorHandle) {
		auto actor = a_actorHandle.get();
		if (actor) {
			if (Settings::bDisableMod) {
				return Actor_GetReach(actor.get());
			}

			{
				ReadLocker locker(attackCollisionsLock);

				auto search = _actorsWithAttackCollisions.find(a_actorHandle);

				if (search != _actorsWithAttackCollisions.end() && !search->second.IsEmpty()) {  // actor has at least one active collision
					search->second.ForEachAttackCollision([&](std::shared_ptr<AttackCollision> attackCollision) {
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

			if (a_collisionType != RequestedAttackCollisionType::Current) {  // actor has no active collisions, calculate a default capsule length
				if (actor->currentProcess && actor->currentProcess->middleHigh) {
					AttackDefinition attackDefinition;
					
					bool bIsLeftHand;
					if (a_collisionType == RequestedAttackCollisionType::LeftWeapon) {
						bIsLeftHand = true;
					} else if (a_collisionType == RequestedAttackCollisionType::RightWeapon) {
						bIsLeftHand = false;
					} else {
						auto attackData = Actor_GetAttackData(actor.get());
						bIsLeftHand = attackData ? attackData->IsLeftAttack() : false;
					}

					if (GetAttackCollisionDefinition(actor.get(), attackDefinition, bIsLeftHand)) {
						for (auto& collisionDef : attackDefinition.collisions) {
							float collisionScale = collisionDef.transform ? collisionDef.transform->scale : 1.f;
							if (collisionDef.lengthMult) {
								collisionScale *= *collisionDef.lengthMult;
							}
							if (collisionDef.nodeName == "WEAPON"sv || collisionDef.nodeName == "SHIELD"sv) {
								auto equipment = bIsLeftHand ? actor->currentProcess->middleHigh->leftHand : actor->currentProcess->middleHigh->rightHand;
								if (equipment && equipment->object) {
									if (auto weapon = equipment->object->As<RE::TESObjectWEAP>()) {
										if (weapon && weapon->weaponData.animationType != RE::WEAPON_TYPE::kHandToHandMelee) {
											RE::NiAVObject* weaponNode = nullptr;
											auto niAVObject = actor->GetNodeByName(collisionDef.nodeName);
											if (niAVObject) {
												auto node = niAVObject->AsNode();
												if (node && node->children.size() > 0 && node->children[0]) {
													weaponNode = node->children[0].get();

													float nodeLength = GetWeaponAttackLength(a_actorHandle, weaponNode, collisionDef.capsuleLength, collisionScale);

													if (nodeLength > length) {
														length = nodeLength;
													}
												}
											}
											
											continue;
										}
									}
								}
								
								// fallthrough when no weapon equipped - set the node name to R hand or we'll try to get node length of the WEAPON/SHIELD node which is 0
								collisionDef.nodeName = "NPC R Hand [RHnd]"sv;
							}
							
							// not weapon
							auto node = actor->GetNodeByName(collisionDef.nodeName);
							if (node) {
								float nodeLength = GetNodeAttackLength(a_actorHandle, node, collisionDef.capsuleLength, collisionScale);
								float nodeRadius = GetNodeAttackRadius(a_actorHandle, node, collisionDef.capsuleRadius, collisionScale);

								nodeLength = std::fmax(nodeLength, nodeRadius);
								if (nodeLength > length) {
									length = nodeLength;
								}
							}
						}
					}
				}
			}
		}
		
		if (length == 0.f) {
			length = Actor_GetReach(actor.get());
		}
	}	

	return length;
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
