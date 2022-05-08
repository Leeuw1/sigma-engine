#include "Instance.h"

#include <iostream>
#include <fstream>
#include <algorithm>

namespace sge::vulkan
{
	Instance::Instance(GLFWwindow* window)
		: m_InstanceHandle(nullptr), m_DebugMessenger(nullptr), m_Surface(nullptr),
		m_PhysicalDevice(nullptr), m_Device(nullptr), m_Swapchain(nullptr),
		m_RenderPass(nullptr), m_Pipeline(nullptr), m_PipelineLayout(nullptr),
		m_GraphicsQueue(nullptr), m_PresentQueue(nullptr), m_CommandPool(nullptr),
		m_CommandBuffer(nullptr), m_ImageAvailableSemaphore(nullptr),
		m_RenderFinishedSemaphore(nullptr), m_InFlightFence(nullptr)
	{
		InitInstance();
		std::cout << "Vulkan instance created.\n";
#ifdef SGE_USING_VALIDATION_LAYERS
		InitDebugMessenger();
		std::cout << "Vulkan debug messenger created.\n";
#endif // SGE_USING_VALIDATION_LAYERS
		InitSurface(window);
		std::cout << "Vulkan surface created.\n";
		InitPhysicalDevice();
		std::cout << "Vulkan physical device created.\n";
		InitLogicalDevice();
		std::cout << "Vulkan logical device created.\n";
		InitSwapchain(window);
		std::cout << "Vulkan swap chain created.\n";

		vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.PresentFamily.value(), 0, &m_PresentQueue);

		InitImageViews();
		std::cout << "Vulkan image views created.\n";
		InitRenderPass();
		std::cout << "Vulkan render pass created.\n";
		InitGraphicsPipeline();
		std::cout << "Vulkan graphics pipeline created.\n";
		InitFramebuffers();
		std::cout << "Vulkan framebuffers created.\n";
		InitCommandPool();
		std::cout << "Vulkan command pool created.\n";
		InitCommandBuffer();
		std::cout << "Vulkan command buffer created.\n";
		InitSyncObjects();
		std::cout << "Vulkan semaphores and fences created.\n";
	}

	Instance::~Instance()
	{
		Destroy();
	}

	void Instance::Destroy()
	{
		vkDeviceWaitIdle(m_Device);

#ifdef SGE_USING_VALIDATION_LAYERS
			DestroyDebugUtilsMessenger(m_InstanceHandle, m_DebugMessenger, nullptr);
#endif // SGE_USING_VALIDATION_LAYERS

		vkDestroySemaphore(m_Device, m_ImageAvailableSemaphore, nullptr);
		vkDestroySemaphore(m_Device, m_RenderFinishedSemaphore, nullptr);
		vkDestroyFence(m_Device, m_InFlightFence, nullptr);

		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

		for (auto framebuffer : m_Framebuffers)
			vkDestroyFramebuffer(m_Device, framebuffer, nullptr);

		vkDestroySurfaceKHR(m_InstanceHandle, m_Surface, nullptr);

		vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
		vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);

		vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

		vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);

		for (auto imageView : m_SwapchainImageViews)
			vkDestroyImageView(m_Device, imageView, nullptr);

		vkDestroyDevice(m_Device, nullptr);

		vkDestroyInstance(m_InstanceHandle, nullptr);
	}

	void Instance::OnUpdate()
	{
		vkWaitForFences(m_Device, 1, &m_InFlightFence, VK_TRUE, UINT64_MAX);
		vkResetFences(m_Device, 1, &m_InFlightFence);

		uint32_t imageIndex;
		vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX, m_ImageAvailableSemaphore, nullptr, &imageIndex);
		vkResetCommandBuffer(m_CommandBuffer, 0);
		RecordCommandBuffer(m_CommandBuffer, imageIndex);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &m_ImageAvailableSemaphore;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_RenderFinishedSemaphore;

		if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFence) != VK_SUCCESS)
			SGE_ASSERTM(false, "Failed to submit Vulkan draw command buffer.");

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_Swapchain;
		presentInfo.pImageIndices = &imageIndex;

		vkQueuePresentKHR(m_PresentQueue, &presentInfo);
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
#else
		createInfo.enabledLayerCount = 0;
#endif // SGE_USING_VALIDATION_LAYERS

		if (vkCreateInstance(&createInfo, nullptr, &m_InstanceHandle) != VK_SUCCESS)
			SGE_ASSERTM(false, "Failed to create Vulkan instance.");
	}

#ifdef SGE_USING_VALIDATION_LAYERS
	void Instance::InitDebugMessenger()
	{
		m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };

		if (!ValidationLayersSupported(m_ValidationLayers))
			SGE_ASSERTM(false, "Validation layers not supported.");

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
			SGE_ASSERTM(false, "Could not create Vulkan debug messenger.");
	}
#endif // SGE_USING_VALIDATION_LAYERS

	void Instance::InitSurface(GLFWwindow* window)
	{
		if (glfwCreateWindowSurface(m_InstanceHandle, window, nullptr, &m_Surface) != VK_SUCCESS)
			SGE_ASSERTM(false, "Failed to create Vulkan surface.");
	}

	void Instance::InitPhysicalDevice()
	{
		m_DeviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_InstanceHandle, &deviceCount, nullptr);
		if (deviceCount == 0)
			SGE_ASSERTM(false, "No GPUs with Vulkan support found.");
		
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
			SGE_ASSERTM(false, "No suitable GPUs found.");
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
			SGE_ASSERTM(false, "Failed to create Vulkan logical device.");
	}

	void Instance::InitSwapchain(GLFWwindow* window)
	{
		SwapchainSupportDetails details = QuerySwapchainSupport(m_PhysicalDevice, m_Surface);

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(details.Formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(details.PresentModes);
		VkExtent2D extent = ChooseSwapExtent(details.Capabilities, window);

		uint32_t imageCount = details.Capabilities.minImageCount + 1;
		if (details.Capabilities.maxImageCount > 0 && imageCount > details.Capabilities.maxImageCount)
			imageCount = details.Capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Surface;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.presentMode = presentMode;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (m_QueueFamilyIndices.GraphicsFamily != m_QueueFamilyIndices.PresentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
		}
		else
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

		createInfo.preTransform = details.Capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_Swapchain) != VK_SUCCESS)
			SGE_ASSERTM(false, "Failed to create Vulkan swap chain.");

		vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr);
		m_SwapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_SwapchainImages.data());
		m_SwapchainImageFormat = surfaceFormat.format;
		m_SwapchainExtent = extent;
	}

	void Instance::InitImageViews()
	{
		m_SwapchainImageViews.resize(m_SwapchainImages.size());
		for (size_t i = 0; i < m_SwapchainImages.size(); i++)
		{
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = m_SwapchainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = m_SwapchainImageFormat;

			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(m_Device, &createInfo, nullptr, &m_SwapchainImageViews[i]) != VK_SUCCESS)
				SGE_ASSERTM(false, "Failed to create Vulkan image view.");
		}
	}

	void Instance::InitGraphicsPipeline()
	{
		// Create shader stages
		std::vector<char> vertexShaderBinary = LoadShaderBinary("E:/C++/sigma-engine/engine/shaders/shader.vert.spv");
		std::cout << "Vertex shader binary size: " << vertexShaderBinary.size() << "bytes.\n";
		std::vector<char> fragShaderBinary = LoadShaderBinary("E:/C++/sigma-engine/engine/shaders/shader.frag.spv");
		std::cout << "Fragment shader binary size: " << fragShaderBinary.size() << "bytes.\n";

		VkShaderModule vertexShaderModule = CreateShaderModule(vertexShaderBinary);
		VkShaderModule fragShaderModule = CreateShaderModule(fragShaderBinary);

		VkPipelineShaderStageCreateInfo vertexStageCreateInfo = {};
		vertexStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexStageCreateInfo.module = vertexShaderModule;
		vertexStageCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragStageCreateInfo = {};
		fragStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragStageCreateInfo.module = fragShaderModule;
		fragStageCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[2] = { vertexStageCreateInfo, fragStageCreateInfo };

		// Vertex input
		VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
		vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
		vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;

		// Input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
		inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

		// Viewport
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_SwapchainExtent.width);
		viewport.height = static_cast<float>(m_SwapchainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = m_SwapchainExtent;

		VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
		viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportCreateInfo.viewportCount = 1;
		viewportCreateInfo.pViewports = &viewport;
		viewportCreateInfo.scissorCount = 1;
		viewportCreateInfo.pScissors = &scissor;

		// Resterizer
		VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
		rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizerCreateInfo.depthClampEnable = VK_FALSE;
		rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizerCreateInfo.lineWidth = 1.0f;
		rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

		// Multisampling
		VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
		multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
		multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		//Blending
		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo blendCreateInfo = {};
		blendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		blendCreateInfo.logicOpEnable = VK_FALSE;
		blendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
		blendCreateInfo.attachmentCount = 1;
		blendCreateInfo.pAttachments = &colorBlendAttachment;
		blendCreateInfo.blendConstants[0] = 0.0f;
		blendCreateInfo.blendConstants[1] = 0.0f;
		blendCreateInfo.blendConstants[2] = 0.0f;
		blendCreateInfo.blendConstants[3] = 0.0f;

		// Dynamic state
		VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };
		
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
		dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCreateInfo.dynamicStateCount = 2;
		dynamicStateCreateInfo.pDynamicStates = dynamicStates;

		// Pipeline layout
		VkPipelineLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		if (vkCreatePipelineLayout(m_Device, &layoutCreateInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
			SGE_ASSERTM(false, "Failed to create Vulkan pipeline layout.");

		// Create pipeline
		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stageCount = 2;
		pipelineCreateInfo.pStages = shaderStages;
		pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
		pipelineCreateInfo.pViewportState = &viewportCreateInfo;
		pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
		pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
		pipelineCreateInfo.pDepthStencilState = nullptr;
		pipelineCreateInfo.pColorBlendState = &blendCreateInfo;
		pipelineCreateInfo.pDynamicState = nullptr;
		pipelineCreateInfo.layout = m_PipelineLayout;
		pipelineCreateInfo.renderPass = m_RenderPass;
		pipelineCreateInfo.subpass = 0;

		if (vkCreateGraphicsPipelines(m_Device, nullptr, 1, &pipelineCreateInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
			SGE_ASSERTM(false, "Failed to create Vulkan graphics pipeline.");
		
		vkDestroyShaderModule(m_Device, vertexShaderModule, nullptr);
		vkDestroyShaderModule(m_Device, fragShaderModule, nullptr);
	}

	void Instance::InitRenderPass()
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = m_SwapchainImageFormat;
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
			SGE_ASSERTM(false, "Failed to create Vulkan render pass.");
	}

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
				SGE_ASSERTM(false, "Failed to create Vulkan framebuffer.");
		}
	}

	void Instance::InitCommandPool()
	{
		VkCommandPoolCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		createInfo.queueFamilyIndex = m_QueueFamilyIndices.GraphicsFamily.value();

		if (vkCreateCommandPool(m_Device, &createInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
			SGE_ASSERTM(false, "Failed to create Vulkan command pool.");
	}

	void Instance::InitCommandBuffer()
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(m_Device, &allocInfo, &m_CommandBuffer) != VK_SUCCESS)
			SGE_ASSERTM(false, "Failed to create Vulkan command buffer.");
	}

	void Instance::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo cmdBeginInfo = {};
		cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo) != VK_SUCCESS)
			SGE_ASSERTM(false, "Failed to begin recording Vulkan command buffer.");

		VkRenderPassBeginInfo renderBeginInfo = {};
		renderBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderBeginInfo.renderPass = m_RenderPass;
		renderBeginInfo.framebuffer = m_Framebuffers[imageIndex];
		renderBeginInfo.renderArea.offset = { 0, 0 };
		renderBeginInfo.renderArea.extent = m_SwapchainExtent;
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderBeginInfo.clearValueCount = 1;
		renderBeginInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffer, &renderBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);
		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
			SGE_ASSERTM(false, "Failed to record command buffer.");
	}

	void Instance::InitSyncObjects()
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphore) != VK_SUCCESS ||
			vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_InFlightFence) != VK_SUCCESS)
			SGE_ASSERTM(false, "Failed to create Vulkan sync objects.");
	}

	std::vector<const char*> GetRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
#ifdef SGE_USING_VALIDATION_LAYERS
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // SGE_USING_VALIDATION_LAYERS
		return extensions;
	}

#ifdef SGE_USING_VALIDATION_LAYERS
	static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
		void* userData)
	{
		if (!(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT))
			std::cerr << "Validation layer: " << callbackData->pMessage << '\n';
		return VK_FALSE;
	}

	VkResult CreateDebugUtilsMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
		const VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT* debugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func)
			return func(instance, createInfo, allocator, debugMessenger);

		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	void DestroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* allocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func)
			func(instance, debugMessenger, allocator);
	}

	bool ValidationLayersSupported(const std::vector<const char*>& validationLayers)
	{
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> layers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;

			for (const auto& layer : layers)
			{
				if (strcmp(layerName, layer.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
				return false;
		}

		return true;
	}
#endif // SGE_USING_VALIDATION_LAYERS

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (!indices.GraphicsFamily.has_value() && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.GraphicsFamily = i;

			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			
			if (!indices.PresentFamily.has_value() && presentSupport)
				indices.PresentFamily = i;

			i++;
		}

		return indices;
	}

	// Check if physical device is suitable and write queue family indices to queueFamilyIndices
	bool IsSuitablePhysicalDevice(VkPhysicalDevice device, VkSurfaceKHR surface,
		const std::vector<const char*>& requiredExtensions, QueueFamilyIndices& queueFamilyIndices)
	{
		QueueFamilyIndices indices = FindQueueFamilies(device, surface);
		
		bool extensionSupport = CheckDeviceExtensionSupport(device, requiredExtensions);
		bool isSwapchainSuitable = false;
		if (extensionSupport)
		{
			SwapchainSupportDetails details = QuerySwapchainSupport(device, surface);
			isSwapchainSuitable = !details.Formats.empty() && !details.PresentModes.empty();
		}

		if (indices.IsComplete() && isSwapchainSuitable)
		{
			queueFamilyIndices = indices;
			return true;
		}

		return false;
	}

	void ListExtensions()
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::cout << extensionCount << " extensions supported.\n";
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		std::cout << "Available extensions:\n";
		for (const auto& e : extensions)
			std::cout << '\t' << e.extensionName << '\n';
	}


	bool CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& extensions)
	{
		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		uint32_t supportedExtensionCount = 0;
		for (const char* extension : extensions)
		{
			for (const auto& extensionProperties : availableExtensions)
			{
				if (strcmp(extensionProperties.extensionName, extension) == 0)
				{
					supportedExtensionCount++;
					break;
				}
			}
		}

		return supportedExtensionCount == static_cast<uint32_t>(extensions.size());
	}

	SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		SwapchainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.Capabilities);

		uint32_t count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);
		if (count)
		{
			details.Formats.resize(count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, details.Formats.data());
		}

		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);
		if (count)
		{
			details.PresentModes.resize(count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, details.PresentModes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& format : availableFormats)
		{
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return format;
		}

		return availableFormats[0];
	}
	
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availableModes)
	{
		for (const auto& mode : availableModes)
		{
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
				return mode;
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
	{
		if (capabilities.currentExtent.width != UINT_MAX)
			return capabilities.currentExtent;
		
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}

	std::vector<char> LoadShaderBinary(const std::string& filepath)
	{
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);
		SGE_ASSERTF(file.is_open(), "Could not open file '%s'.", filepath.c_str());
		
		size_t size = static_cast<size_t>(file.tellg());
		file.seekg(0);
		std::vector<char> binary(size);
		file.read(binary.data(), size);
		
		return binary;
	}

	VkShaderModule Instance::CreateShaderModule(const std::vector<char>& binary)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = binary.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(binary.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(m_Device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
			SGE_ASSERTM(false, "Failed to create Vulkan shader module.");

		return shaderModule;
	}
} // namespace sge::vulkan