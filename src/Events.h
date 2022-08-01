#pragma once

namespace Events
{
	using EventResult = RE::BSEventNotifyControl;

	class InputEventHandler : public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		static InputEventHandler* GetSingleton();
		static void Register();

		virtual EventResult ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>* a_eventSource) override;

	private:
		enum : std::uint32_t
		{
			kInvalid = static_cast<std::uint32_t>(-1),
			kKeyboardOffset = 0,
			kMouseOffset = 256,
			kGamepadOffset = 266
		};

		InputEventHandler() = default;
		InputEventHandler(const InputEventHandler&) = delete;
		InputEventHandler(InputEventHandler&&) = delete;
		virtual ~InputEventHandler() = default;

		InputEventHandler& operator=(const InputEventHandler&) = delete;
		InputEventHandler& operator=(InputEventHandler&&) = delete;

		std::uint32_t GetGamepadIndex(RE::BSWin32GamepadDevice::Key a_key);
	};

	void SinkEventHandlers();
}
