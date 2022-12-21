#pragma once

#include "Havok/Havok.h"

static float* g_deltaTime = (float*)RELOCATION_ID(523660, 410199).address();                             // 2F6B948, 30064C8
static float* g_deltaTimeRealTime = (float*)RELOCATION_ID(523661, 410200).address();                     // 2F6B94C, 30064CC
static uint32_t* g_durationOfApplicationRunTimeMS = (uint32_t*)RELOCATION_ID(523662, 410201).address();  // 2F6B950, 30064D0
static float* g_globalTimeMultiplier = (float*)RELOCATION_ID(511882, 388442).address();                  // 1E05B28, 1E99FD0
static uintptr_t g_worldToCamMatrix = RELOCATION_ID(519579, 406126).address();                           // 2F4C910, 2FE75F0
static RE::NiRect<float>* g_viewPort = (RE::NiRect<float>*)RELOCATION_ID(519618, 406160).address();      // 2F4DED0, 2FE8B98

static float* g_worldScale = (float*)RELOCATION_ID(231896, 188105).address();
static float* g_worldScaleInverse = (float*)RELOCATION_ID(230692, 187407).address();
static DWORD* g_dwTlsIndex = (DWORD*)RELOCATION_ID(520865, 407383).address();  // 2F50B74, 2FEB6F4

static bool* g_bAddBipedWhenKeyframed = (bool*)RELOCATION_ID(500874, 358839).address();          // 1DB1818, 1E45818
static bool* g_bAddBipedWhenKeyframedIndirect = (bool*)RELOCATION_ID(512240, 389098).address();  // 1E08301, 1E9D189

static void* g_unkCloneValue1 = (void*)RELOCATION_ID(501133, 359452).address();          // 1DB348C, 1E47508
static void* g_unkCloneValue2 = (void*)RELOCATION_ID(501132, 359451).address();          // 1DB3488, 1E47504
static uint32_t* g_unkCloneValue3 = (uint32_t*)RELOCATION_ID(523909, 410490).address();  // 3012500, 30AD080
static char* g_unkCloneValue4 = (char*)RELOCATION_ID(511989, 388581).address();

static RE::BSRenderManager* g_renderManager = (RE::BSRenderManager*)RELOCATION_ID(524907, 411393).address();

static RE::BGSAttackData** g_defaultAttackData = (RE::BGSAttackData**)RELOCATION_ID(515609, 401780).address();  // 2F07FC0, 2FA26D0

static float* g_currentCameraShakeStrength = (float*)RELOCATION_ID(516444, 402622).address();  // 2F251E0, 2FBF5D0
static float* g_currentCameraShakeTimer = (float*)RELOCATION_ID(516445, 402623).address();     // 2F251E4, 2FBF5D4

static uintptr_t g_cameraShakeMesh = RELOCATION_ID(516446, 402624).address();  // 2F251E8, 2FBF5D8

static RE::BIPED_OBJECT* g_weaponTypeToBipedObject = (RE::BIPED_OBJECT*)RELOCATION_ID(501493, 360260).address();  // 1DB5168, 1E491D8

static RE::TESObjectWEAP* g_unarmedWeapon = (RE::TESObjectWEAP*)RELOCATION_ID(514923, 401061).address();  // 2EFF868, 2F99F50

static void* g_actorMediator = (void*)RELOCATION_ID(517059, 403567).address();  // 2F271C0, 2FC1C90

static void* g_142EFF990 = (void*)RELOCATION_ID(514960, 401100).address();  // 2EFF990, 2F9A0A0

static void** g_142EC5C60 = (void**)RELOCATION_ID(514725, 400883).address();  // 2EC5C60, 2F603B0

inline volatile std::uint32_t* GetBhkSerializableCount()
{
	static REL::Relocation<volatile std::uint32_t*> bhkSerializableCount{ RELOCATION_ID(525155, 411632) };  // 302E1D0, 30C8448
	return bhkSerializableCount.get();
}

inline volatile std::uint32_t* GetBhkRagdollConstraintCount()
{
	static REL::Relocation<volatile std::uint32_t*> bhkRagdollConstraintCount{ RELOCATION_ID(525232, 411708) };  // 302F138, 30C9398
	return bhkRagdollConstraintCount.get();
}

typedef RE::TESObjectREFR*(__fastcall* tCalculateCurrentHitTargetForWeaponSwing)(RE::Actor* a_this);
static REL::Relocation<tCalculateCurrentHitTargetForWeaponSwing> CalculateCurrentHitTargetForWeaponSwing{ RELOCATION_ID(37674, 38628) };  // 629090, 64ECC0

typedef void*(__fastcall* tDealDamage)(RE::Actor* a_this, RE::Actor* a_target, RE::Projectile* a_sourceProjectile, bool a_bLeftHand);
static REL::Relocation<tDealDamage> DealDamage{ RELOCATION_ID(37673, 38627) };  // 628C20, 64E760

typedef void*(__fastcall* tRemoveMagicEffectsDueToAction)(RE::Actor* a_this, int32_t a2);
static REL::Relocation<tRemoveMagicEffectsDueToAction> RemoveMagicEffectsDueToAction{ RELOCATION_ID(37864, 38819) };  // 634B80, 65AC80

typedef RE::hkpAllCdPointCollector*(__fastcall* tGetAllCdPointCollector)(bool a1, bool a2);
static REL::Relocation<tGetAllCdPointCollector> GetAllCdPointCollector{ RELOCATION_ID(25397, 25925) };  // 39FA60, 3B63D0

typedef RE::BGSMaterialType*(__fastcall* tGetBGSMaterialType)(RE::MATERIAL_ID a_materialID);
static REL::Relocation<tGetBGSMaterialType> GetBGSMaterialType{ RELOCATION_ID(20529, 20968) };  // 2C73F0, 2DA4D0

typedef RE::BGSImpactData*(__fastcall* tGetImpactData)(RE::BGSImpactDataSet* a_impactDataSet, RE::BGSMaterialType* a_materialType);
static REL::Relocation<tGetImpactData> GetImpactData{ RELOCATION_ID(20408, 20860) };  // 2C3490, 2D6460

typedef void(__fastcall* tCombatMeleeAimController__sub_14075BF90)(class CombatMeleeAimController* a_this, uintptr_t a2);
static REL::Relocation<tCombatMeleeAimController__sub_14075BF90> CombatMeleeAimController__sub_14075BF90{ REL::ID(43155) };  // 75BF90

typedef void(__fastcall* tCombatTrackTargetAimController__sub_14075B800)(class CombatTrackTargetAimController* a_this);
static REL::Relocation<tCombatTrackTargetAimController__sub_14075B800> CombatTrackTargetAimController__sub_14075B800{ REL::ID(43148) };  // 75B800

typedef void(_fastcall* thkpEntity_Activate)(RE::hkpEntity* a_this);
static REL::Relocation<thkpEntity_Activate> hkpEntity_Activate{ RELOCATION_ID(60096, 60849) };  // A6C730, A90F40

struct PlayImpactSoundArguments
{
	RE::BGSImpactData* impactData;    // 00
	RE::NiPoint3* worldTranslate;     // 08
	RE::NiAVObject* particle3D;       // 10
	RE::BSSoundHandle* soundHandle1;  // 18
	RE::BSSoundHandle* soundHandle2;  // 20
	uint8_t unkFlags;                 // 28
	bool unk29;                       // 29
	bool unk2A;                       // 2A
	uint32_t pad2B;                   // 2B
	uintptr_t unkPtr;                 // 30
};
static_assert(sizeof(PlayImpactSoundArguments) == 0x38);

typedef void(__fastcall* tsub_1405A2930)(float a1, PlayImpactSoundArguments& a_arguments);
static REL::Relocation<tsub_1405A2930> sub_1405A2930{ REL::ID(35317) };  // 5A2930

typedef void (*tNiMatrixToNiQuaternion)(RE::NiQuaternion& quatOut, const RE::NiMatrix3& matIn);
static REL::Relocation<tNiMatrixToNiQuaternion> NiMatrixToNiQuaternion{ RELOCATION_ID(69467, 70844) };  // C6E2D0, C967D0

typedef RE::hkpWorldExtension* (*thkpWorld_findWorldExtension)(RE::hkpWorld* a_world, int32_t a_id);
static REL::Relocation<thkpWorld_findWorldExtension> hkpWorld_findWorldExtension{ RELOCATION_ID(60549, 61397) };  // A7AEF0, A9F700

typedef bool (*thkpCollisionCallbackUtil_requireCollisionCallbackUtil)(RE::hkpWorld* a_world);
static REL::Relocation<thkpCollisionCallbackUtil_requireCollisionCallbackUtil> hkpCollisionCallbackUtil_requireCollisionCallbackUtil{ RELOCATION_ID(60588, 61437) };  // A7DD00, AA2510

typedef bool (*thkpCollisionCallbackUtil_releaseCollisionCallbackUtil)(RE::hkpWorld* a_world);
static REL::Relocation<thkpCollisionCallbackUtil_releaseCollisionCallbackUtil> hkpCollisionCallbackUtil_releaseCollisionCallbackUtil{ RELOCATION_ID(61800, 62715) };  // AC6230, AEAA40

typedef void* (*thkpWorld_addContactListener)(RE::hkpWorld* a_world, RE::hkpContactListener* a_worldListener);
static REL::Relocation<thkpWorld_addContactListener> hkpWorld_addContactListener{ RELOCATION_ID(60543, 61383) };  // A7AB80, A9F390

//typedef void* (*thkpWorld_removeContactListener)(RE::hkpWorld* a_world, RE::hkpContactListener* a_worldListener);
//static REL::Relocation<thkpWorld_removeContactListener> hkpWorld_removeContactListener{ RELOCATION_ID(68974, 70327) };  // C59BD0, C814A0

typedef void* (*thkpWorld_addWorldPostSimulationListener)(RE::hkpWorld* a_world, RE::hkpWorldPostSimulationListener* a_worldListener);
static REL::Relocation<thkpWorld_addWorldPostSimulationListener> hkpWorld_addWorldPostSimulationListener{ RELOCATION_ID(60538, 61366) };  // A7A880, A9F090

typedef void* (*thkpWorld_removeWorldPostSimulationListener)(RE::hkpWorld* a_world, RE::hkpWorldPostSimulationListener* a_worldListener);
static REL::Relocation<thkpWorld_removeWorldPostSimulationListener> hkpWorld_removeWorldPostSimulationListener{ RELOCATION_ID(60539, 61367) };  // A7A8E0, A9F0F0

typedef void (*tBSAnimationGraphManager_HasRagdollInterface)(RE::BSAnimationGraphManager* a_this, bool* a_out);
static REL::Relocation<tBSAnimationGraphManager_HasRagdollInterface> BSAnimationGraphManager_HasRagdollInterface{ RELOCATION_ID(16163, 34753) };  // 1F99E0, 577C40

typedef RE::hkQsTransform* (*thkbCharacter_getPoseLocal)(RE::hkbCharacter* a_this);
static REL::Relocation<thkbCharacter_getPoseLocal> hkbCharacter_getPoseLocal{ RELOCATION_ID(57885, 58458) };  // 9FBD60, A20570

//typedef void (*tBSAnimationGraphManager_AddRagdollToWorld)(RE::BSAnimationGraphManager* a_this, bool* a2);
//static REL::Relocation<tBSAnimationGraphManager_AddRagdollToWorld> BSAnimationGraphManager_AddRagdollToWorld{ RELOCATION_ID(36023, 0) };  // 5C9560, inlined in AE! check calls of sub_14050C120(AE)

//typedef void (*tBSAnimationGraphManager_RemoveRagdollFromWorld)(RE::BSAnimationGraphManager* a_this, bool* a2);
//static REL::Relocation<tBSAnimationGraphManager_RemoveRagdollFromWorld> BSAnimationGraphManager_RemoveRagdollFromWorld{ RELOCATION_ID(37074, 0) };  // 6115D0, inlined in AE! check calls of sub_14050C0D0(AE)

//typedef void (*tBSAnimationGraphManager_DisableOrEnableSyncOnUpdate)(RE::BSAnimationGraphManager* a_this, bool* a2);
//static REL::Relocation<tBSAnimationGraphManager_DisableOrEnableSyncOnUpdate> BSAnimationGraphManager_DisableOrEnableSyncOnUpdate{ RELOCATION_ID(37075, 0) };  // 6116D0, inlined in AE! check calls of sub_14050C230(AE)

//typedef void (*tBSAnimationGraphManager_SetRagdollConstraintsFromBhkConstraints)(RE::BSAnimationGraphManager* a_this, bool* a2);
//static REL::Relocation<tBSAnimationGraphManager_SetRagdollConstraintsFromBhkConstraints> BSAnimationGraphManager_SetRagdollConstraintsFromBhkConstraints{ RELOCATION_ID(37077, 0) };  // 6118D0, 0

typedef RE::hkVector4& (*thkbBlendPoses)(uint32_t numData, const RE::hkQsTransform* src, const RE::hkQsTransform* dst, float amount, RE::hkQsTransform* out);
static REL::Relocation<thkbBlendPoses> hkbBlendPoses{ RELOCATION_ID(63192, 64112) };  // B12FA0, B38110

typedef RE::hkaRagdollInstance* (*thkbRagdollDriver_getRagdoll)(RE::hkbRagdollDriver* a_this);
static REL::Relocation<thkbRagdollDriver_getRagdoll> hkbRagdollDriver_getRagdoll{ RELOCATION_ID(57718, 58286) };  // 9EAD10, A0F520

typedef void (*thkbRagdollDriver_reset)(RE::hkbRagdollDriver* a_this);
static REL::Relocation<thkbRagdollDriver_reset> hkbRagdollDriver_reset{ RELOCATION_ID(57726, 58294) };  // 9ECE70, A11680

typedef RE::NiAVObject* (*tGetNiObjectFromCollidable)(const RE::hkpCollidable* a_collidable);
static REL::Relocation<tGetNiObjectFromCollidable> GetNiObjectFromCollidable{ RELOCATION_ID(76160, 77988) };  // DAD060, DECF20

typedef void (*tbhkRigidBody_setMotionType)(RE::bhkRigidBody* a_this, RE::hkpMotion::MotionType a_newState, RE::hkpEntityActivation a_preferredActivationState, RE::hkpUpdateCollisionFilterOnEntityMode a_collisionFilterUpdateMode);
static REL::Relocation<tbhkRigidBody_setMotionType> bhkRigidBody_setMotionType{ RELOCATION_ID(76247, 78077) };  // DB30C0, DF37F0

typedef void (*thkpRigidBody_setMotionType)(RE::hkpRigidBody* a_this, RE::hkpMotion::MotionType a_newState, RE::hkpEntityActivation a_preferredActivationState, RE::hkpUpdateCollisionFilterOnEntityMode a_collisionFilterUpdateMode);
static REL::Relocation<thkpRigidBody_setMotionType> hkpRigidBody_setMotionType{ RELOCATION_ID(60153, 60908) };  // A6EB30, A93340

typedef bool (*tbhkRefObject_ctor)(RE::bhkRefObject* a_this);
static REL::Relocation<tbhkRefObject_ctor> bhkRefObject_ctor{ RELOCATION_ID(76771, 78647) };  // DDC1E0, E1D5D0

typedef void (*tbhkCapsuleShape_ctor)(RE::bhkCapsuleShape* a_this);
static REL::Relocation<tbhkCapsuleShape_ctor> bhkCapsuleShape_ctor{ RELOCATION_ID(76929, 78804) };  // DE2520, E24030

typedef void (*tbhkCapsuleShape_SetSize)(RE::bhkCapsuleShape* a_this, RE::hkVector4& a_vertexA, RE::hkVector4& a_vertexB, float a_radius);
static REL::Relocation<tbhkCapsuleShape_SetSize> bhkCapsuleShape_SetSize{ RELOCATION_ID(76925, 78800) };  // DE20B0, E23BB0

typedef void (*tbhkRigidBodyCinfo_ctor)(RE::bhkRigidBodyCinfo* a_this);
static REL::Relocation<tbhkRigidBodyCinfo_ctor> bhkRigidBodyCinfo_ctor{ RELOCATION_ID(76222, 78042) };  // DB1190, DF0ED0

typedef void (*tbhkRigidBody_ctor)(RE::bhkRigidBody* a_this);
static REL::Relocation<tbhkRigidBody_ctor> bhkRigidBody_ctor{ RELOCATION_ID(76315, 78147) };  // DB6EA0, DF7670

typedef void (*tbhkRigidBody_ApplyCinfo)(RE::bhkRigidBody* a_this, RE::bhkRigidBodyCinfo* a_cInfo);
static REL::Relocation<tbhkRigidBody_ApplyCinfo> bhkRigidBody_ApplyCinfo{ RELOCATION_ID(76240, 78070) };  // DB2410, DF23E0

typedef void (*thkVector4_setTransformedPos)(RE::hkVector4& a_this, const RE::hkTransform& a_transform, const RE::hkVector4& a_pos);
static REL::Relocation<thkVector4_setTransformedPos> hkVector4_setTransformedPos{ RELOCATION_ID(56783, 57213) };  // 9CB230, 9EFA30

typedef void (*thkVector4_setTransformedInversePos)(RE::hkVector4& a_this, const RE::hkTransform& a_transform, const RE::hkVector4& a_pos);
static REL::Relocation<thkVector4_setTransformedInversePos> hkVector4_setTransformedInversePos{ RELOCATION_ID(56784, 57214) };  // 9CB270, 9EFA70

//typedef void (*tbhkRigidBody_Cinfo_ctor)(RE::bhkRigidBody* a_this, RE::bhkRigidBodyCinfo* a_cInfo);
//static REL::Relocation<tbhkRigidBody_Cinfo_ctor> bhkRigidBody_Cinfo_ctor{ RELOCATION_ID(19457, 0) };  // 29D510, 0

typedef void (*tbhkRigidBody_setActivated)(RE::bhkRigidBody* a_rigidBody, bool a_bActivate);
static REL::Relocation<tbhkRigidBody_setActivated> bhkRigidBody_setActivated{ RELOCATION_ID(76258, 78088) };  // DB3650, DF3E10

typedef void (*thkpWorld_AddEntity)(RE::hkpWorld* a_world, RE::hkpEntity* a_entity, RE::hkpEntityActivation a_initialActivationState);
static REL::Relocation<thkpWorld_AddEntity> hkpWorld_AddEntity{ RELOCATION_ID(60492, 61304) };  // A762B0, A9AAC0

typedef RE::hkpEntity* (*thkpWorld_RemoveEntity)(RE::hkpWorld* a_world, bool* a_ret, RE::hkpEntity* a_entity);
static REL::Relocation<thkpWorld_RemoveEntity> hkpWorld_RemoveEntity{ RELOCATION_ID(60493, 61305) };  // A76450, A9AC60

typedef bool (*thkRagdollConstraintCinfo_Func4)(RE::hkRagdollConstraintCinfo* a_this);
static REL::Relocation<thkRagdollConstraintCinfo_Func4> hkRagdollConstraintCinfo_Func4{ RELOCATION_ID(77921, 79795) };  // E0F960, E54550

typedef bool (*thkConstraintCinfo_setConstraintData)(RE::hkConstraintCinfo* a_this, RE::hkpConstraintData* a_data);
static REL::Relocation<thkConstraintCinfo_setConstraintData> hkConstraintCinfo_setConstraintData{ RELOCATION_ID(77111, 79011) };  // DE9720, E2C200

typedef void (*tbhkConstraint_Func46)(RE::bhkConstraint* a_this, RE::hkRefPtr<RE::hkpConstraintData> a_data);
static REL::Relocation<tbhkConstraint_Func46> bhkConstraint_Func46{ RELOCATION_ID(77136, 79036) };  // DEA5D0, E2D1B0

typedef float (*thkpRagdollConstraintData_setInBodySpace)(RE::hkpRagdollConstraintData* a_this,
	const RE::hkVector4& a_pivotA, const RE::hkVector4& a_pivotB,
	const RE::hkVector4& a_planeAxisA, const RE::hkVector4& a_planeAxisB,
	const RE::hkVector4& a_twistAxisA, const RE::hkVector4& a_twistAxisB);
static REL::Relocation<thkpRagdollConstraintData_setInBodySpace> hkpRagdollConstraintData_setInBodySpace{ RELOCATION_ID(60748, 61609) };  // A86CE0, AAB4F0

typedef RE::bhkRagdollConstraint* (*tbhkRagdollConstraint_ctor)();
static REL::Relocation<tbhkRagdollConstraint_ctor> bhkRagdollConstraint_ctor{ RELOCATION_ID(77928, 79810) };  // E0FF60, E54B40

typedef bool (*tbhkRagdollConstraint_ApplyCinfo)(RE::bhkRagdollConstraint* a_this, RE::hkRefPtr<RE::hkpConstraintData>& a_constraintData);
static REL::Relocation<tbhkRagdollConstraint_ApplyCinfo> bhkRagdollConstraint_ApplyCinfo{ RELOCATION_ID(77136, 79036) };  // DEA5D0, E2D1B0

//typedef bool (*tbhkRagdollConstraint_ctor)(RE::bhkRagdollConstraint* a_this, RE::hkRagdollConstraintCinfo* a_cInfo);
//static REL::Relocation<tbhkRagdollConstraint_ctor> bhkRagdollConstraint_ctor{ RELOCATION_ID(77367, 0) };  // DF56C0, 0

typedef bool (*thkpEaseConstraintsAction_ctor)(RE::hkpEaseConstraintsAction* a_this, const RE::hkArray<RE::hkpRigidBody*>& a_entities, uint64_t a_userData);
static REL::Relocation<thkpEaseConstraintsAction_ctor> hkpEaseConstraintsAction_ctor{ RELOCATION_ID(64149, 65180) };  // B64600, B89440

typedef bool (*thkpEaseConstraintsAction_loosenConstraints)(RE::hkpEaseConstraintsAction* a_this);
static REL::Relocation<thkpEaseConstraintsAction_loosenConstraints> hkpEaseConstraintsAction_loosenConstraints{ RELOCATION_ID(64150, 65181) };  // B646B0, B894F0

typedef bool (*thkpEaseConstraintsAction_restoreConstraints)(RE::hkpEaseConstraintsAction* a_this, float a_duration);
static REL::Relocation<thkpEaseConstraintsAction_restoreConstraints> hkpEaseConstraintsAction_restoreConstraints{ RELOCATION_ID(64151, 65182) };  // B646D0, B89510

typedef void (*thkpEntity_updateMovedBodyInfo)(RE::hkpEntity* a_this);
static REL::Relocation<thkpEntity_updateMovedBodyInfo> hkpEntity_updateMovedBodyInfo{ RELOCATION_ID(60162, 60918) };  // A6F410, A93C20

typedef void (*thkbRagdollDriver_mapHighResPoseLocalToLowResPoseLocal)(RE::hkbRagdollDriver* a_this, const RE::hkQsTransform* a_highResPoseLocal, RE::hkQsTransform* a_lowResPoseLocal);
static REL::Relocation<thkbRagdollDriver_mapHighResPoseLocalToLowResPoseLocal> hkbRagdollDriver_mapHighResPoseLocalToLowResPoseLocal{ RELOCATION_ID(57734, 58304) };  // 9ED3B0, A11BC0

typedef void (*thkbRagdollDriver_mapHighResPoseLocalToLowResPoseWorld)(RE::hkbRagdollDriver* a_this, const RE::hkQsTransform* a_highResPoseLocal, const RE::hkQsTransform& a_worldFromModel, RE::hkQsTransform* a_lowResPoseWorld);
static REL::Relocation<thkbRagdollDriver_mapHighResPoseLocalToLowResPoseWorld> hkbRagdollDriver_mapHighResPoseLocalToLowResPoseWorld{ RELOCATION_ID(57735, 58305) };  // 9ED570, A11D80

typedef void (*tCopyAndApplyScaleToPose)(bool a_scaleByHavokWorldScale, uint32_t a_numBones, RE::hkQsTransform* a_poseLowResLocal, RE::hkQsTransform* a_poseOut, float a_worldFromModelScale);
static REL::Relocation<tCopyAndApplyScaleToPose> CopyAndApplyScaleToPose{ RELOCATION_ID(57730, 58300) };  // 9ED070, A11880

typedef void (*tCopyAndPotentiallyApplyHavokScaleToTransform)(bool a_bScaleByHavokWorldScale, const RE::hkQsTransform* a_in, RE::hkQsTransform* a_out);
static REL::Relocation<tCopyAndPotentiallyApplyHavokScaleToTransform> CopyAndPotentiallyApplyHavokScaleToTransform{ RELOCATION_ID(57731, 58301) };  // 9ED0F0, A11900

typedef void (*thkbPoseLocalToPoseWorld)(int a_numBones, const int16_t* a_parentIndices, const RE::hkQsTransform& a_worldFromModel, RE::hkQsTransform* a_highResPoseLocal, RE::hkQsTransform* a_outLowResPoseWorld);
static REL::Relocation<thkbPoseLocalToPoseWorld> hkbPoseLocalToPoseWorld{ RELOCATION_ID(63189, 64109) };  // B129E0, B37B50

typedef void (*thkRotation_setFromQuat)(RE::hkRotation* a_this, const RE::hkQuaternion& a_quat);
static REL::Relocation<thkRotation_setFromQuat> hkRotation_setFromQuat{ RELOCATION_ID(56674, 57081) };  // 9C7FC0, 9EC7B0

typedef void (*thkpKeyFrameUtility_applyHardKeyFrame)(const RE::hkVector4& a_nextPosition, const RE::hkQuaternion& a_nextOrientation, float a_invDeltaTime, RE::hkpRigidBody* a_body);
static REL::Relocation<thkpKeyFrameUtility_applyHardKeyFrame> hkpKeyFrameUtility_applyHardKeyFrame{ RELOCATION_ID(61579, 62478) };                // ABC3D0, AE0BE0
static REL::Relocation<thkpKeyFrameUtility_applyHardKeyFrame> hkpKeyFrameUtility_applyHardKeyFrameAsynchronously{ RELOCATION_ID(61580, 62479) };  // ABC700, AE0F10

typedef void (*tbhkNiCollisionObject_ctor)(RE::bhkNiCollisionObject* a_this);
static REL::Relocation<tbhkNiCollisionObject_ctor> bhkNiCollisionObject_ctor{ RELOCATION_ID(76530, 78372) };  // DC59A0, E05EE0

typedef void (*tbhkNiCollisionObject_NiNode_ctor)(RE::bhkNiCollisionObject* a_this, RE::NiNode* a2);
static REL::Relocation<tbhkNiCollisionObject_NiNode_ctor> bhkNiCollisionObject_NiNode_ctor{ RELOCATION_ID(76531, 78373) };  // DC59E0, E05F20

typedef void (*tbhkNiCollisionObject_setWorldObject)(RE::bhkNiCollisionObject* a_this, RE::bhkWorldObject* a_rigidBody);
static REL::Relocation<tbhkNiCollisionObject_setWorldObject> bhkNiCollisionObject_setWorldObject{ RELOCATION_ID(76533, 78375) };  // DC5AF0, E06030

typedef void (*tNiNode_unkDC6140)(RE::NiNode* a_this, bool a2);
static REL::Relocation<tNiNode_unkDC6140> NiNode_unkDC6140{ RELOCATION_ID(76545, 78389) };  // DC6140, E068C0

typedef void (*tTESObjectREFR_unk29C050)(RE::TESObjectREFR* a_this, RE::bhkRigidBody* a_rigidBody, RE::NiNode* a_node);
static REL::Relocation<tTESObjectREFR_unk29C050> TESObjectREFR_unk29C050{ RELOCATION_ID(19432, 19860) };  // 29C050, 2AE760?

typedef RE::hkUFloat8* (*thkRealTohkUFloat8)(RE::hkUFloat8& a_outFloat, const float& a_value);
static REL::Relocation<thkRealTohkUFloat8> hkRealTohkUFloat8{ RELOCATION_ID(56679, 57086) };  // 9C8210, 9ECA00

typedef void (*tAddArtObject)(RE::TESObjectREFR* a_target, RE::BGSArtObject* a_artObject, float a_lifetime, RE::TESObjectREFR* a_facingObject, bool a_bAttachToCamera, bool a_bInheritRotation, void* a7, void* a8);
static REL::Relocation<tAddArtObject> AddArtObject{ RELOCATION_ID(22289, 22769) };  // 30F9A0, 324B60

typedef void (*tAddEffectShader)(RE::TESObjectREFR* a_target, RE::TESEffectShader* a_effectShader, float a_lifetime, RE::TESObjectREFR* a_facingObject, void* a5, void* a6, void* a7, void* a8);
static REL::Relocation<tAddEffectShader> AddEffectShader{ RELOCATION_ID(19446, 19872) };  // 29CB80, 2AF380

typedef void (*tNiSkinInstance_UpdateBoneMatrices)(RE::NiSkinInstance* a_this, RE::NiTransform& a_rootTransform);
static REL::Relocation<tNiSkinInstance_UpdateBoneMatrices> NiSkinInstance_UpdateBoneMatrices{ RELOCATION_ID(75655, 77461) };  // D74F70, DB0E30

typedef void (*tHitData_ctor)(RE::HitData* a_this);
static REL::Relocation<tHitData_ctor> HitData_ctor{ RELOCATION_ID(42826, 43995) };  // 742450, 76E8F0

typedef void (*tHitData_Populate)(RE::HitData* a_this, RE::Actor* a_attacker, RE::Actor* a_target, RE::InventoryEntryData* a_weapon, bool a_bIsLeftHand);
static REL::Relocation<tHitData_Populate> HitData_Populate{ RELOCATION_ID(42832, 44001) };  // 742850, 76EDB0

typedef RE::NiPointer<RE::BGSAttackData>& (*tActor_GetAttackData)(RE::Actor* a_this);
static REL::Relocation<tActor_GetAttackData> Actor_GetAttackData{ RELOCATION_ID(37625, 38578) };  // 625FE0, 64B6E0

typedef void (*tApplyCameraShake)(float a_strength, RE::NiPoint3& a_position, float a3);
static REL::Relocation<tApplyCameraShake> ApplyCameraShake{ RELOCATION_ID(32275, 33012) };  // 4F5420, 50E5F0

typedef RE::TESObjectREFR*(_fastcall* tActor_GetEquippedShield)(RE::Actor* a_actor);
inline static REL::Relocation<tActor_GetEquippedShield> Actor_GetEquippedShield{ RELOCATION_ID(37624, 38577) };  // 625FA0, 64B6A0

typedef RE::hkStringPtr* (*thkStringPtr_ctor)(RE::hkStringPtr*, const char*);
static REL::Relocation<thkStringPtr_ctor> hkStringPtr_ctor{ RELOCATION_ID(56806, 57236) };  // 9CBA10, 9F0210

typedef void (*thkpConvexVerticesShape_ctor)(RE::hkpConvexVerticesShape*, const RE::hkStridedVertices& a_vertices, const RE::hkpConvexVerticesShape::BuildConfig& a_buildConfig);
static REL::Relocation<thkpConvexVerticesShape_ctor> hkpConvexVerticesShape_ctor{ RELOCATION_ID(78843, 80831) };  // E43640, E895C0
//static REL::Relocation<thkpConvexVerticesShape_ctor> hkpConvexVerticesShape_ctor{ RELOCATION_ID(64063, 65089) };  // B5DDC0, B82F30

typedef void (*thkpConvexVerticesShape_getOriginalVertices)(const RE::hkpConvexVerticesShape* a_this, RE::hkArray<RE::hkVector4>& a_outVertices);
static REL::Relocation<thkpConvexVerticesShape_getOriginalVertices> hkpConvexVerticesShape_getOriginalVertices{ RELOCATION_ID(64067, 65093) };  // B5E120, B83290

typedef RE::InventoryEntryData* (*tAIProcess_GetCurrentlyEquippedWeapon)(RE::AIProcess* a_this, bool a_bIsLeftHand);
static REL::Relocation<tAIProcess_GetCurrentlyEquippedWeapon> AIProcess_GetCurrentlyEquippedWeapon{ RELOCATION_ID(38781, 39806) };  // 67A3E0, 6A19E0

typedef void (*tPlayWaterImpact)(float a_worldBoundRadius, const RE::NiPoint3& a_position);
static REL::Relocation<tPlayWaterImpact> PlayWaterImpact{ RELOCATION_ID(31297, 32081) };  // 4C09F0, 4DABB0

typedef bool (*tBSSoundHandle_SetPosition)(RE::BSSoundHandle* a_this, float a_x, float a_y, float a_z);
static REL::Relocation<tBSSoundHandle_SetPosition> BSSoundHandle_SetPosition{ RE::Offset::BSSoundHandle::SetPosition };

typedef RE::NiAVObject*(__fastcall* tNiAVObject_LookupBoneNodeByName)(RE::NiAVObject* a_this, const RE::BSFixedString& a_name, bool a3);
static REL::Relocation<tNiAVObject_LookupBoneNodeByName> NiAVObject_LookupBoneNodeByName{ RELOCATION_ID(74481, 76207) };

typedef void (*tApplyPerkEntryPoint)(RE::BGSEntryPoint::ENTRY_POINT a_entryPoint, RE::Actor* a_actor, RE::TESBoundObject* a_object, float& a_outResult);
static REL::Relocation<tApplyPerkEntryPoint> ApplyPerkEntryPoint{ RELOCATION_ID(23073, 23526) };  // 32ECE0, 3444C0

typedef RE::MagicItem* (*tGetEnchantment)(RE::InventoryEntryData* a_item);
static REL::Relocation<tGetEnchantment> GetEnchantment{ RELOCATION_ID(15788, 16026) };  // 1D7F10, 1E3830

typedef RE::MagicItem* (*tGetPoison)(RE::InventoryEntryData* a_item);
static REL::Relocation<tGetPoison> GetPoison{ RELOCATION_ID(15761, 15999) };  // 1D69C0, 1E2250

typedef RE::ActorValue (*tGetActorValueForCost)(RE::MagicItem* a_item, RE::MagicSystem::CastingSource a_castingSource);
static REL::Relocation<tGetActorValueForCost> GetActorValueForCost{ RELOCATION_ID(33817, 34609) };  // 556780, 572200

typedef void (*tNiBound_Combine)(RE::NiBound& a_this, const RE::NiBound& a_other);
static REL::Relocation<tNiBound_Combine> NiBound_Combine{ RELOCATION_ID(69588, 70973) };  // C73920, C9C220

typedef float (*tActor_GetReach)(RE::Actor* a_this);
static REL::Relocation<tActor_GetReach> Actor_GetReach{ RELOCATION_ID(37588, 38538) };  // 623F10, 649520

typedef RE::NiObject* (*tNiObject_Clone)(RE::NiObject* a_object, const RE::NiCloningProcess& a_cloningProcess);
static REL::Relocation<tNiObject_Clone> NiObject_Clone{ RELOCATION_ID(68836, 70188) };  // C52820, C79F30

typedef void (*tCleanupCloneMap)(RE::BSTHashMap<RE::NiObject*, RE::NiObject*>& a_cloneMap);
static REL::Relocation<tCleanupCloneMap> CleanupCloneMap{ RELOCATION_ID(15231, 15395) };  // 1B8AD0, 1C4460

typedef void (*tCleanupProcessMap)(RE::BSTHashMap<RE::NiObject*, bool>& a_cloneMap);
static REL::Relocation<tCleanupProcessMap> CleanupProcessMap{ RELOCATION_ID(15232, 15396) };  // 1B8B90, 1C4520

typedef void (*tNiAVObject_RemoveFromWorld)(RE::NiAVObject* a_this, bool a2, bool a3);
static REL::Relocation<tNiAVObject_RemoveFromWorld> NiAVObject_RemoveFromWorld{ RELOCATION_ID(76031, 77864) };  // DA80B0, DE7E20

typedef void (*tBSAnimationGraphManager_SetWorld_SEOnly)(RE::BSAnimationGraphManager* a_this, RE::NiPointer<RE::bhkWorld>& a_world);
static REL::Relocation<tBSAnimationGraphManager_SetWorld_SEOnly> BSAnimationGraphManager_SetWorld_SEOnly{ RELOCATION_ID(32189, 0) };  // 4F2400

typedef void (*tIAnimationGraphManagerHolder_SetWorld_AEOnly)(RE::IAnimationGraphManagerHolder* a_this, RE::NiPointer<RE::bhkWorld>& a_world);
static REL::Relocation<tIAnimationGraphManagerHolder_SetWorld_AEOnly> IAnimationGraphManagerHolder_SetWorld_AEOnly{ RELOCATION_ID(0, 38087) };  // 6392E0

typedef void (*tSetActionDataName)(void* a1, RE::TESActionData* a_actionData);
static REL::Relocation<tSetActionDataName> SetActionDataName{ RELOCATION_ID(37998, 38952) };  // 63B060, 661300

typedef void (*tSetActionDataNameSource)(void* a1, RE::TESActionData* a_actionData);
static REL::Relocation<tSetActionDataNameSource> SetActionDataNameSource{ RELOCATION_ID(36174, 37147) };  // 5CCB20, 5F0C20

typedef void (*tSetActionDataNameTarget)(void* a1, RE::TESActionData* a_actionData);
static REL::Relocation<tSetActionDataNameTarget> SetActionDataNameTarget{ RELOCATION_ID(32048, 32802) };  // 4ECF50, 505AD0

typedef float (*tGetPlayerTimeMult)(void* a1);
static REL::Relocation<tGetPlayerTimeMult> GetPlayerTimeMult{ RELOCATION_ID(43104, 44301) };  // 759420, 7873C0

typedef float (*tAIProcess_PushActorAway)(RE::AIProcess* a_this, RE::Actor* a_actor, RE::NiPoint3& a_from, float a_force);
static REL::Relocation<tAIProcess_PushActorAway> AIProcess_PushActorAway{ RELOCATION_ID(38858, 39895) };  // 67D4A0, 6A4B40
