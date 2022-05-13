#pragma once

#include "Event.h"

namespace sge
{
	class KeyPressedEvent : public Event
	{
	private:
		int m_Keycode;
	public:
		KeyPressedEvent(int keycode)
			: Event(EventType::KeyPressed), m_Keycode(keycode) {}

		inline int GetKeyCode() const { return m_Keycode; }
	};

	class KeyReleasedEvent : public Event
	{
	private:
		int m_Keycode;
	public:
		KeyReleasedEvent(int keycode)
			: Event(EventType::KeyReleased), m_Keycode(keycode) {}

		inline int GetKeyCode() const { return m_Keycode; }
	};
} // namespace sge