#include "ActiveActor.h"

#include "Offsets.h" 
#include "PrecisionHandler.h"
#include "Utils.h"

ActiveActor::ActiveActor(RE::ActorHandle a_actorHandle, RE::NiAVObject* a_root, RE::NiAVObject* a_clone, uint16_t a_currentCollisionGroup, CollisionLayer a_collisionLayer) :
	actorHandle(a_actorHandle),
	root(a_root),
	clone(a_clone),
	collisionGroup(a_currentCollisionGroup),
	collisionLayer(a_collisionLayer)
{
	if (auto actor = actorHandle.get()) {
		actorScale = actor->GetScale();
	}

	FillCloneMap(a_clone, a_root);

	// Add to active collision groups
	{
		WriteLocker locker(PrecisionHandler::activeCollisionGroupsLock);

		PrecisionHandler::activeCollisionGroups.emplace(a_currentCollisionGroup);
	}
}

ActiveActor::~ActiveActor()
{
	 // Remove from active collision groups
	 WriteLocker locker(PrecisionHandler::activeCollisionGroupsLock);

	 PrecisionHandler::activeCollisionGroups.erase(collisionGroup);
 }

void ActiveActor::Update(float a_deltaTime)
{
	attackCollisions.Update(a_deltaTime);

	if (inCombat > 0.f) {
		inCombat = std::max(inCombat - a_deltaTime, 0.f);
	}

	if (receivedHitstopCooldown > 0.f) {
		receivedHitstopCooldown -= a_deltaTime;
		if (receivedHitstopCooldown <= 0.f) {
			receivedHitstopCount = 0;
			receivedHitstopCooldown = 0.f;
		}		
	}

	if (Settings::bDebug && Settings::bDisplaySkeletonColliders) {
		glm::vec4 color{ 1.f, 0.5f, 0.f, 1.f };
		Utils::DrawColliders(clone.get(), 0.f, color);
	}
}

bool ActiveActor::UpdateClone()
{
	auto actor = actorHandle.get();
	if (!actor)
	{
		return false;
	}
	
	// Early out if the actor's root node has changed (e.g. post-racemenu)
	auto currentRoot = actor->Get3D(false);
	if (currentRoot != root.get()) {
		return false;
	}

	// Early out if the actor's scale has changed
	auto currentScale = actor->GetScale();
	if (currentScale != actorScale) {
		return false;
	}
	
	// Check whether the collision group is still correct
	bool bCollisionGroupChanged = false;

	uint32_t filterInfo = 0;
	if (!Utils::GetActorCollisionFilterInfo(actor.get(), filterInfo)) {
		actor->GetCollisionFilterInfo(filterInfo);
	}

	uint16_t currentCollisionGroup = filterInfo >> 16;

	if (collisionGroup != currentCollisionGroup) {
		// Remove previous group from active collision groups and add the new one
		{
			WriteLocker locker(PrecisionHandler::activeCollisionGroupsLock);

			PrecisionHandler::activeCollisionGroups.erase(collisionGroup);
			PrecisionHandler::activeCollisionGroups.emplace(currentCollisionGroup);
		}

		//logger::debug("group changed for actor {} from {} to {}", actor->GetFormID(), collisionGroup, currentCollisionGroup);

		collisionGroup = currentCollisionGroup;

		bCollisionGroupChanged = true;
	}
	
	// Copy local transforms of respective bones from original to clone
	for (auto& entry : cloneToOriginalMap) {
		auto cloneNode = entry.first;
		auto originalNode = entry.second;
		
		cloneNode->local = originalNode->local;

		// Update collision group if changed
		if (bCollisionGroupChanged) {
			if (auto collidable = static_cast<RE::bhkCollisionObject*>(cloneNode->collisionObject.get())) {
				if (auto worldObject = collidable->body.get()) {
					if (auto hkpWorldObject = static_cast<RE::hkpWorldObject*>(worldObject->referencedObject.get())) {
						hkpWorldObject->collidable.broadPhaseHandle.collisionFilterInfo &= (0x0000ffff);  // zero out collision group
						hkpWorldObject->collidable.broadPhaseHandle.collisionFilterInfo |= (static_cast<uint32_t>(collisionGroup) << 16);  // set collision group to current
					}
				}
			}
		}
	}

	// Update the clone's local transform location to match the original's world transform location
	Utils::UpdateNodeTransformLocal(clone.get(), root->world);
	
	// Run the game's update function on the clone
	RE::NiUpdateData updateData;
	auto flags = reinterpret_cast<uint32_t*>(&updateData.flags);
	updateData.time = 0;
	*flags = 0;
	//*flags = 0x2000;  // velocity

	clone->Update(updateData);

	return true;
}

bool ActiveActor::CheckInCombat()
{
	auto actor = actorHandle.get();
	if (actor) {
		bool bIsInCombat = actor->IsInCombat();

		// refresh combat linger time because we're still in combat
		if (bIsInCombat) {
			inCombat = Settings::fCombatStateLingerTime;
		}

		return bIsInCombat || inCombat > 0.f;
	}

	return false;
}

RE::NiAVObject* ActiveActor::GetOriginalFromClone(RE::NiAVObject* a_clone)
{
	auto search = cloneToOriginalMap.find(a_clone);
	if (search != cloneToOriginalMap.end()) {
		return search->second;
	}

	return nullptr;
}

RE::hkpRigidBody* ActiveActor::GetOriginalFromClone(RE::hkpRigidBody* a_clone)
{
	if (auto node = GetNiObjectFromCollidable(a_clone->GetCollidable())) {
		if (auto originalNode = GetOriginalFromClone(node)) {
			if (auto collisionObject = originalNode->collisionObject.get()) {
				if (auto bhkNiCollisionObject = skyrim_cast<RE::bhkNiCollisionObject*>(collisionObject)) {
					if (auto body = bhkNiCollisionObject->body.get()) {
						if (auto hkpWorldObject = skyrim_cast<RE::hkpWorldObject*>(body->referencedObject.get())) {
							return static_cast<RE::hkpRigidBody*>(hkpWorldObject);
						}
					}
				}
			}
		}
	}

	return nullptr;
}

void ActiveActor::AddHitstop(float a_hitstopLength, bool a_bReceived)
{
	if (a_bReceived) {
		float diminishingReturnsMultiplier = pow(Settings::fHitstopDurationDiminishingReturnsFactor, receivedHitstopCount);
		a_hitstopLength *= diminishingReturnsMultiplier;
		
		++receivedHitstopCount;
		receivedHitstopCooldown = Settings::fReceivedHitstopCooldown;
	}

	hitstop += a_hitstopLength;
	driveToPoseHitstop += a_hitstopLength * Settings::fDriveToPoseHitstopMultiplier;
}

void ActiveActor::UpdateHitstop(float a_deltaTime)
{
	hitstop = fmax(hitstop - a_deltaTime, 0.f);
	driveToPoseHitstop = fmax(driveToPoseHitstop - a_deltaTime, 0.f);
}

bool ActiveActor::HasHitstop() const
{
	return hitstop > 0.f;
}

bool ActiveActor::HasDriveToPoseHitstop() const
{
	return driveToPoseHitstop > 0.f;
}

float ActiveActor::GetHitstopMultiplier(float a_deltaTime)
{
	float mult = 1.f;
	if (hitstop > 0.f && a_deltaTime > 0.f) {
		float newHitstopLength = hitstop - a_deltaTime;

		if (newHitstopLength <= 0.f) {
			mult = (a_deltaTime + newHitstopLength) / a_deltaTime;
		}

		mult = Settings::fHitstopSlowdownTimeMultiplier + ((1.f - Settings::fHitstopSlowdownTimeMultiplier) * (1.f - mult));
	}

	return mult;
}

float ActiveActor::GetDriveToPoseHitstopMultiplier(float a_deltaTime)
{
	float mult = 1.f;
	if (driveToPoseHitstop > 0.f && a_deltaTime > 0.f) {
		float newHitstopLength = driveToPoseHitstop - a_deltaTime;

		if (newHitstopLength <= 0.f) {
			mult = (a_deltaTime + newHitstopLength) / a_deltaTime;
		}

		mult = Settings::fHitstopSlowdownTimeMultiplier + ((1.f - Settings::fHitstopSlowdownTimeMultiplier) * (1.f - mult));
	}

	return mult;
}

void ActiveActor::FillCloneMap(RE::NiAVObject* a_clone, RE::NiAVObject* a_original)
{
	auto original = a_original->AsNode();
	auto cloned = a_clone->AsNode();

	if (original && cloned) {
		cloneToOriginalMap.emplace(cloned, original);

		for (auto& cloneChild : cloned->children) {
			if (cloneChild) {
				// remove nodes that are dead ends
				if (IsNodeDeadEnd(cloneChild.get())) {
					cloned->DetachChild(cloneChild.get());
					
					continue;
				}				

				// Check collision layers
				if (auto collidable = static_cast<RE::bhkCollisionObject*>(cloneChild->collisionObject.get())) {
					if (auto worldObject = collidable->body.get()) {
						if (auto hkpWorldObject = static_cast<RE::hkpWorldObject*>(worldObject->referencedObject.get())) {
							auto& collisionFilterInfo = hkpWorldObject->collidable.broadPhaseHandle.collisionFilterInfo;
							CollisionLayer layer = static_cast<CollisionLayer>(collisionFilterInfo & 0x7f);

							// remove children that have the char controller layer
							if (layer == CollisionLayer::kCharController) {
								cloned->DetachChild(cloneChild.get());
								
								continue;
							}
							
							// set collision layer
							collisionFilterInfo &= ~0x7F; // zero out collision layer
							collisionFilterInfo |= static_cast<uint32_t>(collisionLayer);
						}
					}
				}

				if (auto originalChild = original->GetObjectByName(cloneChild->name)) {
					FillCloneMap(cloneChild.get(), originalChild);
				}
			}
		}
	}
}

bool ActiveActor::IsNodeDeadEnd(RE::NiAVObject* a_object)
{
	if (a_object->collisionObject) {
		return false;
	}

	if (auto node = a_object->AsNode()) {
		for (auto& child : node->children) {
			if (child) {
				if (!IsNodeDeadEnd(child.get())) {
					return false;
				}
			}
		}
	}

	return true;
}
