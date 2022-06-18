#pragma once

#include "Swapchain.h"
#include "BufferLayout.h"
#include "Shader.h"
#include "base.h"

#include <array>

namespace sge::vulkan
{
	class Pipeline
	{
	private:
		VkPipeline m_PipelineHandle;
		VkPipelineLayout m_Layout;
		BufferLayout m_BufferLayout;
#ifdef DEBUG
		bool m_CleanedUp = false;
#endif // DEBUG
	public:
		Pipeline(VkDevice device, VkRenderPass renderPass, Shader* shader, Swapchain* swapchain,
			BufferLayout vertexBufferLayout, VkDescriptorSetLayout descriptorSetLayout); // TODO: Use ref or pointer of buffer layout
#ifdef DEBUG
		~Pipeline()
		{
			//SGE_ASSERTM(m_CleanedUp, "Vulkan pipeline was not cleaned up.");
		}
#endif // DEBUG
		void Destroy(VkDevice device);
		void Bind(VkCommandBuffer commandBuffer);
	public:
		inline VkPipelineLayout GetLayout() const { return m_Layout; }
		inline BufferLayout GetBufferLayout() const { return m_BufferLayout; }
	};
} // namespace sge::vulkan