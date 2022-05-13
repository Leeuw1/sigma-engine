#include "Buffer.h"

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
			SGE_DEBUG_BREAKM("Failed to create Vulkan vertex buffer.");

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(device, m_BufferHandle, &memoryRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = size;
		allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &m_DeviceMemory) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to allocate memory for vertex buffer.");

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
		VkCommandBufferAllocateInfo commandBufferInfo = {};
		commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferInfo.commandPool = commandPool;
		commandBufferInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		if (vkAllocateCommandBuffers(device, &commandBufferInfo, &commandBuffer) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan command buffer.");

		VkCommandBufferBeginInfo commandBeginInfo = {};
		commandBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkBufferCopy bufferCopy = {};
		bufferCopy.size = size;

		vkBeginCommandBuffer(commandBuffer, &commandBeginInfo);
		vkCmdCopyBuffer(commandBuffer, source, dest, 1, &bufferCopy);
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(transferQueue, 1, &submitInfo, nullptr);
		vkQueueWaitIdle(transferQueue);
	}

	// Note: 'transferQueue' can be a graphics queue because it supports transfer operations
	VertexBuffer::VertexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue transferQueue, const float* vertexData, size_t size)
		: Buffer(device, physicalDevice, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, size)
	{
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

	VkVertexInputBindingDescription VertexBuffer::GetBindingDescription() const
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = static_cast<uint32_t>(5 * sizeof(float));//sizeof(TestVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	std::array<VkVertexInputAttributeDescription, 2> VertexBuffer::GetAttributeDescriptions() const
	{
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = 0;

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = 2 * sizeof(float);

		return attributeDescriptions;
	}

	IndexBuffer::IndexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue transferQueue, const uint32_t* indexData, size_t size)
		: Buffer(device, physicalDevice, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, size)
	{
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
		: Buffer(device, physicalDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, size)
	{
		VkDescriptorSetLayoutBinding layoutBinding = {};
		layoutBinding.binding = 0;
		layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBinding.descriptorCount = 1;
		layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &layoutBinding;

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan descriptor set layout.");

		Upload(device, uniformData, size);
	}

	void UniformBuffer::Destroy(VkDevice device)
	{
		vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
		this->Buffer::Destroy(device);
	}

	void UniformBuffer::Upload(VkDevice device, const void* uniformData, size_t size)
	{
		void* data;
		vkMapMemory(device, m_DeviceMemory, 0, size, 0, &data);
		memcpy(data, uniformData, size);
		vkUnmapMemory(device, m_DeviceMemory);
	}

	VkDescriptorPool CreateDescriptorPool(VkDevice device, uint32_t descriptorCount)
	{
		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = descriptorCount;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = descriptorCount;

		VkDescriptorPool descriptorPool;
		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan descriptor pool.");

		return descriptorPool;
	}
} // namespace sge::vulkan