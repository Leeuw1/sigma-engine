#include "Shader.h"
#include "Util.h"

namespace sge::vulkan
{
	Shader::Shader(VkDevice device, VkDescriptorPool descriptorPool, const std::string& vertPath, const std::string& fragPath)
		: m_VertShaderModule(nullptr), m_FragShaderModule(nullptr)
	{
		auto vertBinary = LoadShaderBinary(vertPath);
		auto fragBinary = LoadShaderBinary(fragPath);

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = vertBinary.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(vertBinary.data());

		if (vkCreateShaderModule(device, &createInfo, nullptr, &m_VertShaderModule) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan shader module.");

		createInfo.codeSize = fragBinary.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(fragBinary.data());

		if (vkCreateShaderModule(device, &createInfo, nullptr, &m_FragShaderModule) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan shader module.");
	}

	void Shader::Destroy(VkDevice device)
	{
		vkDestroyShaderModule(device, m_VertShaderModule, nullptr);
		vkDestroyShaderModule(device, m_FragShaderModule, nullptr);

#ifdef DEBUG
		m_CleanedUp = true;
#endif // DEBUG
	}
} // namespace sge::vulkan