#include "Instance.h"
#include "FileUtil.h"
#include "BufferLayout.h"
#include "Shader.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui/backends/imgui_impl_vulkan.h>

#include <iostream>

#define SGE_CALL_VERBOSE(func) func; SGE_TRACE(#func)

namespace sge::vulkan
{
	Instance::Instance(GLFWwindow* window)
		: m_InstanceHandle(nullptr), m_WindowHandle(window),
#ifdef SGE_USING_VALIDATION_LAYERS
		m_DebugMessenger(nullptr),
#endif // SGE_USING_VALIDATION_LAYERS
		m_Surface(nullptr), m_PhysicalDevice(nullptr), m_Device(nullptr), m_Swapchain(nullptr),
		//m_FramebufferResized(false),
		m_RenderPass(nullptr),
		m_GraphicsQueue(nullptr), m_PresentQueue(nullptr), m_CommandPool(nullptr),
		m_CurrentFrame(0),
		m_PushConstant({ 1.0f, 1.0f, 1.0f, 1.0f, { 0.6f, 0.0f, 0.0f } }),
		m_DescriptorSetLayout(nullptr)
	{
		SGE_CALL_VERBOSE(InitInstance());

#ifdef SGE_USING_VALIDATION_LAYERS
		SGE_CALL_VERBOSE(InitDebugMessenger());
#endif // SGE_USING_VALIDATION_LAYERS

		SGE_CALL_VERBOSE(InitSurface());

		SGE_CALL_VERBOSE(InitPhysicalDevice());

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
		SGE_INFOF("Max push constant size: %u bytes.", properties.limits.maxPushConstantsSize);

		SGE_CALL_VERBOSE(InitLogicalDevice());

		m_Swapchain = new Swapchain(m_Device, m_Surface, m_WindowHandle, QuerySwapchainSupport(m_PhysicalDevice, m_Surface), m_QueueFamilyIndices);
		SGE_TRACE("Vulkan swap chain created.");
		m_Swapchain->InitImageViews(m_Device);
		SGE_TRACE("Vulkan image views created.");
		SGE_CALL_VERBOSE(InitRenderPass());

		SGE_CALL_VERBOSE(InitCommandPool());
		SGE_CALL_VERBOSE(InitDepthResources());

		m_DescriptorPool = CreateDescriptorPool(m_Device);
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			m_DescriptorSets[i] = nullptr;

		m_Swapchain->SGE_CALL_VERBOSE(InitFramebuffers(m_Device, m_RenderPass, m_DepthImageView));
		SGE_CALL_VERBOSE(InitCommandBuffers());

		SGE_CALL_VERBOSE(InitSyncObjects());
	}

	Instance::~Instance()
	{
		vkDeviceWaitIdle(m_Device);

		for (auto& pipeline : m_Pipelines)
			pipeline.Destroy(m_Device);

		m_Swapchain->Destroy(m_Device);
		delete m_Swapchain;

		// Depth resources
		vkDestroyImageView(m_Device, m_DepthImageView, nullptr);
		vkDestroyImage(m_Device, m_DepthImage, nullptr);
		vkFreeMemory(m_Device, m_DepthImageMemory, nullptr);

		vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
			vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
		}

#ifdef SGE_USING_VALIDATION_LAYERS
		DestroyDebugUtilsMessenger(m_InstanceHandle, m_DebugMessenger, nullptr);
#endif // SGE_USING_VALIDATION_LAYERS

		// This frees command buffers
		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

		vkDestroyDevice(m_Device, nullptr);
		vkDestroySurfaceKHR(m_InstanceHandle, m_Surface, nullptr);
		vkDestroyInstance(m_InstanceHandle, nullptr);
	}

	uint32_t Instance::AcquireNextSwapchainImage()
	{
		vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(m_Device, m_Swapchain->GetSwapchainHandle(), UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], nullptr, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			ReInitSwapchain();
			return UINT_MAX;
		}
		SGE_ASSERTM(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Failed to acquire swap chain image.");

		vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

		return imageIndex;
	}

	/*
	void Instance::DrawFrame()
	{
		uint32_t imageIndex = AcquireNextSwapchainImage();

		if (imageIndex == UINT_MAX)
			return;

		//UpdateUniformBuffer(m_CurrentFrame);

		// ImGui
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_CommandBuffers[m_CurrentFrame]);

		Present(&imageIndex);
	}
	*/

	void Instance::Present(uint32_t* imageIndex)
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &m_ImageAvailableSemaphores[m_CurrentFrame];
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffers[m_CurrentFrame];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrame];

		VkResult result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]);
		if (result != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to submit Vulkan draw command buffer.");

		VkSwapchainKHR swapchains[] = { m_Swapchain->GetSwapchainHandle() };
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrame];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = imageIndex;

		result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_Swapchain->GetFramebufferResized())
		{
			m_Swapchain->SetFramebufferResized(false);
			ReInitSwapchain();
		}
		else
			SGE_ASSERTM(result == VK_SUCCESS, "Failed to present swap chain image.");

		m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void Instance::InitInstance()
	{
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Sigma Game Engine Demo";
		appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appInfo.pEngineName = "Sigma Game Engine";
		appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;

		std::vector<const char*> requiredExtensions = GetRequiredExtensions();

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();
#ifdef SGE_USING_VALIDATION_LAYERS
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
		createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
#endif // SGE_USING_VALIDATION_LAYERS

		if (vkCreateInstance(&createInfo, nullptr, &m_InstanceHandle) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan instance.");
	}

#ifdef SGE_USING_VALIDATION_LAYERS
	void Instance::InitDebugMessenger()
	{
		m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };

		if (!ValidationLayersSupported(m_ValidationLayers))
			SGE_DEBUG_BREAKM("Validation layers not supported.");

		// Create debug messenger
		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = VulkanDebugCallback;
		createInfo.pUserData = nullptr;

		if (CreateDebugUtilsMessenger(m_InstanceHandle, &createInfo, nullptr, &m_DebugMessenger))
			SGE_DEBUG_BREAKM("Failed to create Vulkan debug messenger.");
	}
#endif // SGE_USING_VALIDATION_LAYERS

	void Instance::InitSurface()
	{
		if (glfwCreateWindowSurface(m_InstanceHandle, m_WindowHandle, nullptr, &m_Surface) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan surface.");
	}

	void Instance::InitPhysicalDevice()
	{
		m_DeviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_InstanceHandle, &deviceCount, nullptr);
		if (deviceCount == 0)
			SGE_DEBUG_BREAKM("No GPUs with Vulkan support found.");

		std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
		vkEnumeratePhysicalDevices(m_InstanceHandle, &deviceCount, physicalDevices.data());

		for (const auto device : physicalDevices)
		{
			if (IsSuitablePhysicalDevice(device, m_Surface, m_DeviceExtensions, m_QueueFamilyIndices))
			{
				m_PhysicalDevice = device;
				break;
			}
		}
		if (!m_PhysicalDevice)
			SGE_DEBUG_BREAKM("No suitable GPUs found.");
	}

	void Instance::InitLogicalDevice()
	{
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		VkDeviceQueueCreateInfo graphicsQueueCreateInfo = {};
		graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		graphicsQueueCreateInfo.queueFamilyIndex = static_cast<uint32_t>(m_QueueFamilyIndices.GraphicsFamily.value());
		graphicsQueueCreateInfo.queueCount = 1;
		float queuePriority = 1.0f;
		graphicsQueueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(graphicsQueueCreateInfo);

		VkDeviceQueueCreateInfo presentQueueCreateInfo = {};
		presentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		presentQueueCreateInfo.queueFamilyIndex = static_cast<uint32_t>(m_QueueFamilyIndices.PresentFamily.value());
		presentQueueCreateInfo.queueCount = 1;
		presentQueueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(presentQueueCreateInfo);

		VkPhysicalDeviceFeatures features = {};
		features.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.pEnabledFeatures = &features;
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();
#ifdef SGE_USING_VALIDATION_LAYERS
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = m_ValidationLayers.data();
#endif // SGE_USING_VALIDATION_LAYERS

		if (vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan logical device.");

		vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.PresentFamily.value(), 0, &m_PresentQueue);
	}

	void Instance::ReInitSwapchain()
	{
		SGE_DEBUG_BREAK(); // TODO: Make this work

		// If window is minimized, wait until it isn't
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_WindowHandle, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(m_WindowHandle, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(m_Device);

		//DestroySwapchain();
		m_Swapchain->Destroy(m_Device);

		std::cout << "Recreating Vulkan swap chain...\n";

		delete m_Swapchain;
		m_Swapchain = new Swapchain(m_Device, m_Surface, m_WindowHandle, QuerySwapchainSupport(m_PhysicalDevice, m_Surface), m_QueueFamilyIndices);

		m_Swapchain->InitImageViews(m_Device);

		InitRenderPass();
		std::cout << "Vulkan render pass created.\n";

		for (auto& pipeline : m_Pipelines)
			pipeline.Destroy(m_Device);

		InitCommandPool();
		std::cout << "Vulkan command pool created.\n";

		m_Swapchain->InitFramebuffers(m_Device, m_RenderPass, m_DepthImageView);
		std::cout << "Vulkan framebuffers created.\n";
		InitCommandBuffers();
		std::cout << "Vulkan command buffer created.\n";
		InitSyncObjects();
		std::cout << "Vulkan semaphores and fences created.\n";
	}

	void Instance::InitRenderPass()
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = m_Swapchain->GetImageFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = FindDepthFormat(m_PhysicalDevice);
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		//depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };

		VkRenderPassCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = 2;
		createInfo.pAttachments = attachments;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(m_Device, &createInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan render pass.");
	}

	void Instance::InitDepthResources()
	{
		uint32_t indices[2] = { m_QueueFamilyIndices.GraphicsFamily.value(), m_QueueFamilyIndices.PresentFamily.value() };

		VkFormat depthFormat = FindDepthFormat(m_PhysicalDevice);
		CreateImage(m_Device, m_PhysicalDevice, m_Swapchain->GetExtent().width, m_Swapchain->GetExtent().height,
			depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_DepthImage, &m_DepthImageMemory);

		m_DepthImageView = CreateImageView(m_Device, m_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	void Instance::InitCommandPool()
	{
		VkCommandPoolCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		createInfo.queueFamilyIndex = m_QueueFamilyIndices.GraphicsFamily.value();

		if (vkCreateCommandPool(m_Device, &createInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan command pool.");
	}

	void Instance::InitCommandBuffers()
	{
		m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

		if (vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan command buffer.");
	}

	void Instance::BeginRenderPass(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		vkResetCommandBuffer(m_CommandBuffers[m_CurrentFrame], 0);

		VkCommandBufferBeginInfo cmdBeginInfo = {};
		cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to begin recording Vulkan command buffer.");

		VkClearValue clearValues[2] = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderBeginInfo = {};
		renderBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderBeginInfo.renderPass = m_RenderPass;
		renderBeginInfo.framebuffer = m_Swapchain->FramebufferAt(imageIndex);
		renderBeginInfo.renderArea.offset = { 0, 0 };
		renderBeginInfo.renderArea.extent = m_Swapchain->GetExtent();
		renderBeginInfo.clearValueCount = 2;
		renderBeginInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(commandBuffer, &renderBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void Instance::EndRenderPass(VkCommandBuffer commandBuffer)
	{
		vkCmdEndRenderPass(commandBuffer);
		
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to record command buffer.");
	}

	void Instance::DrawIndexed(VkCommandBuffer commandBuffer, uint32_t pipelineIndex, VertexBuffer* vertexBuffer, IndexBuffer* indexBuffer, Shader* shader,
		uint32_t instanceCount)
	{
		Pipeline* p = &m_Pipelines[pipelineIndex];

		p->Bind(commandBuffer);
		vertexBuffer->Bind(commandBuffer);
		indexBuffer->Bind(commandBuffer);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p->GetLayout(),
			0, 1, &m_DescriptorSets[m_CurrentFrame], 0, nullptr);

		vkCmdPushConstants(commandBuffer, p->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
			static_cast<uint32_t>(sizeof(PushConstant)), &m_PushConstant);

		vkCmdDrawIndexed(commandBuffer, indexBuffer->GetCount(), instanceCount, 0, 0, 0);
	}

	uint32_t Instance::CreatePipeline(Shader* shader, const BufferLayout* layout)
	{
		uint32_t index = static_cast<uint32_t>(m_Pipelines.size());
		m_Pipelines.emplace_back(m_Device, m_RenderPass, shader, m_Swapchain, *layout, m_DescriptorSetLayout);
		
		return index;
	}

	void Instance::InitSyncObjects()
	{
		m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS)
				SGE_DEBUG_BREAKM("Failed to create Vulkan sync objects.");
		}
	}

	void Instance::AddLayoutBindingUniformBuffer(std::vector<VkDescriptorSetLayoutBinding>& bindings)
	{
		VkDescriptorSetLayoutBinding binding = {};
		binding.binding = static_cast<uint32_t>(bindings.size());
		binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		binding.descriptorCount = 1;
		binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		bindings.push_back(binding);
	}

	void Instance::AddLayoutBindingTexture(std::vector<VkDescriptorSetLayoutBinding>& bindings)
	{
		VkDescriptorSetLayoutBinding binding = {};
		binding.binding = static_cast<uint32_t>(bindings.size());
		binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding.descriptorCount = 1;
		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		bindings.push_back(binding);
	}

	void Instance::AllocateDescriptorSets(std::vector<VkDescriptorSetLayoutBinding>& bindings)
	{
		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		// TEMP
		SGE_ASSERTM(!m_DescriptorSetLayout, "Only one descriptor set layout can be used currently.\n");

		if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan descriptor set layout.");

		// Set all layouts to newly created one
		FrameGroup<VkDescriptorSetLayout> layouts;
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			layouts[i] = m_DescriptorSetLayout;

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
		allocInfo.pSetLayouts = layouts.data();
		allocInfo.descriptorPool = m_DescriptorPool;

		if (vkAllocateDescriptorSets(m_Device, &allocInfo, m_DescriptorSets.data()) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to allocate Vulkan descriptor sets.");
	}

	void Instance::AddDescriptorWrite(std::vector<VkWriteDescriptorSet>& descriptorWrites, VkDescriptorBufferInfo* bufferInfo,
		uint32_t frameIndex)
	{
		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = m_DescriptorSets[frameIndex];
		write.dstBinding = static_cast<uint32_t>(descriptorWrites.size());
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.descriptorCount = 1;
		write.pBufferInfo = bufferInfo;

		descriptorWrites.push_back(write);
	}

	void Instance::AddDescriptorWrite(std::vector<VkWriteDescriptorSet>& descriptorWrites, VkDescriptorImageInfo* imageInfo,
		uint32_t frameIndex)
	{
		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = m_DescriptorSets[frameIndex];
		write.dstBinding = static_cast<uint32_t>(descriptorWrites.size());
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.descriptorCount = 1;
		write.pImageInfo = imageInfo;

		descriptorWrites.push_back(write);
	}

	VkDescriptorBufferInfo* Instance::GetBufferInfo(UniformBuffer* uniformBuffer)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = uniformBuffer->GetBufferHandle();
		bufferInfo.offset = 0;
		bufferInfo.range = uniformBuffer->GetSize();

		return new VkDescriptorBufferInfo(bufferInfo);
	}

	VkDescriptorImageInfo* Instance::GetImageInfo(Texture* texture)
	{
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = texture->GetImageView();
		imageInfo.sampler = texture->GetSampler();

		return new VkDescriptorImageInfo(imageInfo);
	}
} // namespace sge::vulkan