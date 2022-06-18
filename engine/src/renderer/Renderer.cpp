#include "Renderer.h"
#include "vulkan/Pipeline.h"

#include <imgui/backends/imgui_impl_vulkan.h>

namespace sge
{
	Renderer::Renderer(vulkan::Instance* vulkanInstance)
		: m_VulkanInstance(vulkanInstance)
	{
	}

	uint32_t Renderer::BeginFrame()
	{
		uint32_t imageIndex = m_VulkanInstance->AcquireNextSwapchainImage();

		if (imageIndex == UINT_MAX)
			SGE_DEBUG_BREAKM("Swapchain needs to be recreated.");
		
		VkCommandBuffer commandBuffer = m_VulkanInstance->GetCurrentCommandBuffer();

		m_VulkanInstance->BeginRenderPass(commandBuffer, imageIndex);

		return imageIndex;
	}
	
	void Renderer::EndFrame(uint32_t imageIndex)
	{
		m_VulkanInstance->EndRenderPass(m_VulkanInstance->GetCurrentCommandBuffer());

		// ImGui
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_VulkanInstance->GetCurrentCommandBuffer());

		m_VulkanInstance->Present(&imageIndex);
	}

	void Renderer::DrawScene(Scene& scene)
	{
		scene.m_Registry.ForEach<DrawableComponent>(
		[this](DrawableComponent* drawableComp)
		{
			DrawMesh(drawableComp->Mesh, drawableComp->Material, 1);
		});
	}

	void Renderer::DrawMesh(const Mesh& mesh, const Material& material, uint32_t instanceCount)
	{
		m_VulkanInstance->DrawIndexed(m_VulkanInstance->GetCurrentCommandBuffer(), material.m_PipelineIndex,
			mesh.m_VertexBuffer, mesh.m_IndexBuffer, material.m_Shader, instanceCount);
	}
} // namespace sge