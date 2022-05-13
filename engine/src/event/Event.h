#pragma once

namespace sge
{
	enum class EventType
	{
		None = 0,
		KeyPressed,
		KeyReleased
	};

	class Event
	{
	protected:
		EventType m_EventType;
	public:
		Event(EventType type)
			: m_EventType(type) {}

		inline EventType GetType() const { return m_EventType; }
	};
} // namespace sge