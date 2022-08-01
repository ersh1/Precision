#pragma once

struct AttackTrail
{
	AttackTrail(RE::NiNode* a_node, RE::ActorHandle a_actorHandle, RE::TESObjectCELL* a_cell, RE::TESObjectWEAP* a_weapon);

	void Update(float a_deltaTime);

	RE::ActorHandle actorHandle;
	RE::NiPointer<RE::BSTempEffectParticle> trailParticle;
	RE::NiPointer<RE::NiNode> collisionNode;
	RE::NiPointer<RE::NiNode> weaponNode;
	float scale = 1.f;

	bool bExpired = false;
	bool bActive = true;

	uint32_t currentBoneIdx = 0;
	float currentTime = 0.f;
	float currentTimeOffset = 0.f;
	bool bInit = true;

	float segmentsToAddRemainder = 0.f;

	std::vector<RE::NiTransform> trailHistory;
	std::deque<float> segmentTimestamps;
};
