#pragma once

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
	kPrecision = 56
};

#define DLLEXPORT __declspec(dllexport)

#define RELOCATION_OFFSET(SE, AE) REL::VariantOffset(SE, AE, 0).offset()

#include "Plugin.h"
