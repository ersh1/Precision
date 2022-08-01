#pragma once

struct PendingHit
{
	PendingHit(RE::Actor* a_attacker, RE::TESObjectREFR* a_target, RE::hkpRigidBody* a_hitRigidBody, RE::hkpRigidBody* a_hittingRigidBody, const RE::hkpContactPointEvent& a_event, RE::hkVector4 a_hitPointVelocity, int a_hitBodyIdx, std::shared_ptr<struct AttackCollision> a_attackCollision) :
		attacker(a_attacker), target(a_target), hitRigidBody(a_hitRigidBody), hittingRigidBody(a_hittingRigidBody), contactPoint(*a_event.contactPoint), hitPointVelocity(a_hitPointVelocity), attackCollision(a_attackCollision)
	{
		// the vanilla function needs the correct shape key, getting it this way seems to work correctly
		RE::hkpShapeKey* hittingBodyShapeKeysPtr = a_event.GetShapeKeys(a_hitBodyIdx ? 0 : 1);
		RE::hkpShapeKey* hitBodyShapeKeysPtr = a_event.GetShapeKeys(a_hitBodyIdx);
		hittingBodyShapeKey = hittingBodyShapeKeysPtr ? *hittingBodyShapeKeysPtr : RE::HK_INVALID_SHAPE_KEY;
		hitBodyShapeKey = hitBodyShapeKeysPtr ? *hitBodyShapeKeysPtr : RE::HK_INVALID_SHAPE_KEY;
	}

	void DoHit();

	RE::NiPointer<RE::Actor> attacker;
	RE::NiPointer<RE::TESObjectREFR> target;
	RE::hkRefPtr<RE::hkpRigidBody> hitRigidBody;
	RE::hkRefPtr<RE::hkpRigidBody> hittingRigidBody;

	RE::hkContactPoint contactPoint;
	RE::hkVector4 hitPointVelocity;

	RE::hkpShapeKey hitBodyShapeKey;
	RE::hkpShapeKey hittingBodyShapeKey;

	std::shared_ptr<struct AttackCollision> attackCollision;
};
