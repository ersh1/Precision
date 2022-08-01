#include "ModAPI.h"
#include "PrecisionHandler.h"

namespace Messaging
{
	APIResult PrecisionInterface::AddPreHitCallback(SKSE::PluginHandle a_pluginHandle, PreHitCallback&& a_preHitCallback) noexcept
	{
		if (PrecisionHandler::GetSingleton()->AddPreHitCallback(a_pluginHandle, a_preHitCallback)) {
			return APIResult::OK;
		} else {
			return APIResult::AlreadyRegistered;
		}
	}

	APIResult PrecisionInterface::AddPostHitCallback(SKSE::PluginHandle a_pluginHandle, PostHitCallback&& a_postHitCallback) noexcept
	{
		if (PrecisionHandler::GetSingleton()->AddPostHitCallback(a_pluginHandle, a_postHitCallback)) {
			return APIResult::OK;
		} else {
			return APIResult::AlreadyRegistered;
		}
	}

	APIResult PrecisionInterface::AddPrePhysicsStepCallback(SKSE::PluginHandle a_pluginHandle, PrePhysicsStepCallback&& a_prePhysicsStepCallback) noexcept
	{
		if (PrecisionHandler::GetSingleton()->AddPrePhysicsStepCallback(a_pluginHandle, a_prePhysicsStepCallback)) {
			return APIResult::OK;
		} else {
			return APIResult::AlreadyRegistered;
		}
	}

	APIResult PrecisionInterface::AddCollisionFilterComparisonCallback(SKSE::PluginHandle a_pluginHandle, CollisionFilterComparisonCallback&& a_collisionFilterComparisonCallback) noexcept
	{
		if (PrecisionHandler::GetSingleton()->AddCollisionFilterComparisonCallback(a_pluginHandle, a_collisionFilterComparisonCallback)) {
			return APIResult::OK;
		} else {
			return APIResult::AlreadyRegistered;
		}
	}

	APIResult PrecisionInterface::RemovePreHitCallback(SKSE::PluginHandle a_pluginHandle) noexcept
	{
		if (PrecisionHandler::GetSingleton()->RemovePreHitCallback(a_pluginHandle)) {
			return APIResult::OK;
		} else {
			return APIResult::NotRegistered;
		}
	}

	APIResult PrecisionInterface::RemovePostHitCallback(SKSE::PluginHandle a_pluginHandle) noexcept
	{
		if (PrecisionHandler::GetSingleton()->RemovePostHitCallback(a_pluginHandle)) {
			return APIResult::OK;
		} else {
			return APIResult::NotRegistered;
		}
	}

	APIResult PrecisionInterface::RemovePrePhysicsStepCallback(SKSE::PluginHandle a_pluginHandle) noexcept
	{
		if (PrecisionHandler::GetSingleton()->RemovePrePhysicsStepCallback(a_pluginHandle)) {
			return APIResult::OK;
		} else {
			return APIResult::NotRegistered;
		}
	}

	APIResult PrecisionInterface::RemoveCollisionFilterComparisonCallback(SKSE::PluginHandle a_pluginHandle) noexcept
	{
		if (PrecisionHandler::GetSingleton()->RemoveCollisionFilterComparisonCallback(a_pluginHandle)) {
			return APIResult::OK;
		} else {
			return APIResult::NotRegistered;
		}
	}

	float PrecisionInterface::GetAttackCollisionCapsuleLength(RE::ActorHandle a_actorHandle, RequestedAttackCollisionType a_collisionType /*= RequestedAttackCollisionType::Default*/) const noexcept
	{
		return PrecisionHandler::GetSingleton()->GetAttackCollisionCapsuleLength(a_actorHandle, a_collisionType);
	}
}
