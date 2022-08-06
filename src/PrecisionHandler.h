#pragma once
#include "AttackCollision.h"
#include "AttackTrail.h"
#include "CachedAttackData.h"
#include "Havok/ActiveRagdoll.h"
#include "Havok/ContactListener.h"
#include "Offsets.h"
#include "PendingHit.h"
#include "PrecisionAPI.h"
#include "Settings.h"
#include "Utils.h"

#include <shared_mutex>

enum CollisionEventType : std::uint8_t
{
	kAttackStart = 0,
	kAdd,
	kRemove,
	kClearTargets,
	kAttackEnd
};

class PrecisionHandler :
	public RE::BSTEventSink<RE::BSAnimationGraphEvent>
{
public:
	using EventResult = RE::BSEventNotifyControl;
	using PreHitCallback = PRECISION_API::PreHitCallback;
	using PostHitCallback = PRECISION_API::PostHitCallback;
	using PrePhysicsStepCallback = PRECISION_API::PrePhysicsStepCallback;
	using CollisionFilterComparisonCallback = PRECISION_API::CollisionFilterComparisonCallback;
	using PrecisionHitData = PRECISION_API::PrecisionHitData;
	using PreHitCallbackReturn = PRECISION_API::PreHitCallbackReturn;
	using CollisionFilterComparisonResult = PRECISION_API::CollisionFilterComparisonResult;
	using RequestedAttackCollisionType = PRECISION_API::RequestedAttackCollisionType;

	static PrecisionHandler* GetSingleton()
	{
		static PrecisionHandler singleton;
		return std::addressof(singleton);
	}

	static void AddPlayerSink();
	static void RemovePlayerSink();
	static bool AddActorSink(RE::Actor* a_actor);
	static void RemoveActorSink(RE::Actor* a_actor);

	// override BSTEventSink
	virtual EventResult ProcessEvent(const RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource) override;

	void Update(float a_deltaTime);

	void StartCollision(RE::ActorHandle a_actorHandle, uint32_t a_activeGraphIdx);
	bool AddAttack(RE::ActorHandle a_actorHandle, const AttackDefinition& a_attackDefinition);
	void AddAttackCollision(RE::ActorHandle a_actorHandle, const CollisionDefinition& a_collisionDefinition);
	bool RemoveAttackCollision(RE::ActorHandle a_actorHandle, const CollisionDefinition& a_collisionDefinition);
	bool RemoveAttackCollision(RE::ActorHandle a_actorHandle, std::shared_ptr<AttackCollision> a_attackCollision);
	bool RemoveAllAttackCollisions(RE::ActorHandle a_actorHandle);
	void RemoveActor(RE::ActorHandle a_actorHandle);
	std::shared_ptr<AttackCollision> GetAttackCollision(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_node) const;
	std::shared_ptr<AttackCollision> GetAttackCollision(RE::ActorHandle a_actorHandle, std::string_view a_nodeName) const;

	[[nodiscard]] bool HasActor(RE::ActorHandle a_actorHandle) const;
	[[nodiscard]] bool HasStartedDefaultCollisionWithWeaponSwing(RE::ActorHandle a_actorHandle) const;
	[[nodiscard]] bool HasStartedDefaultCollisionWithWPNSwingUnarmed(RE::ActorHandle a_actorHandle) const;
	[[nodiscard]] bool HasStartedPrecisionCollision(RE::ActorHandle a_actorHandle, uint32_t a_activeGraphIdx) const;

	[[nodiscard]] bool HasHitstop(RE::ActorHandle a_actorHandle) const;

	void SetStartedDefaultCollisionWithWeaponSwing(RE::ActorHandle a_actorHandle);
	void SetStartedDefaultCollisionWithWPNSwingUnarmed(RE::ActorHandle a_actorHandle);

	[[nodiscard]] bool HasHitRef(RE::ActorHandle a_actorHandle, RE::ObjectRefHandle a_handle) const;
	void AddHitRef(RE::ActorHandle a_actorHandle, RE::ObjectRefHandle a_handle, float a_duration, bool a_bIsNPC);
	void ClearHitRefs(RE::ActorHandle a_actorHandle);
	[[nodiscard]] uint32_t GetHitCount(RE::ActorHandle a_actorHandle) const;
	[[nodiscard]] uint32_t GetHitNPCCount(RE::ActorHandle a_actorHandle) const;

	[[nodiscard]] bool HasIDHitRef(RE::ActorHandle a_actorHandle, uint8_t a_ID, RE::ObjectRefHandle a_handle) const;
	void AddIDHitRef(RE::ActorHandle a_actorHandle, uint8_t a_ID, RE::ObjectRefHandle a_handle, float a_duration, bool a_bIsNPC);
	void ClearIDHitRefs(RE::ActorHandle a_actorHandle, uint8_t a_ID);
	void IncreaseIDDamagedCount(RE::ActorHandle a_actorHandle, uint8_t a_ID);
	[[nodiscard]] uint32_t GetIDHitCount(RE::ActorHandle a_actorHandle, uint8_t a_ID) const;
	[[nodiscard]] uint32_t GetIDHitNPCCount(RE::ActorHandle a_actorHandle, uint8_t a_ID) const;
	[[nodiscard]] uint32_t GetIDDamagedCount(RE::ActorHandle a_actorHandle, uint8_t a_ID) const;

	void Initialize();
	void Clear();
	void OnPreLoadGame();
	void OnPostLoadGame();

	void ApplyHitImpulse(RE::ObjectRefHandle a_refHandle, RE::hkpRigidBody* a_rigidBody, const RE::NiPoint3& a_hitVelocity, const RE::hkVector4& a_hitPosition, float a_impulseMult, bool a_bIsActiveRagdoll);

	void AddHitstop(RE::ActorHandle a_refHandle, float a_hitstopLength);
	[[nodiscard]] float GetHitstop(RE::ActorHandle a_actorHandle, float a_deltaTime, bool a_bUpdate);

	void ApplyCameraShake(float a_strength, float a_length, float a_frequency, const RE::NiPoint3& a_axis);

	void ProcessPrePhysicsStepJobs();

	bool GetAttackCollisionDefinition(RE::Actor* a_actor, AttackDefinition& a_outAttackDefinition, std::optional<bool> bIsLeftSwing = std::nullopt) const;

	bool ParseCollisionEvent(const RE::BSAnimationGraphEvent* a_event, CollisionEventType a_eventType, CollisionDefinition& a_outCollisionDefinition) const;

	[[nodiscard]] static float GetVisualWeaponAttackReach(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_weaponNode, float a_reachLength, bool a_bUseVisualLengthMult);
	[[nodiscard]] static float GetWeaponAttackReach(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_weaponNode, RE::TESObjectWEAP* a_weapon, bool a_bVisual, bool a_bUseVisualLengthMult = true);
	[[nodiscard]] static float GetNodeAttackReach(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_node);

	static inline ContactListener contactListener{};
	static inline std::unordered_set<RE::ActorHandle> activeActors{};
	static inline std::unordered_set<uint16_t> hittableCharControllerGroups{};
	static inline std::unordered_set<uint16_t> ragdollCollisionGroups{};
	static inline std::unordered_map<RE::hkbRagdollDriver*, ActiveRagdoll> activeRagdolls{};
	static inline std::vector<PendingHit> pendingHits{};
	static inline std::unordered_map<RE::ActorHandle, float> activeHitstops;

	static inline float bCameraShakeActive = false;
	static inline float currentCameraShake = 0.f;
	static inline float currentCameraShakeTimer = 0.f;
	static inline float currentCameraShakeStrength = 0.f;
	static inline float currentCameraShakeLength = 0.f;
	static inline float currentCameraShakeFrequency = 0.f;
	static inline RE::NiPoint3 currentCameraShakeAxis = {};

	static inline CachedAttackData cachedAttackData{};

	static RE::NiPointer<RE::BGSAttackData>& GetOppositeAttackEvent(RE::NiPointer<RE::BGSAttackData>& a_attackData, RE::BGSAttackDataMap* a_attackDataMap);

	bool AddPreHitCallback(SKSE::PluginHandle a_pluginHandle, PreHitCallback a_preHitCallback);
	bool AddPostHitCallback(SKSE::PluginHandle a_pluginHandle, PostHitCallback a_postHitCallback);
	bool AddPrePhysicsStepCallback(SKSE::PluginHandle a_pluginHandle, PrePhysicsStepCallback a_prePhysicsHitCallback);
	bool AddCollisionFilterComparisonCallback(SKSE::PluginHandle a_pluginHandle, CollisionFilterComparisonCallback a_collisionFilterComparisonCallback);
	bool RemovePreHitCallback(SKSE::PluginHandle a_pluginHandle);
	bool RemovePostHitCallback(SKSE::PluginHandle a_pluginHandle);
	bool RemovePrePhysicsStepCallback(SKSE::PluginHandle a_pluginHandle);
	bool RemoveCollisionFilterComparisonCallback(SKSE::PluginHandle a_pluginHandle);

	[[nodiscard]] float GetAttackCollisionCapsuleLength(RE::ActorHandle a_actorHandle, RequestedAttackCollisionType a_collisionType = RequestedAttackCollisionType::Default) const;

	std::vector<PreHitCallbackReturn> RunPreHitCallbacks(const PrecisionHitData& a_precisionHitData);
	void RunPostHitCallbacks(const PrecisionHitData& a_precisionHitData, const RE::HitData& a_hitData);
	void RunPrePhysicsStepCallbacks(RE::bhkWorld* a_world);
	CollisionFilterComparisonResult RunCollisionFilterComparisonCallbacks(RE::bhkCollisionFilter* a_collisionFilter, uint32_t a_filterInfoA, uint32_t a_filterInfoB);

private:
	using Lock = std::shared_mutex;
	using ReadLocker = std::shared_lock<Lock>;
	using WriteLocker = std::unique_lock<Lock>;

	mutable Lock attackCollisionsLock;
	mutable Lock callbacksLock;
	mutable Lock activeHitstopsLock;

	PrecisionHandler();
	PrecisionHandler(const PrecisionHandler&) = delete;
	PrecisionHandler(PrecisionHandler&&) = delete;
	virtual ~PrecisionHandler() = default;

	PrecisionHandler& operator=(const PrecisionHandler&) = delete;
	PrecisionHandler& operator=(PrecisionHandler&&) = delete;

	std::unordered_map<RE::ActorHandle, AttackCollisions> _actorsWithAttackCollisions;
	std::vector<std::shared_ptr<AttackTrail>> _attackTrails;

	friend struct AttackCollision;
	friend struct AttackCollisions;
	friend struct PendingHit;

	void StartCollision_Impl(RE::ActorHandle a_actorHandle, uint32_t a_activeGraphIdx);

	static RE::NiPoint3 CalculateHitImpulse(RE::hkpRigidBody* a_rigidBody, const RE::NiPoint3& a_hitVelocity, float a_impulseMult, bool a_bIsActiveRagdoll);

	std::unordered_map<SKSE::PluginHandle, PreHitCallback> preHitCallbacks;
	std::unordered_map<SKSE::PluginHandle, PostHitCallback> postHitCallbacks;
	std::unordered_map<SKSE::PluginHandle, PrePhysicsStepCallback> prePhysicsStepCallbacks;
	std::unordered_map<SKSE::PluginHandle, CollisionFilterComparisonCallback> collisionFilterComparisonCallbacks;

	struct GenericJob
	{
		GenericJob() = default;
		virtual ~GenericJob() = default;

		virtual bool Run() = 0;
	};

	struct PointImpulseJob : GenericJob
	{
		RE::hkpRigidBody* rigidBody = nullptr;
		RE::ObjectRefHandle refHandle;
		RE::NiPoint3 hitVelocity;
		RE::hkVector4 hitPoint;
		float impulseMult;
		bool bIsActiveRagdoll;

		PointImpulseJob(RE::hkpRigidBody* a_rigidBody, RE::ObjectRefHandle a_refHandle, const RE::NiPoint3& a_hitVelocity, const RE::hkVector4& a_hitPoint, float a_impulseMult, bool a_bIsActiveRagdoll) :
			rigidBody(a_rigidBody), refHandle(a_refHandle), hitVelocity(a_hitVelocity), hitPoint(a_hitPoint), impulseMult(a_impulseMult), bIsActiveRagdoll(a_bIsActiveRagdoll) {}

		virtual bool Run() override
		{
			// Need to be safe since the job could run next frame where the rigidbody might not exist anymore
			if (refHandle) {
				auto ptr = refHandle.get();
				if (ptr) {
					// don't run for actors with hitstop until the hitstop ends
					if (ptr->formType == RE::FormType::ActorCharacter) {
						auto actor = ptr->As<RE::Actor>();
						if (PrecisionHandler::GetSingleton()->HasHitstop(actor->GetHandle())) {
							// refresh impulse timer
							Utils::ForEachRagdollDriver(actor, [=](RE::hkbRagdollDriver* driver) {
								ActiveRagdoll& ragdoll = activeRagdolls[driver];
								ragdoll.impulseTime = Settings::fRagdollImpulseTime;
							});
							return false;
						}
					}
					auto root = ptr->Get3D();
					if (root && Utils::FindRigidBody(root, rigidBody)) {
						if (Utils::IsMoveableEntity(rigidBody)) {
							hkpEntity_Activate(rigidBody);
							RE::NiPoint3 impulse = PrecisionHandler::CalculateHitImpulse(rigidBody, hitVelocity, impulseMult, bIsActiveRagdoll);
							rigidBody->motion.ApplyPointImpulse(Utils::NiPointToHkVector(impulse), hitPoint);
						}
					}
				}
			}
			return true;
		}
	};

	struct LinearImpulseJob : GenericJob
	{
		RE::hkpRigidBody* rigidBody = nullptr;
		RE::ObjectRefHandle refHandle;
		RE::NiPoint3 hitVelocity;
		float impulseMult;
		bool bIsActiveRagdoll;

		LinearImpulseJob(RE::hkpRigidBody* a_rigidBody, RE::ObjectRefHandle a_refHandle, const RE::NiPoint3& a_hitVelocity, float a_impulseMult, bool a_bIsActiveRagdoll) :
			rigidBody(a_rigidBody), refHandle(a_refHandle), hitVelocity(a_hitVelocity), impulseMult(a_impulseMult), bIsActiveRagdoll(a_bIsActiveRagdoll) {}

		virtual bool Run() override
		{
			// Need to be safe since the job could run next frame where the rigidbody might not exist anymore
			if (refHandle) {
				auto ptr = refHandle.get();
				if (ptr) {
					// don't run for actors with hitstop until the hitstop ends
					if (ptr->formType == RE::FormType::ActorCharacter) {
						auto actor = ptr->As<RE::Actor>();
						if (PrecisionHandler::GetSingleton()->HasHitstop(actor->GetHandle())) {
							// refresh impulse timer
							Utils::ForEachRagdollDriver(actor, [=](RE::hkbRagdollDriver* driver) {
								ActiveRagdoll& ragdoll = activeRagdolls[driver];
								ragdoll.impulseTime = Settings::fRagdollImpulseTime;
							});
							return false;
						}
					}
					auto root = ptr->Get3D();
					if (root && Utils::FindRigidBody(root, rigidBody)) {
						if (Utils::IsMoveableEntity(rigidBody)) {
							hkpEntity_Activate(rigidBody);
							RE::NiPoint3 impulse = PrecisionHandler::CalculateHitImpulse(rigidBody, hitVelocity, impulseMult, bIsActiveRagdoll);
							rigidBody->motion.ApplyLinearImpulse(Utils::NiPointToHkVector(impulse));
						}
					}
				}
			}
			return true;
		}
	};

	std::vector<std::unique_ptr<GenericJob>> _prePhysicsStepJobs;

	template <class T, typename... Args>
	void QueuePrePhysicsJob(Args&&... args)
	{
		static_assert(std::is_base_of<GenericJob, T>::value);
		_prePhysicsStepJobs.push_back(std::make_unique<T>(std::forward<Args>(args)...));
	}
};
