#include "AttackTrail.h"

#include "PrecisionHandler.h"
#include "Settings.h"
#include "Utils.h"
#include "render/line_drawer.h"
#include "render/DrawHandler.h"

AttackTrail::AttackTrail(RE::NiNode* a_node, RE::ActorHandle a_actorHandle, RE::TESObjectCELL* a_cell, RE::InventoryEntryData* a_weaponItem, bool a_bIsLeftHand, bool a_bTrailUseTrueLength) :
	actorHandle(a_actorHandle)
{
	weaponRotation = RE::NiMatrix3(0.f, RE::NI_HALF_PI, -RE::NI_HALF_PI);
	if (a_node && a_node->parent && a_cell) {
		collisionNode = RE::NiPointer<RE::NiNode>(a_node);
		collisionParentNode = RE::NiPointer<RE::NiNode>(a_node->parent);
		collisionNodeLocalTransform = collisionNode->local;
		//collisionNodeLocalTransform.rotate = collisionNodeLocalTransform.rotate * weaponRotation;
		
		std::string_view trailMeshPath = Settings::attackTrailMeshPath;
		std::optional<std::string_view> additionalEmitterPath;
		
		TrailDefinition trailDefinition;
		if (GetTrailDefinition(a_actorHandle, a_weaponItem, a_bIsLeftHand, trailDefinition)) {
			if (trailDefinition.lifetimeMult) {
				lifetimeMult = *trailDefinition.lifetimeMult;
			}
			if (trailDefinition.baseColorOverride) {
				baseColorOverride = *trailDefinition.baseColorOverride;				
			}
			if (trailDefinition.baseColorScaleMult) {
				baseColorScaleMult = *trailDefinition.baseColorScaleMult;
			}
			if (trailDefinition.trailMeshOverride) {
				trailMeshPath = *trailDefinition.trailMeshOverride;
			}
		}

		trailParticle = RE::NiPointer<RE::BSTempEffectParticle>(RE::BSTempEffectParticle::Spawn(a_cell, 10.f, trailMeshPath.data(), collisionParentNode->world.rotate, collisionParentNode->world.translate, 1.f, 7, nullptr));

		if (!a_bTrailUseTrueLength)
		{
			weaponNode = RE::NiPointer<RE::NiNode>(static_cast<RE::NiNode*>(a_node->parent->children[0].get()));
			if (weaponNode && weaponNode != collisionNode) {
				if (a_weaponItem && a_weaponItem->object) {
					if (auto equippedWeapon = a_weaponItem->object->As<RE::TESObjectWEAP>()) {
						float length = PrecisionHandler::GetWeaponMeshLength(weaponNode.get());

						scale = length * 0.01f;

						return;
					}
				}
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

		if (!bExpired && !collisionNode->parent) {
			bExpired = true;
			visibilityPercent += (Settings::fTrailSegmentLifetime * lifetimeMult) * (1.f / Settings::fTrailFadeOutTime);
		}

		constexpr RE::NiPoint3 forwardVector{ 1.f, 0.f, 0.f };

		// add new position
		RE::NiTransform transform = collisionParentNode->world;
		transform = transform * collisionNodeLocalTransform;
		//transform.rotate = transform.rotate * weaponRotation;
		trailHistory.emplace_back(transform);
		
		if (trailParticle && trailParticle->particleObject) {
			if (!bAppliedTrailColorSettings || bExpired) {
				if (bExpired) {
					visibilityPercent = std::fmax(visibilityPercent - (1.f / Settings::fTrailFadeOutTime) * a_deltaTime, 0.f);
				}

				// apply color settings to trail
				RE::BSVisit::TraverseScenegraphGeometries(trailParticle->particleObject.get(), [&](auto&& a_geometry) -> RE::BSVisit::BSVisitControl {
					return ApplyColorSettings(std::forward<decltype(a_geometry)>(a_geometry), !bAppliedTrailColorSettings, bExpired);
				});

				bAppliedTrailColorSettings = true;
			}

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
							segmentsToAdd = segmentsToAddRemainder + (a_deltaTime * Settings::uTrailSegmentsPerSecond);
							segmentsToAddTrunc = trunc(segmentsToAdd);
							segmentsToAddRemainder = segmentsToAdd - segmentsToAddTrunc;	

							// move the tail if expired
							uint32_t segmentsToMove = 0;

							for (uint32_t i = 0; i < currentBoneIdx; ++i) {
								if (segmentTimestamps.size() > i && currentTime + currentTimeOffset > segmentTimestamps[i] + (Settings::fTrailSegmentLifetime * lifetimeMult)) {
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
								currentTimeOffset = segmentTimestamps[timestampIdx] + (Settings::fTrailSegmentLifetime * lifetimeMult) - currentTime;
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
							if (segmentsToAdd > 0.f) {
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

						if (bExpired && (currentBoneIdx == 0 || visibilityPercent == 0.f)) {
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

bool AttackTrail::GetTrailDefinition(RE::ActorHandle a_actorHandle, RE::InventoryEntryData* a_item, bool a_bIsLeftHand, TrailDefinition& a_outTrailDefinition) const
{
	if (!a_item || !a_item->object) {
		return false;
	}

	// search in the ALL trail list - in this case ALL conditions must be met
	auto searchAll = std::find_if(Settings::trailDefinitionsAll.begin(), Settings::trailDefinitionsAll.end(), [&](const TrailDefinition& a_trailDefinition) {
		// check weapon names
		if (a_trailDefinition.weaponNames) {
			if (auto fullNameForm = a_item->object->As<RE::TESFullName>()) {
				for (auto& name : *a_trailDefinition.weaponNames) {
					if (!SKSE::stl::string::icontains(fullNameForm->fullName, name)) {
						return false;
					}
				}
			}
		}

		// check weapon keywords
		if (a_trailDefinition.weaponKeywords) {
			if (auto itemKeywordForm = a_item->object->As<RE::BGSKeywordForm>()) {
				for (auto& keyword : *a_trailDefinition.weaponKeywords) {
					if (!itemKeywordForm->ContainsKeywordString(keyword)) {
						return false;
					}
				}
			} else {
				return false;
			}
		}
		
		// check enchantment
		if (a_trailDefinition.enchantmentNames || a_trailDefinition.effectNames || a_trailDefinition.effectKeywords || a_trailDefinition.effectShaders) {
			if (!a_actorHandle) {
				return false;
			}

			auto actor = a_actorHandle.get();
			if (!actor) {
				return false;
			}

			RE::MagicItem* magicItem = nullptr;

			// try to get poison first, then enchantment
			magicItem = GetPoison(a_item);
			if (!magicItem) {
				magicItem = GetEnchantment(a_item);
			}

			if (!magicItem) {
				return false;
			}
			
			// check enchantment names
			if (a_trailDefinition.enchantmentNames) {
				if (auto fullNameForm = magicItem->As<RE::TESFullName>()) {
					for (auto& name : *a_trailDefinition.enchantmentNames) {
						if (!SKSE::stl::string::icontains(fullNameForm->fullName, name)) {
							return false;
						}
					}
				}
			}

			auto actorValueForCost = GetActorValueForCost(magicItem, !a_bIsLeftHand);
			if (actorValueForCost != RE::ActorValue::kNone) {
				auto cost = magicItem->CalculateMagickaCost(actor.get());
				if (actor->GetActorValue(actorValueForCost) >= cost) {  // enchantment can be applied
					if (auto effect = magicItem->GetCostliestEffectItem()) {
						if (effect->baseEffect) {
							// check effect names
							if (a_trailDefinition.effectNames) {
								for (auto& name : *a_trailDefinition.effectNames) {
									if (!SKSE::stl::string::icontains(effect->baseEffect->fullName, name)) {
										return false;
									}
								}
							}

							// check effect shaders
							if (a_trailDefinition.effectShaders) {
								bool bAtLeastOne = false;  // check if at least one matches as one effect can't match multiple effects
								for (auto& effectShader : *a_trailDefinition.effectShaders) {
									if (effect->baseEffect->data.enchantShader == effectShader) {
										bAtLeastOne = true;
									}
								}

								if (!bAtLeastOne) {
									return false;
								}
							}
							
							// check effect keywords
							if (a_trailDefinition.effectKeywords) {
								for (auto& keyword : *a_trailDefinition.effectKeywords) {
									if (!effect->baseEffect->ContainsKeywordString(keyword)) {
										return false;
									}
								}
							}
						} else {
							return false;
						}
					} else {
						return false;
					}
				} else {
					return false;
				}
			}
		}

		// passed all the conditions!
		return true;
	});

	if (searchAll != Settings::trailDefinitionsAll.end()) {
		a_outTrailDefinition = *searchAll;
		return true;
	}
	
	// search in the ANY trail list - in this case ANY of the conditions has to be met
	auto searchAny = std::find_if(Settings::trailDefinitionsAny.begin(), Settings::trailDefinitionsAny.end(), [&](const TrailDefinition& a_trailDefinition) {
		// check weapon names
		if (a_trailDefinition.weaponNames) {
			if (auto fullNameForm = a_item->object->As<RE::TESFullName>()) {
				for (auto& name : *a_trailDefinition.weaponNames) {
					if (SKSE::stl::string::icontains(fullNameForm->fullName, name)) {
						return true;
					}
				}
			}
		}

		// check weapon keywords
		if (a_trailDefinition.weaponKeywords) {
			if (auto itemKeywordForm = a_item->object->As<RE::BGSKeywordForm>()) {
				for (auto& keyword : *a_trailDefinition.weaponKeywords) {
					if (itemKeywordForm->ContainsKeywordString(keyword)) {
						return true;
					}
				}
			}
		}

		// check enchantment
		if (a_trailDefinition.enchantmentNames || a_trailDefinition.effectKeywords || a_trailDefinition.effectShaders) {
			if (!a_actorHandle) {
				return false;
			}

			auto actor = a_actorHandle.get();
			if (!actor) {
				return false;
			}

			RE::MagicItem* magicItem = nullptr;

			// try to get poison first, then enchantment
			magicItem = GetPoison(a_item);
			if (!magicItem) {
				magicItem = GetEnchantment(a_item);
			}

			if (!magicItem) {
				return false;
			}

			// check enchantment names
			if (a_trailDefinition.enchantmentNames) {
				if (auto fullNameForm = magicItem->As<RE::TESFullName>()) {
					for (auto& name : *a_trailDefinition.enchantmentNames) {
						if (SKSE::stl::string::icontains(fullNameForm->fullName, name)) {
							return true;
						}
					}
				}
			}

			auto actorValueForCost = GetActorValueForCost(magicItem, !a_bIsLeftHand);
			if (actorValueForCost != RE::ActorValue::kNone) {
				auto cost = magicItem->CalculateMagickaCost(actor.get());
				if (actor->GetActorValue(actorValueForCost) >= cost) {  // enchantment can be applied

					auto checkEffect = [&](RE::Effect* a_effect) {
						if (!a_effect) {
							return false;
						}
						
						if (a_effect->baseEffect) {
							// check effect names
							if (a_trailDefinition.effectNames) {
								for (auto& name : *a_trailDefinition.effectNames) {
									if (SKSE::stl::string::icontains(a_effect->baseEffect->fullName, name)) {
										return true;
									}
								}
							}

							// check effect shaders
							if (a_trailDefinition.effectShaders) {
								for (auto& effectShader : *a_trailDefinition.effectShaders) {
									if (a_effect->baseEffect->data.enchantShader == effectShader) {
										return true;
									}
								}
							}

							// check effect keywords
							if (a_trailDefinition.effectKeywords) {
								for (auto& keyword : *a_trailDefinition.effectKeywords) {
									if (a_effect->baseEffect->ContainsKeywordString(keyword)) {
										return true;
									}
								}
							}
						}

						return false;
					};
					
					if (auto effect = magicItem->GetCostliestEffectItem()) {
						if (checkEffect(effect)) {
							return true;
						}
					}

					// if we got nothing, let's try all other effects
					for (auto& effect : magicItem->effects) {
						if (checkEffect(effect)) {
							return true;
						}
					}
				}
			}
		}
		
		// passed none of the conditions
		return false;
	});

	if (searchAny != Settings::trailDefinitionsAny.end()) {
		a_outTrailDefinition = *searchAny;
		return true;
	}

	return false;
}

RE::BSVisit::BSVisitControl AttackTrail::ApplyColorSettings(RE::BSGeometry* a_geometry, bool a_init, bool a_bExpired)
{
	const auto effect = a_geometry->properties[RE::BSGeometry::States::kEffect];
	const auto effectShader = netimmerse_cast<RE::BSEffectShaderProperty*>(effect.get());
	if (effectShader) {
		auto effectShaderMaterial = skyrim_cast<RE::BSEffectShaderMaterial*>(effectShader->material);
		if (effectShaderMaterial) {
			if (a_init) {
				// clone material and set it
				if (auto newMaterial = static_cast<RE::BSEffectShaderMaterial*>(effectShaderMaterial->Create())) {
					newMaterial->CopyMembers(effectShaderMaterial);
					effectShader->SetMaterial(newMaterial, false);
					// delete it because SetMaterial copies it
					newMaterial->~BSEffectShaderMaterial();
					RE::free(newMaterial);
					
					effectShaderMaterial = skyrim_cast<RE::BSEffectShaderMaterial*>(effectShader->material);

					if (baseColorOverride) {
						effectShaderMaterial->baseColor = *baseColorOverride;
					} else {
						effectShaderMaterial->baseColor.red = Settings::fTrailDefaultBaseColorR;
						effectShaderMaterial->baseColor.green = Settings::fTrailDefaultBaseColorG;
						effectShaderMaterial->baseColor.blue = Settings::fTrailDefaultBaseColorB;
						effectShaderMaterial->baseColor.alpha = Settings::fTrailDefaultBaseColorA;
					}

					if (baseColorScaleMult) {
						effectShaderMaterial->baseColorScale = *baseColorScaleMult;
					}

					effectShaderMaterial->baseColorScale *= Settings::fTrailBaseColorScaleMult;
				}				
			}			
			
			if (a_bExpired) {  // fade out alpha
				if (!originalTrailAlpha) {
					originalTrailAlpha = effectShaderMaterial->baseColor.alpha;
				}
				effectShaderMaterial->baseColor.alpha = *originalTrailAlpha * std::fmin(visibilityPercent, 1.f);
			}

			//return RE::BSVisit::BSVisitControl::kStop;
		}
	}
	
	return RE::BSVisit::BSVisitControl::kContinue;
}
