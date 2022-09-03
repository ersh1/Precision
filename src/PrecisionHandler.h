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

enum CollisionEventType : std::uint8_t
{
	kAttackStart = 0,
	kAdd,
	kRemove,
	kClearTargets,
	kAttackEnd
};

struct CameraShake
{
	CameraShake(float a_strength, float a_length, float a_frequency) :
		strength(a_strength),
		length(a_length),
		frequency(a_frequency)
	{
		timer = length;
	}

	bool Update(float a_deltaTime) {
		currentValue = sinf((length - timer) * frequency) * strength * fmax(length - (length - timer), 0.f);
		timer -= a_deltaTime;

		return timer > 0.f;
	}
	
	inline float GetCurrentValue() const {
		return currentValue;
	}
	
private:
	float currentValue = 0.f;
	float timer = 0.f;
	float strength = 0.f;
	float length = 0.f;
	float frequency = 0.f;
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
	using WeaponCollisionCallback = PRECISION_API::WeaponCollisionCallback;
	using WeaponCollisionCallbackReturn = PRECISION_API::WeaponCollisionCallbackReturn;
	using CollisionFilterSetupCallback = PRECISION_API::CollisionFilterSetupCallback;
	using ContactListenerCallback = PRECISION_API::ContactListenerCallback;

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

	[[nodiscard]] static bool HasActiveImpulse(RE::ActorHandle a_actorHandle);

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

	void ApplyCameraShake(float a_strength, float a_length, float a_frequency, float a_distanceSquared);

	void ProcessPrePhysicsStepJobs();
	void ProcessMainUpdateJobs();
	void ProcessDelayedJobs(float a_deltaTime);

	bool GetAttackCollisionDefinition(RE::Actor* a_actor, AttackDefinition& a_outAttackDefinition, std::optional<bool> a_bIsLeftSwing = std::nullopt, AttackDefinition::SwingEvent a_swingEvent = AttackDefinition::SwingEvent::kWeaponSwing) const;

	bool ParseCollisionEvent(const RE::BSAnimationGraphEvent* a_event, CollisionEventType a_eventType, CollisionDefinition& a_outCollisionDefinition) const;

	bool CheckActorInCombat(RE::ActorHandle a_actorHandle);

	[[nodiscard]] static float GetWeaponMeshLength(RE::NiAVObject* a_weaponNode);
	[[nodiscard]] static float GetWeaponAttackLength(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_weaponNode, std::optional<float> a_overrideLength = std::nullopt, float a_lengthMult = 1.f);
	[[nodiscard]] static float GetWeaponAttackRadius(RE::ActorHandle a_actorHandle, std::optional<float> a_overrideRadius = std::nullopt, float a_radiusMult = 1.f);

	[[nodiscard]] static const RE::hkpCapsuleShape* GetNodeCapsuleShape(RE::NiAVObject* a_node);
	[[nodiscard]] static bool GetNodeAttackDimensions(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_node, std::optional<float> a_overrideLength, float a_lengthMult, std::optional<float> a_overrideRadius, float a_radiusMult, RE::hkVector4& a_outVertexA, RE::hkVector4& a_outVertexB, float& a_outRadius);
	[[nodiscard]] static float GetNodeAttackLength(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_node, std::optional<float> a_overrideLength = std::nullopt, float a_lengthMult = 1.f);
	[[nodiscard]] static float GetNodeAttackRadius(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_node, std::optional<float> a_overrideRadius = std::nullopt, float a_radiusMult = 1.f);

	static std::shared_ptr<ActiveRagdoll> GetActiveRagdollFromDriver(RE::hkbRagdollDriver* a_driver);

	static inline ContactListener contactListener{};

	static inline Lock activeActorsLock;
	static inline std::unordered_set<RE::ActorHandle> activeActors{};
	static inline std::unordered_set<uint16_t> activeControllerGroups{};

	static inline Lock hittableCharControllerGroupsLock;
	static inline std::unordered_set<uint16_t> hittableCharControllerGroups{};

	static inline Lock disabledActorsLock;
	static inline std::unordered_set<RE::ActorHandle> disabledActors{};

	static inline std::unordered_set<uint16_t> ragdollCollisionGroups{};

	static inline Lock activeRagdollsLock;
	static inline std::unordered_map<RE::hkbRagdollDriver*, std::shared_ptr<ActiveRagdoll>> activeRagdolls{};
	
	static inline std::vector<PendingHit> pendingHits{};
	static inline std::unordered_map<RE::ActorHandle, float> activeHitstops;

	static inline float bCameraShakeActive = false;
	static inline float currentCameraShake = 0.f;
	static inline float currentCameraShakeTimer = 0.f;
	static inline float currentCameraShakeStrength = 0.f;
	static inline float currentCameraShakeLength = 0.f;
	static inline float currentCameraShakeFrequency = 0.f;

	static inline CachedAttackData cachedAttackData{};

	static RE::NiPointer<RE::BGSAttackData>& GetOppositeAttackEvent(RE::NiPointer<RE::BGSAttackData>& a_attackData, RE::BGSAttackDataMap* a_attackDataMap);

	bool AddPreHitCallback(SKSE::PluginHandle a_pluginHandle, PreHitCallback a_preHitCallback);
	bool AddPostHitCallback(SKSE::PluginHandle a_pluginHandle, PostHitCallback a_postHitCallback);
	bool AddPrePhysicsStepCallback(SKSE::PluginHandle a_pluginHandle, PrePhysicsStepCallback a_prePhysicsHitCallback);
	bool AddCollisionFilterComparisonCallback(SKSE::PluginHandle a_pluginHandle, CollisionFilterComparisonCallback a_collisionFilterComparisonCallback);
	bool AddWeaponWeaponCollisionCallback(SKSE::PluginHandle a_pluginHandle, WeaponCollisionCallback a_weaponCollisionCallback);
	bool AddWeaponProjectileCollisionCallback(SKSE::PluginHandle a_pluginHandle, WeaponCollisionCallback a_weaponCollisionCallback);
	bool AddCollisionFilterSetupCallback(SKSE::PluginHandle a_pluginHandle, CollisionFilterSetupCallback a_collisionFilterSetupCallback);
	bool AddContactListenerCallback(SKSE::PluginHandle a_pluginHandle, ContactListenerCallback a_contactListenerCallback);
	bool RemovePreHitCallback(SKSE::PluginHandle a_pluginHandle);
	bool RemovePostHitCallback(SKSE::PluginHandle a_pluginHandle);
	bool RemovePrePhysicsStepCallback(SKSE::PluginHandle a_pluginHandle);
	bool RemoveCollisionFilterComparisonCallback(SKSE::PluginHandle a_pluginHandle);
	bool RemoveWeaponWeaponCollisionCallback(SKSE::PluginHandle a_pluginHandle);
	bool RemoveWeaponProjectileCollisionCallback(SKSE::PluginHandle a_pluginHandle);
	bool RemoveCollisionFilterSetupCallback(SKSE::PluginHandle a_pluginHandle);
	bool RemoveContactListenerCallback(SKSE::PluginHandle a_pluginHandle);

	[[nodiscard]] float GetAttackCollisionReach(RE::ActorHandle a_actorHandle, RequestedAttackCollisionType a_collisionType = RequestedAttackCollisionType::Default) const;

	[[nodiscard]] static bool IsActorActive(RE::ActorHandle a_actorHandle);
	[[nodiscard]] static bool IsActorActiveCollisionGroup(uint16_t a_collisionGroup);
	[[nodiscard]] static bool IsActorCharacterControllerHittable(RE::ActorHandle a_actorHandle);
	[[nodiscard]] static bool IsCharacterControllerHittable(RE::bhkCharacterController* a_controller);
	[[nodiscard]] static bool IsCharacterControllerHittableCollisionGroup(uint16_t a_collisionGroup);

	[[nodiscard]] static bool IsActorDisabled(RE::ActorHandle a_actorHandle);
	static bool ToggleDisableActor(RE::ActorHandle a_actorHandle, bool a_bDisable);

	std::vector<PreHitCallbackReturn> RunPreHitCallbacks(const PrecisionHitData& a_precisionHitData);
	void RunPostHitCallbacks(const PrecisionHitData& a_precisionHitData, const RE::HitData& a_hitData);
	void RunPrePhysicsStepCallbacks(RE::bhkWorld* a_world);
	CollisionFilterComparisonResult RunCollisionFilterComparisonCallbacks(RE::bhkCollisionFilter* a_collisionFilter, uint32_t a_filterInfoA, uint32_t a_filterInfoB);
	std::vector<WeaponCollisionCallbackReturn> RunWeaponWeaponCollisionCallbacks(const PrecisionHitData& a_precisionHitData);
	std::vector<WeaponCollisionCallbackReturn> RunWeaponProjectileCollisionCallbacks(const PrecisionHitData& a_precisionHitData);
	void RunCollisionFilterSetupCallbacks(RE::bhkCollisionFilter* a_collisionFilter);
	void RunContactListenerCallbacks(const RE::hkpContactPointEvent& a_event);

	struct GenericJob
	{
		GenericJob() = default;
		virtual ~GenericJob() = default;

		virtual bool Run() = 0;
	};

	struct DelayedJob
	{
		DelayedJob() = default;
		DelayedJob(float a_delay) :
			timeRemaining(a_delay)
		{}
		virtual ~DelayedJob() = default;

		virtual bool Run(float a_deltaTime) = 0;

		float timeRemaining;
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
								auto ragdoll = GetActiveRagdollFromDriver(driver);
								if (!ragdoll) {
									return;
								}
								ragdoll->impulseTime = Settings::fRagdollImpulseTime;
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
								auto ragdoll = GetActiveRagdollFromDriver(driver);
								if (!ragdoll) {
									return;
								}
								ragdoll->impulseTime = Settings::fRagdollImpulseTime;
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

	struct DelayedAttackCollisionJob : DelayedJob
	{
		RE::ActorHandle actorHandle;
		CollisionDefinition collisionDefinition;
		
		DelayedAttackCollisionJob(float a_delay, RE::ActorHandle a_actorHandle, const CollisionDefinition& a_collisionDefinition) :
			DelayedJob(a_delay), actorHandle(a_actorHandle), collisionDefinition(a_collisionDefinition) {}
		
		virtual bool Run(float a_deltaTime) override {
			timeRemaining -= a_deltaTime;
			if (timeRemaining > 0.f) {
				return false;
			}

			PrecisionHandler::GetSingleton()->AddAttackCollision(actorHandle, collisionDefinition);
			return true;
		}
	};

	template <class T, typename... Args>
	void QueuePrePhysicsJob(Args&&... args)
	{
		WriteLocker locker(jobsLock);
		
		static_assert(std::is_base_of<GenericJob, T>::value);
		_prePhysicsStepJobs.push_back(std::make_unique<T>(std::forward<Args>(args)...));
	}

	template <class T, typename... Args>
	void QueueMainUpdateJob(Args&&... args)
	{
		WriteLocker locker(jobsLock);
		
		static_assert(std::is_base_of<GenericJob, T>::value);
		_mainUpdateJobs.push_back(std::make_unique<T>(std::forward<Args>(args)...));
	}

	template <class T, typename... Args>
	void QueueDelayedJob(Args&&... args)
	{
		WriteLocker locker(jobsLock);
		
		static_assert(std::is_base_of<DelayedJob, T>::value);
		_delayedJobs.push_back(std::make_unique<T>(std::forward<Args>(args)...));
	}

private:
	mutable Lock attackCollisionsLock;
	mutable Lock callbacksLock;
	mutable Lock activeHitstopsLock;
	mutable Lock jobsLock;
	mutable Lock actorsInCombatLock;

	PrecisionHandler();
	PrecisionHandler(const PrecisionHandler&) = delete;
	PrecisionHandler(PrecisionHandler&&) = delete;
	virtual ~PrecisionHandler() = default;

	PrecisionHandler& operator=(const PrecisionHandler&) = delete;
	PrecisionHandler& operator=(PrecisionHandler&&) = delete;

	std::unordered_map<RE::ActorHandle, AttackCollisions> _actorsWithAttackCollisions;
	std::vector<std::shared_ptr<AttackTrail>> _attackTrails;

	

	std::unordered_map<RE::ActorHandle, float> _actorsInLingeringCombat;

	friend struct AttackCollision;
	friend struct AttackCollisions;
	friend struct PendingHit;
	friend class ContactListener;

	void StartCollision_Impl(RE::ActorHandle a_actorHandle, uint32_t a_activeGraphIdx);

	static RE::NiPoint3 CalculateHitImpulse(RE::hkpRigidBody* a_rigidBody, const RE::NiPoint3& a_hitVelocity, float a_impulseMult, bool a_bIsActiveRagdoll);

	std::unordered_map<SKSE::PluginHandle, PreHitCallback> preHitCallbacks;
	std::unordered_map<SKSE::PluginHandle, PostHitCallback> postHitCallbacks;
	std::unordered_map<SKSE::PluginHandle, PrePhysicsStepCallback> prePhysicsStepCallbacks;
	std::unordered_map<SKSE::PluginHandle, CollisionFilterComparisonCallback> collisionFilterComparisonCallbacks;
	std::unordered_map<SKSE::PluginHandle, WeaponCollisionCallback> weaponWeaponCollisionCallbacks;
	std::unordered_map<SKSE::PluginHandle, WeaponCollisionCallback> weaponProjectileCollisionCallbacks;
	std::unordered_map<SKSE::PluginHandle, CollisionFilterSetupCallback> collisionFilterSetupCallbacks;
	std::unordered_map<SKSE::PluginHandle, ContactListenerCallback> contactListenerCallbacks;

	std::vector<std::unique_ptr<GenericJob>> _prePhysicsStepJobs;
	std::vector<std::unique_ptr<GenericJob>> _mainUpdateJobs;
	std::vector<std::unique_ptr<DelayedJob>> _delayedJobs;
};
