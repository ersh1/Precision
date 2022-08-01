#include "HitRefs.h"

void HitRefs::Update(float a_deltaTime)
{
	// remove hit refs after cooldown
	auto hitRefIt = hitRefs.begin();
	while (hitRefIt != hitRefs.end()) {
		if (hitRefIt->second != FLT_MAX) {
			hitRefIt->second -= a_deltaTime;
			if (hitRefIt->second < 0.f) {
				hitRefIt = hitRefs.erase(hitRefIt);
			} else {
				++hitRefIt;
			}
		} else {
			++hitRefIt;
		}
	}
}

bool HitRefs::IsEmpty() const
{
	return hitRefs.empty();
}

bool HitRefs::HasHitRef(RE::ObjectRefHandle a_handle) const
{
	return hitRefs.contains(a_handle);
}

void HitRefs::AddHitRef(RE::ObjectRefHandle a_handle, float a_duration, bool a_bIsNPC)
{
	hitRefs.emplace(a_handle, a_duration);
	hitCount++;
	if (a_bIsNPC) {
		hitNPCCount++;
	}
}

void HitRefs::ClearHitRefs()
{
	hitRefs.clear();
}
