#pragma once
#include "PrecisionAPI.h"

namespace Messaging
{
	using APIResult = ::PRECISION_API::APIResult;
	using InterfaceVersion1 = ::PRECISION_API::IVPrecision1;
	using InterfaceVersion2 = ::PRECISION_API::IVPrecision2;
	using PreHitCallback = ::PRECISION_API::PreHitCallback;
	using PostHitCallback = ::PRECISION_API::PostHitCallback;
	using PrePhysicsStepCallback = ::PRECISION_API::PrePhysicsStepCallback;
	using CollisionFilterComparisonCallback = ::PRECISION_API::CollisionFilterComparisonCallback;
	using RequestedAttackCollisionType = ::PRECISION_API::RequestedAttackCollisionType;
	using WeaponCollisionCallback = ::PRECISION_API::WeaponCollisionCallback;
	using WeaponCollisionCallbackReturn = ::PRECISION_API::WeaponCollisionCallbackReturn;

	class PrecisionInterface : public InterfaceVersion2
	{
	private:
		PrecisionInterface() = default;
		PrecisionInterface(const PrecisionInterface&) = delete;
		PrecisionInterface(PrecisionInterface&&) = delete;
		virtual ~PrecisionInterface() = default;

		PrecisionInterface& operator=(const PrecisionInterface&) = delete;
		PrecisionInterface& operator=(PrecisionInterface&&) = delete;

	public:
		static PrecisionInterface* GetSingleton() noexcept
		{
			static PrecisionInterface singleton;
			return std::addressof(singleton);
		}

		// InterfaceVersion1
		virtual APIResult AddPreHitCallback(SKSE::PluginHandle a_pluginHandle, PreHitCallback&& a_preHitCallback) noexcept override;
		virtual APIResult AddPostHitCallback(SKSE::PluginHandle a_pluginHandle, PostHitCallback&& a_postHitCallback) noexcept override;
		virtual APIResult AddPrePhysicsStepCallback(SKSE::PluginHandle a_pluginHandle, PrePhysicsStepCallback&& a_prePhysicsStepCallback) noexcept override;
		virtual APIResult AddCollisionFilterComparisonCallback(SKSE::PluginHandle a_pluginHandle, CollisionFilterComparisonCallback&& a_collisionFilterComparisonCallback) noexcept override;
		virtual APIResult RemovePreHitCallback(SKSE::PluginHandle a_pluginHandle) noexcept override;
		virtual APIResult RemovePostHitCallback(SKSE::PluginHandle a_pluginHandle) noexcept override;
		virtual APIResult RemovePrePhysicsStepCallback(SKSE::PluginHandle a_pluginHandle) noexcept override;
		virtual APIResult RemoveCollisionFilterComparisonCallback(SKSE::PluginHandle a_pluginHandle) noexcept override;
		virtual float GetAttackCollisionCapsuleLength(RE::ActorHandle a_actorHandle, RequestedAttackCollisionType a_collisionType = RequestedAttackCollisionType::Default) const noexcept override;

		// InterfaceVersion2
		virtual APIResult AddWeaponWeaponCollisionCallback(SKSE::PluginHandle a_pluginHandle, WeaponCollisionCallback&& a_callback) noexcept override;
		virtual APIResult RemoveWeaponWeaponCollisionCallback(SKSE::PluginHandle a_pluginHandle) noexcept override;
		virtual APIResult AddWeaponProjectileCollisionCallback(SKSE::PluginHandle a_pluginHandle, WeaponCollisionCallback&& a_callback) noexcept override;
		virtual APIResult RemoveWeaponProjectileCollisionCallback(SKSE::PluginHandle a_pluginHandle) noexcept override;
		virtual void ApplyHitImpulse(RE::ActorHandle a_actorHandle, RE::hkpRigidBody* a_rigidBody, const RE::NiPoint3& a_hitVelocity, const RE::hkVector4& a_hitPosition, float a_impulseMult) noexcept override;
	};
}
