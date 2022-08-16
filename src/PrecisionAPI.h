#pragma once

/*
* For modders: Copy this file into your own project if you wish to use this API
*/
namespace PRECISION_API
{
	constexpr const auto PrecisionPluginName = "Precision";

	// Available Precision interface versions
	enum class InterfaceVersion : uint8_t
	{
		V1,
		V2
	};

	// Error types that may be returned by Precision
	enum class APIResult : uint8_t
	{
		// Your API call was successful
		OK,

		// A callback from this plugin has already been registered
		AlreadyRegistered,

		// A callback from this plugin has not been registered
		NotRegistered,
	};

	struct PreHitModifier
	{
		enum class ModifierType : uint8_t
		{
			Damage,
			Stagger
		};

		enum class ModifierOperation : uint8_t
		{
			Additive,
			Multiplicative
		};

		ModifierType modifierType;
		ModifierOperation modifierOperation;
		float modifierValue;
	};

	struct PreHitCallbackReturn
	{
		// if set to true, the hit will be ignored, no matter what. Do this if you need the game to ignore a hit that would otherwise happen (e.g. a parry)
		bool bIgnoreHit = false;

		// modifiers to the hit
		std::vector<PreHitModifier> modifiers;
	};

	struct WeaponCollisionCallbackReturn
	{
		// if set to true, the hit to the weapon owner will be ignored. Otherwise the game will handle the hit normally as if the weapon was the actor's body.
		bool bIgnoreHit = true;
	};

	struct PrecisionHitData
	{
		PrecisionHitData(RE::Actor* a_attacker, RE::TESObjectREFR* a_target, RE::hkpRigidBody* a_hitRigidBody, RE::hkpRigidBody* a_hittingRigidBody, const RE::NiPoint3& a_hitPos,
			const RE::NiPoint3& a_separatingNormal, const RE::NiPoint3& a_hitPointVelocity, RE::hkpShapeKey a_hitBodyShapeKey, RE::hkpShapeKey a_hittingBodyShapeKey) :
			attacker(a_attacker),
			target(a_target), hitRigidBody(a_hitRigidBody), hittingRigidBody(a_hittingRigidBody), hitPos(a_hitPos), separatingNormal(a_separatingNormal),
			hitPointVelocity(a_hitPointVelocity), hitBodyShapeKey(a_hitBodyShapeKey), hittingBodyShapeKey(a_hittingBodyShapeKey)
		{}

		RE::Actor* attacker;
		RE::TESObjectREFR* target;
		RE::hkpRigidBody* hitRigidBody;
		RE::hkpRigidBody* hittingRigidBody;

		RE::NiPoint3 hitPos;
		RE::NiPoint3 separatingNormal;
		RE::NiPoint3 hitPointVelocity;

		RE::hkpShapeKey hitBodyShapeKey;
		RE::hkpShapeKey hittingBodyShapeKey;
	};

	enum class CollisionFilterComparisonResult : uint8_t
	{
		Continue,  // Do not affect whether the two objects should collide
		Collide,   // Force the two objects to collide
		Ignore,    // Force the two objects to not collide
	};

	enum class RequestedAttackCollisionType : uint8_t
	{
		Default,      // Return the largest currently active collision length, otherwise calculate right weapon's collision length
		Current,      // Return the largest currently active collision length, otherwise 0
		RightWeapon,  // Return either the length of the current right weapon collision if it exists, or calculate it
		LeftWeapon,   // Return either the length of the current left weapon collision if it exists, or calculate it
	};

	using PreHitCallback = std::function<PreHitCallbackReturn(const PrecisionHitData&)>;
	using PostHitCallback = std::function<void(const PrecisionHitData&, const RE::HitData&)>;
	using PrePhysicsStepCallback = std::function<void(RE::bhkWorld*)>;
	using CollisionFilterComparisonCallback = std::function<CollisionFilterComparisonResult(RE::bhkCollisionFilter*, uint32_t, uint32_t)>;
	using WeaponCollisionCallback = std::function<WeaponCollisionCallbackReturn(const PrecisionHitData&)>;

	// Precision's modder interface
	class IVPrecision1
	{
	public:
		/// <summary>
		/// Adds a callback that will run before Precision's hit function.
		/// </summary>
		/// <param name="a_myPluginHandle">Your assigned plugin handle</param>
		/// <param name="a_preHitCallback">The callback function</param>
		/// <returns>OK, AlreadyRegistered</returns>
		virtual APIResult AddPreHitCallback(SKSE::PluginHandle a_myPluginHandle, PreHitCallback&& a_preHitCallback) noexcept = 0;

		/// <summary>
		/// Adds a callback that will run after Precision's hit function.
		/// </summary>
		/// <param name="a_myPluginHandle">Your assigned plugin handle</param>
		/// <param name="a_postHitCallback">The callback function</param>
		/// <returns>OK, AlreadyRegistered</returns>
		virtual APIResult AddPostHitCallback(SKSE::PluginHandle a_myPluginHandle, PostHitCallback&& a_postHitCallback) noexcept = 0;

		/// <summary>
		/// Adds a callback that will run right before hkpWorld::stepDeltaTime is called.
		/// </summary>
		/// <param name="a_myPluginHandle">Your assigned plugin handle</param>
		/// <param name="a_prePhysicsStepCallback">The callback function</param>
		/// <returns>OK, AlreadyRegistered</returns>
		virtual APIResult AddPrePhysicsStepCallback(SKSE::PluginHandle a_myPluginHandle, PrePhysicsStepCallback&& a_prePhysicsStepCallback) noexcept = 0;

		/// <summary>
		/// Adds a callback that will run when havok compares collision filter info to determine if two objects should collide. This can be called hundreds of times per frame, so be brief.
		/// </summary>
		/// <param name="a_myPluginHandle">Your assigned plugin handle</param>
		/// <param name="a_collisionFilterComparisonCallback">The callback function</param>
		/// <returns>OK, AlreadyRegistered</returns>
		virtual APIResult AddCollisionFilterComparisonCallback(SKSE::PluginHandle a_myPluginHandle, CollisionFilterComparisonCallback&& a_collisionFilterComparisonCallback) noexcept = 0;

		/// <summary>
		/// Removes the callback that will run before Precision's hit function.
		/// </summary>
		/// <param name="a_myPluginHandle">Your assigned plugin handle</param>
		/// <returns>OK, NotRegistered</returns>
		virtual APIResult RemovePreHitCallback(SKSE::PluginHandle a_myPluginHandle) noexcept = 0;

		/// <summary>
		/// Removes the callback that will run after Precision's hit function.
		/// </summary>
		/// <param name="a_myPluginHandle">Your assigned plugin handle</param>
		/// <returns>OK, NotRegistered</returns>
		virtual APIResult RemovePostHitCallback(SKSE::PluginHandle a_myPluginHandle) noexcept = 0;

		/// <summary>
		/// Removes the callback that will run right before hkpWorld::stepDeltaTime is called.
		/// </summary>
		/// <param name="a_myPluginHandle">Your assigned plugin handle</param>
		/// <returns>OK, NotRegistered</returns>
		virtual APIResult RemovePrePhysicsStepCallback(SKSE::PluginHandle a_myPluginHandle) noexcept = 0;

		/// <summary>
		/// Removes the callback that will run when havok compares collision filter info to determine if two objects should collide.
		/// </summary>
		/// <param name="a_myPluginHandle">Your assigned plugin handle</param>
		/// <returns>OK, NotRegistered</returns>
		virtual APIResult RemoveCollisionFilterComparisonCallback(SKSE::PluginHandle a_myPluginHandle) noexcept = 0;

		/// <summary>
		/// Gets the current attack collision capsule length. In case of multiple active collisions, returns the largest. If there's no active collision, tries the best guess. Can be a bit complex so try not to call it every frame.
		/// </summary>
		/// <param name="a_actorHandle">Actor handle</param>
		/// <param name="a_collisionType">The type of collision to get</param>
		/// <returns>Capsule length</returns>
		virtual float GetAttackCollisionCapsuleLength(RE::ActorHandle a_actorHandle, RequestedAttackCollisionType a_collisionType = RequestedAttackCollisionType::Default) const noexcept = 0;
	};

	class IVPrecision2 : public IVPrecision1
	{
	public:
		/// <summary>
		/// Adds a callback that will run when two weapons collide.
		/// </summary>
		/// <param name="a_myPluginHandle">Your assigned plugin handle</param>
		/// <param name="a_callback">The callback function</param>
		/// <returns>OK, AlreadyRegistered</returns>
		virtual APIResult AddWeaponWeaponCollisionCallback(SKSE::PluginHandle a_myPluginHandle, WeaponCollisionCallback&& a_callback) noexcept = 0;

		/// <summary>
		/// Removes the callback that will run when two weapons collide.
		/// </summary>
		/// <param name="a_myPluginHandle">Your assigned plugin handle</param>
		/// <returns>OK, NotRegistered</returns>
		virtual APIResult RemoveWeaponWeaponCollisionCallback(SKSE::PluginHandle a_myPluginHandle) noexcept = 0;

		/// <summary>
		/// Adds a callback that will run when a weapon and a moving projectile collide.
		/// </summary>
		/// <param name="a_myPluginHandle">Your assigned plugin handle</param>
		/// <param name="a_callback">The callback function</param>
		/// <returns>OK, AlreadyRegistered</returns>
		virtual APIResult AddWeaponProjectileCollisionCallback(SKSE::PluginHandle a_myPluginHandle, WeaponCollisionCallback&& a_callback) noexcept = 0;

		/// <summary>
		/// Removes the callback that will run when a weapon and a moving projectile collide.
		/// </summary>
		/// <param name="a_myPluginHandle">Your assigned plugin handle</param>
		/// <returns>OK, NotRegistered</returns>
		virtual APIResult RemoveWeaponProjectileCollisionCallback(SKSE::PluginHandle a_myPluginHandle) noexcept = 0;
		
		/// <summary>
		/// Applies impulse.
		/// </summary>
		/// <param name="a_refHandle">Actor handle</param>
		/// <param name="a_rigidBody">Hit rigid body</param>
		/// <param name="a_hitVelocity">Hit velocity vector</param>
		/// <param name="a_hitPosition">Hit position</param>
		/// <param name="a_impulseMult">Impulse strength multiplier</param>
		virtual void ApplyHitImpulse(RE::ActorHandle a_actorHandle, RE::hkpRigidBody* a_rigidBody, const RE::NiPoint3& a_hitVelocity, const RE::hkVector4& a_hitPosition, float a_impulseMult) noexcept = 0;
	};

	typedef void* (*_RequestPluginAPI)(const InterfaceVersion interfaceVersion);

	/// <summary>
	/// Request the Precision API interface.
	/// Recommended: Send your request when you need to use the API and cache the pointer. SKSEMessagingInterface::kMessage_PostLoad seems to be unreliable for some users for unknown reasons.
	/// </summary>
	/// <param name="a_interfaceVersion">The interface version to request</param>
	/// <returns>The pointer to the API singleton, or nullptr if request failed</returns>
	[[nodiscard]] inline void* RequestPluginAPI(const InterfaceVersion a_interfaceVersion = InterfaceVersion::V2)
	{
		auto pluginHandle = GetModuleHandleA("Precision.dll");
		_RequestPluginAPI requestAPIFunction = (_RequestPluginAPI)GetProcAddress(pluginHandle, "RequestPluginAPI");
		if (requestAPIFunction) {
			return requestAPIFunction(a_interfaceVersion);
		}
		return nullptr;
	}
}
