#include "Pipeline.h"
#include "base.h"

#include <glm/mat4x4.hpp>

namespace sge::vulkan
{
	Pipeline::Pipeline(VkDevice device, VkRenderPass renderPass, VkShaderModule vertexShaderModule, VkShaderModule fragShaderModule,
		Swapchain* swapchain, VertexBuffer* vertexBuffer, std::optional<std::array<VkDescriptorSetLayout, 2>> descriptorSetLayouts)
		: m_PipelineHandle(nullptr), m_Layout(nullptr)
	{
		// Vertex shader stage
		VkPipelineShaderStageCreateInfo vertexShaderStageInfo = {};
		vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderStageInfo.module = vertexShaderModule;
		vertexShaderStageInfo.pName = "main";

		// Fragment shader stage
		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStageInfos[2] = { vertexShaderStageInfo, fragShaderStageInfo };

		// Vertex input
		auto vertexInputBinding = vertexBuffer->GetBindingDescription();
		auto vertexInputAttributes = vertexBuffer->GetAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &vertexInputBinding;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributes.data();

		// Input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
		inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		// Viewport
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapchain->GetExtent().width);
		viewport.height = static_cast<float>(swapchain->GetExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = swapchain->GetExtent();

		VkPipelineViewportStateCreateInfo viewportInfo = {};
		viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportInfo.viewportCount = 1;
		viewportInfo.pViewports = &viewport;
		viewportInfo.scissorCount = 1;
		viewportInfo.pScissors = &scissor;

		// Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizationInfo = {};
		rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationInfo.depthClampEnable = VK_FALSE;
		rasterizationInfo.depthBiasEnable = VK_FALSE;
		rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT; // Enable backface culling
		rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationInfo.lineWidth = 1.0f;

		// Multisampling
		VkPipelineMultisampleStateCreateInfo multisampleInfo = {};
		multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleInfo.sampleShadingEnable = VK_FALSE;
		multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// Blending
		VkPipelineColorBlendAttachmentState blendAttachment = {};
		blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachment.blendEnable = VK_FALSE; // No Blending for now

		VkPipelineColorBlendStateCreateInfo blendInfo = {};
		blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		blendInfo.logicOpEnable = VK_FALSE;
		blendInfo.logicOp = VK_LOGIC_OP_COPY;
		blendInfo.attachmentCount = 1;
		blendInfo.pAttachments = &blendAttachment;

		// Pipeline layout
		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.offset = 0;
		pushConstantRange.size = static_cast<uint32_t>(sizeof(glm::mat4));
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		// TODO: Add arguments for this step
		if (descriptorSetLayouts.has_value())
		{
			pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts->size());
			pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts->data();
		}

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_Layout) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan pipeline layout.");

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStageInfos;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
		pipelineInfo.pViewportState = &viewportInfo;
		pipelineInfo.pRasterizationState = &rasterizationInfo;
		pipelineInfo.pDynamicState = VK_NULL_HANDLE;
		pipelineInfo.pDepthStencilState = VK_NULL_HANDLE;
		pipelineInfo.pMultisampleState = &multisampleInfo;
		pipelineInfo.pColorBlendState = &blendInfo;
		pipelineInfo.layout = m_Layout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &m_PipelineHandle) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan graphics pipeline.");
	}

	void Pipeline::Destroy(VkDevice device)
	{
		vkDestroyPipelineLayout(device, m_Layout, nullptr);
		vkDestroyPipeline(device, m_PipelineHandle, nullptr);

#ifdef DEBUG
		m_CleanedUp = true;
#endif // DEBUG
	}

	void Pipeline::Bind(VkCommandBuffer commandBuffer)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineHandle);
	}
} // namespace sge::vulkan