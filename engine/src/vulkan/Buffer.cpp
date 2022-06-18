#include "Buffer.h"
#include "BufferLayout.h"

#include "Util.h"

namespace sge::vulkan
{
	Buffer::Buffer(VkDevice device, VkPhysicalDevice physicalDevice, VkBufferUsageFlags usageFlags, size_t size)
		: m_BufferHandle(nullptr)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usageFlags;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_BufferHandle) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan buffer.");

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(device, m_BufferHandle, &memoryRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = size;
		allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		
		if (vkAllocateMemory(device, &allocInfo, nullptr, &m_DeviceMemory) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to allocate memory for Vulkan buffer.");

		vkBindBufferMemory(device, m_BufferHandle, m_DeviceMemory, 0);
	}
	
	void Buffer::Destroy(VkDevice device)
	{
		vkDestroyBuffer(device, m_BufferHandle, nullptr);
		vkFreeMemory(device, m_DeviceMemory, nullptr);

#ifdef DEBUG
		m_CleanedUp = true;
#endif // DEBUG
	}

	void Buffer::CopyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue transferQueue, VkBuffer dest, VkBuffer source, size_t size)
	{
		VkCommandBuffer commandBuffer = BeginOneTimeCommandBuffer(device, commandPool);

		VkBufferCopy bufferCopy = {};
		bufferCopy.size = size;

		vkCmdCopyBuffer(commandBuffer, source, dest, 1, &bufferCopy);
		
		EndOneTimeCommandBuffer(device, commandPool, commandBuffer, transferQueue);
	}

	// Note: 'transferQueue' can be a graphics queue because graphics queues support transfer operations
	VertexBuffer::VertexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue transferQueue,
		const float* vertexData, size_t size, const BufferLayout& layout)
		: Buffer(device, physicalDevice, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, size), m_Layout(layout)
	{
		m_Count = static_cast<uint32_t>(size / m_Layout.GetStride());

		Buffer stagingBuffer(device, physicalDevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size);
		void* data;
		vkMapMemory(device, stagingBuffer.GetDeviceMemory(), 0, size, 0, &data);
		memcpy(data, vertexData, size);
		vkUnmapMemory(device, stagingBuffer.GetDeviceMemory());

		CopyBuffer(device, commandPool, transferQueue, m_BufferHandle, stagingBuffer.GetBufferHandle(), size);

		stagingBuffer.Destroy(device);
	}

	void VertexBuffer::Bind(VkCommandBuffer commandBuffer)
	{
		VkDeviceSize offsets = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_BufferHandle, &offsets);
	}

	IndexBuffer::IndexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue transferQueue, const uint32_t* indexData, size_t size)
		: Buffer(device, physicalDevice, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, size)
	{
		m_Count = static_cast<uint32_t>(size / sizeof(uint32_t));

		Buffer stagingBuffer(device, physicalDevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size);
		void* data;
		vkMapMemory(device, stagingBuffer.GetDeviceMemory(), 0, size, 0, &data);
		memcpy(data, indexData, size);
		vkUnmapMemory(device, stagingBuffer.GetDeviceMemory());

		CopyBuffer(device, commandPool, transferQueue, m_BufferHandle, stagingBuffer.GetBufferHandle(), size);

		stagingBuffer.Destroy(device);
	}

	void IndexBuffer::Bind(VkCommandBuffer commandBuffer)
	{
		vkCmdBindIndexBuffer(commandBuffer, m_BufferHandle, 0, VK_INDEX_TYPE_UINT32);
	}

	UniformBuffer::UniformBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const void* uniformData, size_t size)
		: Buffer(device, physicalDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, size), m_Size(size)
	{
		Upload(device, uniformData, size);
	}

	void UniformBuffer::Upload(VkDevice device, const void* uniformData, size_t size)
	{
		void* data;
		vkMapMemory(device, m_DeviceMemory, 0, size, 0, &data);
		memcpy(data, uniformData, size);
		vkUnmapMemory(device, m_DeviceMemory);
	}

	VkDescriptorPool CreateDescriptorPool(VkDevice device)
	{
		constexpr uint32_t descriptorCount = 1000;

		VkDescriptorPoolSize poolSizes[2] = {};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = descriptorCount;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = descriptorCount;
		
		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 2;
		poolInfo.pPoolSizes = poolSizes;
		poolInfo.maxSets = descriptorCount;

		VkDescriptorPool descriptorPool;
		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan descriptor pool.");

		return descriptorPool;
	}
} // namespace sge::vulkan