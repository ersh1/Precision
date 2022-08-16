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
		return PrecisionHandler::GetSingleton()->GetAttackCollisionReach(a_actorHandle, a_collisionType);
	}

	APIResult PrecisionInterface::AddWeaponWeaponCollisionCallback(SKSE::PluginHandle a_pluginHandle, WeaponCollisionCallback&& a_callback) noexcept
	{
		if (PrecisionHandler::GetSingleton()->AddWeaponWeaponCollisionCallback(a_pluginHandle, a_callback)) {
			return APIResult::OK;
		} else {
			return APIResult::AlreadyRegistered;
		}
	}

	APIResult PrecisionInterface::RemoveWeaponWeaponCollisionCallback(SKSE::PluginHandle a_pluginHandle) noexcept
	{
		if (PrecisionHandler::GetSingleton()->RemoveWeaponWeaponCollisionCallback(a_pluginHandle)) {
			return APIResult::OK;
		} else {
			return APIResult::NotRegistered;
		}
	}

	APIResult PrecisionInterface::AddWeaponProjectileCollisionCallback(SKSE::PluginHandle a_pluginHandle, WeaponCollisionCallback&& a_callback) noexcept
	{
		if (PrecisionHandler::GetSingleton()->AddWeaponProjectileCollisionCallback(a_pluginHandle, a_callback)) {
			return APIResult::OK;
		} else {
			return APIResult::AlreadyRegistered;
		}
	}

	APIResult PrecisionInterface::RemoveWeaponProjectileCollisionCallback(SKSE::PluginHandle a_pluginHandle) noexcept
	{
		if (PrecisionHandler::GetSingleton()->RemoveWeaponProjectileCollisionCallback(a_pluginHandle)) {
			return APIResult::OK;
		} else {
			return APIResult::NotRegistered;
		}
	}

	void PrecisionInterface::ApplyHitImpulse(RE::ActorHandle a_actorHandle, RE::hkpRigidBody* a_rigidBody, const RE::NiPoint3& a_hitVelocity, const RE::hkVector4& a_hitPosition, float a_impulseMult) noexcept
	{		
		PrecisionHandler::GetSingleton()->ApplyHitImpulse(a_actorHandle, a_rigidBody, a_hitVelocity, a_hitPosition, a_impulseMult, true);
	}

}
