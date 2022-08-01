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

	const auto readToml = [&](std::filesystem::path path) {
		logger::info("  Reading {}...", path.string());
		try {
			const auto tbl = toml::parse_file(path.c_str());
			auto& attackDefinitionsArr = *tbl.get_as<toml::array>("AttackDefinitions");
			for (auto&& elem : attackDefinitionsArr) {
				auto& definitionsTbl = *elem.as_table();
				auto formIDs = definitionsTbl["BodyPartDataFormIDs"].as_array();
				for (auto& formIDEntry : *formIDs) {
					auto formID = formIDEntry.value<uint32_t>();
					auto pluginName = definitionsTbl["Plugin"].value<std::string_view>();
					auto bodyPartData = dataHandler->LookupForm<RE::BGSBodyPartData>(*formID, *pluginName);
					if (bodyPartData) {
						auto& attackDefs = attackDefinitions[bodyPartData];

						auto attacksArr = definitionsTbl["Attacks"].as_array();
						for (auto&& attackEntry : *attacksArr) {
							// read attack
							auto& attackTbl = *attackEntry.as_table();

							// event name
							auto eventNames = attackTbl["EventNames"].as_array();
							for (auto& eventNameEntry : *eventNames) {
								auto eventName = eventNameEntry.value<std::string_view>();

								std::vector<CollisionDefinition> collisionDefs{};

								// read collision
								auto collisionArr = attackTbl["Collisions"].as_array();
								for (auto&& collisionEntry : *collisionArr) {
									auto& collisionTbl = *collisionEntry.as_table();

									// node name
									auto nodeName = collisionTbl["NodeName"].value<std::string_view>();

									auto ID = collisionTbl["ID"].value<uint8_t>();

									// no recoil
									bool bNoRecoil = false;
									auto noRecoilVal = collisionTbl["NoRecoil"].value<bool>();
									if (noRecoilVal) {
										bNoRecoil = *noRecoilVal;
									}

									// no trail
									bool bNoTrail = false;
									auto noTrailVal = collisionTbl["NoTrail"].value<bool>();
									if (noTrailVal) {
										bNoTrail = *noTrailVal;
									}

									// weapon tip
									bool bWeaponTip = false;
									auto weaponTipVal = collisionTbl["WeaponTip"].value<bool>();
									if (weaponTipVal) {
										bWeaponTip = *weaponTipVal;
									}

									// damage mult
									float damageMult = 1.f;
									auto damageMultVal = collisionTbl["DamageMult"].value<float>();
									if (damageMultVal) {
										damageMult = *damageMultVal;
									}

									// duration
									float duration = 0.f;
									auto durationVal = collisionTbl["Duration"].value<float>();
									if (durationVal) {
										duration = *durationVal;
									}

									// radius
									auto radius = collisionTbl["Radius"].value<float>();

									// length
									auto length = collisionTbl["Length"].value<float>();

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

										auto rotationTbl = collisionTbl["Rotation"].as_table();
										auto translationTbl = collisionTbl["Translation"].as_table();
										auto scaleVal = collisionTbl["Scale"].value<float>();

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

									collisionDefs.emplace_back(*nodeName, ID, bNoRecoil, bNoTrail, bWeaponTip, damageMult, duration, radius, length, transform);
								}

								attackDefs.emplace(*eventName, AttackDefinition(collisionDefs));
							}
						}
					}
				}
			}

			auto& attackEventPairArr = *tbl.get_as<toml::array>("AttackEventPair");
			for (auto&& elem : attackEventPairArr) {
				auto& attackEventPairTbl = *elem.as_table();
				auto rightEvent = attackEventPairTbl["RightEvent"].value<std::string_view>();
				auto leftEvent = attackEventPairTbl["LeftEvent"].value<std::string_view>();
				if (rightEvent && leftEvent) {
					attackEventPairs.emplace_back(*rightEvent, *leftEvent);
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

	attackDefinitions.clear();

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

	logger::info("...success");

	const auto readMCM = [&](std::filesystem::path path) {
		CSimpleIniA mcm;
		mcm.SetUnicode();

		mcm.LoadFile(path.string().c_str());

		// Attack Collisions
		ReadBoolSetting(mcm, "AttackCollisions", "bAttackCollisionsEnabled", bAttackCollisionsEnabled);
		ReadBoolSetting(mcm, "AttackCollisions", "bUseWeaponReach", bUseWeaponReach);
		ReadBoolSetting(mcm, "AttackCollisions", "bEnableJumpIframes", bEnableJumpIframes);
		ReadBoolSetting(mcm, "AttackCollisions", "bNoPlayerTeammateAttackCollision", bNoPlayerTeammateAttackCollision);
		ReadBoolSetting(mcm, "AttackCollisions", "bNoNonHostileAttackCollision", bNoNonHostileAttackCollision);
		ReadBoolSetting(mcm, "AttackCollisions", "bDisablePhysicalCollisionOnHit", bDisablePhysicalCollisionOnHit);
		ReadFloatSetting(mcm, "AttackCollisions", "fWeaponReachMult", fWeaponReachMult);
		ReadFloatSetting(mcm, "AttackCollisions", "fWeaponLengthMult", fWeaponLengthMult);
		ReadFloatSetting(mcm, "AttackCollisions", "fWeaponCapsuleRadius", fWeaponCapsuleRadius);
		ReadFloatSetting(mcm, "AttackCollisions", "fDefaultCollisionLifetime", fDefaultCollisionLifetime);
		ReadFloatSetting(mcm, "AttackCollisions", "fDefaultCollisionLifetimePowerAttackMult", fDefaultCollisionLifetimePowerAttackMult);
		ReadFloatSetting(mcm, "AttackCollisions", "fHitSameRefCooldown", fHitSameRefCooldown);
		ReadFloatSetting(mcm, "AttackCollisions", "fFirstPersonPlayerWeaponReachMult", fFirstPersonPlayerWeaponReachMult);
		ReadFloatSetting(mcm, "AttackCollisions", "fFirstPersonPlayerCapsuleRadiusMult", fFirstPersonPlayerCapsuleRadiusMult);
		ReadFloatSetting(mcm, "AttackCollisions", "fThirdPersonPlayerWeaponReachMult", fThirdPersonPlayerWeaponReachMult);
		ReadFloatSetting(mcm, "AttackCollisions", "fThirdPersonPlayerCapsuleRadiusMult", fThirdPersonPlayerCapsuleRadiusMult);
		ReadFloatSetting(mcm, "AttackCollisions", "fMountedWeaponReachMult", fMountedWeaponReachMult);
		ReadFloatSetting(mcm, "AttackCollisions", "fMountedCapsuleRadiusMult", fMountedCapsuleRadiusMult);

		// Trails
		ReadBoolSetting(mcm, "Trails", "bDisplayTrails", bDisplayTrails);
		ReadBoolSetting(mcm, "Trails", "bTrailUseWeaponWorldBound", bTrailUseWeaponWorldBound);
		ReadFloatSetting(mcm, "Trails", "fTrailSegmentLifetime", fTrailSegmentLifetime);
		ReadUInt32Setting(mcm, "Trails", "uTrailSegmentsPerSecond", uTrailSegmentsPerSecond);

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
