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
	std::optional<RE::NiPoint3> groundShake;

	RE::ActorHandle actorHandle;
	RE::NiPointer<RE::NiNode> attackCollisionNode;
	RE::NiPointer<RE::NiNode> recoilCollisionNode;
	float capsuleLength = 0.f;
	float visualWeaponLength = 0.f;

	bool Add(const CollisionDefinition& a_collisionDefinition);
	bool Remove();
	bool RemoveRecoilCollision();

	bool IsValid() const;

	float GetVisualWeaponLength() const;

	bool HasHitRef(RE::ObjectRefHandle a_handle) const;
	void AddHitRef(RE::ObjectRefHandle a_handle, float a_duration, bool a_bIsNPC);
	void ClearHitRefs();
	void IncreaseDamagedCount();

	bool HasHitMaterial(RE::MATERIAL_ID a_materialID) const;
	void AddHitMaterial(RE::MATERIAL_ID a_materialID, float a_duration);
	void ClearHitMaterials();

	uint32_t GetHitCount() const;
	uint32_t GetHitNPCCount() const;
	uint32_t GetDamagedCount() const;

	bool Update(float a_deltaTime);
	uint32_t lastUpdate = 0;
	static inline uint32_t lastSplashUpdate = 0;

	bool bIsRecoiling = false;

private:
	mutable Lock hitMaterialsLock;
	
	bool CreateCollision(RE::bhkWorld* a_world, RE::Actor* a_actor, RE::NiNode* a_parentNode, RE::NiNode* a_newNode, RE::hkVector4& a_vertexA, RE::hkVector4& a_vertexB, float a_radius, CollisionLayer a_collisionLayer);
	bool RemoveCollision(RE::NiPointer<RE::NiNode>& a_node);	
	
	HitRefs _hitRefs{};
	std::unordered_map<RE::MATERIAL_ID, float> _hitMaterials;
};

struct AttackCollisions
{
	void Update(float a_deltaTime);

	[[nodiscard]] bool IsEmpty() const;
	[[nodiscard]] std::shared_ptr<AttackCollision> GetAttackCollision(RE::NiAVObject* a_node) const;
	[[nodiscard]] std::shared_ptr<AttackCollision> GetAttackCollisionFromRecoilNode(RE::NiAVObject* a_node) const;
	[[nodiscard]] std::shared_ptr<AttackCollision> GetAttackCollision(std::string_view a_nodeName) const;
	void AddAttackCollision(RE::ActorHandle a_actorHandle, const CollisionDefinition& a_collisionDefinition);
	bool RemoveRecoilCollision();
	[[nodiscard]] bool RemoveAttackCollision(const CollisionDefinition& a_collisionDefinition);
	[[nodiscard]] bool RemoveAttackCollision(std::shared_ptr<AttackCollision> a_attackCollision);
	bool RemoveAllAttackCollisions();

	void OnCollisionRemoved();
	void ClearData();

	void ForEachAttackCollision(std::function<void(std::shared_ptr<AttackCollision>)> a_func) const;

	bool HasHitRef(RE::ObjectRefHandle a_handle) const;
	uint32_t GetHitCount() const;
	uint32_t GetHitNPCCount() const;

	bool HasIDHitRef(uint8_t a_ID, RE::ObjectRefHandle a_handle) const;
	void AddIDHitRef(uint8_t a_ID, RE::ObjectRefHandle a_handle, float a_duration, bool a_bIsNPC);
	void ClearIDHitRefs(uint8_t a_ID);
	void IncreaseIDDamagedCount(uint8_t a_ID);
	uint32_t GetIDHitCount(uint8_t a_ID) const;
	uint32_t GetIDHitNPCCount(uint8_t a_ID) const;
	uint32_t GetIDDamagedCount(uint8_t a_ID) const;

	std::optional<uint32_t> ignoreVanillaAttackEvents = std::nullopt;
	bool bStartedWithWeaponSwing = false;
	bool bStartedWithWPNSwingUnarmed = false;
	

private:
	mutable Lock lock;

	std::vector<std::shared_ptr<AttackCollision>> _attackCollisions{};
	
	// Hit refs shared by collisions with assigned IDs
	std::unordered_map<uint8_t, HitRefs> _IDHitRefs{};
};
