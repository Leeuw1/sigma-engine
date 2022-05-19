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
		VkCommandBuffer commandBuffer = BeginOneTimeCommandBuffer(device, commandPool);

		VkBufferCopy bufferCopy = {};
		bufferCopy.size = size;

		vkCmdCopyBuffer(commandBuffer, source, dest, 1, &bufferCopy);
		
		EndOneTimeCommandBuffer(device, commandPool, commandBuffer, transferQueue);
	}

	// Note: 'transferQueue' can be a graphics queue because it supports transfer operations
	VertexBuffer::VertexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue transferQueue, const float* vertexData, size_t stride, size_t count)
		: Buffer(device, physicalDevice, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, count * stride), m_Stride(stride)
	{
		size_t size = count * stride;

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
		//bindingDescription.stride = static_cast<uint32_t>(m_Stride);
		bindingDescription.stride = static_cast<uint32_t>(6 * sizeof(float)); // TEMPORARY
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	std::array<VkVertexInputAttributeDescription, VERTEX_ATTRIB_COUNT> VertexBuffer::GetAttributeDescriptions() const
	{
		std::array<VkVertexInputAttributeDescription, VERTEX_ATTRIB_COUNT> attributeDescriptions = {};
		// Position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = 0;

		// Normal
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = 3 * sizeof(float);

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

	// TODO: Make ImGui work
	VkDescriptorPool CreateDescriptorPool(VkDevice device, uint32_t descriptorCount)
	{
		descriptorCount = 1000; // testing

		VkDescriptorPoolSize poolSizes[2] = {};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = descriptorCount;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = descriptorCount;
		/*
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};
		*/
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