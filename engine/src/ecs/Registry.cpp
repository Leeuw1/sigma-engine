#include "Registry.h"

namespace sge::ecs
{
	Registry::Registry()
		: m_AvailableEntityID(0), m_Size(0), m_Components(nullptr)
	{
	}

	Registry::~Registry()
	{
		if (m_Components)
			free(m_Components);
	}

	EntityID Registry::NewEntityID()
	{
		EntityID id = m_AvailableEntityID;
		m_AvailableEntityID++;
		return id;
	}

	ComponentID Registry::HashString(const std::string& string)
	{
		uint32_t result = 0;

		for (size_t i = 0; i < string.size(); i++)
			result += static_cast<uint32_t>(i) * static_cast<uint32_t>(string[i]); // Widen each byte to 32 bits

		return result;
	}
} // namespace sge::ecs