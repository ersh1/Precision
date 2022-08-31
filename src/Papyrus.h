#pragma once

namespace Papyrus
{
	class Precision_MCM
	{
	public:
		static void OnConfigClose(RE::TESQuest*);

		static bool Register(RE::BSScript::IVirtualMachine* a_vm);
	};

	class Precision_Utility
	{
	public:
		static bool IsActorActive(RE::StaticFunctionTag*, RE::Actor* a_actor);
		static bool ToggleDisableActor(RE::StaticFunctionTag*, RE::Actor* a_actor, bool a_bDisable);

		static bool Register(RE::BSScript::IVirtualMachine* a_vm);
	};

	void Register();
}
