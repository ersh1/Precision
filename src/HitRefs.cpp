#include "HitRefs.h"

void HitRefs::Update(float a_deltaTime)
{
	WriteLocker locker(lock);

	// remove hit refs after cooldown
	for (auto it = hitRefs.begin(); it != hitRefs.end();) {
		if (it->second == FLT_MAX) {
			++it;
		} else {
			it->second -= a_deltaTime;
			if (it->second < 0.f) {
				it = hitRefs.erase(it);
			} else {
				++it;
			}
		}
	}
}

bool HitRefs::IsEmpty() const
{
	ReadLocker locker(lock);
	
	return hitRefs.empty();
}

bool HitRefs::HasHitRef(RE::ObjectRefHandle a_handle) const
{
	ReadLocker locker(lock);
	
	return hitRefs.contains(a_handle);
}

void HitRefs::AddHitRef(RE::ObjectRefHandle a_handle, float a_duration, bool a_bIsNPC)
{
	WriteLocker locker(lock);
	
	hitRefs.emplace(a_handle, a_duration);
	++hitCount;
	if (a_bIsNPC) {
		++hitNPCCount;
	}
}

void HitRefs::ClearHitRefs()
{
	WriteLocker locker(lock);
	
	hitRefs.clear();
}

void HitRefs::Clear()
{
	ClearHitRefs();
	
	hitCount = 0;
	hitNPCCount = 0;
	damagedCount = 0;
}

void HitRefs::IncreaseDamagedCount()
{
	++damagedCount;
}
