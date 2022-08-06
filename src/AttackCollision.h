#pragma once

#include "HitRefs.h"
#include "Settings.h"

struct AttackCollision
{
	AttackCollision(RE::ActorHandle a_actorHandle, const CollisionDefinition& a_collisionDefinition);

	~AttackCollision();

	std::string nodeName;
	std::optional<uint8_t> ID;
	bool bNoRecoil = false;
	float damageMult = 1.f;
	float staggerMult = 1.f;
	std::optional<float> lifetime;

	RE::ActorHandle actorHandle;
	RE::NiPointer<RE::NiNode> collisionNode;
	float capsuleLength = 0.f;
	float visualWeaponLength = 0.f;

	RE::NiNode* AddCollision(RE::ActorHandle a_actorHandle, const CollisionDefinition& a_collisionDefinition);
	bool RemoveCollision(RE::ActorHandle a_actorHandle);

	float GetVisualWeaponLength() const;

	bool HasHitRef(RE::ObjectRefHandle a_handle) const;
	void AddHitRef(RE::ObjectRefHandle a_handle, float a_duration, bool a_bIsNPC);
	void ClearHitRefs();
	void IncreaseDamagedCount();

	uint32_t GetHitCount() const;
	uint32_t GetHitNPCCount() const;
	uint32_t GetDamagedCount() const;

	bool Update(float a_deltaTime);
	uint32_t lastUpdate = 0;
	static inline uint32_t lastSplashUpdate = 0;

private:
	HitRefs _hitRefs{};
};

struct AttackCollisions
{
	AttackCollisions(std::optional<uint32_t> a_ignoreVanillaAttackEvents = std::nullopt) :
		ignoreVanillaAttackEvents(a_ignoreVanillaAttackEvents)
	{}

	void Update(float a_deltaTime);

	[[nodiscard]] bool IsEmpty() const;
	[[nodiscard]] std::shared_ptr<AttackCollision> GetAttackCollision(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_node) const;
	[[nodiscard]] std::shared_ptr<AttackCollision> GetAttackCollision(RE::ActorHandle a_actorHandle, std::string_view a_nodeName) const;
	void AddAttackCollision(RE::ActorHandle a_actorHandle, const CollisionDefinition& a_collisionDefinition);
	[[nodiscard]] bool RemoveAttackCollision(RE::ActorHandle a_actorHandle, const CollisionDefinition& a_collisionDefinition);
	[[nodiscard]] bool RemoveAttackCollision(RE::ActorHandle a_actorHandle, std::shared_ptr<AttackCollision> a_attackCollision);
	bool RemoveAllAttackCollisions(RE::ActorHandle a_actorHandle);

	void ForEachAttackCollision(std::function<void(std::shared_ptr<AttackCollision>)> a_func) const;

	bool HasHitRef(RE::ObjectRefHandle a_handle) const;
	void AddHitRef(RE::ObjectRefHandle a_handle, float a_duration, bool a_bIsNPC);
	void ClearHitRefs();
	uint32_t GetHitCount() const;
	uint32_t GetHitNPCCount() const;

	bool HasIDHitRef(uint8_t a_ID, RE::ObjectRefHandle a_handle) const;
	void AddIDHitRef(uint8_t a_ID, RE::ObjectRefHandle a_handle, float a_duration, bool a_bIsNPC);
	void ClearIDHitRefs(uint8_t a_ID);
	void IncreaseIDDamagedCount(uint8_t a_ID);
	uint32_t GetIDHitCount(uint8_t a_ID) const;
	uint32_t GetIDHitNPCCount(uint8_t a_ID) const;
	uint32_t GetIDDamagedCount(uint8_t a_ID) const;

	std::optional<uint32_t> ignoreVanillaAttackEvents;
	bool bStartedWithWeaponSwing = false;
	bool bStartedWithWPNSwingUnarmed = false;
	

private:
	using Lock = std::shared_mutex;
	using ReadLocker = std::shared_lock<Lock>;
	using WriteLocker = std::unique_lock<Lock>;

	mutable Lock lock;

	std::vector<std::shared_ptr<AttackCollision>> _attackCollisions{};
	
	// Hit refs shared by collisions with assigned IDs
	std::unordered_map<uint8_t, HitRefs> _IDHitRefs{};

	// Sum of all hit refs
	HitRefs _hitRefs{};
};
