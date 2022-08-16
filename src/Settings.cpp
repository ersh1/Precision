#include "Settings.h"
#include <toml++/toml.h>

#include "PrecisionHandler.h"
#include "render/DrawHandler.h"

void Settings::Initialize()
{
	logger::info("Initializing...");

	auto dataHandler = RE::TESDataHandler::GetSingleton();
	if (dataHandler) {
		auto recoilMaterialsList = dataHandler->LookupForm<RE::BGSListForm>(0x810, "Precision.esp");
		if (recoilMaterialsList) {
			for (auto& form : recoilMaterialsList->forms) {
				auto materialType = form->As<RE::BGSMaterialType>();
				if (materialType) {
					recoilMaterials.emplace(materialType);
				}
			}
		}

		glob_nemesis = dataHandler->LookupForm<RE::TESGlobal>(0x801, "Precision.esp");
	}

	defaultBodyPartData = RE::TESForm::LookupByID<RE::BGSBodyPartData>(0x1D);

	logger::info("...success");
}

void Settings::ReadSettings()
{
	constexpr auto path = L"Data/SKSE/Plugins/Precision";
	constexpr auto ext = L".toml";
	constexpr auto basecfg = L"Data/SKSE/Plugins/Precision/Precision_base.toml";

	constexpr auto defaultSettingsPath = L"Data/MCM/Config/Precision/settings.ini";
	constexpr auto mcmPath = L"Data/MCM/Settings/Precision.ini";

	auto dataHandler = RE::TESDataHandler::GetSingleton();

	const auto readCollision = [](const toml::v2::table& a_collisionTable, std::vector<CollisionDefinition>& a_collisionDefs) {
		// node name
		auto nodeName = a_collisionTable["NodeName"].value<std::string_view>();

		auto ID = a_collisionTable["ID"].value<uint8_t>();

		// no recoil
		bool bNoRecoil = false;
		auto noRecoilVal = a_collisionTable["NoRecoil"].value<bool>();
		if (noRecoilVal) {
			bNoRecoil = *noRecoilVal;
		}

		// no trail
		bool bNoTrail = false;
		auto noTrailVal = a_collisionTable["NoTrail"].value<bool>();
		if (noTrailVal) {
			bNoTrail = *noTrailVal;
		}

		// weapon tip
		bool bWeaponTip = false;
		auto weaponTipVal = a_collisionTable["WeaponTip"].value<bool>();
		if (weaponTipVal) {
			bWeaponTip = *weaponTipVal;
		}

		// damage mult
		float damageMult = 1.f;
		auto damageMultVal = a_collisionTable["DamageMult"].value<float>();
		if (damageMultVal) {
			damageMult = *damageMultVal;
		}

		// duration
		float duration = 0.f;
		auto durationVal = a_collisionTable["Duration"].value<float>();
		if (durationVal) {
			duration = *durationVal;
		}

		// duration mult
		auto durationMult = a_collisionTable["DurationMult"].value<float>();

		// delay
		auto delay = a_collisionTable["Delay"].value<float>();

		// radius
		auto radius = a_collisionTable["Radius"].value<float>();

		// radius mult
		auto radiusMult = a_collisionTable["RadiusMult"].value<float>();

		// length
		auto length = a_collisionTable["Length"].value<float>();

		// length mult
		auto lengthMult = a_collisionTable["LengthMult"].value<float>();

		// transform
		std::optional<RE::NiTransform> transform;
		{
			const auto fillVector = [&](const toml::table* a_table, RE::NiPoint3& a_outVector) {
				if (a_table) {
					auto x = a_table->get("x");
					if (x) {
						a_outVector.x = *x->value<float>();
					}
					auto y = a_table->get("y");
					if (y) {
						a_outVector.y = *y->value<float>();
					}
					auto z = a_table->get("z");
					if (x) {
						a_outVector.z = *z->value<float>();
					}
				}
			};

			auto rotationTbl = a_collisionTable["Rotation"].as_table();
			auto translationTbl = a_collisionTable["Translation"].as_table();
			auto scaleVal = a_collisionTable["Scale"].value<float>();

			if (rotationTbl || translationTbl || scaleVal) {
				transform = RE::NiTransform();

				// rotation
				if (rotationTbl) {
					RE::NiPoint3 rotationVector{ 0.f, 0.f, 0.f };
					fillVector(rotationTbl, rotationVector);
					rotationVector.x = Utils::DegreeToRadian(rotationVector.x);
					rotationVector.y = Utils::DegreeToRadian(rotationVector.y);
					rotationVector.z = Utils::DegreeToRadian(rotationVector.z);
					transform->rotate.SetEulerAnglesXYZ(rotationVector);
				}

				// translation
				if (translationTbl) {
					fillVector(translationTbl, transform->translate);
				}

				// scale
				if (scaleVal) {
					transform->scale = *scaleVal;
				}
			}
		}

		a_collisionDefs.emplace_back(*nodeName, ID, bNoRecoil, bNoTrail, bWeaponTip, damageMult, duration, durationMult, delay, radius, radiusMult, length, lengthMult, transform);
	};

	const auto readToml = [&](std::filesystem::path path) {
		logger::info("  Reading {}...", path.string());
		try {
			const auto tbl = toml::parse_file(path.c_str());
			auto attackDefinitionsArr = tbl.get_as<toml::array>("AttackDefinitions");
			if (attackDefinitionsArr) {
				for (auto&& elem : *attackDefinitionsArr) {
					auto& definitionsTbl = *elem.as_table();
					auto formIDs = definitionsTbl["BodyPartDataFormIDs"].as_array();
					if (formIDs) {
						for (auto& formIDEntry : *formIDs) {
							auto formID = formIDEntry.value<uint32_t>();
							auto pluginName = definitionsTbl["Plugin"].value<std::string_view>();
							auto bodyPartData = dataHandler->LookupForm<RE::BGSBodyPartData>(*formID, *pluginName);
							if (bodyPartData) {
								auto& attackDefs = attackRaceDefinitions[bodyPartData];

								auto attacksArr = definitionsTbl["Attacks"].as_array();
								if (attacksArr) {
									for (auto&& attackEntry : *attacksArr) {
										// read attack
										auto& attackTbl = *attackEntry.as_table();

										// event name
										auto eventNames = attackTbl["EventNames"].as_array();
										if (eventNames) {
											for (auto& eventNameEntry : *eventNames) {
												auto eventName = eventNameEntry.value<std::string_view>();

												// read collision
												auto collisionArr = attackTbl["Collisions"].as_array();
												if (collisionArr) {
													std::vector<CollisionDefinition> collisionDefs{};
													
													for (auto&& collisionEntry : *collisionArr) {
														readCollision(*collisionEntry.as_table(), collisionDefs);
													}

													attackDefs.emplace(*eventName, AttackDefinition(collisionDefs));
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			
			auto attackAnimationDefinitionsArr = tbl.get_as<toml::array>("AttackAnimationDefinitions");
			if (attackAnimationDefinitionsArr) {
				for (auto&& elem : *attackAnimationDefinitionsArr) {
					auto& attackAnimationDefinitionsTbl = *elem.as_table();
					auto projectNames = attackAnimationDefinitionsTbl["ProjectNames"].as_array();
					if (projectNames) {
						for (auto& projectNameEntry : *projectNames) {
							auto projectName = projectNameEntry.value<std::string_view>();
							if (projectName) {
								auto& projectNameSv = *projectName;
								std::string projectNameStr{ projectNameSv };
								std::transform(projectNameStr.begin(), projectNameStr.end(), projectNameStr.begin(), [](unsigned char c) { return (unsigned char)std::tolower(c); });

								auto& attackDefs = attackAnimationDefinitions[projectNameStr.data()];

								auto attacksArr = attackAnimationDefinitionsTbl["Attacks"].as_array();
								if (attacksArr) {
									for (auto&& attackEntry : *attacksArr) {
										// read attack
										auto& attackTbl = *attackEntry.as_table();

										// animation file path
										auto animationFilePath = attackTbl["AnimationFilePath"].value<std::string_view>();
										if (animationFilePath) {
											auto& animationFilePathSv = *animationFilePath;
											std::string animationFilePathStr{ animationFilePathSv };
											std::transform(animationFilePathStr.begin(), animationFilePathStr.end(), animationFilePathStr.begin(), [](unsigned char c) { return (unsigned char)std::tolower(c); });
											std::replace(animationFilePathStr.begin(), animationFilePathStr.end(), '\\', '/');
											
											// read collision
											auto collisionArr = attackTbl["Collisions"].as_array();
											if (collisionArr) {
												std::vector<CollisionDefinition> collisionDefs{};

												for (auto&& collisionEntry : *collisionArr) {
													readCollision(*collisionEntry.as_table(), collisionDefs);
												}

												attackDefs.emplace(animationFilePathStr, AttackDefinition(collisionDefs));
											}
										}
									}
								}
							}
						}
					}
				}
			}

			auto trailDefinitionArr = tbl.get_as<toml::array>("TrailDefinition");
			if (trailDefinitionArr) {
				for (auto&& elem : *trailDefinitionArr) {
					auto& trailDefinitionTbl = *elem.as_table();

					// priority
					int32_t priority = 0;
					auto priorityVal = trailDefinitionTbl["Priority"].value<uint32_t>();
					if (priorityVal) {
						priority = *priorityVal;
					}

					// all
					bool bAll = false;
					auto allVal = trailDefinitionTbl["All"].value<bool>();
					if (allVal) {
						bAll = *allVal;
					}

					// conditions
					auto& trailConditionsTbl = *trailDefinitionTbl["Conditions"].as_table();

					// weapon names
					std::vector<std::string> weaponNames;
					bool bHasWeaponName = false;
					auto weaponNamesArr = trailConditionsTbl["WeaponNames"].as_array();
					if (weaponNamesArr) {
						for (auto& weaponNameEntry : *weaponNamesArr) {
							auto weaponName = weaponNameEntry.value<std::string_view>();
							if (weaponName) {
								weaponNames.emplace_back(*weaponName);
								bHasWeaponName = true;
							}
						}
					}
					
					// weapon keywords
					std::vector<std::string> weaponKeywords;
					bool bHasWeaponKeyword = false;
					auto weaponKeywordsArr = trailConditionsTbl["WeaponKeywords"].as_array();
					if (weaponKeywordsArr) {
						for (auto& weaponKeywordEntry : *weaponKeywordsArr) {
							auto weaponKeyword = weaponKeywordEntry.value<std::string_view>();
							if (weaponKeyword) {
								weaponKeywords.emplace_back(*weaponKeyword);
								bHasWeaponKeyword = true;
							}
						}
					}

					// enchantment names
					std::vector<std::string> enchantmentNames;
					bool bHasEnchantmentName = false;
					auto enchantmentNamesArr = trailConditionsTbl["EnchantmentNames"].as_array();
					if (enchantmentNamesArr) {
						for (auto& enchantmentNameEntry : *enchantmentNamesArr) {
							auto enchantmentName = enchantmentNameEntry.value<std::string_view>();
							if (enchantmentName) {
								enchantmentNames.emplace_back(*enchantmentName);
								bHasEnchantmentName = true;
							}
						}
					}

					// effect names
					std::vector<std::string> effectNames;
					bool bHasEffectName = false;
					auto effectNamesArr = trailConditionsTbl["EffectNames"].as_array();
					if (effectNamesArr) {
						for (auto& effectNameEntry : *effectNamesArr) {
							auto effectName = effectNameEntry.value<std::string_view>();
							if (effectName) {
								effectNames.emplace_back(*effectName);
								bHasEffectName = true;
							}
						}
					}
					
					// effect keywords
					std::vector<std::string> effectKeywords;
					bool bHasEffectKeyword = false;
					auto effectKeywordsArr = trailConditionsTbl["EffectKeywords"].as_array();
					if (effectKeywordsArr) {
						for (auto& effectKeywordEntry : *effectKeywordsArr) {
							auto effectKeyword = effectKeywordEntry.value<std::string_view>();
							if (effectKeyword) {
								effectKeywords.emplace_back(*effectKeyword);
								bHasEffectKeyword = true;
							}
						}
					}

					// effect shaders
					std::vector<RE::TESEffectShader*> effectShaders;
					bool bHasEffectShader = false;
					auto effectShadersArr = trailConditionsTbl["EffectShaders"].as_array();
					if (effectShadersArr) {
						for (auto& effectShadersEntry : *effectShadersArr) {
							auto& effectShaderTbl = *effectShadersEntry.as_table();
							
							auto formID = effectShaderTbl["FormID"].value<RE::FormID>();
							auto pluginName = effectShaderTbl["Plugin"].value<std::string_view>();

							auto effectShader = dataHandler->LookupForm<RE::TESEffectShader>(*formID, *pluginName);
							if (effectShader) {
								effectShaders.emplace_back(effectShader);
								bHasEffectShader = true;
							}
						}
					}

					if (bHasWeaponKeyword || bHasEffectKeyword || bHasEffectShader) {

						// conditions
						auto& trailVisualsTbl = *trailDefinitionTbl["Visuals"].as_table();
						
						// lifetime mult
						auto lifetimeMult = trailVisualsTbl["LifetimeMult"].value<float>();
						
						const auto fillColor = [&](const toml::table* a_table, RE::NiColorA& a_outColor) {
							if (a_table) {
								auto r = a_table->get("r");
								if (r) {
									a_outColor.red = *r->value<float>();
								}
								auto g = a_table->get("g");
								if (g) {
									a_outColor.green = *g->value<float>();
								}
								auto b = a_table->get("b");
								if (b) {
									a_outColor.blue = *b->value<float>();
								}
								auto a = a_table->get("a");
								if (a) {
									a_outColor.alpha = *a->value<float>();
								}
							}
						};

						// base color override
						std::optional<RE::NiColorA> baseColorOverride;
						auto baseColorOverrideTbl = trailVisualsTbl["BaseColorOverride"].as_table();
						if (baseColorOverrideTbl) {
							baseColorOverride = RE::NiColorA();
							fillColor(baseColorOverrideTbl, *baseColorOverride);
						}

						// base color scale mult
						auto baseColorScaleMult = trailVisualsTbl["BaseColorScaleMult"].value<float>();
						
						// trail mesh override
						std::optional<std::string> trailMeshOverride;
						auto trailMeshOverrideVal = trailVisualsTbl["TrailMeshOverride"].value<std::string_view>();
						if (trailMeshOverrideVal) {
							trailMeshOverride = *trailMeshOverrideVal;							
						}

						std::vector<TrailDefinition>* trailDefinitions = nullptr;
						if (bAll) {
							trailDefinitions = &trailDefinitionsAll;
						} else {
							trailDefinitions = &trailDefinitionsAny;
						}
						
						trailDefinitions->emplace_back(priority, bHasWeaponName ? std::optional(weaponNames) : std::nullopt, bHasWeaponKeyword ? std::optional(weaponKeywords) : std::nullopt, bHasEnchantmentName ? std::optional(enchantmentNames) : std::nullopt, bHasEffectName ? std::optional(effectNames) : std::nullopt, bHasEffectKeyword ? std::optional(effectKeywords) : std::nullopt, bHasEffectShader ? std::optional(effectShaders) : std::nullopt, lifetimeMult, baseColorOverride, baseColorScaleMult, trailMeshOverride);
					}
				}
			}
			
			auto attackEventPairArr = tbl.get_as<toml::array>("AttackEventPair");
			if (attackEventPairArr) {
				for (auto&& elem : *attackEventPairArr) {
					auto& attackEventPairTbl = *elem.as_table();
					auto rightEvent = attackEventPairTbl["RightEvent"].value<std::string_view>();
					auto leftEvent = attackEventPairTbl["LeftEvent"].value<std::string_view>();
					if (rightEvent && leftEvent) {
						attackEventPairs.emplace_back(*rightEvent, *leftEvent);
					}
				}
			}

		} catch (const toml::parse_error& e) {
			std::ostringstream ss;
			ss
				<< "Error parsing file \'" << *e.source().path << "\':\n"
				<< '\t' << e.description() << '\n'
				<< "\t\t(" << e.source().begin << ')';
			logger::error(ss.str());
			util::report_and_fail("failed to load settings"sv);
		} catch (const std::exception& e) {
			util::report_and_fail(e.what());
		} catch (...) {
			util::report_and_fail("unknown failure"sv);
		}
	};

	logger::info("Reading .toml files...");

	attackRaceDefinitions.clear();
	attackAnimationDefinitions.clear();
	trailDefinitionsAny.clear();
	attackEventPairs.clear();

	auto baseToml = std::filesystem::path(basecfg);
	readToml(baseToml);
	if (std::filesystem::is_directory(path)) {
		for (const auto& file : std::filesystem::directory_iterator(path)) {  // read all toml files in Data/SKSE/Plugins/Precision folder
			if (std::filesystem::is_regular_file(file) && file.path().extension() == ext) {
				auto filePath = file.path();
				if (filePath != basecfg) {
					readToml(filePath);
				}
			}
		}
	}
	
	// sort trailDefinitions based on priority
	std::stable_sort(trailDefinitionsAny.begin(), trailDefinitionsAny.end(), [](const auto& a, const auto& b) {
		return a.priority > b.priority;
	});
	std::stable_sort(trailDefinitionsAll.begin(), trailDefinitionsAll.end(), [](const auto& a, const auto& b) {
		return a.priority > b.priority;
	});

	logger::info("...success");

	const auto readMCM = [&](std::filesystem::path path) {
		CSimpleIniA mcm;
		mcm.SetUnicode();

		mcm.LoadFile(path.string().c_str());

		// Attack Collisions
		ReadBoolSetting(mcm, "AttackCollisions", "bAttackCollisionsEnabled", bAttackCollisionsEnabled);
		ReadBoolSetting(mcm, "AttackCollisions", "bEnableJumpIframes", bEnableJumpIframes);
		ReadBoolSetting(mcm, "AttackCollisions", "bNoPlayerTeammateAttackCollision", bNoPlayerTeammateAttackCollision);
		ReadBoolSetting(mcm, "AttackCollisions", "bNoNonHostileAttackCollision", bNoNonHostileAttackCollision);
		ReadFloatSetting(mcm, "AttackCollisions", "fCombatStateLingerTime", fCombatStateLingerTime);
		ReadBoolSetting(mcm, "AttackCollisions", "bDisablePhysicalCollisionOnHit", bDisablePhysicalCollisionOnHit);
		ReadFloatSetting(mcm, "AttackCollisions", "fWeaponLengthMult", fWeaponLengthMult);
		ReadFloatSetting(mcm, "AttackCollisions", "fWeaponCapsuleRadius", fWeaponCapsuleRadius);
		ReadFloatSetting(mcm, "AttackCollisions", "fMinWeaponLength", fMinWeaponLength);
		ReadFloatSetting(mcm, "AttackCollisions", "fDefaultCollisionLifetime", fDefaultCollisionLifetime);
		ReadFloatSetting(mcm, "AttackCollisions", "fDefaultCollisionLifetimePowerAttackMult", fDefaultCollisionLifetimePowerAttackMult);
		ReadFloatSetting(mcm, "AttackCollisions", "fHitSameRefCooldown", fHitSameRefCooldown);
		ReadFloatSetting(mcm, "AttackCollisions", "fFirstPersonAttackLengthOffset", fFirstPersonAttackLengthOffset);
		ReadFloatSetting(mcm, "AttackCollisions", "fPlayerAttackLengthMult", fPlayerAttackLengthMult);
		ReadFloatSetting(mcm, "AttackCollisions", "fPlayerAttackRadiusMult", fPlayerAttackRadiusMult);
		ReadFloatSetting(mcm, "AttackCollisions", "fMountedAttackLengthMult", fMountedAttackLengthMult);
		ReadFloatSetting(mcm, "AttackCollisions", "fMountedAttackRadiusMult", fMountedAttackRadiusMult);
		ReadUInt32Setting(mcm, "AttackCollisions", "uSweepAttackMode", (uint32_t&)uSweepAttackMode);
		ReadUInt32Setting(mcm, "AttackCollisions", "uMaxTargetsNoSweepAttack", uMaxTargetsNoSweepAttack);
		ReadUInt32Setting(mcm, "AttackCollisions", "uMaxTargetsSweepAttack", uMaxTargetsSweepAttack);
		ReadFloatSetting(mcm, "AttackCollisions", "fSweepAttackDiminishingReturnsFactor", fSweepAttackDiminishingReturnsFactor);

		// Trails
		ReadBoolSetting(mcm, "Trails", "bDisplayTrails", bDisplayTrails);
		ReadFloatSetting(mcm, "Trails", "fTrailSegmentLifetime", fTrailSegmentLifetime);
		ReadFloatSetting(mcm, "Trails", "fTrailFadeOutTime", fTrailFadeOutTime);
		ReadUInt32Setting(mcm, "Trails", "uTrailSegmentsPerSecond", uTrailSegmentsPerSecond);
		ReadFloatSetting(mcm, "Trails", "fTrailDefaultBaseColorR", fTrailDefaultBaseColorR);
		ReadFloatSetting(mcm, "Trails", "fTrailDefaultBaseColorG", fTrailDefaultBaseColorG);
		ReadFloatSetting(mcm, "Trails", "fTrailDefaultBaseColorB", fTrailDefaultBaseColorB);
		ReadFloatSetting(mcm, "Trails", "fTrailDefaultBaseColorA", fTrailDefaultBaseColorA);
		ReadFloatSetting(mcm, "Trails", "fTrailBaseColorScaleMult", fTrailBaseColorScaleMult);

		// Hitstop
		ReadBoolSetting(mcm, "Hitstop", "bEnableHitstop", bEnableHitstop);
		ReadBoolSetting(mcm, "Hitstop", "bApplyHitstopToTarget", bApplyHitstopToTarget);
		ReadFloatSetting(mcm, "Hitstop", "fHitstopDurationNPC", fHitstopDurationNPC);
		ReadFloatSetting(mcm, "Hitstop", "fHitstopDurationOther", fHitstopDurationOther);
		ReadFloatSetting(mcm, "Hitstop", "fHitstopSlowdownTimeMultiplier", fHitstopSlowdownTimeMultiplier);
		ReadFloatSetting(mcm, "Hitstop", "fHitstopDurationPowerAttackMultiplier", fHitstopDurationPowerAttackMultiplier);
		ReadFloatSetting(mcm, "Hitstop", "fHitstopDurationTwoHandedMultiplier", fHitstopDurationTwoHandedMultiplier);
		ReadFloatSetting(mcm, "Hitstop", "fHitstopDurationDiminishingReturnsFactor", fHitstopDurationDiminishingReturnsFactor);
		ReadFloatSetting(mcm, "Hitstop", "fHitstopGroundFeetDistanceThreshold", fHitstopGroundFeetDistanceThreshold);

		ReadBoolSetting(mcm, "Hitstop", "bEnableHitstopCameraShake", bEnableHitstopCameraShake);
		ReadFloatSetting(mcm, "Hitstop", "fHitstopCameraShakeStrengthNPC", fHitstopCameraShakeStrengthNPC);
		ReadFloatSetting(mcm, "Hitstop", "fHitstopCameraShakeStrengthOther", fHitstopCameraShakeStrengthOther);
		ReadFloatSetting(mcm, "Hitstop", "fHitstopCameraShakeDurationNPC", fHitstopCameraShakeDurationNPC);
		ReadFloatSetting(mcm, "Hitstop", "fHitstopCameraShakeDurationOther", fHitstopCameraShakeDurationOther);
		ReadFloatSetting(mcm, "Hitstop", "fHitstopCameraShakeFrequency", fHitstopCameraShakeFrequency);
		ReadFloatSetting(mcm, "Hitstop", "fHitstopCameraShakePowerAttackMultiplier", fHitstopCameraShakePowerAttackMultiplier);
		ReadFloatSetting(mcm, "Hitstop", "fHitstopCameraShakeTwoHandedMultiplier", fHitstopCameraShakeTwoHandedMultiplier);
		ReadFloatSetting(mcm, "Hitstop", "fHitstopCameraShakeDurationDiminishingReturnsFactor", fHitstopCameraShakeDurationDiminishingReturnsFactor);
		ReadFloatSetting(mcm, "Hitstop", "fHitstopCameraShakeGroundFeetDistanceThreshold", fHitstopCameraShakeGroundFeetDistanceThreshold);

		// Recoil
		ReadBoolSetting(mcm, "Recoil", "bRecoilPlayer", bRecoilPlayer);
		ReadBoolSetting(mcm, "Recoil", "bRecoilNPC", bRecoilNPC);
		ReadBoolSetting(mcm, "Recoil", "bRecoilPowerAttack", bRecoilPowerAttack);
		ReadBoolSetting(mcm, "Recoil", "bUseVanillaRecoil", bUseVanillaRecoil);
		ReadFloatSetting(mcm, "Recoil", "fRecoilFirstPersonDistanceThreshold", fRecoilFirstPersonDistanceThreshold);
		ReadFloatSetting(mcm, "Recoil", "fRecoilThirdPersonDistanceThreshold", fRecoilThirdPersonDistanceThreshold);
		ReadFloatSetting(mcm, "Recoil", "fRecoilGroundFeetDistanceThreshold", fRecoilGroundFeetDistanceThreshold);

		ReadBoolSetting(mcm, "Recoil", "bEnableRecoilCameraShake", bEnableRecoilCameraShake);
		ReadFloatSetting(mcm, "Recoil", "fRecoilCameraShakeStrength", fRecoilCameraShakeStrength);
		ReadFloatSetting(mcm, "Recoil", "fRecoilCameraShakeDuration", fRecoilCameraShakeDuration);
		ReadFloatSetting(mcm, "Recoil", "fRecoilCameraShakeFrequency", fRecoilCameraShakeFrequency);

		// Hit Impulse
		ReadBoolSetting(mcm, "HitImpulse", "bApplyImpulseOnHit", bApplyImpulseOnHit);
		ReadBoolSetting(mcm, "HitImpulse", "bApplyImpulseOnKill", bApplyImpulseOnKill);
		ReadFloatSetting(mcm, "HitImpulse", "fHitImpulseBaseMult", fHitImpulseBaseMult);
		ReadFloatSetting(mcm, "HitImpulse", "fHitImpulseBlockMult", fHitImpulseBlockMult);
		ReadFloatSetting(mcm, "HitImpulse", "fHitImpulsePowerAttackMult", fHitImpulsePowerAttackMult);
		ReadFloatSetting(mcm, "HitImpulse", "fHitImpulseRagdollMult", fHitImpulseRagdollMult);
		ReadFloatSetting(mcm, "HitImpulse", "fHitImpulseKillMult", fHitImpulseKillMult);

		ReadFloatSetting(mcm, "HitImpulse", "fHitImpulseBaseStrength", fHitImpulseBaseStrength);
		ReadFloatSetting(mcm, "HitImpulse", "fHitImpulseProportionalStrength", fHitImpulseProportionalStrength);
		ReadFloatSetting(mcm, "HitImpulse", "fHitImpulseMassExponent", fHitImpulseMassExponent);
		ReadFloatSetting(mcm, "HitImpulse", "fHitImpulseMinStrength", fHitImpulseMinStrength);
		ReadFloatSetting(mcm, "HitImpulse", "fHitImpulseMaxStrength", fHitImpulseMaxStrength);
		ReadFloatSetting(mcm, "HitImpulse", "fHitImpulseMaxVelocity", fHitImpulseMaxVelocity);
		ReadFloatSetting(mcm, "HitImpulse", "fHitImpulseDownwardsMultiplier", fHitImpulseDownwardsMultiplier);
		ReadFloatSetting(mcm, "HitImpulse", "fHitImpulseDecayMult1", fHitImpulseDecayMult1);
		ReadFloatSetting(mcm, "HitImpulse", "fHitImpulseDecayMult2", fHitImpulseDecayMult2);
		ReadFloatSetting(mcm, "HitImpulse", "fHitImpulseDecayMult3", fHitImpulseDecayMult3);

		// Active Ragdoll
		ReadFloatSetting(mcm, "ActiveRagdoll", "fActiveRagdollStartDistance", fActiveRagdollStartDistance);
		ReadFloatSetting(mcm, "ActiveRagdoll", "fActiveRagdollEndDistance", fActiveRagdollEndDistance);

		ReadBoolSetting(mcm, "ActiveRagdoll", "bUseRagdollCollisionWhenAllowed", bUseRagdollCollisionWhenAllowed);

		// Debug
		ReadBoolSetting(mcm, "Debug", "bDebug", bDebug);
		ReadBoolSetting(mcm, "Debug", "bDisplayWeaponCapsule", bDisplayWeaponCapsule);
		ReadBoolSetting(mcm, "Debug", "bDisplayHitNodeCollisions", bDisplayHitNodeCollisions);
		ReadBoolSetting(mcm, "Debug", "bDisplayHitLocations", bDisplayHitLocations);
		ReadBoolSetting(mcm, "Debug", "bDisplayIframeHits", bDisplayIframeHits);
		ReadBoolSetting(mcm, "Debug", "bDisplayRecoilCollisions", bDisplayRecoilCollisions);
		ReadBoolSetting(mcm, "Debug", "bDisplaySkeletonColliders", bDisplaySkeletonColliders);
		ReadUInt32Setting(mcm, "Debug", "uToggleKey", (uint32_t&)uToggleKey);
		ReadUInt32Setting(mcm, "Debug", "uReloadSettingsKey", (uint32_t&)uReloadSettingsKey);
	};

	logger::info("Reading MCM .ini...");

	readMCM(defaultSettingsPath);  // read the default ini first
	readMCM(mcmPath);

	logger::info("...success");

	DrawHandler::GetSingleton()->OnSettingsUpdated();
}

void Settings::OnPostLoadGame()
{
	UpdateGlobals();
}

void Settings::UpdateGlobals()
{
	if (glob_nemesis && glob_nemesis->value == 0) {
		auto playerCharacter = RE::PlayerCharacter::GetSingleton();
		RE::BSTSmartPointer<RE::BSAnimationGraphManager> animationGraphManagerPtr;
		playerCharacter->GetAnimationGraphManager(animationGraphManagerPtr);
		if (animationGraphManagerPtr) {
			RE::BShkbAnimationGraph* animationGraph = animationGraphManagerPtr->graphs[0].get();
			if (animationGraph) {
				bool dummy;
				glob_nemesis->value = animationGraph->GetGraphVariableBool("Collision_Installed", dummy);
			}
		}
	}
}

void Settings::ReadBoolSetting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, bool& a_setting)
{
	const char* bFound = nullptr;
	bFound = a_ini.GetValue(a_sectionName, a_settingName);
	if (bFound) {
		a_setting = a_ini.GetBoolValue(a_sectionName, a_settingName);
	}
}

void Settings::ReadFloatSetting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, float& a_setting)
{
	const char* bFound = nullptr;
	bFound = a_ini.GetValue(a_sectionName, a_settingName);
	if (bFound) {
		a_setting = static_cast<float>(a_ini.GetDoubleValue(a_sectionName, a_settingName));
	}
}

void Settings::ReadUInt32Setting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, uint32_t& a_setting)
{
	const char* bFound = nullptr;
	bFound = a_ini.GetValue(a_sectionName, a_settingName);
	if (bFound) {
		a_setting = static_cast<uint32_t>(a_ini.GetLongValue(a_sectionName, a_settingName));
	}
}
