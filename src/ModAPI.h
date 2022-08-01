#pragma once
#include "PrecisionAPI.h"

namespace Messaging
{
	using APIResult = ::PRECISION_API::APIResult;
	using InterfaceVersion1 = ::PRECISION_API::IVPrecision1;
	using PreHitCallback = ::PRECISION_API::PreHitCallback;
	using PostHitCallback = ::PRECISION_API::PostHitCallback;
	using PrePhysicsStepCallback = ::PRECISION_API::PrePhysicsStepCallback;
	using CollisionFilterComparisonCallback = ::PRECISION_API::CollisionFilterComparisonCallback;
	using RequestedAttackCollisionType = ::PRECISION_API::RequestedAttackCollisionType;

	class PrecisionInterface : public InterfaceVersion1
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
	};
}
