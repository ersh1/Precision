#pragma once
#include "Offsets.h"
#include "PrecisionAPI.h"
#include "PrecisionHandler.h"

namespace RE
{
	struct RE::BSAnimationUpdateData
	{
		float deltaTime;                            // 00
		uint32_t pad04;                             // 04
		void* unkFunctionPtr;                       // 08
		TESObjectREFR* refr;                        // 10
		NiPoint3* worldCameraPositionPtr;           // 18
		IPostAnimationChannelUpdateFunctor* unk20;  // 20
		uint16_t flags;                             // 28
		bool unk2A;                                 // 2A
		bool unk2B;                                 // 2B
		bool unk2C;                                 // 2C
		bool unk2D;                                 // 2D
		bool unk2E;                                 // 2E
		bool unk2F;                                 // 2F
	};
	static_assert(sizeof(BSAnimationUpdateData) == 0x30);
}

namespace Hooks
{
	class UpdateHooks
	{
	public:
		static void Hook()
		{
			REL::Relocation<uintptr_t> hook1{ RELOCATION_ID(35565, 36564) };  // 5B2FF0, 5D9F50, main update
			REL::Relocation<uintptr_t> hook2{ RELOCATION_ID(40436, 41453) };  // 6E1990, 70A840, RunOneActorAnimationUpdateJob
			REL::Relocation<uintptr_t> hook3{ RELOCATION_ID(36359, 37350) };  // 5D8170, 5FCF20, UpdateMovementJob internal function

			auto& trampoline = SKSE::GetTrampoline();
			_Nullsub = trampoline.write_call<5>(hook1.address() + RELOCATION_OFFSET(0x748, 0xC26), Nullsub);            // 5B3738, 5DAB76
			_ApplyMovement = trampoline.write_call<5>(hook3.address() + RELOCATION_OFFSET(0xF0, 0xFB), ApplyMovement);  // 5D8260, 5FD01B

			REL::Relocation<std::uintptr_t> CharacterVtbl{ RE::VTABLE_Character[0] };
			_Character_ModifyAnimationUpdateData = CharacterVtbl.write_vfunc(0x79, Character_ModifyAnimationUpdateData);

			REL::Relocation<std::uintptr_t> PlayerCharacterVtbl{ RE::VTABLE_PlayerCharacter[0] };
			_PlayerCharacter_ModifyAnimationUpdateData = PlayerCharacterVtbl.write_vfunc(0x79, PlayerCharacter_ModifyAnimationUpdateData);
		}

	private:
		static void Nullsub();
		static void ApplyMovement(RE::Actor* a_this, float a_deltaTime);

		static void Character_ModifyAnimationUpdateData(RE::Character* a_this, RE::BSAnimationUpdateData& a_data);
		static void PlayerCharacter_ModifyAnimationUpdateData(RE::PlayerCharacter* a_this, RE::BSAnimationUpdateData& a_data);

		static inline REL::Relocation<decltype(Nullsub)> _Nullsub;
		static inline REL::Relocation<decltype(ApplyMovement)> _ApplyMovement;

		static inline REL::Relocation<decltype(Character_ModifyAnimationUpdateData)> _Character_ModifyAnimationUpdateData;
		static inline REL::Relocation<decltype(PlayerCharacter_ModifyAnimationUpdateData)> _PlayerCharacter_ModifyAnimationUpdateData;

		static bool ShouldSetPitchModifier(RE::Actor* a_this);
		static void ModifyAnimationUpdateData(RE::Actor* a_this, RE::BSAnimationUpdateData& a_data);

		static RE::Actor* GetActorFromRbx();
	};

	class AttackHooks
	{
	public:
		static void Hook()
		{
			REL::Relocation<uintptr_t> hook1{ RELOCATION_ID(37650, 38603) };  // 627930, 64D350
			REL::Relocation<uintptr_t> hook2{ RELOCATION_ID(37673, 38627) };  // 628C20, 64E760 - HitActor
			REL::Relocation<uintptr_t> hook3{ RELOCATION_ID(37674, 38628) };  // 629090, 64ECC0 - CalculateCurrentHitTargetForWeaponSwing
			REL::Relocation<uintptr_t> hook4{ RELOCATION_ID(36682, 37690) };  // 5F87F0, 61F5D0, called by TESObjectREFR::Hit
			REL::Relocation<uintptr_t> hook5{ RELOCATION_ID(42832, 44001) };  // 742850, 76EDB0, PopulateHitData
			REL::Relocation<uintptr_t> hook6{ RELOCATION_ID(37675, 38629) };  // 62A0D0, 64FF80, Apply death force

			auto& trampoline = SKSE::GetTrampoline();
			_Func1 = trampoline.write_call<5>(hook1.address() + RELOCATION_OFFSET(0x2C1, 0x2CA), Func1);                                                          // 627BF1, 64D61A
			_Func2 = trampoline.write_call<5>(hook1.address() + RELOCATION_OFFSET(0x2DD, 0x2E6), Func2);                                                          // 627C0D, 64D636
			_ApplyPerkEntryPoint = trampoline.write_call<5>(hook1.address() + RELOCATION_OFFSET(0x343, 0x34F), ApplyPerkEntryPoint);                              // 627C73, 64D69F
			_HitData_Populate1 = trampoline.write_call<5>(hook2.address() + RELOCATION_OFFSET(0x1B7, 0x1C6), HitData_Populate1);                                  // 628DD7, 64E926
			_HitData_Populate2 = trampoline.write_call<5>(hook3.address() + RELOCATION_OFFSET(0xEB, 0x110), HitData_Populate2);                                   // 62917B, 64EDD0
			_TESObjectCELL_PlaceParticleEffect = trampoline.write_call<5>(hook4.address() + RELOCATION_OFFSET(0xABD, 0xB39), TESObjectCELL_PlaceParticleEffect);  // 5F92AD, 620109
			_ApplyDeathForce = trampoline.write_call<5>(hook6.address() + RELOCATION_OFFSET(0x10A, 0x10A), ApplyDeathForce);                                      // 62A1DA, 65008A
			_HitActor_GetAttackData = trampoline.write_call<5>(hook2.address() + RELOCATION_OFFSET(0xB3, 0xC2), HitActor_GetAttackData);                          // 628CD3, 64E822

			_CdPointCollectorCast = trampoline.write_call<5>(hook3.address() + RELOCATION_OFFSET(0x26A, 0x294), CdPointCollectorCast);  // 6292FA, 64EF54

			_HitData_GetAttackData = trampoline.write_call<5>(hook5.address() + RELOCATION_OFFSET(0xD3, 0xDD), HitData_GetAttackData);          // 742923, 76EE8D
			_HitData_GetWeaponDamage = trampoline.write_call<5>(hook5.address() + RELOCATION_OFFSET(0x1A5, 0x1A4), HitData_GetWeaponDamage);    // 7429F5, 76EF54
			_HitData_GetBashDamage = trampoline.write_call<5>(hook5.address() + RELOCATION_OFFSET(0x22F, 0x226), HitData_GetBashDamage);        // 742A7F, 76EFD6
			_HitData_GetUnarmedDamage = trampoline.write_call<5>(hook5.address() + RELOCATION_OFFSET(0x26A, 0x24D), HitData_GetUnarmedDamage);  // 742ABA, 76EFFD
																																				//_HitData_GetStagger = trampoline.write_call<5>(hook5.address() + RELOCATION_OFFSET(0x2FC, 0x2D7), HitData_GetStagger);              // 742B4C, 76F087
		}

	private:
		static RE::Actor* Func1(RE::Actor* a_this);
		static RE::TESObjectREFR* Func2(RE::Actor* a_this);
		static void ApplyPerkEntryPoint(RE::BGSEntryPoint::ENTRY_POINT a_entryPoint, RE::Actor* a_actor, RE::TESObjectREFR* a_object, float& a_outResult);
		static void HitData_Populate1(RE::HitData* a_this, RE::TESObjectREFR* a_source, RE::TESObjectREFR* a_target, RE::InventoryEntryData* a_weapon, bool a_bIsOffhand);
		static void HitData_Populate2(RE::HitData* a_this, RE::TESObjectREFR* a_source, RE::TESObjectREFR* a_target, RE::InventoryEntryData* a_weapon, bool a_bIsOffhand);
		static RE::BSTempEffectParticle* TESObjectCELL_PlaceParticleEffect(RE::TESObjectCELL* a_this, float a_lifetime, const char* a_modelName, RE::NiPoint3& a_rotation, RE::NiPoint3& a_pos, float a_scale, int32_t a_flags, RE::NiAVObject* a_target);
		static void ApplyDeathForce(RE::Actor* a_this, RE::Actor* a_attacker, float a_deathForce, float a_mult, const RE::NiPoint3& a_hitDirection, const RE::NiPoint3& a_hitPosition, bool a_bIsRanged, RE::HitData* a_hitData);
		static RE::NiPointer<RE::BGSAttackData>& HitActor_GetAttackData(RE::AIProcess* a_source);
		static bool CdPointCollectorCast(RE::hkpAllCdPointCollector* a_collector, RE::bhkWorld* a_world, RE::NiPoint3& a_origin, RE::NiPoint3& a_direction, float a_length);
		static RE::NiPointer<RE::BGSAttackData>& HitData_GetAttackData(RE::Actor* a_source);
		static float HitData_GetWeaponDamage(RE::InventoryEntryData* a_weapon, RE::ActorValueOwner* a_actorValueOwner, float a_damageMult, bool a4);
		static void HitData_GetBashDamage(RE::ActorValueOwner* a_actorValueOwner, float& a_outDamage);
		static void HitData_GetUnarmedDamage(RE::ActorValueOwner* a_actorValueOwner, float& a_outDamage);
		static float HitData_GetStagger(RE::Actor* a_source, RE::Actor* a_target, RE::TESObjectWEAP* a_weapon, float a4);

		static RE::NiPointer<RE::BGSAttackData>& FixAttackData(RE::NiPointer<RE::BGSAttackData>& a_attackData, RE::TESRace* a_race);

		static inline REL::Relocation<decltype(Func1)> _Func1;
		static inline REL::Relocation<decltype(Func2)> _Func2;
		static inline REL::Relocation<decltype(ApplyPerkEntryPoint)> _ApplyPerkEntryPoint;
		static inline REL::Relocation<decltype(HitData_Populate1)> _HitData_Populate1;
		static inline REL::Relocation<decltype(HitData_Populate2)> _HitData_Populate2;
		static inline REL::Relocation<decltype(TESObjectCELL_PlaceParticleEffect)> _TESObjectCELL_PlaceParticleEffect;
		static inline REL::Relocation<decltype(ApplyDeathForce)> _ApplyDeathForce;
		static inline REL::Relocation<decltype(HitActor_GetAttackData)> _HitActor_GetAttackData;
		static inline REL::Relocation<decltype(CdPointCollectorCast)> _CdPointCollectorCast;
		static inline REL::Relocation<decltype(HitData_GetAttackData)> _HitData_GetAttackData;
		static inline REL::Relocation<decltype(HitData_GetWeaponDamage)> _HitData_GetWeaponDamage;
		static inline REL::Relocation<decltype(HitData_GetBashDamage)> _HitData_GetBashDamage;
		static inline REL::Relocation<decltype(HitData_GetUnarmedDamage)> _HitData_GetUnarmedDamage;
		static inline REL::Relocation<decltype(HitData_GetStagger)> _HitData_GetStagger;
	};

	class AIHooks
	{
	public:
		static void Hook()
		{
			//REL::Relocation<std::uintptr_t> CombatInventoryItemMeleeVtbl{ RE::VTABLE_CombatInventoryItemMelee[0] };
			//_GetMaxRange = CombatInventoryItemMeleeVtbl.write_vfunc(0x6, GetMaxRange);

			//REL::Relocation<std::uintptr_t> TESObjectWEAPVtbl{ RE::VTABLE_TESObjectWEAP[0] };
			//_Clone3D = TESObjectWEAPVtbl.write_vfunc(0x40, Clone3D);

			REL::Relocation<std::uintptr_t> hook1{ RELOCATION_ID(43656, 44889) };  // 778290, 7A64E0

			auto& trampoline = SKSE::GetTrampoline();
			_GetMaxRange = trampoline.write_call<5>(hook1.address() + RELOCATION_OFFSET(0x147, 0x128), GetMaxRange);  // 7783D7, 7A6608
		}

	private:
		//static float GetMaxRange(RE::CombatInventoryItem* a_this);
		static RE::NiAVObject* Clone3D(RE::TESObjectWEAP* a_this, RE::TESObjectREFR* a_ref, bool a_arg3);
		static float GetMaxRange(RE::Actor* a_actor, RE::TESBoundObject* a_object, int64_t a3);

		static inline REL::Relocation<decltype(Clone3D)> _Clone3D;
		static inline REL::Relocation<decltype(GetMaxRange)> _GetMaxRange;
	};

	class FirstPersonStateHook
	{
	public:
		static void Hook()
		{
			REL::Relocation<std::uintptr_t> FirstPersonStateVtbl{ RE::VTABLE_FirstPersonState[0] };
			_OnEnterState = FirstPersonStateVtbl.write_vfunc(0x1, OnEnterState);
			_OnExitState = FirstPersonStateVtbl.write_vfunc(0x2, OnExitState);
		}

	private:
		static void OnEnterState(RE::FirstPersonState* a_this);
		static void OnExitState(RE::FirstPersonState* a_this);

		static inline REL::Relocation<decltype(OnEnterState)> _OnEnterState;
		static inline REL::Relocation<decltype(OnExitState)> _OnExitState;
	};

	class CameraShakeHook
	{
	public:
		static void Hook()
		{
			REL::Relocation<std::uintptr_t> hook1{ RELOCATION_ID(49852, 50784) };  // 84AB90, 876700

			auto& trampoline = SKSE::GetTrampoline();
			_TESCamera_Update = trampoline.write_call<5>(hook1.address() + RELOCATION_OFFSET(0x1A6, 0x1A6), TESCamera_Update);  // 84AD36, 8768A6
		}

	private:
		static void TESCamera_Update(RE::TESCamera* a_this);

		static inline REL::Relocation<decltype(TESCamera_Update)> _TESCamera_Update;
	};

	class MovementHook
	{
	public:
		static void Hook()
		{
			REL::Relocation<std::uintptr_t> MovementHandlerVtbl{ RE::VTABLE_MovementHandler[0] };
			_ProcessThumbstick = MovementHandlerVtbl.write_vfunc(0x2, ProcessThumbstick);
			_ProcessButton = MovementHandlerVtbl.write_vfunc(0x4, ProcessButton);
		}

	private:
		static void ProcessThumbstick(RE::MovementHandler* a_this, RE::ThumbstickEvent* a_event, RE::PlayerControlsData* a_data);
		static void ProcessButton(RE::MovementHandler* a_this, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data);

		static inline REL::Relocation<decltype(ProcessThumbstick)> _ProcessThumbstick;
		static inline REL::Relocation<decltype(ProcessButton)> _ProcessButton;
	};

	// a lot of the havok related code is based on https://github.com/adamhynek/activeragdoll or https://github.com/adamhynek/higgs
	class HavokHooks
	{
	public:
		static void Hook()
		{
			REL::Relocation<std::uintptr_t> HighActorCullerVtbl{ RE::VTABLE_HighActorCuller[0] };
			_CullActors = HighActorCullerVtbl.write_vfunc(0x1, CullActors);

			REL::Relocation<std::uintptr_t> ActorVtbl{ RE::VTABLE_Actor[0] };
			//_Actor_DetachHavok = ActorVtbl.write_vfunc(0x65, Actor_DetachHavok);
			//_Actor_InitHavok = ActorVtbl.write_vfunc(0x66, Actor_InitHavok);

			REL::Relocation<std::uintptr_t> CharacterVtbl{ RE::VTABLE_Character[0] };
			//_Character_DetachHavok = CharacterVtbl.write_vfunc(0x65, Character_DetachHavok);
			//_Character_InitHavok = CharacterVtbl.write_vfunc(0x66, Character_InitHavok);

			REL::Relocation<std::uintptr_t> PlayerCharacterVtbl{ RE::VTABLE_PlayerCharacter[0] };
			//_PlayerCharacter_DetachHavok = PlayerCharacterVtbl.write_vfunc(0x65, PlayerCharacter_DetachHavok);
			//_PlayerCharacter_InitHavok = PlayerCharacterVtbl.write_vfunc(0x66, PlayerCharacter_InitHavok);

			REL::Relocation<std::uintptr_t> ChairFurnitureExitHandlerVtbl{ RE::VTABLE_ChairFurnitureExitHandler[0] };

			REL::Relocation<std::uintptr_t> BShkbAnimationGraphVtbl{ RE::VTABLE_BShkbAnimationGraph[0] };
			_SetWorld = BShkbAnimationGraphVtbl.write_vfunc(0x4, SetWorld);

			REL::Relocation<uintptr_t> hook1{ RELOCATION_ID(38112, 39068) };  // 6403D0, 666990, Job_Ragdoll_post_physics
			REL::Relocation<uintptr_t> hook2{ RELOCATION_ID(62416, 63358) };  // AE1F60, B06870, BSAnimationGraphManager::sub_140AE1F60
			REL::Relocation<uintptr_t> hook3{ RELOCATION_ID(62621, 63562) };  // AEB8F0, B10A30, BShkbAnimationGraph::sub_140AEB8F0
			REL::Relocation<uintptr_t> hook4{ RELOCATION_ID(62622, 63563) };  // AEBBF0, B10D30, BShkbAnimationGraph::sub_140AEBBF0
			REL::Relocation<uintptr_t> hook5{ RELOCATION_ID(62642, 63587) };  // AEF630, B14F90, BShkbAnimationGraph::sub_140AEF630

			REL::Relocation<uintptr_t> hook6{ RELOCATION_ID(41805, 42886) };  // 723070, 74E1E0, EnableCharacterBumperHandler::Handle

			REL::Relocation<uintptr_t> collisionFilterHook1{ RELOCATION_ID(76181, 78009) };  // DAF370, DEF410, hkpRigidBody
			REL::Relocation<uintptr_t> collisionFilterHook2{ RELOCATION_ID(76676, 78548) };  // DD6780, E17640, hkpCollidableCollidableFilter_isCollisionEnabled
			REL::Relocation<uintptr_t> collisionFilterHook3{ RELOCATION_ID(76677, 78549) };  // DD67B0, E17670, hkpShapeCollectionFilter_numShapeKeyHitsLimitBreached
			REL::Relocation<uintptr_t> collisionFilterHook4{ RELOCATION_ID(76678, 78550) };  // DD68A0, E17760, hkpRayShapeCollectionFilter_isCollisionEnabled
			REL::Relocation<uintptr_t> collisionFilterHook5{ RELOCATION_ID(76679, 78551) };  // DD6900, E177C0, hkpRayCollidableFilter_isCollisionEnabled
			REL::Relocation<uintptr_t> collisionFilterHook6{ RELOCATION_ID(76680, 78552) };  // DD6930, E177F0, hkpShapeCollectionFilter_isCollisionEnabled
			REL::Relocation<uintptr_t> collisionFilterHook7{ RELOCATION_ID(77228, 79115) };  // DEE700, E30D60, hkpCachingShapePhantom

			auto& trampoline = SKSE::GetTrampoline();
			_ProcessHavokHitJobs = trampoline.write_call<5>(hook1.address() + RELOCATION_OFFSET(0x104, 0xFC), ProcessHavokHitJobs);  // 6404D4, 666A8C
			//_BShkbAnimationGraph_UpdateAnimation = trampoline.write_call<5>(hook2.address() + RELOCATION_OFFSET(0xF5, 0x185), BShkbAnimationGraph_UpdateAnimation);  // AE2055, B069F5

			_hkbRagdollDriver_DriveToPose = trampoline.write_call<5>(hook3.address() + RELOCATION_OFFSET(0x25B, 0x256), hkbRagdollDriver_DriveToPose);  // AEBB4B, B10C86
			_hkbRagdollDriver_PostPhysics = trampoline.write_call<5>(hook4.address() + RELOCATION_OFFSET(0x18C, 0x18B), hkbRagdollDriver_PostPhysics);  // AEBD7C, B10EBB

			//_BShkbAnimationGraph_ShouldAddToGraphListeners = trampoline.write_call<5>(hook5.address() + RELOCATION_OFFSET(0x37F, 0x38D), BShkbAnimationGraph_ShouldAddToGraphListeners);  // AEF9AF, B1531D

			_QueueTask_ToggleCharacterBumper = trampoline.write_call<5>(hook6.address() + RELOCATION_OFFSET(0x1F, 0x1F), QueueTask_ToggleCharacterBumper);
			_ToggleCharacterBumper = trampoline.write_call<5>(hook6.address() + RELOCATION_OFFSET(0x56, 0x56), ToggleCharacterBumper);

			_bhkCollisionFilter_CompareFilterInfo1 = trampoline.write_call<5>(collisionFilterHook1.address() + RELOCATION_OFFSET(0x16F, 0x16F), bhkCollisionFilter_CompareFilterInfo1);  // DAF4DF, DEF57F
			_bhkCollisionFilter_CompareFilterInfo2 = trampoline.write_call<5>(collisionFilterHook2.address() + RELOCATION_OFFSET(0x17, 0x17), bhkCollisionFilter_CompareFilterInfo2);    // DD6797, E17657
			_bhkCollisionFilter_CompareFilterInfo3 = trampoline.write_call<5>(collisionFilterHook3.address() + RELOCATION_OFFSET(0xBC, 0xBC), bhkCollisionFilter_CompareFilterInfo3);    // DD686C, E1772C
			_bhkCollisionFilter_CompareFilterInfo4 = trampoline.write_call<5>(collisionFilterHook4.address() + RELOCATION_OFFSET(0x31, 0x31), bhkCollisionFilter_CompareFilterInfo4);    // DD68D1, E17791
			_bhkCollisionFilter_CompareFilterInfo5 = trampoline.write_call<5>(collisionFilterHook5.address() + RELOCATION_OFFSET(0x17, 0x17), bhkCollisionFilter_CompareFilterInfo5);    // DD6917, E177D7
			_bhkCollisionFilter_CompareFilterInfo6 = trampoline.write_call<5>(collisionFilterHook6.address() + RELOCATION_OFFSET(0x118, 0x118), bhkCollisionFilter_CompareFilterInfo6);  // DD6A48, E17908
			_bhkCollisionFilter_CompareFilterInfo7 = trampoline.write_call<5>(collisionFilterHook7.address() + RELOCATION_OFFSET(0x128, 0x128), bhkCollisionFilter_CompareFilterInfo7);  // DEE828, E30E88

			HookPrePhysicsStep();
		}

	private:
		using CollisionFilterComparisonResult = PRECISION_API::CollisionFilterComparisonResult;

		static void CullActors(void* a_this, RE::Actor* a_actor);

		static void SetWorld(RE::BShkbAnimationGraph* a_this, RE::bhkWorld* a_world);

		static void ProcessHavokHitJobs(void* a1);
		static void BShkbAnimationGraph_UpdateAnimation(RE::BShkbAnimationGraph* a_this, RE::BShkbAnimationGraph_UpdateData* a_updateData, void* a3);
		static void hkbRagdollDriver_DriveToPose(RE::hkbRagdollDriver* a_driver, float a_deltaTime, const RE::hkbContext& a_context, RE::hkbGeneratorOutput& a_generatorOutput);
		static void hkbRagdollDriver_PostPhysics(RE::hkbRagdollDriver* a_driver, const RE::hkbContext& a_context, RE::hkbGeneratorOutput& a_generatorInOut);

		static bool BShkbAnimationGraph_ShouldAddToGraphListeners(RE::BShkbAnimationGraph* a_this);

		static void QueueTask_ToggleCharacterBumper(void* a_taskManager, RE::Actor* a_actor, bool a_enable);
		static void ToggleCharacterBumper(RE::bhkCharacterController* a_charController, bool a_enable);

		static bool bhkCollisionFilter_CompareFilterInfo1(RE::bhkCollisionFilter* a_this, uint32_t a_filterInfoA, uint32_t a_filterInfoB);
		static bool bhkCollisionFilter_CompareFilterInfo2(RE::bhkCollisionFilter* a_this, uint32_t a_filterInfoA, uint32_t a_filterInfoB);
		static bool bhkCollisionFilter_CompareFilterInfo3(RE::bhkCollisionFilter* a_this, uint32_t a_filterInfoA, uint32_t a_filterInfoB);
		static bool bhkCollisionFilter_CompareFilterInfo4(RE::bhkCollisionFilter* a_this, uint32_t a_filterInfoA, uint32_t a_filterInfoB);
		static bool bhkCollisionFilter_CompareFilterInfo5(RE::bhkCollisionFilter* a_this, uint32_t a_filterInfoA, uint32_t a_filterInfoB);
		static bool bhkCollisionFilter_CompareFilterInfo6(RE::bhkCollisionFilter* a_this, uint32_t a_filterInfoA, uint32_t a_filterInfoB);
		static bool bhkCollisionFilter_CompareFilterInfo7(RE::bhkCollisionFilter* a_this, uint32_t a_filterInfoA, uint32_t a_filterInfoB);

		static void HookPrePhysicsStep();

		static void AddPrecisionCollisionLayers(RE::bhkWorld* a_world);
		static void EnsurePrecisionCollisionLayers(RE::bhkWorld* a_world);
		static void ReSyncLayerBitfields(RE::bhkCollisionFilter* a_filter, CollisionLayer a_layer);

		static bool CanAddToWorld(RE::ActorHandle a_actorHandle);

		static bool IsSkeletonAddedToWorld(RE::ActorHandle a_actorHandle);
		static bool AddSkeletonToWorld(RE::ActorHandle a_actorHandle);
		static bool RemoveSkeletonFromWorld(RE::ActorHandle a_actorHandle);

		static bool IsRagdollAddedToWorld(RE::ActorHandle a_actorHandle);
		static bool AddRagdollToWorld(RE::ActorHandle a_actorHandle);
		static bool RemoveRagdollFromWorld(RE::ActorHandle a_actorHandle);
		static bool ShouldRemoveRagdollFromWorld(RE::ActorHandle a_actorHandle);
		static bool CanRemoveRagdollFromWorld(RE::ActorHandle a_actorHandle);

		static void ModifyConstraints(RE::Actor* a_actor);
		static void DisableSyncOnUpdate(RE::Actor* a_actor);

		static void ConvertLimitedHingeDataToRagdollConstraintData(RE::hkpRagdollConstraintData* a_ragdollData, RE::hkpLimitedHingeConstraintData* a_limitedHingeData);
		static void SetPivotInWorldSpace(RE::hkpRagdollConstraintData* a_constraint, const RE::hkTransform& a_bodyATransform, const RE::hkTransform& a_bodyBTransform, const RE::hkVector4& a_pivot);
		static RE::bhkRagdollConstraint* ConvertToRagdollConstraint(RE::bhkConstraint* a_constraint);

		static void PreDriveToPose(RE::hkbRagdollDriver* a_driver, float a_deltaTime, const RE::hkbContext& a_context, RE::hkbGeneratorOutput& a_generatorOutput);
		static void PostDriveToPose(RE::hkbRagdollDriver* a_driver, float a_deltaTime, const RE::hkbContext& a_context, RE::hkbGeneratorOutput& a_generatorOutput);
		static void PrePostPhysics(RE::hkbRagdollDriver* a_driver, const RE::hkbContext& a_context, RE::hkbGeneratorOutput& a_generatorInOut);
		static void PostPostPhysics(RE::hkbRagdollDriver* a_driver, const RE::hkbContext& a_context, RE::hkbGeneratorOutput& a_generatorInOut);
		static void PostPostPostPhysics(RE::hkbRagdollDriver* a_driver);
		static void PrePhysicsStep(RE::bhkWorld* a_world);

		static void TryForceRigidBodyControls(RE::hkbGeneratorOutput& a_generatorOutput, RE::hkbGeneratorOutput::TrackHeader& a_header);
		static void TryForcePoweredControls(RE::hkbGeneratorOutput& a_generatorOutput, RE::hkbGeneratorOutput::TrackHeader& a_header);
		static void SetBonesKeyframedReporting(RE::hkbRagdollDriver* a_driver, RE::hkbGeneratorOutput& a_generatorOutput, RE::hkbGeneratorOutput::TrackHeader& a_header);
		static void MapHighResPoseLocalToLowResPoseWorld(RE::hkbRagdollDriver* a_driver, const RE::hkQsTransform& a_worldFromModel, const RE::hkQsTransform* a_highResPoseLocal, std::vector<RE::hkQsTransform>& a_poseWorld);

		static CollisionFilterComparisonResult CompareFilterInfo(RE::bhkCollisionFilter* a_collisionFilter, uint32_t a_filterInfoA, uint32_t a_filterInfoB);

		static bool CloneSkeleton(RE::ActorHandle a_actorHandle);

		static inline REL::Relocation<decltype(CullActors)> _CullActors;

		static inline REL::Relocation<decltype(SetWorld)> _SetWorld;

		static inline REL::Relocation<decltype(ProcessHavokHitJobs)> _ProcessHavokHitJobs;
		static inline REL::Relocation<decltype(BShkbAnimationGraph_UpdateAnimation)> _BShkbAnimationGraph_UpdateAnimation;
		static inline REL::Relocation<decltype(hkbRagdollDriver_DriveToPose)> _hkbRagdollDriver_DriveToPose;
		static inline REL::Relocation<decltype(hkbRagdollDriver_PostPhysics)> _hkbRagdollDriver_PostPhysics;

		static inline REL::Relocation<decltype(BShkbAnimationGraph_ShouldAddToGraphListeners)> _BShkbAnimationGraph_ShouldAddToGraphListeners;

		static inline REL::Relocation<decltype(QueueTask_ToggleCharacterBumper)> _QueueTask_ToggleCharacterBumper;
		static inline REL::Relocation<decltype(ToggleCharacterBumper)> _ToggleCharacterBumper;

		static inline REL::Relocation<decltype(bhkCollisionFilter_CompareFilterInfo1)> _bhkCollisionFilter_CompareFilterInfo1;
		static inline REL::Relocation<decltype(bhkCollisionFilter_CompareFilterInfo2)> _bhkCollisionFilter_CompareFilterInfo2;
		static inline REL::Relocation<decltype(bhkCollisionFilter_CompareFilterInfo3)> _bhkCollisionFilter_CompareFilterInfo3;
		static inline REL::Relocation<decltype(bhkCollisionFilter_CompareFilterInfo4)> _bhkCollisionFilter_CompareFilterInfo4;
		static inline REL::Relocation<decltype(bhkCollisionFilter_CompareFilterInfo5)> _bhkCollisionFilter_CompareFilterInfo5;
		static inline REL::Relocation<decltype(bhkCollisionFilter_CompareFilterInfo6)> _bhkCollisionFilter_CompareFilterInfo6;
		static inline REL::Relocation<decltype(bhkCollisionFilter_CompareFilterInfo7)> _bhkCollisionFilter_CompareFilterInfo7;
	};

	bool ActorHasAttackCollision(const RE::ActorHandle a_actorHandle);

	void Install();
}
