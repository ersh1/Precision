#pragma once

#include <shared_mutex>

struct HitRefs
{
	void Update(float a_deltaTime);

	bool IsEmpty() const;
	bool HasHitRef(RE::ObjectRefHandle a_handle) const;
	void AddHitRef(RE::ObjectRefHandle a_handle, float a_duration, bool a_bIsNPC);
	void ClearHitRefs();
	void Clear();
	void IncreaseDamagedCount();

	inline uint32_t GetHitCount() const { return hitCount; }
	inline uint32_t GetHitNPCCount() const { return hitNPCCount; }
	inline uint32_t GetDamagedCount() const { return damagedCount; }

private:
	mutable Lock lock;

	std::unordered_map<RE::ObjectRefHandle, float> hitRefs;
	uint32_t hitCount = 0;
	uint32_t hitNPCCount = 0;
	uint32_t damagedCount = 0;
};
