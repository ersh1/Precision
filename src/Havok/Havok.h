#pragma once

// a lot of the code is from https://github.com/adamhynek

namespace RE
{
	enum hkpUpdateCollisionFilterOnEntityMode
	{
		kFullCheck,
		kDisableEntityEntityCollisionsOnly
	};

	enum hkpEntityActivation
	{
		kDoNotActivate,
		kDoActivate
	};

	class hkClass
	{
	public:
		const char* name;
	};

	struct hkbGeneratorOutput
	{
		enum class StandardTracks
		{
			TRACK_WORLD_FROM_MODEL,                       // 00
			TRACK_EXTRACTED_MOTION,                       // 01
			TRACK_POSE,                                   // 02
			TRACK_FLOAT_SLOTS,                            // 03
			TRACK_RIGID_BODY_RAGDOLL_CONTROLS,            // 04
			TRACK_RIGID_BODY_RAGDOLL_BLEND_TIME,          // 05
			TRACK_POWERED_RAGDOLL_CONTROLS,               // 06
			TRACK_POWERED_RAGDOLL_WORLD_FROM_MODEL_MODE,  // 07
			TRACK_KEYFRAMED_RAGDOLL_BONES,                // 08
			TRACK_KEYFRAME_TARGETS,                       // 09
			TRACK_ANIMATION_BLEND_FRACTION,               // 0A
			TRACK_ATTRIBUTES,                             // 0B
			TRACK_FOOT_IK_CONTROLS,                       // 0C
			TRACK_CHARACTER_CONTROLLER_CONTROLS,          // 0D
			TRACK_HAND_IK_CONTROLS_0,                     // 0E
			TRACK_HAND_IK_CONTROLS_1,                     // 0F
			TRACK_HAND_IK_CONTROLS_2,                     // 10
			TRACK_HAND_IK_CONTROLS_3,                     // 11
			TRACK_HAND_IK_CONTROLS_NON_BLENDABLE_0,       // 12
			TRACK_HAND_IK_CONTROLS_NON_BLENDABLE_1,       // 13
			TRACK_HAND_IK_CONTROLS_NON_BLENDABLE_2,       // 14
			TRACK_HAND_IK_CONTROLS_NON_BLENDABLE_3,       // 15
			TRACK_DOCKING_CONTROLS,                       // 16
			TRACK_AI_CONTROL_CONTROLS_BLENDABLE,          // 17
			TRACK_AI_CONTROL_CONTROLS_NON_BLENDABLE,      // 18
			NUM_STANDARD_TRACKS,                          // 19
		};

		enum class TrackTypes
		{
			TRACK_TYPE_REAL,         // 0
			TRACK_TYPE_QSTRANSFORM,  // 1
			TRACK_TYPE_BINARY,       // 2
		};

		enum class TrackFlags
		{
			TRACK_FLAG_ADDITIVE_POSE = 1,
			TRACK_FLAG_PALETTE = 1 << 1,
			TRACK_FLAG_SPARSE = 1 << 2,
		};

		struct TrackHeader
		{
			int16_t capacity;                            // 00
			int16_t numData;                             // 02
			int16_t dataOffset;                          // 04
			int16_t elementSizeBytes;                    // 06
			float onFraction;                            // 08
			stl::enumeration<TrackFlags, int8_t> flags;  // 0C
			stl::enumeration<TrackTypes, int8_t> type;   // 0D
		};
		static_assert(sizeof(TrackHeader) == 0x10);

		struct TrackMasterHeader
		{
			int32_t numBytes;   // 00
			int32_t numTracks;  // 04
			int8_t unused[8];   // 08
		};

		struct Tracks
		{
			struct TrackMasterHeader masterHeader;  // 00
			struct TrackHeader trackHeaders[1];     // 10
		};

		struct Track
		{
			TrackHeader* header;  // 00
			float* data;          // 08
		};

		struct Tracks* tracks;  // 00
		bool deleteTracks;      // 08
	};

	class hkaKeyFrameHierarchyUtility
	{
	public:
		struct ControlData
		{
			float hierarchyGain;
			float velocityDamping;
			float accelerationGain;
			float velocityGain;
			float positionGain;
			float positionMaxLinearVelocity;
			float positionMaxAngularVelocity;
			float snapGain;
			float snapMaxLinearVelocity;
			float snapMaxAngularVelocity;
			float snapMaxLinearDistance;
			float snapMaxAngularDistance;

			ControlData() :
				hierarchyGain(0.17f),
				velocityDamping(0.0f),
				accelerationGain(1.0f),
				velocityGain(0.6f),
				positionGain(0.05f),
				positionMaxLinearVelocity(1.4f),
				positionMaxAngularVelocity(1.8f),
				snapGain(0.1f),
				snapMaxLinearVelocity(0.3f),
				snapMaxAngularVelocity(0.3f),
				snapMaxLinearDistance(0.03f),
				snapMaxAngularDistance(0.1f) {}
		};
	};

	struct hkbPoweredRagdollControlData
	{
		float maxForce = 50.f;                     // 00
		float tau = 0.8f;                          // 04
		float damping = 1.f;                       // 08
		float proportionalRecoveryVelocity = 2.f;  // 0C
		float constantRecoveryVelocity = 1.f;      // 10
	};

	struct hkbWorldFromModelModeData
	{
		enum class WorldFromModelMode : uint8_t
		{
			WORLD_FROM_MODEL_MODE_USE_OLD,        // 0
			WORLD_FROM_MODEL_MODE_USE_INPUT,      // 1
			WORLD_FROM_MODEL_MODE_COMPUTE,        // 2
			WORLD_FROM_MODEL_MODE_NONE,           // 3
			WORLD_FROM_MODEL_MODE_USE_ROOT_BONE,  // 4
		};

		int16_t poseMatchingBone0;  // 00
		int16_t poseMatchingBone1;  // 02
		int16_t poseMatchingBone2;  // 04
		WorldFromModelMode mode;    // 06
	};

	class hkbEventInfo
	{
	public:
		uint32_t flags;
	};

	class hkaBone
	{
	public:
		hkStringPtr name;
		bool lockTranslation;
	};

	class hkaSkeleton : hkReferencedObject
	{
	public:
		hkStringPtr name;
		hkArray<int16_t> parentIndices;
		hkArray<hkaBone> bones;
		hkArray<hkQsTransform> referencePose;
		hkArray<float> referenceFloats;
		hkArray<hkStringPtr> floatSlots;
		hkArray<struct LocalFrameOnBone> localFrames;
	};

	class hkaSkeletonMapperData
	{
	public:
		enum MappingType
		{
			kRagdollMapping = 0,
			kRetargetingMapping = 1
		};

		struct SimpleMapping
		{
			int16_t boneA;
			int16_t boneB;
			hkQsTransform aFromBTransform;
		};

		struct ChainMapping
		{
			int16_t startBoneA;
			int16_t endBoneA;
			int16_t startBoneB;
			int16_t endBoneB;
			hkQsTransform startAFromBTransform;
			hkQsTransform endAFromBTransform;
		};

		hkRefPtr<hkaSkeleton> skeletonA;
		hkRefPtr<hkaSkeleton> skeletonB;
		hkArray<SimpleMapping> simpleMappings;
		hkArray<ChainMapping> chainMappings;
		hkArray<int16_t> unmappedBones;
		hkQsTransform extractedMotionMapping;
		bool keepUnmappedLocal;
		stl::enumeration<hkaSkeletonMapperData::MappingType, int32_t> mappingType;
	};

	class hkaSkeletonMapper : public hkReferencedObject
	{
	public:
		hkaSkeletonMapperData mapping;
	};

	struct BShkbAnimationGraph_UpdateData
	{
		float deltaTime;                            // 00
		void* unk08;                                // function pointer to some Character function
		Character* character;                       // 10
		NiPoint3* cameraPos;                        // 18 - literally points to the pos within the worldTransform of the WorldRoot NiCamera node
		IPostAnimationChannelUpdateFunctor* unk20;  // points to the IPostAnimationChannelUpdateFunctor part of the Character
		uint8_t unk28;
		uint8_t unk29;
		uint8_t unk2A;
		uint8_t unk2B;
		uint8_t unk2C;
		uint8_t unk2D;  // if 0, call generate(). If not 0, do something else?
		uint8_t unk2E;
		uint8_t unk2F;
		float scale1;  // 30 - for rabbit, 1.3f
		float scale2;  // 34 - for rabbit, 1.3f
					   // ...
	};
	static_assert(offsetof(BShkbAnimationGraph_UpdateData, unk2A) == 0x2A);
	static_assert(offsetof(BShkbAnimationGraph_UpdateData, scale1) == 0x30);

	class bhkConstraint : public bhkSerializable
	{
	public:
		inline static constexpr auto RTTI = RTTI_bhkConstraint;
		inline static constexpr auto Ni_RTTI = NiRTTI_bhkConstraint;

		~bhkConstraint() override;  // 00
	};
	static_assert(sizeof(bhkConstraint) == 0x20);

	class bhkRagdollConstraint : public bhkConstraint
	{
	public:
		inline static constexpr auto RTTI = RTTI_bhkRagdollConstraint;
		inline static constexpr auto Ni_RTTI = NiRTTI_bhkRagdollConstraint;
		inline static constexpr auto VTABLE = VTABLE_bhkRagdollConstraint;

		~bhkRagdollConstraint() override;  // 00
	};
	static_assert(sizeof(bhkRagdollConstraint) == 0x20);

	struct hkConstraintCinfo
	{
		~hkConstraintCinfo();

		void* vtbl = 0;                                            // 00
		RE::hkRefPtr<hkpConstraintData> constraintData = nullptr;  // 08
		uint32_t unk10 = 0;
		uint32_t unk14 = 0;
		hkpRigidBody* rigidBodyA = nullptr;  // 18
		hkpRigidBody* rigidBodyB = nullptr;  // 20
	};

	struct hkRagdollConstraintCinfo : hkConstraintCinfo
	{
		hkRagdollConstraintCinfo()
		{
			this->vtbl = (void*)RE::VTABLE_hkRagdollConstraintCinfo[0].address();
		}
	};

	class hkMemoryRouter
	{
	public:
		uint64_t unk00;             // 00
		uint64_t unk08;             // 08
		uint64_t unk10;             // 10
		uint64_t unk18;             // 18
		uint64_t unk20;             // 20
		uint64_t unk28;             // 28
		uint64_t unk30;             // 30
		uint64_t unk38;             // 38
		uint64_t unk40;             // 40
		uint64_t unk48;             // 48
		hkMemoryAllocator* temp;    // 50
		hkMemoryAllocator* heap;    // 58
		hkMemoryAllocator* debug;   // 60
		hkMemoryAllocator* solver;  // 68
		void* userData;             // 70
	};
	static_assert(offsetof(hkMemoryRouter, heap) == 0x58);

	class bhkSphereRepShape : public bhkShape
	{};

	class bhkConvexShape : public bhkSphereRepShape
	{};

	class bhkCapsuleShape : public bhkConvexShape
	{};
	static_assert(sizeof(bhkCapsuleShape) == 0x28);

	enum hkpCollidableQualityType
	{
		kInvalid = -1,
		kFixed = 0,
		kKeyframed,
		kDebris,
		kDebrisSimpleTOI,
		kMoving,
		kCritical,
		kBullet,
		kUser,
		kCharacter,
		kKeyframedReporting,
		kMAX
	};

	struct hkpRigidBodyCinfo
	{
		enum SolverDeactivation
		{
			kInvalid,
			kOff,
			kLow,
			kMedium,
			kHigh,
			kMax
		};

		uint32_t collisionFilterInfo;                                            // 00
		const hkpShape* shape;                                                   // 08
		hkLocalFrame* localFrame;                                                // 10
		stl::enumeration<hkpMaterial::ResponseType, uint8_t> collisionRepsonse;  // 18
		uint16_t contactPointCallbackDelay;                                      // 1A
		hkVector4 position;                                                      // 20
		hkQuaternion rotation;                                                   // 30
		hkVector4 linearVelocity;                                                // 40
		hkVector4 angularVelocity;                                               // 50
		hkMatrix3 inertiaTensor;                                                 // 60
		hkVector4 centerOfMass;                                                  // 90
		float mass;                                                              // A0
		float linearDamping;                                                     // A4
		float angularDamping;                                                    // A8
		float timeFactor;                                                        // AC
		float gravityFactor;                                                     // B0
		float friction;                                                          // B4
		float rollingFrictionMultiplier;                                         // B8
		float restitution;                                                       // BC
		float maxLinearVelocity;                                                 // C0
		float maxAngularVelocity;                                                // C4
		float allowedPenetrationDepth;                                           // C8
		stl::enumeration<hkpMotion::MotionType, uint8_t> motionType;             // CC
		bool enableDeactivation;                                                 // CD
		stl::enumeration<SolverDeactivation, uint8_t> solverDeactivation;        // CE
		stl::enumeration<hkpCollidableQualityType, uint8_t> qualityType;         // CF
		int8_t autoRemoveLevel;                                                  // D0
		uint8_t responseModifierFlags;                                           // D1
		int8_t numShapeKeysInContactPointProperties;                             // D2
		bool forceCollideOntoPpu;                                                // D3
	};
	static_assert(sizeof(hkpRigidBodyCinfo) == 0xE0);

	struct bhkRigidBodyCinfo
	{
		uint32_t collisionFilterInfo;  // 00 - initd to 0
		hkpShape* shape;               // 08 - initd to 0
		uint8_t unk10;                 // initd to 1
		uint64_t unk18;                // initd to 0
		uint32_t unk20;                // initd to 0
		float unk24;                   // initd to -0
		uint8_t unk28;                 // initd to 1
		uint16_t unk2A;                // initd to -1 - quality type?
		hkpRigidBodyCinfo hkCinfo;     // 30 - size == 0xE0
	};
	static_assert(offsetof(bhkRigidBodyCinfo, shape) == 0x08);
	static_assert(offsetof(bhkRigidBodyCinfo, hkCinfo) == 0x30);
	static_assert(sizeof(bhkRigidBodyCinfo) == 0x110);

	class hkpSolverResults
	{
	public:
		float impulseApplied;
		float internalSolverData;
	};

	class hkContactPointMaterial
	{
	public:
		enum FlagEnum
		{
			kIsNew = 1,
			kUsesSolverPath2 = 2,
			kBreakoffObjectID = 4,
			kIsDisabled = 8
		};

		uint64_t userData;
		hkUFloat8 friction;
		uint8_t restitution;
		hkUFloat8 maxImpulse;
		uint8_t flags;
	};

	class hkpContactPointProperties : public hkpSolverResults, public hkContactPointMaterial
	{
		float internalDataA;
	};

	class hkpConvexVerticesShape : public hkpConvexShape
	{
	public:
		struct BuildConfig
		{
			bool createConnectivity;
			bool shrinkByConvexRadius;
			bool useOptimizedShrinking;
			float convexRadius;
			int32_t maxVertices;
			float maxRelativeShrink;
			float maxShrinkingVerticesDisplacement;
			float maxCosAngleForBevelPlanes;
		};
		static_assert(sizeof(BuildConfig) == 0x18);

		uint64_t pad40[13];
	};
	static_assert(sizeof(hkpConvexVerticesShape) == 0x90);

	struct hkStridedVertices
	{
	public:
		hkStridedVertices() :
			numVertices(0)
		{}

		hkStridedVertices(const hkArrayBase<hkVector4>& a_vertices) { set(a_vertices); }

		hkStridedVertices(const hkVector4* a_vertices, int a_numVertices) { set(a_vertices, a_numVertices); }

		const float* vertices;
		int numVertices;
		int striding;

		inline void set(const hkArrayBase<hkVector4>& a_vertices)
		{
			set(a_vertices.begin(), a_vertices.size());
		}

		template <typename T>
		inline void set(const T* a_vertices, int a_numVertices)
		{
			vertices = (const float*)a_vertices;
			numVertices = a_numVertices;
			striding = sizeof(T);
		}
	};

	struct hkbNodeInfo
	{
		void* unk00;            //00
		int64_t unk08;          //08
		int64_t unk10;          //10
		void* unk18;            //18
		char unk20[48];         //20
		hkbNode* nodeTemplate;  //50
		hkbNode* nodeClone;     //58
		hkbNode* behavior;      //60
		int64_t unk68;          //68
		int64_t unk70;          //70
		int64_t unk78;          //78
		int64_t unk80;          //80
		bool unk88;             //88
	};
	static_assert(sizeof(hkbNodeInfo) == 0x90);

	using NodeList = hkArray<hkbNodeInfo>;

	class bhkBlendCollisionObject : public bhkCollisionObject
	{
	public:
		inline static constexpr auto RTTI = RTTI_bhkBlendCollisionObject;
		inline static auto Ni_RTTI = NiRTTI_bhkBlendCollisionObject;

		~bhkBlendCollisionObject() override;  // 00

		float blendStrength;               // 28 - this affects how intensely to go from rigidBody position to node position. 0 means strictly follow rigidbody, 1 means strictly follow node.
		float unk2C;                       // 2C
		hkpMotion::MotionType motionType;  // 30
		bhkWorld* world;                   // 38
		uint32_t unk40;                    // 40
	};
	static_assert(sizeof(bhkBlendCollisionObject) == 0x48);

	class bhkRigidBodyT : public bhkRigidBody
	{
	public:
		inline static constexpr auto RTTI = RTTI_bhkRigidBodyT;
		inline static auto Ni_RTTI = NiRTTI_bhkRigidBodyT;

		~bhkRigidBodyT() override;  // 00

		// members
		hkQuaternion rotation;  // 40
		hkVector4 translation;  // 50
	};
	static_assert(sizeof(bhkRigidBodyT) == 0x60);
}

void hkpWorld_removeContactListener(RE::hkpWorld* a_this, RE::hkpContactListener* a_worldListener);
bool hkpWorld_hasContactListener(RE::hkpWorld* a_this, RE::hkpContactListener* a_worldListener);

RE::bhkCharProxyController* GetCharProxyController(RE::Actor* a_actor);

RE::hkMemoryRouter& hkGetMemoryRouter();
inline void* hkHeapAlloc(int numBytes) { return hkGetMemoryRouter().heap->BlockAlloc(numBytes); }

inline float* Track_getData(RE::hkbGeneratorOutput& a_output, RE::hkbGeneratorOutput::TrackHeader& a_header)
{
	return reinterpret_cast<float*>(reinterpret_cast<char*>(a_output.tracks) + a_header.dataOffset);
}

inline int8_t* Track_getIndices(RE::hkbGeneratorOutput& a_output, RE::hkbGeneratorOutput::TrackHeader& a_header)
{
	auto NextMultipleOf = [](auto a_alignment, auto a_value) { return ((a_value) + ((a_alignment)-1)) & (~((a_alignment)-1)); };

	// must be sparse or pallette track
	int numDataBytes = NextMultipleOf(16, a_header.elementSizeBytes * a_header.capacity);
	return reinterpret_cast<int8_t*>(Track_getData(a_output, a_header)) + numDataBytes;
}

inline RE::hkbGeneratorOutput::TrackHeader* GetTrackHeader(RE::hkbGeneratorOutput& a_generatorOutput, RE::hkbGeneratorOutput::StandardTracks a_track)
{
	int trackId = (int)a_track;
	int32_t numTracks = a_generatorOutput.tracks->masterHeader.numTracks;
	return numTracks > trackId ? &(a_generatorOutput.tracks->trackHeaders[trackId]) : nullptr;
}

RE::bhkRagdollConstraint* bhkRagdolConstraint_ctor(RE::bhkRagdollConstraint* a_this, RE::hkConstraintCinfo* a_cInfo);
