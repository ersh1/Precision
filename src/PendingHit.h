#pragma once

struct PendingHit
{
	PendingHit(RE::Actor* a_attacker, RE::TESObjectREFR* a_target, RE::NiPointer<RE::bhkRigidBody>& a_hitRigidBodyWrapper, RE::NiPointer<RE::bhkRigidBody>& a_hittingRigidBodyWrapper, const RE::hkpContactPointEvent& a_event, RE::hkVector4 a_hitPointVelocity, int a_hitBodyIdx, std::shared_ptr<struct AttackCollision> a_attackCollision, bool a_bOnlyFX = false) :
		attacker(a_attacker), target(a_target), hitRigidBodyWrapper(a_hitRigidBodyWrapper), hittingRigidBodyWrapper(a_hittingRigidBodyWrapper), contactPoint(*a_event.contactPoint), hitPointVelocity(a_hitPointVelocity), attackCollision(a_attackCollision), bOnlyFX(a_bOnlyFX)
	{
		// the vanilla function needs the correct shape key, getting it this way seems to work correctly
		RE::hkpShapeKey* hittingBodyShapeKeysPtr = a_event.GetShapeKeys(a_hitBodyIdx ? 0 : 1);
		RE::hkpShapeKey* hitBodyShapeKeysPtr = a_event.GetShapeKeys(a_hitBodyIdx);
		hittingBodyShapeKey = hittingBodyShapeKeysPtr ? *hittingBodyShapeKeysPtr : RE::HK_INVALID_SHAPE_KEY;
		hitBodyShapeKey = hitBodyShapeKeysPtr ? *hitBodyShapeKeysPtr : RE::HK_INVALID_SHAPE_KEY;
	}

	void Run();
	void RunFXOnly();

	RE::NiPointer<RE::Actor> attacker;
	RE::NiPointer<RE::TESObjectREFR> target;
	RE::NiPointer<RE::bhkRigidBody> hitRigidBodyWrapper;
	RE::NiPointer<RE::bhkRigidBody> hittingRigidBodyWrapper;

	RE::hkContactPoint contactPoint;
	RE::hkVector4 hitPointVelocity;

	RE::hkpShapeKey hitBodyShapeKey;
	RE::hkpShapeKey hittingBodyShapeKey;

	std::shared_ptr<struct AttackCollision> attackCollision;

	bool bOnlyFX = false;
};
