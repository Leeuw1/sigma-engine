#pragma once

#include "base.h"

#include <vulkan/vulkan.h>

#include <array>

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
	public:
		VertexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue transferQueue, const float* vertexData, size_t size);
		void Bind(VkCommandBuffer commandBuffer);
		VkVertexInputBindingDescription GetBindingDescription() const;
		std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions() const;
	};

	class IndexBuffer : public Buffer
	{
	public:
		IndexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue transferQueue, const uint32_t* indexData, size_t size);
		void Bind(VkCommandBuffer commandBuffer);
	};
	
	class UniformBuffer : public Buffer
	{
	private:
		VkDescriptorSetLayout m_DescriptorSetLayout;
	public:
		UniformBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const void* uniformData, size_t size);
		void Destroy(VkDevice device);
		void Upload(VkDevice device, const void* uniformData, size_t size);
		inline VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }
	};

	VkDescriptorPool CreateDescriptorPool(VkDevice device, uint32_t descriptorCount);
} // namespace sge::vulkan