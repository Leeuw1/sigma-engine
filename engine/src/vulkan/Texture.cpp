#include "Texture.h"
#include "Buffer.h"
#include "Util.h"

#include <stb_image/stb_image.h>

// TODO: Interpret normal map data properly

namespace sge::vulkan
{
	Texture::Texture(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue,
		const std::string& filepath)
		: m_Image(nullptr), m_ImageMemory(nullptr), m_ImageView(nullptr), m_Sampler(nullptr)
	{
		// Load image
		stbi_set_flip_vertically_on_load(1);
		int width, height, channels;
		constexpr int desiredChannels = 4;
		constexpr VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

		uint8_t* imageData = stbi_load(filepath.c_str(), &width, &height, &channels, desiredChannels);
		SGE_ASSERTF(imageData, "Failed to load texture '%s'.", filepath.c_str());
		size_t size = static_cast<size_t>(desiredChannels * width * height);
		SGE_INFOF("Size of '%s': %d bytes.", filepath.c_str(), size);

		Buffer stagingBuffer(device, physicalDevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size);
		void* data;
		vkMapMemory(device, stagingBuffer.GetDeviceMemory(), 0, size, 0, &data);
		memcpy(data, imageData, size);
		vkUnmapMemory(device, stagingBuffer.GetDeviceMemory());

		stbi_image_free(imageData);

		CreateImage(device, physicalDevice, static_cast<uint32_t>(width), static_cast<uint32_t>(height), imageFormat,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_Image, &m_ImageMemory);

		TransitionImageLayout(device, commandPool, graphicsQueue, m_Image, imageFormat,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(device, commandPool, graphicsQueue, stagingBuffer.GetBufferHandle(), m_Image,
			static_cast<uint32_t>(width), static_cast<uint32_t>(height));
		TransitionImageLayout(device, commandPool, graphicsQueue, m_Image, imageFormat,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		stagingBuffer.Destroy(device);

		// Create image view
		m_ImageView = CreateImageView(device, m_Image, imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

		// Create sampler
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.magFilter = VK_FILTER_LINEAR;

		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		//samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		//samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		//samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.anisotropyEnable = VK_TRUE;

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		if (vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan sampler.");
	}

	void Texture::Destroy(VkDevice device)
	{
		vkDestroySampler(device, m_Sampler, nullptr);
		vkDestroyImageView(device, m_ImageView, nullptr);
		vkDestroyImage(device, m_Image, nullptr);
		vkFreeMemory(device, m_ImageMemory, nullptr);

#ifdef DEBUG
		m_CleanedUp = true;
#endif // DEBUG
	}
} // namespace sge::vulkan