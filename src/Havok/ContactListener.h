#pragma once

class ContactListener : public RE::hkpContactListener
{
	struct CollisionEvent
	{
		enum class Type
		{
			Added,
			Removed
		};

		RE::hkpRigidBody* rbA = nullptr;
		RE::hkpRigidBody* rbB = nullptr;
		Type type;
	};

public:
	void ContactPointCallback(const RE::hkpContactPointEvent& a_event) override;

	void CollisionAddedCallback(const RE::hkpCollisionEvent& a_event) override;

	void CollisionRemovedCallback(const RE::hkpCollisionEvent& a_event) override;

	RE::bhkWorld* world = nullptr;
};
