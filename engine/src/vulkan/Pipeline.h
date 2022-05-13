#pragma once

#include "Buffer.h"
#include "Swapchain.h"

namespace sge::vulkan
{
	class Pipeline
	{
	private:
		VkPipeline m_PipelineHandle;
		VkPipelineLayout m_Layout;
#ifdef DEBUG
		bool m_CleanedUp = false;
#endif // DEBUG
	public:
		Pipeline(VkDevice device, VkRenderPass renderPass, VkShaderModule vertexShaderModule, VkShaderModule fragShaderModule,
			Swapchain* swapchain, VertexBuffer* vertexBuffer, std::optional<std::array<VkDescriptorSetLayout, 2>> descriptorSetLayouts);
#ifdef DEBUG
		~Pipeline()
		{
			SGE_ASSERTM(m_CleanedUp, "Vulkan pipeline was not cleaned up.");
		}
#endif // DEBUG
		void Destroy(VkDevice device);
		void Bind(VkCommandBuffer commandBuffer);
	public:
		inline VkPipelineLayout GetLayout() const { return m_Layout; }
	};
} // namespace sge::vulkan