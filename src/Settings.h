#pragma once

#include <Simpleini.h>
#include <unordered_set>

struct TrailOverride
{
	TrailOverride() = default;

	TrailOverride(std::optional<float> a_lifetimeMult,
		std::optional<RE::NiColorA> a_baseColorOverride,
		std::optional<float> a_baseColorScaleMult,
		std::optional<std::string> a_meshOverride) :
		lifetimeMult(a_lifetimeMult),
		baseColorOverride(a_baseColorOverride), baseColorScaleMult(a_baseColorScaleMult), meshOverride(a_meshOverride)
	{}

	std::optional<float> lifetimeMult;
	std::optional<RE::NiColorA> baseColorOverride;
	std::optional<float> baseColorScaleMult;
	std::optional<std::string> meshOverride;
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
		TrailOverride& a_trailOverride) :
		priority(a_priority),
		weaponNames(a_weaponNames), weaponKeywords(a_weaponKeywords), enchantmentNames(a_enchantmentNames), effectNames(a_effectNames), effectKeywords(a_effectKeywords), effectShaders(a_effectShaders), trailOverride(a_trailOverride)
	{}

	int32_t priority;
	std::optional<std::vector<std::string>> weaponNames;
	std::optional<std::vector<std::string>> weaponKeywords;
	std::optional<std::vector<std::string>> enchantmentNames;
	std::optional<std::vector<std::string>> effectNames;
	std::optional<std::vector<std::string>> effectKeywords;
	std::optional<std::vector<RE::TESEffectShader*>> effectShaders;
	TrailOverride trailOverride;
};

struct CollisionDefinition
{
	CollisionDefinition() = default;

	CollisionDefinition(std::string_view a_nodeName,
		std::optional<uint8_t> a_ID = std::nullopt,
		bool a_bNoRecoil = false,
		bool a_bNoTrail = false,
		bool a_bTrailUseTrueLength = false,
		bool a_bWeaponTip = false,
		float a_damageMult = 1.f,
		std::optional<float> a_duration = std::nullopt,
		std::optional<float> a_durationMult = std::nullopt,
		std::optional<float> a_delay = std::nullopt,
		std::optional<float> a_capsuleRadius = std::nullopt,
		std::optional<float> a_radiusMult = std::nullopt,
		std::optional<float> a_capsuleLength = std::nullopt,
		std::optional<float> a_lengthMult = std::nullopt,
		std::optional<RE::NiTransform> a_transform = std::nullopt,
		std::optional<RE::NiPoint3> a_groundShake = std::nullopt,
		std::optional<TrailOverride> a_trailOverride = std::nullopt) :
		nodeName(a_nodeName),
		ID(a_ID), bNoRecoil(a_bNoRecoil), bNoTrail(a_bNoTrail), bTrailUseTrueLength(a_bTrailUseTrueLength), bWeaponTip(a_bWeaponTip), damageMult(a_damageMult), duration(a_duration), durationMult(a_durationMult), delay(a_delay), capsuleRadius(a_capsuleRadius), radiusMult(a_radiusMult), capsuleLength(a_capsuleLength), lengthMult(a_lengthMult), transform(a_transform), groundShake(a_groundShake), trailOverride(a_trailOverride)
	{}

	std::string nodeName;
	std::optional<uint8_t> ID;
	bool bNoRecoil = false;
	bool bNoTrail = false;
	bool bTrailUseTrueLength = false;
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
	std::optional<RE::NiPoint3> groundShake;
	std::optional<TrailOverride> trailOverride;
};

struct AttackDefinition
{
	enum class SwingEvent : uint8_t
	{
		kWeaponSwing = 0,
		kPreHitFrame = 1,
		kCastOKStart = 2,
		kCastOKStop = 3
	};
	AttackDefinition() = default;

	AttackDefinition(std::vector<CollisionDefinition> a_collisions, SwingEvent a_swingEvent) :
		collisions(a_collisions), swingEvent(a_swingEvent)
	{}

	std::vector<CollisionDefinition> collisions;
	SwingEvent swingEvent = SwingEvent::kWeaponSwing;
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
	static inline float fMinWeaponLength = 40.f;
	static inline float fDefaultCollisionLifetime = 0.3f;
	static inline float fDefaultCollisionLifetimePowerAttackMult = 1.8f;
	static inline float fHitSameRefCooldown = 0.30f;
	static inline float fHitSameMaterialCooldown = 0.30f;
	static inline float fFirstPersonAttackLengthOffset = 0.f;
	static inline float fPlayerAttackLengthMult = 1.f;
	static inline float fPlayerAttackRadiusMult = 1.5f;
	static inline float fMountedAttackLengthMult = 1.5f;
	static inline float fMountedAttackRadiusMult = 1.5f;
	static inline SweepAttackMode uSweepAttackMode = SweepAttackMode::kUnlimited;
	static inline uint32_t uMaxTargetsNoSweepAttack = 1;
	static inline uint32_t uMaxTargetsSweepAttack = 0;
	static inline float fSweepAttackDiminishingReturnsFactor = 0.5f;
	static inline float fGroundFeetDistanceThreshold = 30.f;

	// Trails
	static inline bool bDisplayTrails = true;
	static inline bool bTrailUseAttackCollisionLength = false;
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

	static inline bool bEnableHitstopCameraShake = true;
	static inline float fHitstopCameraShakeStrengthNPC = 6.f;
	static inline float fHitstopCameraShakeStrengthOther = 4.f;
	static inline float fHitstopCameraShakeDurationNPC = 0.3f;
	static inline float fHitstopCameraShakeDurationOther = 0.3f;
	static inline float fHitstopCameraShakeFrequency = 40.f;
	static inline float fHitstopCameraShakePowerAttackMultiplier = 1.3f;
	static inline float fHitstopCameraShakeTwoHandedMultiplier = 1.2f;
	static inline float fHitstopCameraShakeDurationDiminishingReturnsFactor = 0.5f;

	// Recoil
	static inline bool bRecoilPlayer = true;
	static inline bool bRecoilNPC = true;
	static inline bool bRecoilPowerAttack = true;
	static inline bool bUseVanillaRecoil = false;
	static inline bool bRemoveRecoilOnHitframe = true;
	static inline float fRecoilCollisionLength = 25.f;

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

	// Miscellaneous
	static inline float fActiveActorDistance = 4000.f;
	static inline bool bHookAIWeaponReach = true;
	static inline float fAIWeaponReachOffset = 0.f;
	static inline bool bDisableCharacterBumper = true;
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
	static inline float fBlendInTime = 0.05f;
	static inline float fBlendOutTime = 0.05f;
	static inline float fGetUpBlendTime = 0.2f;
	static inline bool bFadeInComputedWorldFromModel = true;
	static inline bool bFadeOutComputedWorldFromModel = true;
	static inline float fComputeWorldFromModelFadeInTime = 0.5f;
	static inline float fComputeWorldFromModelFadeOutTime = 0.3f;
	static inline bool bConvertHingeConstraintsToRagdollConstraints = true;
	static inline bool bLoosenRagdollContraintsToMatchPose = true;
	static inline bool bLoosenRagdollContraintPivots = true;
	static inline bool bDoBlending = true;
	static inline bool bBlendWhenGettingUp = false;
	static inline bool bForceAnimPose = false;
	static inline bool bForceRagdollPose = false;
	static inline float fPoweredControllerOnFraction = 0.05f;
	static inline bool bEnableKeyframes = false;
	static inline float fBlendInKeyframeTime = 0.05f;
	static inline float fAddRagdollSettleTime = 0.05f;
	static inline bool bKnockDownAfterBuggedGetUp = true;
	static inline float fRagdollImpulseTime = 0.75f;
	static inline float fHierarchyGain = 0.6f;
	static inline float fVelocityGain = 0.6f;
	static inline float fPositionGain = 0.05f;
	static inline float fPoweredMaxForce = 500.f;
	static inline float fPoweredTau = 0.8f;
	static inline float fPoweredDamping = 1.0f;
	static inline float fPoweredProportionalRecoveryVelocity = 5.f;
	static inline float fPoweredConstantRecoveryVelocity = 0.2f;
	static inline bool bCopyFootIkToPoseTrack = true;
	static inline bool bDoWarp = false;
	static inline bool bDisableWarpWhenGettingUp = true;
	static inline float fMaxAllowedDistBeforeWarp = 3.f;
	static inline float fHitImpulseFeetDistanceThreshold = 20.f;
	static inline bool bEnableWaterSplashes = true;
	static inline uint32_t iWaterSplashCooldownMs = 50;
	static inline float fReceivedHitstopCooldown = 0.7f;
	static inline float fDriveToPoseHitstopMultiplier = 0.3f;

	// Non-MCM
	static inline bool bDisableMod = false;

	static inline uint32_t iPrecisionAttackLayerIndex = 56;
	static inline uint32_t iPrecisionBodyLayerIndex = 57;
	static inline uint32_t iPrecisionRecoilLayerIndex = 58;
	static inline RE::BSFixedString sPrecisionAttackLayerName = "L_PRECISION_ATTACK";
	static inline RE::BSFixedString sPrecisionBodyLayerName = "L_PRECISION_BODY";
	static inline RE::BSFixedString sPrecisionRecoilLayerName = "L_PRECISION_RECOIL";
	static inline uint64_t iPrecisionAttackLayerBitfield = 0x7053341561B7EFF;  // same as L_WEAPON layer, but -biped_no_cc (layer 33) +self-collision (layer 56) +precision body (layer 57) +precision body_no_cc (layer 58)
	static inline uint64_t iPrecisionBodyLayerBitfield = 0x100000040000000;    // only collide with precision attack (layer 56) and charcontroller (layer 30)
	static inline uint64_t iPrecisionRecoilLayerBitfield = 0x141A661F;         // only relevant layers

	static inline uint64_t iBipedLayerBitfield = 0x407BC01C037A8F;  // same as L_BIPED layer

	static inline std::unordered_map<RE::BGSBodyPartData*, std::unordered_map<std::string, AttackDefinition>> attackRaceDefinitions;
	static inline std::unordered_map<RE::BGSBodyPartData*, std::unordered_map<std::string, AttackDefinition>> attackRaceDefinitionsPreHitFrame;
	static inline std::unordered_map<RE::BGSBodyPartData*, std::unordered_map<std::string, AttackDefinition>> attackRaceDefinitionsCastOKStart;
	static inline std::unordered_map<RE::BGSBodyPartData*, std::unordered_map<std::string, AttackDefinition>> attackRaceDefinitionsCastOKStop;
	static inline std::unordered_map<std::string, std::unordered_map<std::string, AttackDefinition>> attackAnimationDefinitions;
	static inline std::unordered_map<std::string, std::unordered_map<std::string, AttackDefinition>> attackAnimationDefinitionsPreHitFrame;
	static inline std::unordered_map<std::string, std::unordered_map<std::string, AttackDefinition>> attackAnimationDefinitionsCastOKStart;
	static inline std::unordered_map<std::string, std::unordered_map<std::string, AttackDefinition>> attackAnimationDefinitionsCastOKStop;
	static inline std::vector<TrailDefinition> trailDefinitionsAny;
	static inline std::vector<TrailDefinition> trailDefinitionsAll;
	static inline std::vector<std::pair<std::string, std::string>> attackEventPairs;
	static inline std::unordered_map<RE::TESObjectWEAP*, float> weaponLengthOverrides;
	static inline std::unordered_map<RE::TESObjectWEAP*, float> weaponRadiusOverrides;

	static inline std::unordered_set<RE::BGSMaterialType*> recoilMaterials;
	static inline RE::BGSBodyPartData* defaultBodyPartData;
	static inline std::string attackTrailMeshPath = "Effects/WeaponTrails/AttackTrail.nif";
	static inline RE::BSFixedString recoilEvent = "Collision_Recoil";
	static inline RE::BSFixedString firstPersonRecoilEvent = "recoilStart";
	static inline RE::BSFixedString vanillaRecoilEvent = "recoilLargeStart";
	static inline RE::BSFixedString jumpIframeNode = "NPC Spine2 [Spn2]";

	static inline RE::BSFixedString defaultMaleBehaviorGraph = "DefaultMale";
	static inline RE::BSFixedString defaultFemaleBehaviorGraph = "DefaultFemale";

	static inline std::string bloodTrailMeshPath = "Effects/WeaponTrails/BloodHitHuman.nif";

	static inline float fActiveDistanceHysteresis = 0.1f;
	static inline float fWeaponLengthAICachedRadiusMult = 1.f;
	static inline float fWeaponLengthUnarmedOffset = -10.f;

	static inline float fMinWeaponRadius = 1.f;

	static inline float defaultMeshLengthOneHandSword = 70.f;
	static inline float defaultMeshLengthOneHandDagger = 32.4f;
	static inline float defaultMeshLengthOneHandAxe = 43.f;
	static inline float defaultMeshLengthOneHandMace = 48.3f;
	static inline float defaultMeshLengthTwoHandSword = 95.f;
	static inline float defaultMeshLengthTwoHandAxe = 56.5f;

	static inline float cameraShakeRadiusSquared = 2000000.f;
	static inline RE::NiPoint3 cameraShakeAxis = { 1.f, 0.f, 0.f };

	static inline RE::TESGlobal* glob_nemesis = nullptr;
	static inline RE::TESGlobal* glob_IFPVFirstPerson = nullptr;
};
