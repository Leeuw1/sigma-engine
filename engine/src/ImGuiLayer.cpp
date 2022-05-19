#include "ImGuiLayer.h"

#include "vulkan/Util.h"

#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace sge
{
	ImGuiLayer::ImGuiLayer(vulkan::Instance* vulkanInstance)
		: m_VulkanInstance(vulkanInstance)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		int windowWidth, windowHeight;
		glfwGetWindowSize(m_VulkanInstance->GetWindowHandle(), &windowWidth, &windowHeight);
		io.DisplaySize = { static_cast<float>(windowWidth), static_cast<float>(windowHeight) };

		ImGui::StyleColorsDark();

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGuiStyle& style = ImGui::GetStyle();
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImGui_ImplGlfw_InitForVulkan(m_VulkanInstance->GetWindowHandle(), true);

		ImGui_ImplVulkan_InitInfo vulkanInfo = {};
		vulkanInfo.Instance = m_VulkanInstance->GetInstanceHandle();
		vulkanInfo.PhysicalDevice = m_VulkanInstance->GetPhysicalDevice();
		vulkanInfo.Device = m_VulkanInstance->GetDevice();
		vulkanInfo.Queue = m_VulkanInstance->GetGraphicsQueue();
		vulkanInfo.QueueFamily = m_VulkanInstance->GetQueueFamilyIndices().GraphicsFamily.value();
		vulkanInfo.DescriptorPool = m_VulkanInstance->GetDescriptorPool();
		vulkanInfo.MinImageCount = m_VulkanInstance->GetSwapchainImageCount();
		vulkanInfo.ImageCount = m_VulkanInstance->GetSwapchainImageCount();
		vulkanInfo.CheckVkResultFn = CheckVkResult;

		ImGui_ImplVulkan_Init(&vulkanInfo, m_VulkanInstance->GetRenderPass());

		// Upload fonts
		io.Fonts->AddFontDefault();
		VkCommandBuffer commandBuffer = vulkan::BeginOneTimeCommandBuffer(m_VulkanInstance->GetDevice(), m_VulkanInstance->GetCommandPool());
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
		vulkan::EndOneTimeCommandBuffer(m_VulkanInstance->GetDevice(), m_VulkanInstance->GetCommandPool(), commandBuffer, m_VulkanInstance->GetGraphicsQueue());
		ImGui_ImplVulkan_DestroyFontUploadObjects();

		ClearTextEntry();
	}

	ImGuiLayer::~ImGuiLayer()
	{
		vkDeviceWaitIdle(m_VulkanInstance->GetDevice());

		ImGui_ImplGlfw_Shutdown();
		ImGui_ImplVulkan_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::CheckVkResult(VkResult result)
	{
		SGE_ASSERTM(result == VK_SUCCESS, "VkResult was not VK_SUCCESS.");
	}

	void ImGuiLayer::OnUpdate()
	{
		NewFrame();
		DrawFrame();
	}

	void ImGuiLayer::NewFrame()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiLayer::ClearTextEntry()
	{
		memset(m_TextEntryBuffer, 0, TEXT_ENTRY_SIZE);
	}

	void ImGuiLayer::DrawFrame()
	{
		if (ImGui::Begin("Phong shader controls"))
		{
			ImGui::SetWindowSize({ 800.0f, 300.0f }, ImGuiCond_FirstUseEver);

			vulkan::PushConstant& push = m_VulkanInstance->GetPushConstant();
			ImGui::SliderFloat("Specular reflection constant", &push.ks, 0.0f, 1.0f, nullptr, 1.0f);
			ImGui::SliderFloat("Diffuse reflection constant", &push.kd, 0.0f, 1.0f, nullptr, 1.0f);
			ImGui::SliderFloat("Ambient reflection constant", &push.ka, 0.0f, 1.0f, nullptr, 1.0f);
			ImGui::SliderFloat("Shininess constant", &push.a, 0.0f, 10.0f, nullptr, 1.0f);

			ImGui::NewLine();
			ImGui::PushItemWidth(200.0f);
			ImGui::ColorPicker3("Colour", push.color);
		}
		ImGui::End();

		ImGui::Begin("Console");
		ImGui::SetWindowSize({ 800.0f, 300.0f }, ImGuiCond_FirstUseEver);
		ImGui::Button("Test button");
		if (ImGui::IsItemClicked())
			ImGui::GetCurrentContext()->LogBuffer.append("Test\n");

		ImGui::Text("Log:");
		ImGui::TextUnformatted(ImGui::GetCurrentContext()->LogBuffer.c_str());
		if (ImGui::InputText(" ", m_TextEntryBuffer, TEXT_ENTRY_SIZE, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			ImGui::GetCurrentContext()->LogBuffer.appendf("%s\n", m_TextEntryBuffer);
			ClearTextEntry();
		}
		ImGui::End();
	}
} // namespace sge