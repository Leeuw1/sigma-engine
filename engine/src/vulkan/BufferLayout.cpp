#include "BufferLayout.h"

namespace sge::vulkan
{
	static constexpr size_t SizeofAttribute(AttributeType type)
	{
		switch (type)
		{
		case _Float:
			return sizeof(float);
		case _Vec2:
			return 2 * sizeof(float);
		case _Vec3:
			return 3 * sizeof(float);
		default:
			return 0;
		}
	};

	BufferLayout::BufferLayout()
		: m_Stride(0) {}

	BufferLayout::BufferLayout(std::initializer_list<AttributeType>&& attribTypes)
		: m_Stride(0)
	{
		m_Attribs.reserve(attribTypes.size() * sizeof(Attribute));
		for (auto type : attribTypes)
			AddAttribute(type);
	}

	void BufferLayout::AddAttribute(AttributeType attribType)
	{
		m_Attribs.emplace_back(attribType, m_Stride);
		m_Stride += SizeofAttribute(attribType);
	}

	VkVertexInputBindingDescription BufferLayout::GetBindingDescription() const
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = static_cast<uint32_t>(m_Stride);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	std::vector<VkVertexInputAttributeDescription> BufferLayout::GetAttributeDescriptions() const
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(m_Attribs.size());

		for (size_t i = 0; i < attributeDescriptions.size(); i++)
		{
			attributeDescriptions[i].binding = 0;
			attributeDescriptions[i].location = static_cast<uint32_t>(i);
			attributeDescriptions[i].format = static_cast<VkFormat>(m_Attribs[i].Type);
			attributeDescriptions[i].offset = static_cast<uint32_t>(m_Attribs[i].Offset);
		}

		return attributeDescriptions;
	}

	bool BufferLayout::operator==(const BufferLayout& other)
	{
		for (size_t i = 0; i < m_Attribs.size(); i++)
		{
			if (m_Attribs[i].Type != other.m_Attribs[i].Type)
				return false;
		}

		return true;
	}
} // namespace sge::vulkan