#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace sge::vulkan
{
	enum AttributeType
	{
		_Float = VK_FORMAT_R32_SFLOAT,
		_Vec2 = VK_FORMAT_R32G32_SFLOAT,
		_Vec3 = VK_FORMAT_R32G32B32_SFLOAT
	};
	
	struct Attribute
	{
		AttributeType Type;
		size_t Offset;
	};

	class BufferLayout
	{
	public:
		BufferLayout();
		BufferLayout(std::initializer_list<AttributeType>&& attribTypes);
		void AddAttribute(AttributeType attribType);
		VkVertexInputBindingDescription GetBindingDescription() const;
		std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() const;
		bool operator==(const BufferLayout& other);
	public:
		inline size_t GetStride() const { return m_Stride; }
		inline const std::vector<Attribute>& GetAttributes() const { return m_Attribs; }
		inline void Resize(size_t size) { m_Attribs.resize(size); }
	private:
		size_t m_Stride;
		std::vector<Attribute> m_Attribs;
	};
} // namespace sge::vulkan