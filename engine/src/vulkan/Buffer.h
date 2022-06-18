#pragma once

#include "base.h"
#include "BufferLayout.h"

#include <vulkan/vulkan.h>

namespace sge::vulkan
{
	class Buffer
	{
	protected:
		VkBuffer m_BufferHandle;
		VkDeviceMemory m_DeviceMemory;
#ifdef DEBUG
		bool m_CleanedUp = false;
#endif //DEBUG
	public:
		Buffer(VkDevice device, VkPhysicalDevice physicalDevice, VkBufferUsageFlags usageFlags, size_t size);
#ifdef DEBUG
		~Buffer()
		{
			SGE_ASSERTM(m_CleanedUp, "Vulkan buffer was not cleaned up.");
		}
#endif
		void Destroy(VkDevice device);
	public:
		static void CopyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue transferQueue, VkBuffer dest, VkBuffer source, size_t size);
	public:
		inline VkBuffer GetBufferHandle() const { return m_BufferHandle; }
		inline VkDeviceMemory GetDeviceMemory() const { return m_DeviceMemory; }
	};

	class VertexBuffer : public Buffer
	{
	private:
		BufferLayout m_Layout;
		uint32_t m_Count;
	public:
		VertexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue transferQueue, const float* vertexData,
			size_t size, const BufferLayout& layout);
		void Bind(VkCommandBuffer commandBuffer);
	public:
		inline uint32_t GetCount() const { return m_Count; }
		inline const BufferLayout* GetLayout() const { return &m_Layout; }
	};

	class IndexBuffer : public Buffer
	{
	private:
		uint32_t m_Count;
	public:
		IndexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue transferQueue, const uint32_t* indexData, size_t size);
		void Bind(VkCommandBuffer commandBuffer);
	public:
		inline uint32_t GetCount() const { return m_Count; }
	};
	
	class UniformBuffer : public Buffer
	{
	public:
		UniformBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const void* uniformData, size_t size);
		void Upload(VkDevice device, const void* uniformData, size_t size);
	public:
		inline size_t GetSize() const { return m_Size; }
	private:
		size_t m_Size;
	};

	VkDescriptorPool CreateDescriptorPool(VkDevice device);
} // namespace sge::vulkan