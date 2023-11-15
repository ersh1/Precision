#include "Hooks.h"

#include <xbyak/xbyak.h>

#include "Havok/ContactListener.h"
#include "PrecisionHandler.h"
#include "Settings.h"
#include "Utils.h"
#include "render/DrawHandler.h"
#include <glm/glm.hpp>

namespace Hooks
{
	void Install()
	{
		logger::trace("Hooking...");

		UpdateHooks::Hook();
		AttackHooks::Hook();

		AIHooks::Hook();

		HavokHooks::Hook();

		CameraShakeHook::Hook();
		MovementHook::Hook();

		FirstPersonStateHook::Hook();

		logger::trace("...success");
	}

	bool ActorHasAttackCollision(const RE::ActorHandle a_actorHandle)
	{
		return Settings::bAttackCollisionsEnabled && !Settings::bDisableMod && PrecisionHandler::ActorHasAttackCollision(a_actorHandle);
	}

	void UpdateHooks::Nullsub()
	{
		_Nullsub();

		PrecisionHandler::GetSingleton()->Update(*g_deltaTime);
		DrawHandler::GetSingleton()->Update(*g_deltaTime);
	}

	void UpdateHooks::ApplyMovement(RE::Actor* a_this, float a_deltaTime)
	{
		if (Settings::bEnableHitstop) {
			a_deltaTime *= PrecisionHandler::GetHitstopMultiplier(a_this->GetHandle(), a_deltaTime);
		}

		_ApplyMovement(a_this, a_deltaTime);
	}

	void UpdateHooks::Character_ModifyAnimationUpdateData(RE::Character* a_this, RE::BSAnimationUpdateData& a_data)
	{
		ModifyAnimationUpdateData(a_this, a_data);

		// set pitch for upper body modifier, don't use vanilla pitch because it's weird
		if (ShouldSetPitchModifier(a_this)) {
			//if (actorState->actorState2.weaponState == RE::WEAPON_STATE::kDrawn) {
			float dummy;
			auto& currentCombatTarget = a_this->GetActorRuntimeData().currentCombatTarget;
			if (currentCombatTarget && a_this->GetGraphVariableFloat("Collision_PitchMult"sv, dummy)) {  // only do this if the actor has a target and the graph contains the variable
				if (auto combatTarget = currentCombatTarget.get()) {
					RE::NiPoint3 actorPos;
					if (!Utils::GetTorsoPos(a_this, actorPos)) {
						actorPos = a_this->GetLookingAtLocation();
					}

					RE::NiPoint3 targetPos;
					if (!Utils::GetTorsoPos(combatTarget.get(), targetPos)) {
						targetPos = combatTarget->GetLookingAtLocation();
					}

					RE::NiPoint3 direction = targetPos - actorPos;
					direction.Unitize();

					// Pitch angle
					float pitch = atan2(direction.z, std::sqrtf(direction.x * direction.x + direction.y * direction.y));

					a_this->SetGraphVariableFloat("Collision_PitchMult"sv, Utils::RadianToDegree(pitch));
				}
			}
		} else {
			a_this->SetGraphVariableFloat("Collision_PitchMult"sv, 0.f);
		}

		_Character_ModifyAnimationUpdateData(a_this, a_data);
	}

	void UpdateHooks::PlayerCharacter_ModifyAnimationUpdateData(RE::PlayerCharacter* a_this, RE::BSAnimationUpdateData& a_data)
	{
		ModifyAnimationUpdateData(a_this, a_data);

		a_this->SetGraphVariableFloat("Collision_PitchMult"sv, -Utils::RadianToDegree(a_this->GetAngleX()));

		_PlayerCharacter_ModifyAnimationUpdateData(a_this, a_data);
	}

	bool UpdateHooks::ShouldSetPitchModifier(RE::Actor* a_this)
	{
		auto actorState = a_this->AsActorState();

		if (actorState->actorState1.meleeAttackState > RE::ATTACK_STATE_ENUM::kDraw /*&& actorState->actorState1.meleeAttackState < RE::ATTACK_STATE_ENUM::kBowDraw*/) {
			return true;
		}

		// additional checks for NickNak's creature mod (casting Draugrs etc)
		auto checkHand = [&](RE::TESForm* a_equippedObject) {
			if (a_equippedObject) {
				if (auto equippedSpell = a_equippedObject->As<RE::SpellItem>()) {
					if (a_this->IsCasting(equippedSpell)) {
						if (equippedSpell->GetDelivery() != RE::MagicSystem::Delivery::kSelf) {
							return true;
						}
					}
				}

				if (auto equippedWeapon = a_equippedObject->As<RE::TESObjectWEAP>()) {
					if (equippedWeapon->IsStaff()) {
						int iState = 0;
						a_this->GetGraphVariableInt("iState"sv, iState);

						if (iState == 10) {
							return true;
						}
					}
				}
			}

			return false;
		};

		if (checkHand(a_this->GetEquippedObject(false))) {
			return true;
		}

		if (checkHand(a_this->GetEquippedObject(true))) {
			return true;
		}

		return false;
	}

	void UpdateHooks::ModifyAnimationUpdateData(RE::Actor* a_this, RE::BSAnimationUpdateData& a_data)
	{
		if (Settings::bEnableHitstop) {
			auto actorHandle = a_this->GetHandle();
			float mult = PrecisionHandler::GetHitstopMultiplier(actorHandle, a_data.deltaTime);
			PrecisionHandler::UpdateHitstop(actorHandle, a_data.deltaTime);
			a_data.deltaTime *= mult;
		}
	}

	RE::Actor* UpdateHooks::GetActorFromRbx()
	{
		struct GetActorFromRbx : Xbyak::CodeGenerator
		{
			GetActorFromRbx()
			{
				mov(rax, rbx);
				ret();
			}
		};

		static GetActorFromRbx getActorFromRbx;

		return getActorFromRbx.getCode<RE::Actor* (*)()>()();
	}

	RE::Actor* AttackHooks::Func1(RE::Actor* a_this)
	{
		if (!Settings::bAttackCollisionsEnabled || Settings::bDisableMod || !PrecisionHandler::GetSingleton()->ActorHasAttackCollision(a_this->GetHandle())) {
			return _Func1(a_this);
		}

		return nullptr;
	}

	RE::TESObjectREFR* AttackHooks::Func2(RE::Actor* a_this)
	{
		if (!Settings::bAttackCollisionsEnabled || Settings::bDisableMod || !PrecisionHandler::GetSingleton()->ActorHasAttackCollision(a_this->GetHandle())) {
			return _Func2(a_this);
		}

		return nullptr;
	}

	void AttackHooks::ApplyPerkEntryPoint(RE::BGSEntryPoint::ENTRY_POINT a_entryPoint, RE::Actor* a_actor, RE::TESObjectREFR* a_object, float& a_outResult)
	{
		_ApplyPerkEntryPoint(a_entryPoint, a_actor, a_object, a_outResult);

		// Skip sweep attack if the actor has an active collision
		if (a_entryPoint == RE::BGSEntryPoint::ENTRY_POINT::kSetSweepAttack && ActorHasAttackCollision(a_actor->GetHandle())) {
			a_outResult = 0.f;
		}
	}

	void AttackHooks::HitData_Populate1(RE::HitData* a_this, RE::TESObjectREFR* a_source, RE::TESObjectREFR* a_target, RE::InventoryEntryData* a_weapon, bool a_bIsOffhand)
	{
		_HitData_Populate1(a_this, a_source, a_target, a_weapon, a_bIsOffhand);
		PrecisionHandler::cachedAttackData.GetPreciseHitVectors(a_this->hitPosition, a_this->hitDirection);
	}

	void AttackHooks::HitData_Populate2(RE::HitData* a_this, RE::TESObjectREFR* a_source, RE::TESObjectREFR* a_target, RE::InventoryEntryData* a_weapon, bool a_bIsOffhand)
	{
		_HitData_Populate2(a_this, a_source, a_target, a_weapon, a_bIsOffhand);
		PrecisionHandler::cachedAttackData.GetPreciseHitVectors(a_this->hitPosition, a_this->hitDirection);
	}

	RE::BSTempEffectParticle* AttackHooks::TESObjectCELL_PlaceParticleEffect(RE::TESObjectCELL* a_this, float a_lifetime, const char* a_modelName, RE::NiPoint3& a_rotation, RE::NiPoint3& a_pos, float a_scale, int32_t a_flags, RE::NiAVObject* a_target)
	{
		auto ret = _TESObjectCELL_PlaceParticleEffect(a_this, a_lifetime, a_modelName, a_rotation, a_pos, a_scale, a_flags, a_target ? a_target : PrecisionHandler::cachedAttackData.GetLastHitNode());
		return ret;
	}

	void AttackHooks::ApplyDeathForce(RE::Actor* a_this, RE::Actor* a_attacker, float a_deathForce, float a_mult, const RE::NiPoint3& a_hitDirection, const RE::NiPoint3& a_hitPosition, bool a_bIsRanged, RE::HitData* a_hitData)
	{
		if (!Settings::bApplyImpulseOnHit || !Settings::bApplyImpulseOnKill) {  // don't apply vanilla death force when bApplyImpulseOnKill is enabled
			_ApplyDeathForce(a_this, a_attacker, a_deathForce, a_mult, a_hitDirection, a_hitPosition, a_bIsRanged, a_hitData);
		}
	}

	RE::NiPointer<RE::BGSAttackData>& AttackHooks::HitActor_GetAttackData(RE::AIProcess* a_source)
	{
		auto& attackData = _HitActor_GetAttackData(a_source);

		if (attackData && a_source) {
			if (auto actor = a_source->GetUserData()) {
				if (auto race = actor->GetRace()) {
					return FixAttackData(attackData, race);
				}
			}
		}

		return attackData;
	}

	bool AttackHooks::CdPointCollectorCast(RE::hkpAllCdPointCollector* a_collector, RE::bhkWorld* a_world, RE::NiPoint3& a_origin, RE::NiPoint3& a_direction, float a_length)
	{
		if (PrecisionHandler::cachedAttackData.IsDataCached() && ActorHasAttackCollision(PrecisionHandler::cachedAttackData.GetAttackingActorHandle())) {
			return true;
		}

		return _CdPointCollectorCast(a_collector, a_world, a_origin, a_direction, a_length);
	}

	RE::NiPointer<RE::BGSAttackData>& AttackHooks::HitData_GetAttackData(RE::Actor* a_source)
	{
		auto& attackData = _HitData_GetAttackData(a_source);
		if (attackData && a_source) {
			if (auto race = a_source->GetRace()) {
				return FixAttackData(attackData, race);
			}
		}

		return attackData;
	}

	float AttackHooks::HitData_GetWeaponDamage(RE::InventoryEntryData* a_weapon, RE::ActorValueOwner* a_actorValueOwner, float a_damageMult, bool a4)
	{
		return _HitData_GetWeaponDamage(a_weapon, a_actorValueOwner, a_damageMult * PrecisionHandler::cachedAttackData.GetDamageMult(), a4);
	}

	void AttackHooks::HitData_GetBashDamage(RE::ActorValueOwner* a_actorValueOwner, float& a_outDamage)
	{
		_HitData_GetBashDamage(a_actorValueOwner, a_outDamage);
		a_outDamage *= PrecisionHandler::cachedAttackData.GetDamageMult();
	}

	void AttackHooks::HitData_GetUnarmedDamage(RE::ActorValueOwner* a_actorValueOwner, float& a_outDamage)
	{
		_HitData_GetUnarmedDamage(a_actorValueOwner, a_outDamage);
		a_outDamage *= PrecisionHandler::cachedAttackData.GetDamageMult();
	}

	float AttackHooks::HitData_GetStagger(RE::Actor* a_source, RE::Actor* a_target, RE::TESObjectWEAP* a_weapon, float a4)
	{
		/*if (PrecisionHandler::currentAttackStaggerOverride > 0.f) {
			return PrecisionHandler::currentAttackStaggerOverride;
		}*/

		return _HitData_GetStagger(a_source, a_target, a_weapon, a4) * PrecisionHandler::cachedAttackData.GetStaggerMult();
	}

	RE::NiPointer<RE::BGSAttackData>& AttackHooks::FixAttackData(RE::NiPointer<RE::BGSAttackData>& a_attackData, RE::TESRace* a_race)
	{
		if (a_attackData && a_race) {
			if (auto hittingNode = PrecisionHandler::cachedAttackData.GetHittingNode()) {
				if (hittingNode->parent) {
					bool bCollisionIsRightHand = hittingNode->parent->name == "WEAPON"sv;
					bool bCollisionIsLeftHand = hittingNode->parent->name == "SHIELD"sv;
					if ((bCollisionIsRightHand && a_attackData->IsLeftAttack()) || (bCollisionIsLeftHand && !a_attackData->IsLeftAttack())) {  // collision is on the opposite hand from the reported attack data, replace
						if (auto& leftHandAttackData = PrecisionHandler::GetOppositeAttackEvent(a_attackData, a_race->attackDataMap.get())) {
							return leftHandAttackData;
						}
					}
				}
			}
		}

		return a_attackData;
	}

	void HavokHooks::CullActors(void* a_this, RE::Actor* a_actor)
	{
		_CullActors(a_this, a_actor);

		if (!PrecisionHandler::GetSingleton()->IsActorActive(a_actor->GetHandle())) {
			return;
		}

		uint32_t cullState = 7;  // do not cull this actor

		auto& unk274 = a_actor->GetActorRuntimeData().unk274;
		unk274 &= 0xFFFFFFF0;
		unk274 |= cullState & 0xF;
	}

	void HavokHooks::SetWorld(RE::BShkbAnimationGraph* a_this, RE::bhkWorld* a_world)
	{
		_SetWorld(a_this, a_world);
	}

	static float worldChangedTime = 0.f;

	void HavokHooks::ProcessHavokHitJobs(void* a1)
	{
		_ProcessHavokHitJobs(a1);

		enum class hkpKnownWorldExtensionIds : int32_t
		{
			kAnonymous = -1,
			kBreakOffParts = 1000,
			kCollisionCallback = 1001
		};

		auto playerCharacter = RE::PlayerCharacter::GetSingleton();

		auto cell = playerCharacter->GetParentCell();
		if (!cell) {
			return;
		}

		auto world = RE::NiPointer<RE::bhkWorld>(cell->GetbhkWorld());
		if (!world) {
			return;
		}

		ContactListener* contactListener = &PrecisionHandler::contactListener;

		auto precisionHandler = PrecisionHandler::GetSingleton();

		if (world.get() != PrecisionHandler::contactListener.world) {
			precisionHandler->Clear();

			// Havok world changed
			{
				RE::BSWriteLockGuard lock(world->worldLock);

				RE::bhkCollisionFilter* filter = static_cast<RE::bhkCollisionFilter*>(world->GetWorld2()->collisionFilter);

				// run callbacks from other plugins
				precisionHandler->RunPrecisionLayerSetupCallbacks();

				AddPrecisionCollisionLayers(world.get());

				if (!hkpWorld_hasContactListener(world->GetWorld2(), contactListener)) {
					hkpCollisionCallbackUtil_requireCollisionCallbackUtil(world->GetWorld2());
					hkpWorld_addContactListener(world->GetWorld2(), contactListener);
					//hkpWorld_addWorldPostSimulationListener(world->GetWorld2(), &PrecisionHandler::contactListener);
				}

				// run callbacks from other plugins
				precisionHandler->RunCollisionFilterSetupCallbacks(filter);
			}

			PrecisionHandler::contactListener.world = world.get();
			worldChangedTime = 0.f;
		}

		EnsurePrecisionCollisionLayers(world.get());

		{  // Ensure our listener is the last one (will be called first)
			RE::hkArray<RE::hkpContactListener*>& listeners = world->GetWorld2()->contactListeners;
			if (listeners[listeners.size() - 1] != contactListener) {
				RE::BSWriteLockGuard lock(world->worldLock);

				int numListeners = listeners.size();
				int listenerIndex = -1;

				// get current index of our listener
				for (int i = 0; i < numListeners; ++i) {
					if (listeners[i] == contactListener) {
						listenerIndex = i;
						break;
					}
				}

				if (listenerIndex >= 0) {
					for (int i = listenerIndex + 1; i < numListeners; ++i) {
						listeners[i - 1] = listeners[i];
					}
					listeners[numListeners - 1] = contactListener;
				}
			}
		}

		if (worldChangedTime < Settings::fWorldChangedWaitTime) {
			worldChangedTime += *g_deltaTime;
			return;
		}

		const float activeDistanceSq = Settings::fActiveActorDistance * Settings::fActiveActorDistance;
		const float startDistanceSq = activeDistanceSq * (1 + Settings::fActiveDistanceHysteresis);
		const float endDistanceSq = activeDistanceSq * (1 - Settings::fActiveDistanceHysteresis);

		auto processActor = [&](RE::ActorHandle actorHandle) {
			if (auto actor = actorHandle.get().get()) {
				if (!actor || !actor->Get3D()) {
					return;
				}

				uint32_t filterInfo = 0;
				auto charController = actor->GetCharController();
				if (charController) {
					charController->GetCollisionFilterInfo(filterInfo);
				}
				uint16_t collisionGroup = filterInfo >> 16;

				bool bIsHittableCharController = PrecisionHandler::IsCharacterControllerHittableCollisionGroup(collisionGroup);

				bool bIsActorDisabled = PrecisionHandler::IsActorDisabled(actorHandle);

				bool bIsActorDead = actor->IsDead();

				bool bShouldAddToWorld = !Settings::bDisableMod && !bIsActorDisabled && !bIsActorDead && actor->GetPosition().GetSquaredDistance(playerCharacter->GetPosition()) < startDistanceSq;
				bool bShouldRemoveFromWorld = Settings::bDisableMod || bIsActorDisabled || bIsActorDead || actor->GetPosition().GetSquaredDistance(playerCharacter->GetPosition()) > endDistanceSq;

				bool bIsAddedToWorld = IsSkeletonAddedToWorld(actorHandle);

				bool bActiveActorsContainsHandle = PrecisionHandler::IsActorActive(actorHandle);
				bool bIsRagdollAdded = PrecisionHandler::IsRagdollAdded(actorHandle);
				bool bIsRagdollAddedToWorld = IsRagdollAddedToWorld(actorHandle);

				bool bIsActiveActor = bActiveActorsContainsHandle || bIsHittableCharController;
				bool bCanAddToWorld = CanAddToWorld(actorHandle);

				// Queue active ragdoll for removal if it's added even though ragdoll isn't
				if (bIsRagdollAdded && !bIsRagdollAddedToWorld) {
					WriteLocker locker(PrecisionHandler::ragdollsToRemoveLock);
					PrecisionHandler::ragdollsToRemove.emplace(actor->GetHandle());
				}

				if (bShouldAddToWorld) {
					if (!bIsAddedToWorld || !bIsActiveActor) {
						if (bCanAddToWorld) {
							AddSkeletonToWorld(actorHandle);
							if (auto race = actor->GetRace()) {
								if (race->data.flags.any(RE::RACE_DATA::Flag::kAllowRagdollCollision)) {
									WriteLocker locker(PrecisionHandler::ragdollCollisionGroupsLock);
									PrecisionHandler::ragdollCollisionGroups.insert(collisionGroup);
								}
							}
						} else {
							// There is no ragdoll instance, but we still need a way to hit the enemy, e.g. for the wisp (witchlight).
							// In this case, we need to register collisions against their charcontroller.
							WriteLocker locker(PrecisionHandler::hittableCharControllerGroupsLock);
							PrecisionHandler::hittableCharControllerGroups.insert(collisionGroup);
						}

						if (actorHandle.native_handle() == 0x100000) {
							PrecisionHandler::AddPlayerSink();
						} else {
							PrecisionHandler::AddActorSink(actor);
						}
					}

					// Add ragdoll if needed
					if (!bIsRagdollAdded && bCanAddToWorld) {
						bool bShouldAddRagdollToWorld = false;
						{
							ReadLocker locker(PrecisionHandler::ragdollsToAddLock);
							bShouldAddRagdollToWorld = PrecisionHandler::ragdollsToAdd.contains(actorHandle);
						}
						if (bShouldAddRagdollToWorld) {
							if (!bIsRagdollAddedToWorld || !bIsRagdollAdded) {
								AddRagdollToWorld(actorHandle);
							}
						}
					}

					if (bIsRagdollAdded) {
						// Sometimes the game re-enables sync-on-update e.g. when switching outfits, so we need to make sure it's disabled.
						DisableSyncOnUpdate(actor);

						if (Settings::bForceAnimationUpdateForActiveActors) {
							// Force the game to run the animation graph update (and hence driveToPose, etc.)
							actor->GetActorRuntimeData().boolFlags.set(RE::Actor::BOOL_FLAGS::kForceAnimGraphUpdate);
						}
					}
				} else if (bShouldRemoveFromWorld) {
					if (bIsAddedToWorld) {
						if (bCanAddToWorld) {
							RemoveSkeletonFromWorld(actorHandle);

							bool bIsRagdollCollision = false;

							{
								ReadLocker locker(PrecisionHandler::ragdollCollisionGroupsLock);
								bIsRagdollCollision = PrecisionHandler::ragdollCollisionGroups.size() > 0 && PrecisionHandler::ragdollCollisionGroups.contains(collisionGroup);
							}

							if (bIsRagdollCollision) {
								WriteLocker locker(PrecisionHandler::ragdollCollisionGroupsLock);
								PrecisionHandler::ragdollCollisionGroups.erase(collisionGroup);
							}
						} else {
							if (bIsHittableCharController) {
								WriteLocker locker(PrecisionHandler::hittableCharControllerGroupsLock);
								PrecisionHandler::hittableCharControllerGroups.erase(collisionGroup);
							}
						}

						if (actorHandle.native_handle() == 0x100000) {
							PrecisionHandler::RemovePlayerSink();
						} else {
							PrecisionHandler::RemoveActorSink(actor);
						}
					}
				}

				// Remove ragdoll if necessary
				bool bShouldRemoveRagdollFromWorld = ShouldRemoveRagdollFromWorld(actorHandle);
				if (bShouldRemoveFromWorld || bShouldRemoveRagdollFromWorld) {
					if (bIsRagdollAdded && bCanAddToWorld) {
						RemoveRagdollFromWorld(actorHandle);
					}
				}

				//// bonus fix for character controller position desync with 'tai'
				//if (charController && !actor->GetActorRuntimeData().boolBits.any(RE::Actor::BOOL_BITS::kProcessMe) && bIsHittableCharController) {
				//	auto actorRoot = actor->Get3D();
				//	auto rootPos = actorRoot->world.translate;
				//	auto rootPosHavok = rootPos * *g_worldScale;

				//	RE::hkTransform transform;
				//	charController->GetTransformImpl(transform);
				//	float zOffset = transform.translation.quad.m128_f32[2] - rootPosHavok.z;
				//	transform.translation = Utils::NiPointToHkVector(rootPosHavok);
				//	transform.translation.quad.m128_f32[2] += zOffset;
				//	charController->SetTransformImpl(transform);
				//}
			}
		};

		{
			processActor(playerCharacter->GetHandle());

			auto processLists = RE::ProcessLists::GetSingleton();
			for (auto& actorHandle : processLists->highActorHandles) {
				processActor(actorHandle);
			}
		}

		{
			WriteLocker locker(PrecisionHandler::ragdollsToAddLock);
			WriteLocker locker2(PrecisionHandler::ragdollsToRemoveLock);
			PrecisionHandler::ragdollsToAdd.clear();
			PrecisionHandler::ragdollsToRemove.clear();
		}

		precisionHandler->ProcessPostHavokHitJobs();
	}

	void HavokHooks::BShkbAnimationGraph_UpdateAnimation(RE::BShkbAnimationGraph* a_this, RE::BShkbAnimationGraph_UpdateData* a_updateData, void* a3)
	{
		//RE::Actor* actor = a_this->holder;
		//if (a3 && actor) {  // a3 is null if the graph is not active
		//	auto actorHandle = actor->GetHandle();
		//	if (actorHandle && PrecisionHandler::activeActors.count(actorHandle)) {
		//		a_updateData->unk2A = true;                      // forces animation update (hkbGenerator::generate()) without skipping frames
		//	}
		//}

		_BShkbAnimationGraph_UpdateAnimation(a_this, a_updateData, a3);
	}

	void HavokHooks::hkbRagdollDriver_DriveToPose(RE::hkbRagdollDriver* a_driver, float a_deltaTime, const RE::hkbContext& a_context, RE::hkbGeneratorOutput& a_generatorOutput)
	{
		PreDriveToPose(a_driver, a_deltaTime, a_context, a_generatorOutput);
		_hkbRagdollDriver_DriveToPose(a_driver, a_deltaTime, a_context, a_generatorOutput);
		PostDriveToPose(a_driver, a_deltaTime, a_context, a_generatorOutput);
	}

	void HavokHooks::hkbRagdollDriver_PostPhysics(RE::hkbRagdollDriver* a_driver, const RE::hkbContext& a_context, RE::hkbGeneratorOutput& a_generatorInOut)
	{
		PrePostPhysics(a_driver, a_context, a_generatorInOut);
		_hkbRagdollDriver_PostPhysics(a_driver, a_context, a_generatorInOut);
		PostPostPhysics(a_driver, a_context, a_generatorInOut);
		PostPostPostPhysics(a_driver);
	}

	bool HavokHooks::BShkbAnimationGraph_ShouldAddToGraphListeners(RE::BShkbAnimationGraph* a_this)
	{
		/*if (a_this->holder && a_this->holder->IsInRagdollState()) {
			return _BShkbAnimationGraph_ShouldAddToGraphListeners(a_this);
		}

		return false;*/

		//a_this->physicsWorld = nullptr;
		return _BShkbAnimationGraph_ShouldAddToGraphListeners(a_this);
	}

	void HavokHooks::QueueTask_ToggleCharacterBumper(void* a_taskManager, RE::Actor* a_actor, bool a_enable)
	{
		if (Settings::bDisableCharacterBumper) {
			a_enable = false;
		}

		_QueueTask_ToggleCharacterBumper(a_taskManager, a_actor, a_enable);
	}

	void HavokHooks::ToggleCharacterBumper(RE::bhkCharacterController* a_charController, bool a_enable)
	{
		if (Settings::bDisableCharacterBumper) {
			a_enable = false;
		}

		_ToggleCharacterBumper(a_charController, a_enable);
	}

	bool HavokHooks::bhkCollisionFilter_CompareFilterInfo1(RE::bhkCollisionFilter* a_this, uint32_t a_filterInfoA, uint32_t a_filterInfoB)
	{
		switch (CompareFilterInfo(a_this, a_filterInfoA, a_filterInfoB)) {
		case CollisionFilterComparisonResult::Continue:
		default:
			return _bhkCollisionFilter_CompareFilterInfo1(a_this, a_filterInfoA, a_filterInfoB);
		case CollisionFilterComparisonResult::Collide:
			return true;
		case CollisionFilterComparisonResult::Ignore:
			return false;
		}
	}

	bool HavokHooks::bhkCollisionFilter_CompareFilterInfo2(RE::bhkCollisionFilter* a_this, uint32_t a_filterInfoA, uint32_t a_filterInfoB)
	{
		switch (CompareFilterInfo(a_this, a_filterInfoA, a_filterInfoB)) {
		case CollisionFilterComparisonResult::Continue:
		default:
			return _bhkCollisionFilter_CompareFilterInfo2(a_this, a_filterInfoA, a_filterInfoB);
		case CollisionFilterComparisonResult::Collide:
			return true;
		case CollisionFilterComparisonResult::Ignore:
			return false;
		}
	}

	bool HavokHooks::bhkCollisionFilter_CompareFilterInfo3(RE::bhkCollisionFilter* a_this, uint32_t a_filterInfoA, uint32_t a_filterInfoB)
	{
		switch (CompareFilterInfo(a_this, a_filterInfoA, a_filterInfoB)) {
		case CollisionFilterComparisonResult::Continue:
		default:
			return _bhkCollisionFilter_CompareFilterInfo3(a_this, a_filterInfoA, a_filterInfoB);
		case CollisionFilterComparisonResult::Collide:
			return true;
		case CollisionFilterComparisonResult::Ignore:
			return false;
		}
	}

	bool HavokHooks::bhkCollisionFilter_CompareFilterInfo4(RE::bhkCollisionFilter* a_this, uint32_t a_filterInfoA, uint32_t a_filterInfoB)
	{
		switch (CompareFilterInfo(a_this, a_filterInfoA, a_filterInfoB)) {
		case CollisionFilterComparisonResult::Continue:
		default:
			return _bhkCollisionFilter_CompareFilterInfo4(a_this, a_filterInfoA, a_filterInfoB);
		case CollisionFilterComparisonResult::Collide:
			return true;
		case CollisionFilterComparisonResult::Ignore:
			return false;
		}
	}

	bool HavokHooks::bhkCollisionFilter_CompareFilterInfo5(RE::bhkCollisionFilter* a_this, uint32_t a_filterInfoA, uint32_t a_filterInfoB)
	{
		switch (CompareFilterInfo(a_this, a_filterInfoA, a_filterInfoB)) {
		case CollisionFilterComparisonResult::Continue:
		default:
			return _bhkCollisionFilter_CompareFilterInfo5(a_this, a_filterInfoA, a_filterInfoB);
		case CollisionFilterComparisonResult::Collide:
			return true;
		case CollisionFilterComparisonResult::Ignore:
			return false;
		}
	}

	bool HavokHooks::bhkCollisionFilter_CompareFilterInfo6(RE::bhkCollisionFilter* a_this, uint32_t a_filterInfoA, uint32_t a_filterInfoB)
	{
		switch (CompareFilterInfo(a_this, a_filterInfoA, a_filterInfoB)) {
		case CollisionFilterComparisonResult::Continue:
		default:
			return _bhkCollisionFilter_CompareFilterInfo6(a_this, a_filterInfoA, a_filterInfoB);
		case CollisionFilterComparisonResult::Collide:
			return true;
		case CollisionFilterComparisonResult::Ignore:
			return false;
		}
	}

	bool HavokHooks::bhkCollisionFilter_CompareFilterInfo7(RE::bhkCollisionFilter* a_this, uint32_t a_filterInfoA, uint32_t a_filterInfoB)
	{
		switch (CompareFilterInfo(a_this, a_filterInfoA, a_filterInfoB)) {
		case CollisionFilterComparisonResult::Continue:
		default:
			return _bhkCollisionFilter_CompareFilterInfo7(a_this, a_filterInfoA, a_filterInfoB);
		case CollisionFilterComparisonResult::Collide:
			return true;
		case CollisionFilterComparisonResult::Ignore:
			return false;
		}
	}

	void HavokHooks::HookPrePhysicsStep()
	{
		static REL::Relocation<uintptr_t> hook{ RELOCATION_ID(76018, 77851) };  // DA64E0, DE5FE0 - main loop

		struct PatchSE : Xbyak::CodeGenerator
		{
			explicit PatchSE(uintptr_t funcAddr)
			{
				Xbyak::Label originalLabel;

				// DA6789
				sub(rsp, 0x20);  // Need an additional 0x20 bytes for scratch space
				mov(rcx, r12);   // the bhkWorld is at r12

				// Call our hook
				mov(rax, funcAddr);
				call(rax);

				add(rsp, 0x20);

				// Original code
				mov(rcx, r13);
				test(r14b, r14b);

				jmp(ptr[rip + originalLabel]);

				L(originalLabel);
				dq(hook.address() + 0x2AF);
			}
		};

		struct PatchAE : Xbyak::CodeGenerator
		{
			explicit PatchAE(uintptr_t funcAddr)
			{
				Xbyak::Label originalLabel;

				// DE63E7
				sub(rsp, 0x20);  // Need an additional 0x20 bytes for scratch space
				mov(rcx, r14);   // the bhkWorld is at r14

				// Call our hook
				mov(rax, funcAddr);
				call(rax);

				add(rsp, 0x20);

				// Original code
				mov(rcx, r13);
				cmp(r12d, 3);

				jmp(ptr[rip + originalLabel]);

				L(originalLabel);
				dq(hook.address() + 0x40E);
			}
		};

		if (REL::Module::IsAE()) {
			PatchAE patch(reinterpret_cast<uintptr_t>(PrePhysicsStep));
			patch.ready();

			auto& trampoline = SKSE::GetTrampoline();
			trampoline.write_branch<6>(hook.address() + RELOCATION_OFFSET(0x2A9, 0x407), trampoline.allocate(patch));
		} else {
			PatchSE patch(reinterpret_cast<uintptr_t>(PrePhysicsStep));
			patch.ready();

			auto& trampoline = SKSE::GetTrampoline();
			trampoline.write_branch<6>(hook.address() + RELOCATION_OFFSET(0x2A9, 0x407), trampoline.allocate(patch));
		}
	}

	void HavokHooks::AddPrecisionCollisionLayers(RE::bhkWorld* a_world)
	{
		RE::bhkCollisionFilter* worldFilter = (RE::bhkCollisionFilter*)a_world->GetWorld1()->collisionFilter;

		// Create our attack layer in the first unused vanilla layer (56)
		worldFilter->layerBitfields[static_cast<int32_t>(CollisionLayer::kPrecisionAttack)] = Settings::iPrecisionAttackLayerBitfield;
		worldFilter->collisionLayerNames[static_cast<int32_t>(CollisionLayer::kPrecisionAttack)] = Settings::sPrecisionAttackLayerName;
		// Set whether other layers should collide with our new layer
		ReSyncLayerBitfields(worldFilter, CollisionLayer::kPrecisionAttack);

		// Do the same for our body layer, in the next unused vanilla layer (57)
		worldFilter->layerBitfields[static_cast<int32_t>(CollisionLayer::kPrecisionBody)] = Settings::iPrecisionBodyLayerBitfield;
		worldFilter->collisionLayerNames[static_cast<int32_t>(CollisionLayer::kPrecisionBody)] = Settings::sPrecisionBodyLayerName;
		// Set whether other layers should collide with our new layer
		ReSyncLayerBitfields(worldFilter, CollisionLayer::kPrecisionBody);

		// Do the same for our recoil layer, in the next unused vanilla layer (58)
		worldFilter->layerBitfields[static_cast<int32_t>(CollisionLayer::kPrecisionRecoil)] = Settings::iPrecisionRecoilLayerBitfield;
		worldFilter->collisionLayerNames[static_cast<int32_t>(CollisionLayer::kPrecisionRecoil)] = Settings::sPrecisionRecoilLayerName;
		// Set whether other layers should collide with our new layer
		ReSyncLayerBitfields(worldFilter, CollisionLayer::kPrecisionRecoil);
	}

	void HavokHooks::EnsurePrecisionCollisionLayers(RE::bhkWorld* a_world)
	{
		RE::bhkCollisionFilter* worldFilter = (RE::bhkCollisionFilter*)a_world->GetWorld1()->collisionFilter;

		uint64_t currentPrecisionAttackBitfield = worldFilter->layerBitfields[static_cast<int32_t>(CollisionLayer::kPrecisionAttack)];
		if (currentPrecisionAttackBitfield != Settings::iPrecisionAttackLayerBitfield) {
			RE::BSWriteLockGuard lock(a_world->worldLock);
			worldFilter->layerBitfields[static_cast<int32_t>(CollisionLayer::kPrecisionAttack)] = Settings::iPrecisionAttackLayerBitfield;
			ReSyncLayerBitfields(worldFilter, CollisionLayer::kPrecisionAttack);
		}

		uint64_t currentPrecisionBodyBitfield = worldFilter->layerBitfields[static_cast<int32_t>(CollisionLayer::kPrecisionBody)];
		if (currentPrecisionBodyBitfield != Settings::iPrecisionBodyLayerBitfield) {
			RE::BSWriteLockGuard lock(a_world->worldLock);
			worldFilter->layerBitfields[static_cast<int32_t>(CollisionLayer::kPrecisionBody)] = Settings::iPrecisionBodyLayerBitfield;
			ReSyncLayerBitfields(worldFilter, CollisionLayer::kPrecisionBody);
		}

		uint64_t currentPrecisionRecoilBitfield = worldFilter->layerBitfields[static_cast<int32_t>(CollisionLayer::kPrecisionRecoil)];
		if (currentPrecisionRecoilBitfield != Settings::iPrecisionRecoilLayerBitfield) {
			RE::BSWriteLockGuard lock(a_world->worldLock);
			worldFilter->layerBitfields[static_cast<int32_t>(CollisionLayer::kPrecisionRecoil)] = Settings::iPrecisionRecoilLayerBitfield;
			ReSyncLayerBitfields(worldFilter, CollisionLayer::kPrecisionRecoil);
		}
	}

	void HavokHooks::ReSyncLayerBitfields(RE::bhkCollisionFilter* a_filter, CollisionLayer a_layer)
	{
		uint64_t bitfield = a_filter->layerBitfields[static_cast<uint8_t>(a_layer)];
		for (int i = 0; i < 64; i++) {  // 56 layers in vanilla
			if ((bitfield >> i) & 1) {
				a_filter->layerBitfields[i] |= (static_cast<uint64_t>(1) << static_cast<uint8_t>(a_layer));
			} else {
				a_filter->layerBitfields[i] &= ~(static_cast<uint64_t>(1) << static_cast<uint8_t>(a_layer));
			}
		}
	}

	bool HavokHooks::CanAddToWorld(RE::ActorHandle a_actorHandle)
	{
		if (!a_actorHandle) {
			return false;
		}

		RE::Actor* actor = a_actorHandle.get().get();

		RE::BSAnimationGraphManagerPtr animGraphManager;
		if (!actor->GetAnimationGraphManager(animGraphManager)) {
			return false;
		}

		if (!actor->GetActorRuntimeData().boolBits.any(RE::Actor::BOOL_BITS::kProcessMe)) {
			return false;
		}

		{
			RE::BSSpinLockGuard animGraphLocker(animGraphManager->GetRuntimeData().updateLock);

			if (animGraphManager->graphs.size() <= 0) {
				return false;
			}

			for (auto& graph : animGraphManager->graphs) {
				auto& driver = graph->characterInstance.ragdollDriver;
				if (!driver || !driver->ragdoll) {
					return false;
				} else if (a_actorHandle.native_handle() == 0x100000) {  // is player, so return true before we check the first person driver's ragdoll and find a nullptr
					return true;
				}
			}
		}

		return true;
	}

	bool HavokHooks::IsSkeletonAddedToWorld(RE::ActorHandle a_actorHandle)
	{
		ReadLocker locker(PrecisionHandler::activeActorsLock);

		return PrecisionHandler::activeActors.size() > 0 && PrecisionHandler::activeActors.contains(a_actorHandle);
	}

	bool HavokHooks::AddSkeletonToWorld(RE::ActorHandle a_actorHandle)
	{
		return CloneSkeleton(a_actorHandle);
	}

	bool HavokHooks::RemoveSkeletonFromWorld(RE::ActorHandle a_actorHandle)
	{
		WriteLocker locker(PrecisionHandler::activeActorsLock);
		return PrecisionHandler::activeActors.erase(a_actorHandle);
	}

	bool HavokHooks::IsRagdollAddedToWorld(RE::ActorHandle a_actorHandle)
	{
		if (!a_actorHandle) {
			return false;
		}

		RE::Actor* actor = a_actorHandle.get().get();

		RE::BSAnimationGraphManagerPtr animGraphManager;
		if (!actor->GetAnimationGraphManager(animGraphManager)) {
			return false;
		}

		{
			RE::BSSpinLockGuard animGraphLocker(animGraphManager->GetRuntimeData().updateLock);

			if (animGraphManager->graphs.size() <= 0) {
				return false;
			}

			for (auto& graph : animGraphManager->graphs) {
				if (!graph->physicsWorld) {
					return false;
				}

				auto& driver = graph->characterInstance.ragdollDriver;
				if (!driver) {
					return false;
				}

				auto& ragdoll = driver->ragdoll;
				if (!ragdoll) {
					return false;
				}

				auto& root = ragdoll->rigidBodies[0];
				if (!root || !root->world) {
					return false;
				}

				if (a_actorHandle.native_handle() == 0x100000) {  // is player, so return true before we check the first person driver's ragdoll and find a nullptr
					return true;
				}
			}
		}

		return true;
	}

	bool HavokHooks::AddRagdollToWorld(RE::ActorHandle a_actorHandle)
	{
		if (!a_actorHandle) {
			return false;
		}

		RE::Actor* actor = a_actorHandle.get().get();

		if (actor->IsInRagdollState()) {
			return false;
		}

		bool bHasRagdollInterface = false;
		RE::BSAnimationGraphManagerPtr animGraphManager;
		if (actor->GetAnimationGraphManager(animGraphManager)) {
			BSAnimationGraphManager_HasRagdollInterface(animGraphManager.get(), &bHasRagdollInterface);
		}

		if (bHasRagdollInterface) {
			{
				RE::BSSpinLockGuard animGraphLocker(animGraphManager->GetRuntimeData().updateLock);
				for (auto& graph : animGraphManager->graphs) {
					auto& driver = graph->characterInstance.ragdollDriver;
					if (driver) {
						{
							WriteLocker locker(PrecisionHandler::activeRagdollsLock);
							auto activeRagdoll = std::make_shared<ActiveRagdoll>();
							PrecisionHandler::activeRagdolls.emplace(driver.get(), activeRagdoll);
							PrecisionHandler::activeActorsWithRagdolls.emplace(a_actorHandle);

							Blender& blender = activeRagdoll->blender;
							blender.StartBlend(Blender::BlendType::kAnimToRagdoll, Settings::fBlendInTime);

							RE::hkQsTransform* poseLocal = hkbCharacter_getPoseLocal(driver->character);
							blender.initialPose.assign(poseLocal, poseLocal + driver->character->numPoseLocal);
							blender.bIsFirstBlendFrame = false;

							activeRagdoll->state = RagdollState::kBlendIn;

							if (!graph->physicsWorld) {
								// World must be set before calling BShkbAnimationGraph::AddRagdollToWorld(), and is required for the graph to register its physics step listener (and hence call hkbRagdollDriver::driveToPose())
								graph->physicsWorld = actor->parentCell->GetbhkWorld();
								activeRagdoll->shouldNullOutWorldWhenRemovingFromWorld = true;
							}
						}
					}
				}
			}

			{
				RE::BSSpinLockGuard animGraphLocker(animGraphManager->GetRuntimeData().updateLock);
				for (auto& graph : animGraphManager->graphs) {
					if (graph->AddRagdollToWorld()) {
						break;
					}
				}
			}

			ModifyConstraints(actor);

			{
				RE::BSSpinLockGuard animGraphLocker(animGraphManager->GetRuntimeData().updateLock);
				for (auto& graph : animGraphManager->graphs) {
					graph->SetRagdollConstraintsFromBhkConstraints();
				}
			}
		}

		return true;
	}

	bool HavokHooks::RemoveRagdollFromWorld(RE::ActorHandle a_actorHandle)
	{
		if (!a_actorHandle) {
			return false;
		}

		RE::Actor* actor = a_actorHandle.get().get();

		// only remove if the actor isn't in a state that should have a ragdoll added (dead, using furniture etc.)
		bool bCanRemoveFromWorld = CanRemoveRagdollFromWorld(a_actorHandle);

		bool bHasRagdollInterface = false;
		RE::BSAnimationGraphManagerPtr animGraphManager;
		if (actor->GetAnimationGraphManager(animGraphManager)) {
			BSAnimationGraphManager_HasRagdollInterface(animGraphManager.get(), &bHasRagdollInterface);
		}

		if (bHasRagdollInterface) {
			if (bCanRemoveFromWorld) {
				RE::BSSpinLockGuard animGraphLocker(animGraphManager->GetRuntimeData().updateLock);
				for (auto& graph : animGraphManager->graphs) {
					if (graph->RemoveRagdollFromWorld()) {
						break;
					}
				}
			}

			{
				RE::BSSpinLockGuard animGraphLocker(animGraphManager->GetRuntimeData().updateLock);
				for (auto& graph : animGraphManager->graphs) {
					auto& driver = graph->characterInstance.ragdollDriver;
					if (driver) {
						if (bCanRemoveFromWorld) {
							if (auto ragdoll = PrecisionHandler::GetActiveRagdollFromDriver(driver.get())) {
								if (ragdoll->shouldNullOutWorldWhenRemovingFromWorld) {
									graph->physicsWorld = nullptr;
								}
							}
						} else {
							// Set bones to keyframed
							Utils::SetBonesKeyframed(driver.get());
						}

						{
							WriteLocker locker(PrecisionHandler::activeRagdollsLock);
							PrecisionHandler::activeRagdolls.erase(driver.get());
							PrecisionHandler::activeActorsWithRagdolls.erase(a_actorHandle);
						}
					}
				}
			}
		}

		return true;
	}

	bool HavokHooks::ShouldRemoveRagdollFromWorld(RE::ActorHandle a_actorHandle)
	{
		if (!a_actorHandle) {
			return false;
		}

		{
			ReadLocker locker(PrecisionHandler::ragdollsToRemoveLock);
			if (PrecisionHandler::ragdollsToRemove.contains(a_actorHandle)) {
				return true;
			}
		}

		RE::Actor* actor = a_actorHandle.get().get();
		if (actor->IsInKillMove()) {
			return true;
		}

		return false;
	}

	bool HavokHooks::CanRemoveRagdollFromWorld(RE::ActorHandle a_actorHandle)
	{
		if (!a_actorHandle) {
			return false;
		}

		RE::Actor* actor = a_actorHandle.get().get();

		if (actor->IsInRagdollState() || actor->GetOccupiedFurniture()) {
			return false;
		}

		if (auto race = actor->GetRace()) {
			if (race->data.flags.any(RE::RACE_DATA::Flag::kAllowRagdollCollision)) {
				return false;
			}
		}

		return true;
	}

	void HavokHooks::ModifyConstraints(RE::Actor* a_actor)
	{
		using ConstraintType = RE::hkpConstraintData::ConstraintType;

		Utils::ForEachRagdollDriver(a_actor, [](RE::hkbRagdollDriver* a_driver) {
			RE::hkaRagdollInstance* ragdoll = hkbRagdollDriver_getRagdoll(a_driver);
			if (!ragdoll) {
				return;
			}

			for (RE::hkpRigidBody* rigidBody : ragdoll->rigidBodies) {
				auto node = GetNiObjectFromCollidable(rigidBody->GetCollidable());
				if (node) {
					auto wrapper = Utils::GetRigidBody(node);
					if (wrapper) {
						bhkRigidBody_setMotionType(wrapper, RE::hkpMotion::MotionType::kDynamic, RE::hkpEntityActivation::kDoActivate, RE::hkpUpdateCollisionFilterOnEntityMode::kFullCheck);

						hkRealTohkUFloat8(rigidBody->GetMotionState()->maxLinearVelocity, Settings::fRagdollBoneMaxLinearVelocity);
						hkRealTohkUFloat8(rigidBody->GetMotionState()->maxAngularVelocity, Settings::fRagdollBoneMaxAngularVelocity);
					}
				}
			}

			if (Settings::bConvertHingeConstraintsToRagdollConstraints) {
				// Convert any limited hinge constraints to ragdoll constraints so that they can be loosened properly
				for (RE::hkpRigidBody* rigidBody : ragdoll->rigidBodies) {
					auto node = GetNiObjectFromCollidable(rigidBody->GetCollidable());
					if (node) {
						auto wrapper = Utils::GetRigidBody(node);
						if (wrapper) {
							for (auto& unk : wrapper->unk28) {
								auto constraint = reinterpret_cast<RE::bhkConstraint*>(unk);
								auto constraintInstance = static_cast<RE::hkpConstraintInstance*>(constraint->referencedObject.get());
								if (constraintInstance->data->GetType() == ConstraintType::kLimitedHinge) {
									RE::bhkRagdollConstraint* ragdollConstraint = ConvertToRagdollConstraint(constraint);
									if (ragdollConstraint) {
										constraint->RemoveFromCurrentWorld();

										RE::bhkWorld* world = reinterpret_cast<RE::bhkWorld*>(wrapper->GetWorld2()->unk430);
										ragdollConstraint->MoveToWorld(world);
										unk = ragdollConstraint;
									}
								}
							}
						}
					}
				}
			}
		});
	}

	void HavokHooks::DisableSyncOnUpdate(RE::Actor* a_actor)
	{
		if (a_actor->IsInRagdollState())
			return;

		bool bHasRagdollInterface = false;
		RE::BSAnimationGraphManagerPtr animGraphManager;
		if (a_actor->GetAnimationGraphManager(animGraphManager)) {
			BSAnimationGraphManager_HasRagdollInterface(animGraphManager.get(), &bHasRagdollInterface);
		}

		if (bHasRagdollInterface) {
			RE::BSSpinLockGuard animGraphLocker(animGraphManager->GetRuntimeData().updateLock);
			for (auto& graph : animGraphManager->graphs) {
				graph->ToggleSyncOnUpdate(true);
			}
		}
	}

	void HavokHooks::ConvertLimitedHingeDataToRagdollConstraintData(RE::hkpRagdollConstraintData* a_ragdollData, RE::hkpLimitedHingeConstraintData* a_limitedHingeData)
	{
		// gather data from limited hinge constraint
		RE::hkVector4& childPivot = a_limitedHingeData->atoms.transforms.transformA.translation;
		RE::hkVector4& parentPivot = a_limitedHingeData->atoms.transforms.transformB.translation;
		RE::hkVector4& childPlane = a_limitedHingeData->atoms.transforms.transformA.rotation.col0;
		RE::hkVector4& parentPlane = a_limitedHingeData->atoms.transforms.transformB.rotation.col0;

		// get childTwist axis and compute a parentTwist axis which closely matches the limited hinge's min/max limits.
		float minAng = a_limitedHingeData->atoms.angLimit.minAngle;
		float maxAng = a_limitedHingeData->atoms.angLimit.maxAngle;
		float angExtents = (maxAng - minAng) / 2.0f;

		RE::hkQuaternion minAngRotation = Utils::NiQuatToHkQuat(Utils::MatrixToQuaternion(Utils::MatrixFromAxisAngle(Utils::HkVectorToNiPoint(parentPlane), -angExtents + maxAng)));
		Utils::NormalizeHkQuat(minAngRotation);

		RE::hkVector4& childTwist = a_limitedHingeData->atoms.transforms.transformA.rotation.col2;
		RE::hkVector4& parentLimitedAxis = a_limitedHingeData->atoms.transforms.transformB.rotation.col2;
		RE::hkVector4 parentTwist;
		Utils::SetRotatedDir(parentTwist, minAngRotation, parentLimitedAxis);

		hkpRagdollConstraintData_setInBodySpace(a_ragdollData, childPivot, parentPivot, childPlane, parentPlane, childTwist, parentTwist);

		// adjust limits to make it like the hinge constraint
		a_ragdollData->atoms.coneLimit.maxAngle = angExtents;
		a_ragdollData->atoms.twistLimit.minAngle = 0.f;
		a_ragdollData->atoms.twistLimit.maxAngle = 0.f;
		a_ragdollData->atoms.angFriction.maxFrictionTorque = a_limitedHingeData->atoms.angFriction.maxFrictionTorque;
		a_ragdollData->atoms.twistLimit.angularLimitsTauFactor = a_limitedHingeData->atoms.angLimit.angularLimitsTauFactor;
		a_ragdollData->atoms.coneLimit.angularLimitsTauFactor = a_limitedHingeData->atoms.angLimit.angularLimitsTauFactor;
		a_ragdollData->atoms.planesLimit.angularLimitsTauFactor = a_limitedHingeData->atoms.angLimit.angularLimitsTauFactor;
	}

	void HavokHooks::SetPivotInWorldSpace(RE::hkpRagdollConstraintData* a_constraint, const RE::hkTransform& a_bodyATransform, const RE::hkTransform& a_bodyBTransform, const RE::hkVector4& a_pivot)
	{
		hkVector4_setTransformedInversePos(a_constraint->atoms.transforms.transformA.translation, a_bodyATransform, a_pivot);
		hkVector4_setTransformedInversePos(a_constraint->atoms.transforms.transformB.translation, a_bodyBTransform, a_pivot);
	}

	RE::bhkRagdollConstraint* HavokHooks::ConvertToRagdollConstraint(RE::bhkConstraint* a_constraint)
	{
		if (skyrim_cast<RE::bhkRagdollConstraint*>(a_constraint)) {  // already a bhkRagdollConstraint
			return nullptr;
		}

		RE::hkRagdollConstraintCinfo cInfo;
		hkRagdollConstraintCinfo_Func4(&cInfo);  // Creates constraintData and calls hkpRagdollConstraintData_ctor()
		auto constraintInstance = static_cast<RE::hkpConstraintInstance*>(a_constraint->referencedObject.get());
		cInfo.rigidBodyA = constraintInstance->GetRigidBodyA();
		cInfo.rigidBodyB = constraintInstance->GetRigidBodyB();
		ConvertLimitedHingeDataToRagdollConstraintData(static_cast<RE::hkpRagdollConstraintData*>(cInfo.constraintData.get()), static_cast<RE::hkpLimitedHingeConstraintData*>(constraintInstance->data));

		RE::bhkRagdollConstraint* ragdollConstraint = bhkRagdollConstraint_ctor();
		if (ragdollConstraint) {
			bhkRagdollConstraint_ApplyCinfo(ragdollConstraint, cInfo.constraintData);
			return ragdollConstraint;
		}

		return nullptr;
	}

	void HavokHooks::PreDriveToPose(RE::hkbRagdollDriver* a_driver, float a_deltaTime, [[maybe_unused]] const RE::hkbContext& a_context, RE::hkbGeneratorOutput& a_generatorOutput)
	{
		using KnockState = RE::KNOCK_STATE_ENUM;

		RE::Actor* actor = Utils::GetActorFromRagdollDriver(a_driver);
		if (!actor) {
			return;
		}

		auto ragdoll = PrecisionHandler::GetActiveRagdollFromDriver(a_driver);
		if (!ragdoll) {
			return;
		}

		RE::hkbGeneratorOutput::TrackHeader* poseHeader = GetTrackHeader(a_generatorOutput, RE::hkbGeneratorOutput::StandardTracks::TRACK_POSE);
		RE::hkbGeneratorOutput::TrackHeader* worldFromModelHeader = GetTrackHeader(a_generatorOutput, RE::hkbGeneratorOutput::StandardTracks::TRACK_WORLD_FROM_MODEL);
		RE::hkbGeneratorOutput::TrackHeader* keyframedBonesHeader = GetTrackHeader(a_generatorOutput, RE::hkbGeneratorOutput::StandardTracks::TRACK_KEYFRAMED_RAGDOLL_BONES);
		RE::hkbGeneratorOutput::TrackHeader* rigidBodyHeader = GetTrackHeader(a_generatorOutput, RE::hkbGeneratorOutput::StandardTracks::TRACK_RIGID_BODY_RAGDOLL_CONTROLS);
		RE::hkbGeneratorOutput::TrackHeader* poweredHeader = GetTrackHeader(a_generatorOutput, RE::hkbGeneratorOutput::StandardTracks::TRACK_POWERED_RAGDOLL_CONTROLS);
		RE::hkbGeneratorOutput::TrackHeader* poweredWorldFromModelModeHeader = GetTrackHeader(a_generatorOutput, RE::hkbGeneratorOutput::StandardTracks::TRACK_POWERED_RAGDOLL_WORLD_FROM_MODEL_MODE);

		ragdoll->deltaTime = a_deltaTime;
		ragdoll->elapsedTime += a_deltaTime;

		if (ragdoll->impulseTime > 0.f) {
			ragdoll->impulseTime -= a_deltaTime;
		}

		auto prevKnockState = ragdoll->knockState;
		ragdoll->knockState = actor->AsActorState()->GetKnockState();

		bool isRigidBodyOn = rigidBodyHeader && rigidBodyHeader->onFraction > 0.f;
		bool isPoweredOn = poweredHeader && poweredHeader->onFraction > 0.f;

		bool isComputingWorldFromModel = false;
		bool isUsingRootBoneAsWorldFromModel = false;
		if (poweredWorldFromModelModeHeader && poweredWorldFromModelModeHeader->onFraction > 0.f) {
			RE::hkbWorldFromModelModeData& worldFromModelMode = *(RE::hkbWorldFromModelModeData*)Track_getData(a_generatorOutput, *poweredWorldFromModelModeHeader);
			if (worldFromModelMode.mode == RE::hkbWorldFromModelModeData::WorldFromModelMode::WORLD_FROM_MODEL_MODE_COMPUTE) {
				isComputingWorldFromModel = true;
			} else if (worldFromModelMode.mode == RE::hkbWorldFromModelModeData::WorldFromModelMode::WORLD_FROM_MODEL_MODE_USE_ROOT_BONE) {
				isUsingRootBoneAsWorldFromModel = true;
			}
		}

		if (isComputingWorldFromModel && !ragdoll->bWasComputingWorldFromModel) {
			// Went from not computing worldfrommodel to computing it
			if (Settings::bFadeInComputedWorldFromModel) {
				ragdoll->stickyWorldFromModel = ragdoll->worldFromModel;  // We haven't updated ragdoll->worldFromModel yet this frame, so this actually the previous worldFromModel
				ragdoll->worldFromModelFadeInTime = ragdoll->elapsedTime;
				ragdoll->bFadeInWorldFromModel = true;
			}
		} else if (!isComputingWorldFromModel && ragdoll->bWasComputingWorldFromModel) {
			// Went from computing worldfrommodel to not computing it any more
			if (Settings::bFadeOutComputedWorldFromModel) {
				ragdoll->stickyWorldFromModel = ragdoll->worldFromModel;  // We haven't updated ragdoll->worldFromModel yet this frame, so this actually the previous worldFromModel
				ragdoll->worldFromModelFadeOutTime = ragdoll->elapsedTime;
				ragdoll->bFadeOutWorldFromModel = true;
			}
		}
		ragdoll->bWasComputingWorldFromModel = isComputingWorldFromModel;

		if (ragdoll->bFadeOutWorldFromModel) {
			if (worldFromModelHeader && worldFromModelHeader->onFraction > 0.f) {
				RE::hkQsTransform& worldFromModel = *(RE::hkQsTransform*)Track_getData(a_generatorOutput, *worldFromModelHeader);

				double elapsedTime = (ragdoll->elapsedTime - ragdoll->worldFromModelFadeOutTime);
				double elapsedTimeFraction = elapsedTime / Settings::fComputeWorldFromModelFadeOutTime;

				if (elapsedTimeFraction <= 1.0) {
					worldFromModel = Utils::lerphkQsTransform(ragdoll->stickyWorldFromModel, worldFromModel, elapsedTimeFraction);
				} else {
					ragdoll->bFadeOutWorldFromModel = false;
				}
			}
		}

		if (worldFromModelHeader && worldFromModelHeader->onFraction > 0.f) {
			RE::hkQsTransform& worldFromModel = *(RE::hkQsTransform*)Track_getData(a_generatorOutput, *worldFromModelHeader);
			ragdoll->worldFromModel = worldFromModel;
		}

		auto bActorInRagdollState = actor->IsInRagdollState();

		if (!bActorInRagdollState) {
			if (ragdoll->impulseTime <= 0.f && ragdoll->state < RagdollState::kBlendOut) {
				ragdoll->blender.StartBlend(Blender::BlendType::kRagdollToAnim, Settings::fBlendOutTime);
				ragdoll->state = RagdollState::kBlendOut;
			}
			if (ragdoll->impulseTime > 0.f && ragdoll->state == RagdollState::kBlendOut) {
				ragdoll->blender.StartBlend(Blender::BlendType::kRagdollToCurrentRagdoll, Settings::fBlendOutTime);
				ragdoll->state = RagdollState::kBlendIn;
			}
		}

		bool isPoweredOnly = !isRigidBodyOn && isPoweredOn;
		if (isPoweredOnly) {
			bool allNoForce = true;
			if (poweredHeader->numData > 0) {
				RE::hkbPoweredRagdollControlData* data = (RE::hkbPoweredRagdollControlData*)(Track_getData(a_generatorOutput, *poweredHeader));
				for (int i = 0; i < poweredHeader->numData; i++) {
					RE::hkbPoweredRagdollControlData& elem = data[i];
					if (elem.maxForce > 0.f) {
						allNoForce = false;
					}
				}
			}

			if (Settings::bKnockDownAfterBuggedGetUp) {
				if (allNoForce && isUsingRootBoneAsWorldFromModel && !bActorInRagdollState) {
					if (auto process = actor->GetActorRuntimeData().currentProcess) {
						AIProcess_PushActorAway(process, actor, RE::PlayerCharacter::GetSingleton()->data.location, 0.f);
					}
				}
			}

			if (allNoForce) {
				// Only powered constraints are active and they are effectively disabled
				return;
			}
		}

		if (!isRigidBodyOn && (!isPoweredOn || !isComputingWorldFromModel)) {
			// No controls are active, or powered only and not computing world from model - try and force it to use the rigidbody controller
			if (rigidBodyHeader) {
				TryForceRigidBodyControls(a_generatorOutput, *rigidBodyHeader);
				isRigidBodyOn = rigidBodyHeader->onFraction > 0.f;
			}
		}

		if (isRigidBodyOn && !isPoweredOn) {
			if (poweredHeader) {
				TryForcePoweredControls(a_generatorOutput, *poweredHeader);
				isPoweredOn = poweredHeader->onFraction > 0.f;
				if (isPoweredOn) {
					poweredHeader->onFraction = Settings::fPoweredControllerOnFraction;
					rigidBodyHeader->onFraction = 1.1f;  // something > 1 makes the hkbRagdollDriver blend between the rigidbody and powered controllers
				}
			}
		}

		if (Settings::bBlendWhenGettingUp) {
			if (prevKnockState == KnockState::kQueued && ragdoll->knockState == KnockState::kGetUp) {
				// Went from starting to get up to actually getting up
				ragdoll->blender.StartBlend(Blender::BlendType::kRagdollToCurrentRagdoll, Settings::fGetUpBlendTime, true);
				ragdoll->blender.initialPose = ragdoll->ragdollPose;  // the previous ragdoll pose
				ragdoll->blender.bIsFirstBlendFrame = false;
			}
		}

		ragdoll->bWasRigidBodyOn = isRigidBodyOn;

		if (Settings::bEnableKeyframes) {
			if (keyframedBonesHeader && keyframedBonesHeader->onFraction > 0.f) {
				if (ragdoll->elapsedTime <= Settings::fBlendInKeyframeTime) {
					SetBonesKeyframedReporting(a_driver, a_generatorOutput, *keyframedBonesHeader);
				}
			}
		}

		float deltaTimeMult = 1.f;
		float deltaTime = a_deltaTime;
		if (deltaTime > 0.f) {  // avoid division by zero
			if (actor->IsPlayerRef()) {
				deltaTime *= Utils::GetPlayerTimeMultiplier();
			}

			constexpr float div = 1.f / 60.f;
			deltaTimeMult = div / deltaTime;
		}

		if (rigidBodyHeader && rigidBodyHeader->onFraction > 0.f && rigidBodyHeader->numData > 0) {
			RE::hkaKeyFrameHierarchyUtility::ControlData* data = (RE::hkaKeyFrameHierarchyUtility::ControlData*)(Track_getData(a_generatorOutput, *rigidBodyHeader));
			for (int i = 0; i < rigidBodyHeader->numData; i++) {
				RE::hkaKeyFrameHierarchyUtility::ControlData& elem = data[i];

				if (ragdoll->elapsedTime <= Settings::fAddRagdollSettleTime) {  // override values so it settles instantly
					elem.accelerationGain = 1.f;
					elem.velocityGain = 1.f;
					elem.positionGain = 1.f;
					elem.positionMaxLinearVelocity = 0.f;
					elem.positionMaxAngularVelocity = 0.f;
					elem.snapGain = 1.f;
					elem.snapMaxLinearVelocity = 100.f;
					elem.snapMaxAngularVelocity = 100.f;
					elem.snapMaxLinearDistance = 100.f;
					elem.snapMaxAngularDistance = 100.f;
				} else {
					elem.hierarchyGain = Settings::fHierarchyGain;
					elem.velocityGain = Settings::fVelocityGain;
					elem.positionGain = Settings::fPositionGain;

					// Fix impulses at high framerate
					elem.accelerationGain = Utils::Clamp(elem.accelerationGain / deltaTimeMult, 0.f, 1.f);
					elem.velocityGain = Utils::Clamp(elem.velocityGain / deltaTimeMult, 0.f, 1.f);
					elem.positionGain = Utils::Clamp(elem.positionGain / deltaTimeMult, 0.f, 1.f);
					elem.positionMaxLinearVelocity = Utils::Clamp(elem.positionMaxLinearVelocity / deltaTimeMult, 0.f, 100.f);
					elem.positionMaxAngularVelocity = Utils::Clamp(elem.positionMaxAngularVelocity / deltaTimeMult, 0.f, 100.f);
					elem.snapGain = Utils::Clamp(elem.snapGain / deltaTimeMult, 0.f, 1.f);
					elem.snapMaxLinearVelocity = Utils::Clamp(elem.snapMaxLinearVelocity / deltaTimeMult, 0.f, 100.f);
					elem.snapMaxAngularVelocity = Utils::Clamp(elem.snapMaxAngularVelocity / deltaTimeMult, 0.f, 100.f);
					elem.snapMaxLinearDistance = Utils::Clamp(elem.snapMaxLinearDistance / deltaTimeMult, 0.f, 10.f);
					elem.snapMaxAngularDistance = Utils::Clamp(elem.snapMaxAngularDistance / deltaTimeMult, 0.f, 10.f);
				}
			}
		}

		if (poweredHeader && poweredHeader->onFraction > 0.f && poweredHeader->numData > 0) {
			RE::hkbPoweredRagdollControlData* data = (RE::hkbPoweredRagdollControlData*)(Track_getData(a_generatorOutput, *poweredHeader));
			for (int i = 0; i < poweredHeader->numData; i++) {
				RE::hkbPoweredRagdollControlData& elem = data[i];
				elem.maxForce = Settings::fPoweredMaxForce;
				elem.tau = Settings::fPoweredTau;
				elem.damping = Settings::fPoweredDamping;
				elem.proportionalRecoveryVelocity = Settings::fPoweredProportionalRecoveryVelocity;
				elem.constantRecoveryVelocity = Settings::fPoweredConstantRecoveryVelocity;
			}
		}

		ragdoll->isOn = isRigidBodyOn;
		if (!ragdoll->isOn) {
			ragdoll->state = RagdollState::kIdle;  // reset state
			return;
		}

		if (bActorInRagdollState) {
			return;
		}

		if (isPoweredOnly) {
			// Don't want to do foot ik / constraint loosening / disabling gravity when powered only
			return;
		}

		if (Settings::bCopyFootIkToPoseTrack) {
			// When the game does foot ik, the output of the foot ik is put into a temporary hkbGeneratorOutput and copied into the hkbCharacter.poseLocal.
			// However, the physics ragdoll driving is done on the hkbGeneratorOutput from hkbBehaviorGraph::generate() which does not have the foot ik incorporated.
			// So, copy the pose from hkbCharacter.poseLocal into the hkbGeneratorOutput pose track to have the ragdoll driving take the foot ik into account.
			RE::hkbCharacter* character = a_driver->character;
			if (character && poseHeader && poseHeader->onFraction > 0.f) {
				RE::BShkbAnimationGraph* graph = Utils::GetAnimationGraph(character);
				if (graph && graph->doFootIK) {
					if (character->footIkDriver && character->setup && character->setup->data && character->setup->data->footIkDriverInfo) {
						RE::hkQsTransform* poseLocal = hkbCharacter_getPoseLocal(character);
						int16_t numPoses = poseHeader->numData;
						memcpy(Track_getData(a_generatorOutput, *poseHeader), poseLocal, numPoses * sizeof(RE::hkQsTransform));
					}
				}
			}
		}

		if (Settings::bLoosenRagdollContraintsToMatchPose) {
			if (poseHeader && poseHeader->onFraction > 0.f && worldFromModelHeader && worldFromModelHeader->onFraction > 0.f) {
				RE::hkQsTransform& worldFromModel = *(RE::hkQsTransform*)Track_getData(a_generatorOutput, *worldFromModelHeader);
				RE::hkQsTransform* highResPoseLocal = (RE::hkQsTransform*)Track_getData(a_generatorOutput, *poseHeader);

				static std::vector<RE::hkQsTransform> poseWorld{};
				MapHighResPoseLocalToLowResPoseWorld(a_driver, worldFromModel, highResPoseLocal, poseWorld);

				// Set rigidbody transforms to the anim pose ones and save the old values
				static std::vector<RE::hkTransform> savedTransforms{};
				savedTransforms.clear();
				for (int i = 0; i < a_driver->ragdoll->rigidBodies.size(); i++) {
					RE::hkpRigidBody* rb = a_driver->ragdoll->rigidBodies[i];
					RE::hkQsTransform& transform = poseWorld[i];

					savedTransforms.push_back(rb->GetMotionState()->transform);
					rb->GetMotionState()->transform.translation = transform.translation;
					hkRotation_setFromQuat(&rb->GetMotionState()->transform.rotation, transform.rotation);
				}

				{  // Loosen ragdoll constraints to allow the anim pose
					RE::hkRefPtr<RE::hkpEaseConstraintsAction> actionPtr = ragdoll->easeConstraintsAction;

					if (!actionPtr) {
						RE::hkpEaseConstraintsAction* easeConstraintsAction = reinterpret_cast<RE::hkpEaseConstraintsAction*>(hkHeapAlloc(sizeof(RE::hkpEaseConstraintsAction)));
						actionPtr = RE::hkRefPtr<RE::hkpEaseConstraintsAction>(easeConstraintsAction);
						hkpEaseConstraintsAction_ctor(easeConstraintsAction, a_driver->ragdoll->rigidBodies, 0);
						ragdoll->easeConstraintsAction = actionPtr;  // must do this after ctor since the ctor sets the refcount to 1
					}

					if (actionPtr) {
						// Loosen constraint pivots first
						if (Settings::bLoosenRagdollContraintPivots) {
							ragdoll->originalConstraintPivots.clear();

							for (auto constraint : a_driver->ragdoll->constraints) {
								if (constraint->data->GetType() == RE::hkpConstraintData::ConstraintType::kRagdoll) {
									RE::hkpRagdollConstraintData* data = static_cast<RE::hkpRagdollConstraintData*>(constraint->data);
									if (constraint->internal) {  // needed to tell master from slave
										RE::hkpRigidBody* bodyA = constraint->GetRigidBodyA();
										RE::hkpRigidBody* bodyB = constraint->GetRigidBodyB();

										RE::hkVector4 pivotAbodySpace = data->atoms.transforms.transformA.translation;
										RE::hkVector4 pivotBbodySpace = data->atoms.transforms.transformB.translation;
										ragdoll->originalConstraintPivots[constraint] = { pivotAbodySpace, pivotBbodySpace };

										RE::hkVector4 pivotA;
										hkVector4_setTransformedPos(pivotA, bodyA->motion.motionState.transform, pivotAbodySpace);
										RE::hkVector4 pivotB;
										hkVector4_setTransformedPos(pivotB, bodyB->motion.motionState.transform, pivotBbodySpace);

										RE::hkVector4 slavePivot = bodyA == constraint->internal->entities[1 - constraint->internal->whoIsMaster] ? pivotA : pivotB;

										SetPivotInWorldSpace(data, bodyA->motion.motionState.transform, bodyB->motion.motionState.transform, slavePivot);
									}
								}
							}
						}

						// Loosen angular constraints second
						hkpEaseConstraintsAction_loosenConstraints(actionPtr.get());
					}
				}

				// Restore rigidbody transforms
				for (int i = 0; i < a_driver->ragdoll->rigidBodies.size(); i++) {
					RE::hkpRigidBody* rb = a_driver->ragdoll->rigidBodies[i];
					rb->GetMotionState()->transform = savedTransforms[i];
				}
			}
		}

		if (Settings::bDisableGravityForActiveRagdolls) {
			for (auto& rigidBody : a_driver->ragdoll->rigidBodies) {
				rigidBody->motion.gravityFactor = 0.f;
			}
		}

		// Root motion
		if (auto root = actor->Get3D()) {
			if (poseHeader && poseHeader->onFraction > 0.f && worldFromModelHeader && worldFromModelHeader->onFraction > 0.f) {
				if (a_driver->ragdoll->skeleton->bones.size() > 0) {
					if (int32_t rootIndex = a_driver->ragdoll->boneToRigidBodyMap[0] >= 0) {
						if (auto rootRigidBody = a_driver->ragdoll->rigidBodies[rootIndex]) {
							if (auto rootNode = RE::NiPointer<RE::NiAVObject>(GetNiObjectFromCollidable(&rootRigidBody->collidable))) {
								const RE::hkQsTransform& worldFromModel = *(RE::hkQsTransform*)Track_getData(a_generatorOutput, *worldFromModelHeader);
								RE::hkQsTransform* highResPoseLocal = (RE::hkQsTransform*)Track_getData(a_generatorOutput, *poseHeader);

								static std::vector<RE::hkQsTransform> poseWorld{};
								MapHighResPoseLocalToLowResPoseWorld(a_driver, worldFromModel, highResPoseLocal, poseWorld);

								RE::hkQsTransform poseT = poseWorld[0];

								if (ragdoll->bHasRootBoneTransform)  // We compare against last frame's pose transform since the rigidbody transforms aren't updated yet for this frame until after the physics step.
								{
									RE::hkTransform actualT = rootRigidBody->motion.motionState.transform;

									RE::NiPoint3 posePos = Utils::HkVectorToNiPoint(ragdoll->rootBoneTransform.translation);
									RE::NiPoint3 actualPos = Utils::HkVectorToNiPoint(actualT.translation);
									ragdoll->rootOffset = actualPos - posePos;

									RE::NiPoint3 poseForward = Utils::ForwardVectorFromNiMatrix3(Utils::QuaternionToMatrix(Utils::NormalizeNiQuat(Utils::HkQuatToNiQuat(ragdoll->rootBoneTransform.rotation))));
									RE::NiMatrix3 actualRot;
									Utils::HkMatrixToNiMatrix(actualT.rotation, actualRot);
									RE::NiPoint3 actualForward = Utils::ForwardVectorFromNiMatrix3(actualRot);
									float poseAngle = atan2f(poseForward.x, poseForward.y);
									float actualAngle = atan2f(actualForward.x, actualForward.y);
									ragdoll->rootOffsetAngle = Utils::AngleDifference(poseAngle, actualAngle);

									if (Settings::bDoWarp &&
										(!Settings::bDisableWarpWhenGettingUp || ragdoll->knockState != KnockState::kGetUp) &&
										ragdoll->rootOffset.Length() > Settings::fMaxAllowedDistBeforeWarp) {
										// Set rigidbody transforms to the anim pose ones
										for (int i = 0; i < std::size(poseWorld); i++) {
											if (int32_t index = a_driver->ragdoll->boneToRigidBodyMap[i] >= 0) {
												if (auto rb = a_driver->ragdoll->rigidBodies[index]) {
													RE::hkQsTransform& transform = poseWorld[i];

													RE::hkTransform newTransform;
													newTransform.translation = Utils::NiPointToHkVector(Utils::HkVectorToNiPoint(transform.translation));
													hkRotation_setFromQuat(&newTransform.rotation, transform.rotation);

													rb->motion.SetTransform(newTransform);
													hkpEntity_updateMovedBodyInfo(rb);

													RE::hkVector4 zeroVector = RE::hkVector4(0.f);
													rb->motion.SetLinearVelocity(zeroVector);
													rb->motion.SetAngularVelocity(zeroVector);
												}
											}
										}

										// Reset all ragdoll controller state to stop it from going crazy
										hkbRagdollDriver_reset(a_driver);
									}
								}

								ragdoll->rootBoneTransform = poseT;
								ragdoll->bHasRootBoneTransform = true;
							}
						}
					}
				}
			}
		}
	}

	void HavokHooks::PostDriveToPose([[maybe_unused]] RE::hkbRagdollDriver* a_driver, [[maybe_unused]] float a_deltaTime, [[maybe_unused]] const RE::hkbContext& a_context, [[maybe_unused]] RE::hkbGeneratorOutput& a_generatorOutput)
	{
		if (Settings::bEnableHitstop && a_driver && a_driver->ragdoll) {
			if (RE::Actor* actor = Utils::GetActorFromRagdollDriver(a_driver)) {
				float multiplier = PrecisionHandler::GetDriveToPoseHitstopMultiplier(actor->GetHandle(), a_deltaTime);
				if (multiplier != 1.f) {
					RE::hkVector4 mult = multiplier;
					for (auto& rigidBody : a_driver->ragdoll->rigidBodies) {
						rigidBody->motion.linearVelocity = rigidBody->motion.linearVelocity * mult;
						rigidBody->motion.angularVelocity = rigidBody->motion.angularVelocity * mult;
					}
				}
			}
		}
	}

	void HavokHooks::PrePostPhysics(RE::hkbRagdollDriver* a_driver, [[maybe_unused]] const RE::hkbContext& a_context, RE::hkbGeneratorOutput& a_generatorInOut)
	{
		// This hook is called right before hkbRagdollDriver::postPhysics()

		RE::Actor* actor = Utils::GetActorFromRagdollDriver(a_driver);
		if (!actor)
			return;

		// All we're doing here is storing the anim pose, so it's fine to run this even if the actor is fully ragdolled or getting up.

		auto ragdoll = PrecisionHandler::GetActiveRagdollFromDriver(a_driver);
		if (!ragdoll) {
			return;
		}

		RE::hkbGeneratorOutput::TrackHeader* poseHeader = GetTrackHeader(a_generatorInOut, RE::hkbGeneratorOutput::StandardTracks::TRACK_POSE);
		if (poseHeader && poseHeader->onFraction > 0.f) {
			int numPoses = poseHeader->numData;
			RE::hkQsTransform* animPose = (RE::hkQsTransform*)Track_getData(a_generatorInOut, *poseHeader);
			// Copy anim pose track before postPhysics() as postPhysics() will overwrite it with the ragdoll pose
			ragdoll->animPose.assign(animPose, animPose + numPoses);
		}
	}

	void HavokHooks::PostPostPhysics(RE::hkbRagdollDriver* a_driver, [[maybe_unused]] const RE::hkbContext& a_context, RE::hkbGeneratorOutput& a_generatorInOut)
	{
		// This hook is called right after hkbRagdollDriver::postPhysics()

		RE::Actor* actor = Utils::GetActorFromRagdollDriver(a_driver);
		if (!actor)
			return;

		// All we're doing here is storing the ragdoll pose and blending, and we do want to have the option to blend even while getting up.

		auto ragdoll = PrecisionHandler::GetActiveRagdollFromDriver(a_driver);
		if (!ragdoll) {
			return;
		}

		RE::hkbGeneratorOutput::TrackHeader* poseHeader = GetTrackHeader(a_generatorInOut, RE::hkbGeneratorOutput::StandardTracks::TRACK_POSE);

		if (poseHeader && poseHeader->onFraction > 0.f) {
			int numPoses = poseHeader->numData;
			RE::hkQsTransform* poseOut = (RE::hkQsTransform*)Track_getData(a_generatorInOut, *poseHeader);

			// Copy pose track now since postPhysics() just set it to the high-res ragdoll pose
			ragdoll->ragdollPose.assign(poseOut, poseOut + numPoses);
		}

		RE::hkbGeneratorOutput::TrackHeader* worldFromModelHeader = GetTrackHeader(a_generatorInOut, RE::hkbGeneratorOutput::StandardTracks::TRACK_WORLD_FROM_MODEL);
		if (worldFromModelHeader && worldFromModelHeader->onFraction > 0.f) {
			RE::hkQsTransform& worldFromModel = *(RE::hkQsTransform*)Track_getData(a_generatorInOut, *worldFromModelHeader);
			ragdoll->worldFromModelPostPhysics = worldFromModel;
		}

		if (!ragdoll->isOn) {
			return;
		}

		if (ragdoll->bFadeInWorldFromModel) {
			double elapsedTime = (ragdoll->elapsedTime - ragdoll->worldFromModelFadeInTime);
			double elapsedTimeFraction = elapsedTime / Settings::fComputeWorldFromModelFadeInTime;
			if (elapsedTimeFraction > 1.0) {
				ragdoll->bFadeInWorldFromModel = false;
			}
		}

		RagdollState state = ragdoll->state;

		if (Settings::bLoosenRagdollContraintsToMatchPose) {
			if (auto easeConstraintsAction = ragdoll->easeConstraintsAction.get()) {
				// Restore constraint limits from before we loosened them
				hkpEaseConstraintsAction_restoreConstraints(easeConstraintsAction, 0.f);
				ragdoll->easeConstraintsAction = nullptr;

				if (Settings::bLoosenRagdollContraintPivots) {
					for (RE::hkpConstraintInstance* constraint : a_driver->ragdoll->constraints) {
						if (constraint->data->GetType() == RE::hkpConstraintData::ConstraintType::kRagdoll) {
							RE::hkpRagdollConstraintData* data = (RE::hkpRagdollConstraintData*)constraint->data;

							if (auto it = ragdoll->originalConstraintPivots.find(constraint); it != ragdoll->originalConstraintPivots.end()) {
								data->atoms.transforms.transformA.translation = it->second.first;
								data->atoms.transforms.transformB.translation = it->second.second;
							}
						}
					}
				}
			}
		}

		if (Settings::bDisableGravityForActiveRagdolls) {
			for (auto& rigidBody : a_driver->ragdoll->rigidBodies) {
				rigidBody->motion.gravityFactor = 1.f;
			}
		}

		Blender& blender = ragdoll->blender;
		if (blender.bIsActive) {
			ragdoll->bReadyToRemove = false;
			bool bDone = !Settings::bDoBlending;
			if (!bDone) {
				bDone = blender.Update(*ragdoll, *a_driver, a_generatorInOut, ragdoll->deltaTime);
			}
			if (bDone) {
				if (state == RagdollState::kBlendIn) {
					state = RagdollState::kIdle;
				} else if (state == RagdollState::kBlendOut) {
					state = RagdollState::kIdle;
					ragdoll->bReadyToRemove = true;
				}
			}
		}

		if (Settings::bForceAnimPose) {
			if (poseHeader && poseHeader->onFraction > 0.f) {
				//int numPoses = poseHeader->numData;
				RE::hkQsTransform* poseOut = (RE::hkQsTransform*)Track_getData(a_generatorInOut, *poseHeader);
				memcpy(poseOut, ragdoll->animPose.data(), ragdoll->animPose.size() * sizeof(RE::hkQsTransform));
			}
		} else if (Settings::bForceRagdollPose) {
			if (poseHeader && poseHeader->onFraction > 0.f) {
				//int numPoses = poseHeader->numData;
				RE::hkQsTransform* poseOut = (RE::hkQsTransform*)Track_getData(a_generatorInOut, *poseHeader);
				memcpy(poseOut, ragdoll->ragdollPose.data(), ragdoll->ragdollPose.size() * sizeof(RE::hkQsTransform));
			}
		}

		ragdoll->state = state;
	}

	void HavokHooks::PostPostPostPhysics(RE::hkbRagdollDriver* a_driver)
	{
		RE::Actor* actor = Utils::GetActorFromRagdollDriver(a_driver);
		if (!actor) {
			return;
		}

		auto ragdoll = PrecisionHandler::GetActiveRagdollFromDriver(a_driver);
		if (!ragdoll) {
			return;
		}

		// Queue the ragdoll for removal if necessary
		if (ragdoll->bReadyToRemove) {
			WriteLocker locker(PrecisionHandler::ragdollsToRemoveLock);
			PrecisionHandler::ragdollsToRemove.emplace(actor->GetHandle());
		}
	}

	void HavokHooks::PrePhysicsStep(RE::bhkWorld* a_world)
	{
		{
			WriteLocker locker(PrecisionHandler::activeActorsLock);

			// run UpdateClone on active actors and remove any that are no longer valid
			for (auto it = PrecisionHandler::activeActors.begin(); it != PrecisionHandler::activeActors.end();) {
				if (!it->second->UpdateClone()) {
					it = PrecisionHandler::activeActors.erase(it);
				} else {
					++it;
				}
			}
		}

		PrecisionHandler::GetSingleton()->RunPrePhysicsStepCallbacks(a_world);
	}

	void HavokHooks::TryForceRigidBodyControls(RE::hkbGeneratorOutput& a_generatorOutput, RE::hkbGeneratorOutput::TrackHeader& a_header)
	{
		if (a_header.capacity > 0) {
			RE::hkaKeyFrameHierarchyUtility::ControlData* data = (RE::hkaKeyFrameHierarchyUtility::ControlData*)(Track_getData(a_generatorOutput, a_header));
			data[0] = RE::hkaKeyFrameHierarchyUtility::ControlData();

			int8_t* indices = Track_getIndices(a_generatorOutput, a_header);
			for (int i = 0; i < a_header.capacity; i++) {
				indices[i] = 0;
			}

			a_header.numData = 1;
			a_header.onFraction = 1.f;
		}
	}

	void HavokHooks::TryForcePoweredControls(RE::hkbGeneratorOutput& a_generatorOutput, RE::hkbGeneratorOutput::TrackHeader& a_header)
	{
		if (a_header.capacity > 0) {
			RE::hkbPoweredRagdollControlData* data = (RE::hkbPoweredRagdollControlData*)(Track_getData(a_generatorOutput, a_header));
			data[0] = RE::hkbPoweredRagdollControlData{};

			int8_t* indices = Track_getIndices(a_generatorOutput, a_header);
			for (int i = 0; i < a_header.capacity; i++) {
				indices[i] = 0;
			}

			a_header.numData = 1;
			a_header.onFraction = 1.f;
		}
	}

	void HavokHooks::SetBonesKeyframedReporting(RE::hkbRagdollDriver* a_driver, RE::hkbGeneratorOutput& a_generatorOutput, RE::hkbGeneratorOutput::TrackHeader& a_header)
	{
		// - Set onFraction > 1.0f
		// - Set value of keyframed bones tracks to > 1.0f for bones we want keyframed, <= 1.0f for bones we don't want keyframed. Index of track data == index of bone.
		// - Set reportingWhenKeyframed in the ragdoll driver for the bones we care about

		a_header.onFraction = 1.1f;
		float* data = Track_getData(a_generatorOutput, a_header);
		auto& skeleton = a_driver->ragdoll->skeleton;
		for (int i = 0; i < skeleton->bones.size(); i++) {
			data[i] = 1.1f;  // anything > 1
			// Indexed by (boneIdx >> 5), and then you >> (boneIdx & 0x1F) & 1 to extract the specific bit
			a_driver->reportingWhenKeyframed[i >> 5] |= (1 << (i & 0x1F));
		}
	}

	void HavokHooks::MapHighResPoseLocalToLowResPoseWorld(RE::hkbRagdollDriver* a_driver, const RE::hkQsTransform& a_worldFromModel, const RE::hkQsTransform* a_highResPoseLocal, std::vector<RE::hkQsTransform>& a_poseWorld)
	{
		// We need this because hkbRagdollDriver::mapHighResPoseLocalToLowResPoseWorld() does not actually give correct results.
		// This is essentially what hkbRagdollDriver::driveToPose() does when computing what transforms to drive the rigidbodies to.

		int numPosesLow = a_driver->ragdoll->skeleton->bones.size();

		static std::vector<RE::hkQsTransform> lowResPoseLocal{};
		lowResPoseLocal.resize(numPosesLow);

		hkbRagdollDriver_mapHighResPoseLocalToLowResPoseLocal(a_driver, a_highResPoseLocal, lowResPoseLocal.data());

		static std::vector<RE::hkQsTransform> scaledLowResPoseLocal{};
		scaledLowResPoseLocal.resize(numPosesLow);

		CopyAndApplyScaleToPose(true, numPosesLow, lowResPoseLocal.data(), scaledLowResPoseLocal.data(), a_worldFromModel.scale.quad.m128_f32[0]);

		RE::hkQsTransform worldFromModelWithScaledPositionButScaleIs1;
		CopyAndPotentiallyApplyHavokScaleToTransform(true, &a_worldFromModel, &worldFromModelWithScaledPositionButScaleIs1);
		worldFromModelWithScaledPositionButScaleIs1.scale = RE::hkVector4(1.f, 1.f, 1.f, 1.f);

		a_poseWorld.resize(numPosesLow);
		hkbPoseLocalToPoseWorld(numPosesLow, a_driver->ragdoll->skeleton->parentIndices.begin(), worldFromModelWithScaledPositionButScaleIs1, scaledLowResPoseLocal.data(), a_poseWorld.data());
	}

	Hooks::HavokHooks::CollisionFilterComparisonResult HavokHooks::CompareFilterInfo(RE::bhkCollisionFilter* a_collisionFilter, uint32_t a_filterInfoA, uint32_t a_filterInfoB)
	{
		auto callbacksResult = PrecisionHandler::GetSingleton()->RunCollisionFilterComparisonCallbacks(a_collisionFilter, a_filterInfoA, a_filterInfoB);
		if (callbacksResult != CollisionFilterComparisonResult::Continue) {
			return callbacksResult;
		}

		if (Settings::bDisableMod) {
			return CollisionFilterComparisonResult::Continue;
		}

		CollisionLayer layerA = static_cast<CollisionLayer>(a_filterInfoA & 0x7f);
		CollisionLayer layerB = static_cast<CollisionLayer>(a_filterInfoB & 0x7f);

		if (layerA != CollisionLayer::kCharController && layerB != CollisionLayer::kCharController) {
			// Neither collidee is a character controller
			return CollisionFilterComparisonResult::Continue;
		}

		if (layerA == CollisionLayer::kCharController && layerB == CollisionLayer::kCharController) {
			// Both collidees are character controllers
			if (Settings::bUseRagdollCollisionWhenAllowed) {
				// Disable char controller collisions with the character controller of an actor that has the AllowRagdollCollision flag

				uint16_t groupA = a_filterInfoA >> 16;
				uint16_t groupB = a_filterInfoB >> 16;

				bool bIsRagdollCollision = PrecisionHandler::IsRagdollCollsionGroup(groupA) || PrecisionHandler::IsRagdollCollsionGroup(groupB);
				if (bIsRagdollCollision) {
					return CollisionFilterComparisonResult::Ignore;
				} else {
					return CollisionFilterComparisonResult::Continue;
				}
			}
			return CollisionFilterComparisonResult::Continue;
		}

		// Only one of the collidees is a character controller
		uint32_t charControllerFilter = layerA == CollisionLayer::kCharController ? a_filterInfoA : a_filterInfoB;
		uint16_t charControllerGroup = charControllerFilter >> 16;

		uint32_t otherFilter = charControllerFilter == a_filterInfoA ? a_filterInfoB : a_filterInfoA;
		CollisionLayer otherLayer = static_cast<CollisionLayer>(otherFilter & 0x7f);
		uint16_t otherGroup = otherFilter >> 16;

		if (otherLayer == CollisionLayer::kPrecisionAttack && !PrecisionHandler::IsCharacterControllerHittableCollisionGroup(charControllerGroup)) {
			// Attack vs. (non-hittable) character controller
			return CollisionFilterComparisonResult::Ignore;
		}

		bool bIsRagdollCollision = PrecisionHandler::IsRagdollCollsionGroup(otherGroup);

		// Ignore char controller vs bipedNoCC collision for actors with ragdoll collision, as they will collide with our body layer instead
		// Letting them collide with the ragdolled on impulse body would make them push the ragdoll around, resulting in weird behavior
		// and a very ugly snap back to keyframed positions once the ragdoll is removed after the impulse
		if (otherLayer == CollisionLayer::kBipedNoCC && bIsRagdollCollision) {
			return CollisionFilterComparisonResult::Ignore;
		}

		// Ignore char controller vs precision body collisions for actors that don't have ragdoll collision
		if (otherLayer == CollisionLayer::kPrecisionBody && !bIsRagdollCollision) {
			return CollisionFilterComparisonResult::Ignore;
		}

		return CollisionFilterComparisonResult::Continue;
	}

	bool HavokHooks::CloneSkeleton(RE::ActorHandle a_actorHandle)
	{
		struct Unk58
		{
			uint64_t unk00;
			uint32_t unk08;
			uint32_t flags;
			uint64_t unk10;
			uint64_t unk18;
			uint64_t unk20;
			RE::NiAVObject* object;
		};

		if (a_actorHandle) {
			if (auto actor = a_actorHandle.get()) {
				if (auto root = actor->Get3D(false)) {
					auto rootNode = root->AsNode();
					if (rootNode && actor->loadedData && actor->loadedData->unk58) {
						auto pUnk58 = reinterpret_cast<Unk58**>(actor->loadedData->unk58);
						if (auto unk58 = *pUnk58) {
							if (auto skeleton = unk58->object) {
								if (auto cell = actor->GetParentCell()) {
									if (auto world = cell->GetbhkWorld()) {
										float scale = actor->GetScale();
										auto clone = RE::NiPointer<RE::NiAVObject>(Utils::Clone<RE::NiAVObject>(skeleton, { scale, scale, scale }));
										if (clone) {
											// set user data so collision callbacks can identify the hit actor
											clone->SetUserData(actor.get());
											//rootNode->AttachChild(clone.get());

											// fix invalid motion type after cloning
											Utils::TraverseAllScenegraphCollisions(clone.get(), [&](auto&& a_collisionObject) -> RE::BSVisit::BSVisitControl {
												if (auto blendCollisionObject = skyrim_cast<RE::bhkBlendCollisionObject*>(a_collisionObject)) {
													blendCollisionObject->motionType = RE::hkpMotion::MotionType::kKeyframed;
												}
												return RE::BSVisit::BSVisitControl::kContinue;
											});

											uint32_t collisionFilterInfo;
											actor->GetCollisionFilterInfo(collisionFilterInfo);
											uint16_t collisionGroup = collisionFilterInfo >> 16;

											CollisionLayer collisionLayer = CollisionLayer::kPrecisionBody;

											{
												WriteLocker locker(PrecisionHandler::activeActorsLock);

												PrecisionHandler::activeActors.emplace(a_actorHandle, std::make_shared<ActiveActor>(a_actorHandle, root, clone.get(), collisionGroup, collisionLayer));
											}

											typedef void (__thiscall RE::bhkWorld::*AddHavok)(RE::NiAVObject * a_node, bool a_recurse, bool a_notify, uint32_t a_collisionGroup, bool a_ignoreBSXFlags);

											*g_bAddBipedWhenKeyframedIndirect = true;
											(world->*reinterpret_cast<AddHavok>(&RE::bhkWorld::Unk_35))(clone.get(), true, true, collisionGroup, false);
											*g_bAddBipedWhenKeyframedIndirect = false;

											return true;
										}
									}
								}
							}
						}
					}
				}
			}
		}

		return false;
	}

	void CameraShakeHook::TESCamera_Update(RE::TESCamera* a_this)
	{
		_TESCamera_Update(a_this);

		if ((Settings::bEnableHitstopCameraShake || Settings::bEnableRecoilCameraShake) && PrecisionHandler::bCameraShakeActive) {
			//a_this->cameraRoot->local.translate += PrecisionHandler::currentCameraShakeAxis * PrecisionHandler::currentCameraShake;
			a_this->cameraRoot->local.rotate = a_this->cameraRoot->local.rotate * Utils::MatrixFromAxisAngle(Settings::cameraShakeAxis, PrecisionHandler::currentCameraShake * 0.001f);

			RE::NiUpdateData updateData;
			a_this->cameraRoot->UpdateDownwardPass(updateData, 0);
		}
	}

	void MovementHook::ProcessThumbstick(RE::MovementHandler* a_this, RE::ThumbstickEvent* a_event, RE::PlayerControlsData* a_data)
	{
		auto playerCharacter = RE::PlayerCharacter::GetSingleton();
		if (a_event && a_event->IsLeft() && playerCharacter) {
			if ((a_event->xValue != 0.f || a_event->yValue != 0.f)) {
				bool bCanCancelRecoil = false;
				playerCharacter->GetGraphVariableBool("bCanCancelRecoil"sv, bCanCancelRecoil);
				if (bCanCancelRecoil) {
					playerCharacter->NotifyAnimationGraph("Collision_RecoilStop"sv);
				}
			}
		}

		_ProcessThumbstick(a_this, a_event, a_data);
	}

	void MovementHook::ProcessButton(RE::MovementHandler* a_this, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data)
	{
		auto playerCharacter = RE::PlayerCharacter::GetSingleton();
		if (a_event && playerCharacter) {
			auto& userEvent = a_event->QUserEvent();
			auto userEvents = RE::UserEvents::GetSingleton();

			if ((userEvent == userEvents->forward) ||
				(userEvent == userEvents->back) ||
				(userEvent == userEvents->strafeLeft) ||
				(userEvent == userEvents->strafeRight)) {
				bool bCanCancelRecoil = false;
				playerCharacter->GetGraphVariableBool("bCanCancelRecoil"sv, bCanCancelRecoil);
				if (bCanCancelRecoil) {
					playerCharacter->NotifyAnimationGraph("Collision_RecoilStop"sv);
				}
			}
		}

		_ProcessButton(a_this, a_event, a_data);
	}

	void FirstPersonStateHook::OnEnterState(RE::FirstPersonState* a_this)
	{
		_OnEnterState(a_this);

		PrecisionHandler::GetSingleton()->RemoveAllAttackCollisions(RE::PlayerCharacter::GetSingleton()->GetHandle());
	}

	void FirstPersonStateHook::OnExitState(RE::FirstPersonState* a_this)
	{
		_OnExitState(a_this);

		PrecisionHandler::GetSingleton()->RemoveAllAttackCollisions(RE::PlayerCharacter::GetSingleton()->GetHandle());
	}

	bool AIHooks::IsHuman(RE::Actor* a_actor)
	{
		if (auto race = a_actor->GetRace()) {
			if (race->behaviorGraphProjectNames[0] == Settings::defaultMaleBehaviorGraph || race->behaviorGraphProjectNames[1] == Settings::defaultFemaleBehaviorGraph) {
				return true;
			}
		}

		return false;
	}

	RE::NiAVObject* AIHooks::Clone3D(RE::TESObjectWEAP* a_this, RE::TESObjectREFR* a_ref, bool a_arg3)
	{
		auto ret = _Clone3D(a_this, a_ref, a_arg3);
		PrecisionHandler::CacheWeaponMeshReach(a_this, ret);
		return ret;
	}

	float AIHooks::GetMaxRange(RE::Actor* a_actor, RE::TESBoundObject* a_object, int64_t a3)
	{
		if (Settings::bHookAIWeaponReach && IsHuman(a_actor)) {
			float reach = 0.f;
			if (PrecisionHandler::GetInventoryWeaponReach(a_actor, a_object, reach)) {
				return reach;
			}
		}

		return _GetMaxRange(a_actor, a_object, a3);
	}

	float AIHooks::GetUnarmedReach(RE::Actor* a_actor)
	{
		if (Settings::bHookAIWeaponReach && IsHuman(a_actor)) {
			if (a_actor) {
				if (auto& currentProcess = a_actor->GetActorRuntimeData().currentProcess) {
					if (currentProcess->cachedValues) {
						float radius = currentProcess->cachedValues->cachedRadius / a_actor->GetScale();
						float lengthMult = PrecisionHandler::GetAttackLengthMult(a_actor);
						float reach = (Settings::fMinWeaponLength + Settings::fWeaponLengthUnarmedOffset + Settings::fAIWeaponReachOffset) * lengthMult + radius;
						return std::fmin(reach, _GetUnarmedReach(a_actor));
					}
				}
			}
		}

		return _GetUnarmedReach(a_actor);
	}

}
