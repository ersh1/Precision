#pragma once

// CommonLibSSE-NG 6.x uses coroutines in its Papyrus interfaces (IVirtualMachine::Awaitable) and
// relies on its own precompiled header having pulled <coroutine> in. This project has its own PCH,
// so it has to be included here before any CommonLibSSE header.
#include <coroutine>

#pragma warning(push)
#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>

#pragma warning(disable: 4702)
#include "ModAPI.h"
#include <SimpleIni.h>
#include <toml++/toml.h>

#pragma warning(disable: 4201)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#ifdef NDEBUG
#	include <spdlog/sinks/basic_file_sink.h>
#else
#	include <spdlog/sinks/msvc_sink.h>
#endif
#pragma warning(pop)

#include <shared_mutex>

using namespace std::literals;

namespace logger = SKSE::log;

namespace util
{
	using SKSE::stl::report_and_fail;
}

namespace std
{
	template <class T>
	struct hash<RE::BSPointerHandle<T>>
	{
		uint32_t operator()(const RE::BSPointerHandle<T>& a_handle) const
		{
			uint32_t nativeHandle = const_cast<RE::BSPointerHandle<T>*>(&a_handle)->native_handle();  // ugh
			return nativeHandle;
		}
	};
}

using Lock = std::shared_mutex;
using ReadLocker = std::shared_lock<Lock>;
using WriteLocker = std::unique_lock<Lock>;

enum class WeaponType
{
	// RE::WEAPON_TYPE
	kHandToHandMelee = 0,
	kOneHandSword = 1,
	kOneHandDagger = 2,
	kOneHandAxe = 3,
	kOneHandMace = 4,
	kTwoHandSword = 5,
	kTwoHandAxe = 6,
	kBow = 7,
	kStaff = 8,
	kCrossbow = 9,

	// added
	kShield = 10
};

enum class CollisionLayer
{
	kUnidentified = 0,
	kStatic = 1,
	kAnimStatic = 2,
	kTransparent = 3,
	kClutter = 4,
	kWeapon = 5,
	kProjectile = 6,
	kSpell = 7,
	kBiped = 8,
	kTrees = 9,
	kProps = 10,
	kWater = 11,
	kTrigger = 12,
	kTerrain = 13,
	kTrap = 14,
	kNonCollidable = 15,
	kCloudTrap = 16,
	kGround = 17,
	kPortal = 18,
	kDebrisSmall = 19,
	kDebrisLarge = 20,
	kAcousticSpace = 21,
	kActorZone = 22,
	kProjectileZone = 23,
	kGasTrap = 24,
	kShellCasting = 25,
	kTransparentWall = 26,
	kInvisibleWall = 27,
	kTransparentSmallAnim = 28,
	kClutterLarge = 29,
	kCharController = 30,
	kStairHelper = 31,
	kDeadBip = 32,
	kBipedNoCC = 33,
	kAvoidBox = 34,
	kCollisionBox = 35,
	kCameraSphere = 36,
	kDoorDetection = 37,
	kConeProjectile = 38,
	kCamera = 39,
	kItemPicker = 40,
	kLOS = 41,
	kPathingPick = 42,
	kUnused0 = 43,
	kUnused1 = 44,
	kSpellExplosion = 45,
	kDroppingPick = 46,

	// Added
	kPrecisionAttack = 56,
	kPrecisionBody = 57,
	kPrecisionRecoil = 58
};

#define DLLEXPORT __declspec(dllexport)

#define RELOCATION_OFFSET(SE, AE) REL::VariantOffset(SE, AE, 0).offset()

// An AE offset that moved within its function in 1.7.99.
//
// The functions themselves were not changed; the executable was rebuilt and the compiler laid the
// same code out differently, so a call that used to sit at +0x294 now sits at +0x2AC. Only the
// offset moved, which is why the hook still works once it is pointed at the right place.
//
// Chosen at runtime rather than at build time, the way CommonLibSSE handles its own values that
// changed inside AE (see REL::RelocateMemberIfNewer and RUNTIME_DATA_ACCESSOR_VERSIONED), so one
// binary stays correct on 1.6.x and on 1.7.99 alike.
[[nodiscard]] inline std::size_t AEOffsetSince1799(std::size_t a_pre1799, std::size_t a_1799) noexcept
{
	return REL::Module::IsAtLeast(SKSE::RUNTIME_SSE_1_7_99) ? a_1799 : a_pre1799;
}

#include "Plugin.h"
