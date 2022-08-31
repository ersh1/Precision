#pragma once

#include <shared_mutex>

struct CachedAttackData
{
	CachedAttackData() :
		lock()
	{}

	bool IsDataCached() const
	{
		ReadLocker locker(lock);

		return bIsCached;
	}

	void Clear()
	{
		WriteLocker locker(lock);

		bIsCached = false;
		currentAttackDamageMult = 1.f;
		currentStaggerMult = 1.f;
		currentAttackingActorHandle = RE::ActorHandle();
		preciseHitPosition = RE::NiPoint3();
		preciseHitDirection = RE::NiPoint3();
		lastHitNode = nullptr;
		hittingNode = nullptr;
	}

	float GetDamageMult() const
	{
		ReadLocker locker(lock);

		return currentAttackDamageMult;
	}

	void SetDamageMult(float a_damageMult)
	{
		WriteLocker locker(lock);

		currentAttackDamageMult = a_damageMult;
		bIsCached = true;
	}

	float GetStaggerMult() const
	{
		ReadLocker locker(lock);

		return currentStaggerMult;
	}

	void SetStaggerMult(float a_staggerMult)
	{
		WriteLocker locker(lock);

		currentStaggerMult = a_staggerMult;
		bIsCached = true;
	}

	RE::ActorHandle GetAttackingActorHandle() const
	{
		ReadLocker locker(lock);

		return currentAttackingActorHandle;
	}

	void SetAttackingActorHandle(RE::ActorHandle a_actorHandle)
	{
		WriteLocker locker(lock);

		currentAttackingActorHandle = a_actorHandle;
		bIsCached = true;
	}

	void GetPreciseHitVectors(RE::NiPoint3& a_outHitPos, RE::NiPoint3& a_outHitDir) const
	{
		ReadLocker locker(lock);

		if (bIsCached) {
			a_outHitPos = preciseHitPosition;
			a_outHitDir = preciseHitDirection;
		}
	}

	void SetPreciseHitVectors(const RE::NiPoint3& a_hitPos, const RE::NiPoint3& a_hitDir)
	{
		WriteLocker locker(lock);

		preciseHitPosition = a_hitPos;
		preciseHitDirection = a_hitDir;
		bIsCached = true;
	}

	RE::NiAVObject* GetHittingNode() const
	{
		ReadLocker locker(lock);

		return hittingNode.get();
	}

	void SetHittingNode(RE::NiAVObject* a_node)
	{
		WriteLocker locker(lock);

		hittingNode = RE::NiPointer<RE::NiAVObject>(a_node);
		bIsCached = true;
	}

	RE::NiAVObject* GetLastHitNode() const
	{
		ReadLocker locker(lock);

		return lastHitNode.get();
	}

	void SetLastHitNode(RE::NiAVObject* a_node)
	{
		WriteLocker locker(lock);

		lastHitNode = RE::NiPointer<RE::NiAVObject>(a_node);
		bIsCached = true;
	}

	mutable Lock lock;

private:
	bool bIsCached = false;
	float currentAttackDamageMult = 1.f;
	float currentStaggerMult = 1.f;
	RE::ActorHandle currentAttackingActorHandle;
	RE::NiPoint3 preciseHitPosition;
	RE::NiPoint3 preciseHitDirection;
	RE::NiPointer<RE::NiAVObject> hittingNode = nullptr;
	RE::NiPointer<RE::NiAVObject> lastHitNode = nullptr;
};
