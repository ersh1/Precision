#pragma once

#include "Settings.h"

struct AttackTrail
{
	AttackTrail(RE::NiNode* a_node, RE::ActorHandle a_actorHandle, RE::TESObjectCELL* a_cell, RE::InventoryEntryData* a_weaponItem, bool a_bIsLeftHand, bool a_bTrailUseTrueLength, std::optional<TrailOverride> a_trailOverride = std::nullopt);

	void Update(float a_deltaTime);

	bool GetTrailDefinition(RE::ActorHandle a_actorHandle, RE::InventoryEntryData* a_item, bool a_bIsLeftHand, TrailDefinition& a_outTrailDefinition) const;

	RE::ActorHandle actorHandle;
	RE::NiPointer<RE::BSTempEffectParticle> trailParticle;
	RE::NiPointer<RE::BSTempEffectParticle> bloodTrailParticle;
	RE::NiPointer<RE::NiNode> collisionNode;
	RE::NiPointer<RE::NiNode> collisionParentNode;
	RE::NiPointer<RE::NiNode> weaponNode;
	float scale = 1.f;
	float lifetimeMult = 1.f;
	std::optional<RE::NiColorA> baseColorOverride;
	std::optional<float> baseColorScaleMult;

	bool bExpired = false;
	bool bActive = true;
	bool bAppliedTrailColorSettings = false;

	std::optional<float> originalTrailAlpha;
	float visibilityPercent = 1.f;

	uint32_t currentBoneIdx = 0;
	float currentTime = 0.f;
	float currentTimeOffset = 0.f;

	float segmentsToAddRemainder = 0.f;

	RE::NiMatrix3 weaponRotation;
	RE::NiTransform collisionNodeLocalTransform;

	std::vector<RE::NiTransform> trailHistory;
	std::deque<float> segmentTimestamps;

private:
	RE::BSVisit::BSVisitControl ApplyColorSettings(RE::BSGeometry* a_geometry, bool a_init, bool a_bExpired);
		
};
