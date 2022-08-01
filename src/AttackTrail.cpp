#include "AttackTrail.h"

#include "PrecisionHandler.h"
#include "Settings.h"
#include "Utils.h"
#include "render/line_drawer.h"

AttackTrail::AttackTrail(RE::NiNode* a_node, RE::ActorHandle a_actorHandle, RE::TESObjectCELL* a_cell, RE::TESObjectWEAP* a_weapon) :
	actorHandle(a_actorHandle), collisionNode(a_node)
{
	if (a_node && a_node->parent && a_cell) {
		trailParticle = RE::NiPointer<RE::BSTempEffectParticle>(RE::BSTempEffectParticle::Spawn(a_cell, 10.f, Settings::attackTrailMeshPath.data(), collisionNode->parent->world.rotate, collisionNode->parent->world.translate, 1.f, 7, nullptr));

		if (Settings::bTrailUseWeaponWorldBound) {
			weaponNode = RE::NiPointer<RE::NiNode>(static_cast<RE::NiNode*>(a_node->parent->children[0].get()));
			if (weaponNode && weaponNode != collisionNode) {
				float length = PrecisionHandler::GetWeaponAttackReach(a_actorHandle, weaponNode.get(), a_weapon, true, false);

				scale = length * 0.01f;

				return;
			}
		}

		Utils::Capsule capsule;
		Utils::GetCapsuleParams(a_node, capsule);

		float capsuleLength = capsule.a.GetDistance(capsule.b);
		scale = fmax(capsuleLength, capsule.radius) * 0.01f;
	}
}

void AttackTrail::Update(float a_deltaTime)
{
	if (Settings::bDisplayTrails) {
		if (!bActive) {
			return;
		}

		//a_deltaTime = PrecisionHandler::GetHitstop(actorHandle, a_deltaTime, false);

		// add new position
		if (bInit) {
			bInit = false;
			if (!collisionNode->parent) {
				return;
			}
			trailHistory.emplace_back(collisionNode->parent->world);
			return;
		} else if (!collisionNode->parent) {
			bExpired = true;
		} else {
			trailHistory.emplace_back(collisionNode->world);
		}

		constexpr RE::NiPoint3 forwardVector{ 1.f, 0.f, 0.f };

		if (trailParticle && trailParticle->particleObject) {
			if (auto fadeNode = trailParticle->particleObject->AsFadeNode()) {
				fadeNode->currentFade = 1.f;

				if (fadeNode->children.size() > 0) {
					auto trailRoot = fadeNode->GetObjectByName("TrailRoot"sv);
					auto trailRootNode = trailRoot->AsNode();
					if (trailRootNode && !trailRootNode->children.empty()) {
						if (trailHistory.size() >= 4) {
							float segmentsToAdd = 0.f;
							uint32_t segmentsToAddTrunc = 0;

							// calculate how many segments we'll be adding on this update
							if (!bExpired) {
								segmentsToAdd = segmentsToAddRemainder + (a_deltaTime * Settings::uTrailSegmentsPerSecond);
								segmentsToAddTrunc = trunc(segmentsToAdd);
								segmentsToAddRemainder = segmentsToAdd - segmentsToAddTrunc;
							}

							// move the tail if expired
							uint32_t segmentsToMove = 0;

							for (uint32_t i = 0; i < currentBoneIdx; ++i) {
								if (segmentTimestamps.size() > i && currentTime + currentTimeOffset > segmentTimestamps[i] + Settings::fTrailSegmentLifetime) {
									++segmentsToMove;
								} else {
									break;
								}
							}

							// check if there's gonna be enough bones left to add new segments, if not - forcibly move the tail, even if it's not expired yet
							uint32_t totalSegments = currentBoneIdx + segmentsToAddTrunc - segmentsToMove;
							if (totalSegments >= trailRootNode->children.size()) {
								segmentsToMove += totalSegments - (trailRootNode->children.size() - 1);
								uint32_t timestampIdx = segmentTimestamps.size() > segmentsToMove ? segmentsToMove : segmentTimestamps.size() - 1;
								currentTimeOffset = segmentTimestamps[timestampIdx] + Settings::fTrailSegmentLifetime - currentTime;
							}

							if (segmentsToMove > 0) {
								segmentTimestamps.erase(segmentTimestamps.begin(), segmentTimestamps.begin() + segmentsToMove);

								for (uint32_t i = 0; i < currentBoneIdx; ++i) {
									if (trailRootNode->children.size() > i + segmentsToMove) {
										auto& segmentBone = trailRootNode->children[i];
										auto& segmentToRead = trailRootNode->children[i + segmentsToMove];
										if (segmentBone && segmentToRead) {
											segmentBone->local = segmentToRead->local;
										}
									}
								}

								currentBoneIdx -= segmentsToMove;
							}

							// add new segment(s)
							if (!bExpired) {
								auto p3_it = trailHistory.rbegin();
								auto p2_it = p3_it + 1;
								auto p1_it = p2_it + 1;
								auto p0_it = p1_it + 1;

								auto& p0 = p0_it->translate;
								auto& p1 = p1_it->translate;
								auto& p2 = p2_it->translate;
								auto& p3 = p3_it->translate;

								for (uint32_t i = 0; i < segmentsToAddTrunc; ++i) {
									if (trailRootNode->children.size() > currentBoneIdx) {
										auto& segmentBone = trailRootNode->children[currentBoneIdx];
										if (segmentBone) {
											float t = (i + 1.f) / segmentsToAdd;

											RE::NiPoint3 p0end = p0 + (p0_it->rotate * forwardVector) * 50.f;
											RE::NiPoint3 p1end = p1 + (p1_it->rotate * forwardVector) * 50.f;
											RE::NiPoint3 p2end = p2 + (p2_it->rotate * forwardVector) * 50.f;
											RE::NiPoint3 p3end = p3 + (p3_it->rotate * forwardVector) * 50.f;

											RE::NiPoint3 interpolatedPos = Utils::CatmullRom(p0, p1, p2, p3, t);
											RE::NiPoint3 interpolatedEnd = Utils::CatmullRom(p0end, p1end, p2end, p3end, t);

											RE::NiPoint3 interpolatedDir = interpolatedEnd - interpolatedPos;
											interpolatedDir.Unitize();

											RE::NiTransform newTransform = segmentBone->world;

											Utils::SetRotationMatrix(newTransform.rotate, -interpolatedDir.x, interpolatedDir.y, interpolatedDir.z);

											RE::NiMatrix3 weaponRotation(0.f, RE::NI_HALF_PI, -RE::NI_HALF_PI);
											newTransform.rotate = newTransform.rotate * weaponRotation;
											newTransform.translate = interpolatedPos;
											newTransform.scale = scale;

											Utils::UpdateNodeTransformLocal(segmentBone.get(), newTransform);
											segmentBone->world = newTransform;

											segmentTimestamps.emplace_back(currentTime + a_deltaTime * t);
											++currentBoneIdx;
										}
									}
								}
							}
						}

						if (bExpired && currentBoneIdx == 0) {
							bActive = false;
							trailParticle->age += trailParticle->lifetime;
						}

						// move unused bones to the weapon pos
						if (trailHistory.size() > 0 && currentBoneIdx < trailRootNode->children.size()) {
							RE::NiTransform worldTransform = *(trailHistory.rbegin());

							RE::NiPoint3 end = worldTransform.translate + (worldTransform.rotate * forwardVector) * 50.f;
							RE::NiPoint3 dir = end - worldTransform.translate;
							dir.Unitize();

							//worldTransform.rotate.SetEulerAnglesXYZ(dir);
							Utils::SetRotationMatrix(worldTransform.rotate, -dir.x, dir.y, dir.z);
							RE::NiMatrix3 weaponRotation(0.f, RE::NI_HALF_PI, -RE::NI_HALF_PI);
							worldTransform.rotate = worldTransform.rotate * weaponRotation;
							worldTransform.scale = scale;

							RE::NiTransform localTransform = Utils::GetLocalTransform(trailRootNode->children[currentBoneIdx].get(), worldTransform);

							for (uint32_t i = currentBoneIdx; i < trailRootNode->children.size(); ++i) {
								auto& segmentBone = trailRootNode->children[i];
								if (segmentBone) {
									segmentBone->local = localTransform;
									segmentBone->world = worldTransform;
									//segmentBone->flags.set(RE::NiAVObject::Flag::kForceUpdate);
									//segmentBone->lastUpdatedFrameCounter = static_cast<uint32_t>(-1);
								}
							}

							//Settings::g_trueHUD->DrawArrow(worldTransform.translate, end, 10.f, 0.f);
						}
					}
				}
			}
		}

		currentTime += a_deltaTime;
	}
}
