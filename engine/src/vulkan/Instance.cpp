#include "Instance.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

namespace sge::vulkan
{
	Instance::Instance(GLFWwindow* window)
		: m_InstanceHandle(nullptr), m_WindowHandle(window), m_DebugMessenger(nullptr),
		m_Surface(nullptr), m_PhysicalDevice(nullptr), m_Device(nullptr), m_Swapchain(nullptr),
		//m_FramebufferResized(false),
		m_RenderPass(nullptr),
		//m_Pipeline(nullptr), m_PipelineLayout(nullptr),
		m_GraphicsQueue(nullptr), m_PresentQueue(nullptr), m_CommandPool(nullptr),
		m_CurrentFrame(0),
		m_Rotation0(glm::identity<glm::mat4>()),
		m_Rotation1(glm::identity<glm::mat4>()),
		m_Rotation2(glm::identity<glm::mat4>()),
		m_Rotation3(glm::identity<glm::mat4>())
	{
		InitInstance();
		std::cout << "Vulkan instance created.\n";
#ifdef SGE_USING_VALIDATION_LAYERS
		InitDebugMessenger();
		std::cout << "Vulkan debug messenger created.\n";
#endif // SGE_USING_VALIDATION_LAYERS
		InitSurface();
		std::cout << "Vulkan surface created.\n";
		
		InitPhysicalDevice();
		std::cout << "Vulkan physical device created.\n";
		InitLogicalDevice();
		std::cout << "Vulkan logical device created.\n";
		
		m_Swapchain = new Swapchain(m_Device, m_Surface, m_WindowHandle, QuerySwapchainSupport(m_PhysicalDevice, m_Surface), m_QueueFamilyIndices);
		std::cout << "Vulkan swap chain created.\n";
		m_Swapchain->InitImageViews(m_Device);
		std::cout << "Vulkan image views created.\n";
		InitRenderPass();
		std::cout << "Vulkan render pass created.\n";

		InitUniformBuffers();
		InitDescriptorSets();

		InitGraphicsPipeline();
		std::cout << "Vulkan graphics pipeline created.\n";
		m_Swapchain->InitFramebuffers(m_Device, m_RenderPass);
		std::cout << "Vulkan framebuffers created.\n";
		InitCommandPool();
		std::cout << "Vulkan command pool created.\n";
		InitVertexBuffer();
		std::cout << "Vulkan vertex buffer created.\n";
		InitCommandBuffers();
		std::cout << "Vulkan command buffer created.\n";
		InitSyncObjects();
		std::cout << "Vulkan semaphores and fences created.\n";
	}

	Instance::~Instance()
	{
		vkDeviceWaitIdle(m_Device);

		m_Pipeline->Destroy(m_Device);
		delete m_Pipeline;

		m_Swapchain->Destroy(m_Device);
		delete m_Swapchain;

		m_VertexBuffer->Destroy(m_Device);
		delete m_VertexBuffer;

		m_IndexBuffer->Destroy(m_Device);
		delete m_IndexBuffer;

		for (auto uniformBuffer : m_UniformBuffers)
		{
			uniformBuffer->Destroy(m_Device);
			delete uniformBuffer;
		}

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

	void Instance::OnUpdate()
	{
		vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(m_Device, m_Swapchain->GetSwapchainHandle(), UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], nullptr, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			ReInitSwapchain();
			return;
		}
		SGE_ASSERTM(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Failed to acquire swap chain image.");
		
		vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

		vkResetCommandBuffer(m_CommandBuffers[m_CurrentFrame], 0);
		RecordCommandBuffer(m_CommandBuffers[m_CurrentFrame], imageIndex);

		UpdateUniformBuffer(m_CurrentFrame);

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

		if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to submit Vulkan draw command buffer.");

		VkSwapchainKHR swapchains[] = { m_Swapchain->GetSwapchainHandle() };
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrame];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &imageIndex;

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
		appInfo.pApplicationName = "Sigma Game Engine";
		appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appInfo.pEngineName = "No Engine";
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
#else
		deviceCreateInfo.enabledLayerCount = 0;
#endif // SGE_USING_VALIDATION_LAYERS

		if (vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan logical device.");

		vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.PresentFamily.value(), 0, &m_PresentQueue);
	}

	void Instance::ReInitSwapchain()
	{
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

		m_Pipeline->Destroy(m_Device);
		delete m_Pipeline;
		
		InitUniformBuffers();

		InitGraphicsPipeline();
		std::cout << "Vulkan graphics pipeline created.\n";
		
		//InitFramebuffers();
		m_Swapchain->InitFramebuffers(m_Device, m_RenderPass);
		std::cout << "Vulkan framebuffers created.\n";
		InitCommandPool();
		std::cout << "Vulkan command pool created.\n";
		InitVertexBuffer();
		std::cout << "Vulkan vertex buffer created.\n";
		InitCommandBuffers();
		std::cout << "Vulkan command buffer created.\n";
		InitSyncObjects();
		std::cout << "Vulkan semaphores and fences created.\n";
	}

	void Instance::InitGraphicsPipeline()
	{
		// Create shader stages
		std::vector<char> vertexShaderBinary = LoadShaderBinary("E:/C++/sigma-engine/engine/shaders/shader.vert.spv");
		std::cout << "Vertex shader binary size: " << vertexShaderBinary.size() << " bytes.\n";
		std::vector<char> fragShaderBinary = LoadShaderBinary("E:/C++/sigma-engine/engine/shaders/shader.frag.spv");
		std::cout << "Fragment shader binary size: " << fragShaderBinary.size() << " bytes.\n";

		VkShaderModule vertexShaderModule = CreateShaderModule(vertexShaderBinary);
		VkShaderModule fragShaderModule = CreateShaderModule(fragShaderBinary);
		
		m_Pipeline = new Pipeline(m_Device, m_RenderPass, vertexShaderModule, fragShaderModule, m_Swapchain, m_VertexBuffer, m_DescriptorSetLayouts);

		vkDestroyShaderModule(m_Device, vertexShaderModule, nullptr);
		vkDestroyShaderModule(m_Device, fragShaderModule, nullptr);
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

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &colorAttachment;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(m_Device, &createInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan render pass.");
	}

	/*
	void Instance::InitFramebuffers()
	{
		m_Framebuffers.resize(m_SwapchainImageViews.size());

		for (size_t i = 0; i < m_SwapchainImageViews.size(); i++)
		{
			VkImageView attachment = m_SwapchainImageViews[i];

			VkFramebufferCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			createInfo.renderPass = m_RenderPass;
			createInfo.attachmentCount = 1;
			createInfo.pAttachments = &attachment;
			createInfo.width = m_SwapchainExtent.width;
			createInfo.height = m_SwapchainExtent.height;
			createInfo.layers = 1;

			if (vkCreateFramebuffer(m_Device, &createInfo, nullptr, &m_Framebuffers[i]))
				SGE_DEBUG_BREAKM("Failed to create Vulkan framebuffer.");
		}
	}
	*/

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

	void Instance::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo cmdBeginInfo = {};
		cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to begin recording Vulkan command buffer.");

		VkRenderPassBeginInfo renderBeginInfo = {};
		renderBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderBeginInfo.renderPass = m_RenderPass;
		renderBeginInfo.framebuffer = m_Swapchain->FramebufferAt(imageIndex);
		renderBeginInfo.renderArea.offset = { 0, 0 };
		renderBeginInfo.renderArea.extent = m_Swapchain->GetExtent();
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderBeginInfo.clearValueCount = 1;
		renderBeginInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffer, &renderBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		m_Pipeline->Bind(commandBuffer);
		m_VertexBuffer->Bind(commandBuffer);
		m_IndexBuffer->Bind(commandBuffer);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetLayout(),
			0, 1, &m_DescriptorSets[m_CurrentFrame], 0, nullptr);

		m_Rotation0 = glm::rotate(m_Rotation0, 0.0002f, glm::vec3(0.0f, 0.0f, 1.0f)); // Temporary
		m_Rotation1 = glm::rotate(m_Rotation1, 0.0003f, glm::vec3(0.0f, 0.0f, 1.0f)); // Temporary
		m_Rotation2 = glm::rotate(m_Rotation2, 0.0004f, glm::vec3(0.0f, 0.0f, 1.0f)); // Temporary
		m_Rotation3 = glm::rotate(m_Rotation3, 0.0005f, glm::vec3(0.0f, 0.0f, 1.0f)); // Temporary
		auto matrix = glm::perspectiveRH(glm::half_pi<float>(), 800.0f / 600.0f, 0.0f, 10.0f);
		vkCmdPushConstants(commandBuffer, m_Pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0,
			static_cast<uint32_t>(sizeof(glm::mat4)), glm::value_ptr(matrix));
		//vkCmdDraw(commandBuffer, 3, 1, 0, 0);
		vkCmdDrawIndexed(commandBuffer, 3, 4, 0, 0, 0);
		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to record command buffer.");
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

	void Instance::InitVertexBuffer()
	{
		float vertices[] = {
			0.0f, -0.5f, 1.0f, 0.0f, 0.0f,
			0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
			-0.5f, 0.5f, 0.0f, 0.0f, 1.0f
		};

		m_VertexBuffer = new VertexBuffer(m_Device, m_PhysicalDevice, m_CommandPool, m_GraphicsQueue, vertices, 15 * sizeof(float));

		uint32_t indices[] = { 0, 1, 2 };
		m_IndexBuffer = new IndexBuffer(m_Device, m_PhysicalDevice, m_CommandPool, m_GraphicsQueue, indices, 3 * sizeof(uint32_t));
	}

	void Instance::InitUniformBuffers()
	{
		TestUniformBuffer uBuffer = {
			m_Rotation0,
			m_Rotation1,
			m_Rotation2,
			m_Rotation3
		};
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			m_UniformBuffers[i] = new UniformBuffer(m_Device, m_PhysicalDevice, &uBuffer, UNIFORM_BUFFER_SIZE);
	}

	void Instance::UpdateUniformBuffer(uint32_t index)
	{
		TestUniformBuffer uBuffer = {
			m_Rotation0,
			m_Rotation1,
			m_Rotation2,
			m_Rotation3
		};
		m_UniformBuffers[index]->Upload(m_Device, &uBuffer, UNIFORM_BUFFER_SIZE);
	}

	void Instance::InitDescriptorSets()
	{
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			m_DescriptorSetLayouts[i] = m_UniformBuffers[i]->GetDescriptorSetLayout();

		m_DescriptorPool = CreateDescriptorPool(m_Device, MAX_FRAMES_IN_FLIGHT);

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
		allocInfo.pSetLayouts = m_DescriptorSetLayouts.data();
		allocInfo.descriptorPool = m_DescriptorPool;

		if (vkAllocateDescriptorSets(m_Device, &allocInfo, m_DescriptorSets.data()) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to allocate Vulkan descriptor sets.");

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = m_UniformBuffers[i]->GetBufferHandle();
			bufferInfo.offset = 0;
			bufferInfo.range = UNIFORM_BUFFER_SIZE;

			VkWriteDescriptorSet descriptorWrite = {};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_DescriptorSets[i];
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(m_Device, 1, &descriptorWrite, 0, nullptr);
		}
	}

	VkShaderModule Instance::CreateShaderModule(const std::vector<char>& binary)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = binary.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(binary.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(m_Device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan shader module.");

		return shaderModule;
	}
} // namespace sge::vulkan