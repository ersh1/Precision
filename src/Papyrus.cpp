#include "Papyrus.h"

#include "PrecisionHandler.h"
#include "Settings.h"

namespace Papyrus
{
	void Precision_MCM::OnConfigClose(RE::TESQuest*)
	{
		Settings::ReadSettings();
	}

	bool Precision_MCM::Register(RE::BSScript::IVirtualMachine* a_vm)
	{
		a_vm->RegisterFunction("OnConfigClose", "Precision_MCM", OnConfigClose);

		logger::info("Registered Precision_MCM class");
		return true;
	}

	bool Precision_Utility::IsActorActive(RE::StaticFunctionTag*, RE::Actor* a_actor)
	{
		if (a_actor) {
			return PrecisionHandler::IsActorActive(a_actor->GetHandle());
		}

		return false;
	}

	bool Precision_Utility::ToggleDisableActor(RE::StaticFunctionTag*, RE::Actor* a_actor, bool a_bDisable)
	{
		if (a_actor) {
			return PrecisionHandler::ToggleDisableActor(a_actor->GetHandle(), a_bDisable);
		}

		return false;
	}

	bool Precision_Utility::Register(RE::BSScript::IVirtualMachine* a_vm)
	{
		a_vm->RegisterFunction("IsActorActive", "Precision_Utility", IsActorActive);
		a_vm->RegisterFunction("ToggleDisableActor", "Precision_Utility", ToggleDisableActor);

		logger::info("Registered Precision_Utility class");
		return true;
	}

	void Register()
	{
		auto papyrus = SKSE::GetPapyrusInterface();
		papyrus->Register(Precision_MCM::Register);
		papyrus->Register(Precision_Utility::Register);
		logger::info("Registered papyrus functions");
	}
}
