#include "Havok/Havok.h"

#include "Offsets.h"

RE::hkConstraintCinfo::~hkConstraintCinfo()
{
	hkConstraintCinfo_setConstraintData(this, nullptr);
}

void hkpWorld_removeContactListener(RE::hkpWorld* a_this, RE::hkpContactListener* a_worldListener)
{
	RE::hkArray<RE::hkpContactListener*>& listeners = a_this->contactListeners;

	for (int i = 0; i < listeners.size(); i++) {
		RE::hkpContactListener* listener = listeners[i];
		if (listener == a_worldListener) {
			listeners[i] = nullptr;
			return;
		}
	}
}

RE::bhkCharProxyController* GetCharProxyController(RE::Actor* a_actor)
{
	auto controller = a_actor->GetCharController();
	if (!controller) {
		return nullptr;
	}

	return skyrim_cast<RE::bhkCharProxyController*>(controller);
}

RE::hkMemoryRouter& hkGetMemoryRouter()
{
	return *(RE::hkMemoryRouter*)(uintptr_t)TlsGetValue(*g_dwTlsIndex);
}

RE::bhkRagdollConstraint* bhkRagdolConstraint_ctor(RE::bhkRagdollConstraint* a_this, [[maybe_unused]] RE::hkConstraintCinfo* a_cInfo)
{
	bhkRefObject_ctor(a_this);
	a_this->serializable = nullptr;

	SKSE::stl::atomic_ref bhkSerializableCount{ *GetBhkSerializableCount() };
	++bhkSerializableCount;

	SKSE::stl::emplace_vtable(a_this);

	bhkConstraint_Func46(a_this, nullptr);

	SKSE::stl::atomic_ref bhkRagdollConstraintCount{ *GetBhkRagdollConstraintCount() };
	++bhkRagdollConstraintCount;

	return a_this;
}
