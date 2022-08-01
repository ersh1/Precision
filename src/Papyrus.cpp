#include "Papyrus.h"

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

	void Register()
	{
		auto papyrus = SKSE::GetPapyrusInterface();
		papyrus->Register(Precision_MCM::Register);
		logger::info("Registered papyrus functions");
	}
}
