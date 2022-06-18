#pragma once

#include "base.h"
#include "Buffer.h"
#include "Texture.h"
#include "FrameGroup.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace sge::vulkan
{
	class Shader
	{
	public:
		Shader(VkDevice device, VkDescriptorPool descriptorPool, const std::string& vertPath, const std::string& fragPath);
		void Destroy(VkDevice device);
#ifdef DEBUG
		~Shader()
		{
			SGE_ASSERTM(m_CleanedUp, "Shader was not cleaned up.");
		}
#endif // DEBUG
	private:
		VkShaderModule m_VertShaderModule;
		VkShaderModule m_FragShaderModule;
#ifdef DEBUG
		bool m_CleanedUp = false;
#endif // DEBUG

		friend class Pipeline;
	};
} // namespace sge::vulkan