#pragma once

class ContactListener : public RE::hkpContactListener
{
public:
	void ContactPointCallback(const RE::hkpContactPointEvent& a_event) override;

	void CollisionAddedCallback(const RE::hkpCollisionEvent& a_event) override;

	void CollisionRemovedCallback(const RE::hkpCollisionEvent& a_event) override;

	RE::bhkWorld* world = nullptr;
};
