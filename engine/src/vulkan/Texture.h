#pragma once

#include "base.h"

#include <vulkan/vulkan.h>

#include <string>

namespace sge::vulkan
{
	class Texture
	{
	public:
		Texture(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue,
			const std::string& filepath);
		void Destroy(VkDevice device);
#ifdef DEBUG
		~Texture()
		{
			SGE_ASSERTM(m_CleanedUp, "Texture was not cleaned up.");
		}
#endif // DEBUG
	public:
		inline VkImageView GetImageView() const { return m_ImageView; }
		inline VkSampler GetSampler() const { return m_Sampler; }
	private:
#ifdef DEBUG
		bool m_CleanedUp = false;
#endif // DEBUG
		VkImage m_Image;
		VkDeviceMemory m_ImageMemory;
		VkImageView m_ImageView;
		VkSampler m_Sampler;
	};

} // sge::vulkan