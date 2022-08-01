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

	uint32_t GetHitCount() const;
	uint32_t GetHitNPCCount() const;

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

	bool HasHitRef(RE::ObjectRefHandle a_handle) const;
	void AddHitRef(RE::ObjectRefHandle a_handle, float a_duration, bool a_bIsNPC);
	void ClearHitRefs();
	uint32_t GetHitCount() const;
	uint32_t GetHitNPCCount() const;

	bool HasIDHitRef(uint8_t a_ID, RE::ObjectRefHandle a_handle) const;
	void AddIDHitRef(uint8_t a_ID, RE::ObjectRefHandle a_handle, float a_duration, bool a_bIsNPC);
	void ClearIDHitRefs(uint8_t a_ID);
	uint32_t GetIDHitCount(uint8_t a_ID) const;
	uint32_t GetIDHitNPCCount(uint8_t a_ID) const;

	std::optional<uint32_t> ignoreVanillaAttackEvents;
	bool bStartedWithWeaponSwing = false;
	bool bStartedWithWPNSwingUnarmed = false;
	std::vector<std::shared_ptr<AttackCollision>> attackCollisions{};

private:
	// Hit refs shared by collisions with assigned IDs
	std::unordered_map<uint8_t, HitRefs> _IDHitRefs{};

	// Sum of all hit refs
	HitRefs _hitRefs{};
};
