#pragma once

#include <Simpleini.h>
#include <unordered_set>

struct CollisionDefinition
{
	CollisionDefinition() = default;

	CollisionDefinition(std::string_view a_nodeName,
		std::optional<uint8_t> a_ID = std::nullopt,
		bool a_bNoRecoil = false,
		bool a_bNoTrail = false,
		bool a_bWeaponTip = false,
		float a_damageMult = 1.f,		
		std::optional<float> a_duration = std::nullopt,
		std::optional<float> a_durationMult = std::nullopt,
		std::optional<float> a_delay = std::nullopt,
		std::optional<float> a_capsuleRadius = std::nullopt,
		std::optional<float> a_radiusMult = std::nullopt,
		std::optional<float> a_capsuleLength = std::nullopt,
		std::optional<float> a_lengthMult = std::nullopt,
		std::optional<RE::NiTransform> a_transform = std::nullopt) :
		nodeName(a_nodeName),
		ID(a_ID), bNoRecoil(a_bNoRecoil), bNoTrail(a_bNoTrail), bWeaponTip(a_bWeaponTip), damageMult(a_damageMult), duration(a_duration), durationMult(a_durationMult), delay(a_delay), capsuleRadius(a_capsuleRadius), radiusMult(a_radiusMult), capsuleLength(a_capsuleLength), lengthMult(a_lengthMult), transform(a_transform)
	{}

	std::string nodeName;
	std::optional<uint8_t> ID;
	bool bNoRecoil = false;
	bool bNoTrail = false;
	bool bWeaponTip = false;
	float damageMult = 1.f;
	std::optional<float> duration;
	std::optional<float> durationMult;
	std::optional<float> delay;
	std::optional<float> capsuleRadius;
	std::optional<float> radiusMult;
	std::optional<float> capsuleLength;
	std::optional<float> lengthMult;
	std::optional<RE::NiTransform> transform;
};

struct AttackDefinition
{
	AttackDefinition() = default;

	AttackDefinition(std::vector<CollisionDefinition> a_collisions) :
		collisions(a_collisions)
	{}

	std::vector<CollisionDefinition> collisions;
};

struct TrailDefinition
{
	TrailDefinition() = default;

	TrailDefinition(int32_t a_priority,
		std::optional<std::vector<std::string>> a_weaponNames,
		std::optional<std::vector<std::string>> a_weaponKeywords,
		std::optional<std::vector<std::string>> a_enchantmentNames,
		std::optional<std::vector<std::string>> a_effectNames,
		std::optional<std::vector<std::string>> a_effectKeywords,
		std::optional<std::vector<RE::TESEffectShader*>> a_effectShaders,
		std::optional<float> a_lifetimeMult,
		std::optional<RE::NiColorA> a_baseColorOverride,
		std::optional<float> a_baseColorScaleMult,
		std::optional<std::string> a_trailMeshOverride) :
		priority(a_priority), weaponNames(a_weaponNames), weaponKeywords(a_weaponKeywords), enchantmentNames(a_enchantmentNames), effectNames(a_effectNames), effectKeywords(a_effectKeywords), effectShaders(a_effectShaders), lifetimeMult(a_lifetimeMult), baseColorOverride(a_baseColorOverride), baseColorScaleMult(a_baseColorScaleMult), trailMeshOverride(a_trailMeshOverride)
	{}
	
	int32_t priority;
	std::optional<std::vector<std::string>> weaponNames;
	std::optional<std::vector<std::string>> weaponKeywords;
	std::optional<std::vector<std::string>> enchantmentNames;
	std::optional<std::vector<std::string>> effectNames;
	std::optional<std::vector<std::string>> effectKeywords;
	std::optional<std::vector<RE::TESEffectShader*>> effectShaders;
	std::optional<float> lifetimeMult;
	std::optional<RE::NiColorA> baseColorOverride;
	std::optional<float> baseColorScaleMult;
	std::optional<std::string> trailMeshOverride;
};

enum class SweepAttackMode : std::uint32_t
{
	kUnlimited = 0,
	kMaxTargets = 1,
	kDiminishingReturns = 2
};

struct Settings
{
	static void Initialize();
	static void ReadSettings();
	static void OnPostLoadGame();
	static void UpdateGlobals();

	static void ReadBoolSetting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, bool& a_setting);
	static void ReadFloatSetting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, float& a_setting);
	static void ReadUInt32Setting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, uint32_t& a_setting);

	// Attack Collisions
	static inline bool bAttackCollisionsEnabled = true;
	static inline bool bEnableJumpIframes = true;
	static inline bool bNoPlayerTeammateAttackCollision = true;
	static inline bool bNoNonHostileAttackCollision = true;
	static inline float fCombatStateLingerTime = 3.f;
	static inline bool bDisablePhysicalCollisionOnHit = true;
	static inline float fWeaponLengthMult = 1.2f;
	static inline float fWeaponCapsuleRadius = 12.f;
	static inline float fMinWeaponLength = 50.f;
	static inline float fDefaultCollisionLifetime = 0.3f;
	static inline float fDefaultCollisionLifetimePowerAttackMult = 1.8f;
	static inline float fHitSameRefCooldown = 0.30f;
	static inline float fFirstPersonAttackLengthOffset = 0.f;
	static inline float fPlayerAttackLengthMult = 1.f;
	static inline float fPlayerAttackRadiusMult = 1.5f;
	static inline float fMountedAttackLengthMult = 1.5f;
	static inline float fMountedAttackRadiusMult = 1.5f;
	static inline SweepAttackMode uSweepAttackMode = SweepAttackMode::kUnlimited;
	static inline uint32_t uMaxTargetsNoSweepAttack = 1;
	static inline uint32_t uMaxTargetsSweepAttack = 0;
	static inline float fSweepAttackDiminishingReturnsFactor = 0.5f;

	// Trails
	static inline bool bDisplayTrails = true;
	static inline float fTrailSegmentLifetime = 0.1f;
	static inline float fTrailFadeOutTime = 0.1f;
	static inline uint32_t uTrailSegmentsPerSecond = 120;
	static inline float fTrailDefaultBaseColorR = 0.530f;
	static inline float fTrailDefaultBaseColorG = 0.530f;
	static inline float fTrailDefaultBaseColorB = 0.530f;
	static inline float fTrailDefaultBaseColorA = 1.f;
	static inline float fTrailBaseColorScaleMult = 1.f;

	// Hitstop
	static inline bool bEnableHitstop = true;
	static inline bool bApplyHitstopToTarget = true;
	static inline float fHitstopDurationNPC = 0.07f;
	static inline float fHitstopDurationOther = 0.035f;
	static inline float fHitstopSlowdownTimeMultiplier = 0.1f;
	static inline float fHitstopDurationPowerAttackMultiplier = 1.3f;
	static inline float fHitstopDurationTwoHandedMultiplier = 1.2f;
	static inline float fHitstopDurationDiminishingReturnsFactor = 0.5f;
	static inline float fHitstopGroundFeetDistanceThreshold = 40.f;

	static inline bool bEnableHitstopCameraShake = true;
	static inline float fHitstopCameraShakeStrengthNPC = 6.f;
	static inline float fHitstopCameraShakeStrengthOther = 4.f;
	static inline float fHitstopCameraShakeDurationNPC = 0.3f;
	static inline float fHitstopCameraShakeDurationOther = 0.3f;
	static inline float fHitstopCameraShakeFrequency = 40.f;
	static inline float fHitstopCameraShakePowerAttackMultiplier = 1.3f;
	static inline float fHitstopCameraShakeTwoHandedMultiplier = 1.2f;
	static inline float fHitstopCameraShakeDurationDiminishingReturnsFactor = 0.5f;
	static inline float fHitstopCameraShakeGroundFeetDistanceThreshold = 40.f;

	// Recoil
	static inline bool bRecoilPlayer = true;
	static inline bool bRecoilNPC = true;
	static inline bool bRecoilPowerAttack = true;
	static inline bool bUseVanillaRecoil = false;
	static inline float fRecoilFirstPersonDistanceThreshold = 80.f;
	static inline float fRecoilThirdPersonDistanceThreshold = 20.f;
	static inline float fRecoilGroundFeetDistanceThreshold = 40.f;

	static inline bool bEnableRecoilCameraShake = true;
	static inline float fRecoilCameraShakeStrength = 10.f;
	static inline float fRecoilCameraShakeDuration = 0.4f;
	static inline float fRecoilCameraShakeFrequency = 40.f;

	// Hit Impulse
	static inline bool bApplyImpulseOnHit = true;
	static inline bool bApplyImpulseOnKill = true;
	static inline float fHitImpulseBaseMult = 1.f;
	static inline float fHitImpulseBlockMult = 0.6f;
	static inline float fHitImpulsePowerAttackMult = 2.f;
	static inline float fHitImpulseRagdollMult = 1.f;
	static inline float fHitImpulseKillMult = 1.f;

	static inline float fHitImpulseBaseStrength = 1.f;
	static inline float fHitImpulseProportionalStrength = -0.15f;
	static inline float fHitImpulseMassExponent = 0.5f;
	static inline float fHitImpulseMinStrength = 0.2f;
	static inline float fHitImpulseMaxStrength = 1.f;
	static inline float fHitImpulseMaxVelocity = 1500.f;  // skyrim units
	static inline float fHitImpulseDownwardsMultiplier = 0.5f;
	static inline float fHitImpulseDecayMult1 = 0.225f;
	static inline float fHitImpulseDecayMult2 = 0.125f;
	static inline float fHitImpulseDecayMult3 = 0.075f;

	// Active Ragdoll
	static inline float fActiveRagdollStartDistance = 3500.f;
	static inline float fActiveRagdollEndDistance = 4500.f;

	static inline bool bUseRagdollCollisionWhenAllowed = true;

	// Debug
	static inline bool bDebug = false;
	static inline bool bDisplayWeaponCapsule = false;
	static inline bool bDisplayHitNodeCollisions = false;
	static inline bool bDisplayHitLocations = false;
	static inline bool bDisplayIframeHits = false;
	static inline bool bDisplayRecoilCollisions = false;
	static inline bool bDisplaySkeletonColliders = false;
	static inline std::uint32_t uToggleKey = static_cast<std::uint32_t>(-1);
	static inline std::uint32_t uReloadSettingsKey = static_cast<std::uint32_t>(-1);

	// Internal settings
	static inline bool bDisableGravityForActiveRagdolls = true;
	static inline bool bForceAnimationUpdateForActiveActors = true;
	static inline float fRagdollBoneMaxLinearVelocity = 500.f;
	static inline float fRagdollBoneMaxAngularVelocity = 500.f;
	static inline float fWorldChangedWaitTime = 0.4f;
	static inline float fBlendInTime = 0.f;
	static inline float fBlendOutTime = 0.1f;
	static inline float fGetUpBlendTime = 0.2f;
	static inline bool bConvertHingeConstraintsToRagdollConstraints = true;
	static inline bool bLoosenRagdollContraintsToMatchPose = true;
	static inline bool bDoBlending = false;
	static inline bool bBlendWhenGettingUp = false;
	static inline bool bForceAnimPose = false;
	static inline bool bForceRagdollPose = false;
	static inline float fPoweredControllerOnFraction = 0.05f;
	static inline bool bEnableKeyframes = true;
	static inline float fBlendInKeyframeTime = 0.05f;
	static inline float fRagdollImpulseTime = 0.5f;
	static inline float fHierarchyGain = 0.6f;
	static inline float fVelocityGain = 0.6f;
	static inline float fPositionGain = 0.05f;
	static inline float fPoweredMaxForce = 500.f;
	static inline float fPoweredTau = 0.8f;
	static inline float fPoweredDamping = 1.0f;
	static inline float fPoweredProportionalRecoveryVelocity = 5.f;
	static inline float fPoweredConstantRecoveryVelocity = 0.2f;
	static inline bool bCopyFootIkToPoseTrack = true;
	static inline bool bDoWarp = true;
	static inline float fMaxAllowedDistBeforeWarp = 3.f;
	static inline float fHitImpulseFeetDistanceThreshold = 20.f;
	static inline bool bEnableWaterSplashes = false;
	static inline uint32_t iWaterSplashCooldownMs = 50;

	// Non-MCM
	static inline bool bDisableMod = false;

	static inline uint32_t iPrecisionLayerIndex = 56;
	//static inline uint64_t iPrecisionLayerBitfield = 0x53343561B7FFF;  // same as L_WEAPON layer
	static inline uint64_t iPrecisionLayerBitfield = 0x1053343561B7FFF;  // same as L_WEAPON layer, but + self-collision (layer 56)

	static inline std::unordered_map<RE::BGSBodyPartData*, std::unordered_map<std::string, AttackDefinition>> attackRaceDefinitions;
	static inline std::unordered_map<std::string, std::unordered_map<std::string, AttackDefinition>> attackAnimationDefinitions;
	static inline std::vector<TrailDefinition> trailDefinitionsAny;
	static inline std::vector<TrailDefinition> trailDefinitionsAll;
	static inline std::vector<std::pair<std::string, std::string>> attackEventPairs;
	
	static inline std::unordered_set<RE::BGSMaterialType*> recoilMaterials;
	static inline RE::BGSBodyPartData* defaultBodyPartData;
	static inline std::string attackTrailMeshPath = "Effects/WeaponTrails/AttackTrail.nif";
	static inline RE::BSFixedString recoilEvent = "Collision_Recoil";
	static inline RE::BSFixedString firstPersonRecoilEvent = "recoilStart";
	static inline RE::BSFixedString vanillaRecoilEvent = "recoilLargeStart";
	static inline RE::BSFixedString jumpIframeNode = "NPC Spine2 [Spn2]";

	static inline RE::TESGlobal* glob_nemesis = nullptr;
};


