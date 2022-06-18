#include "Util.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <fstream>

namespace sge::vulkan
{
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
	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
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

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		constexpr VkQueueFlags desiredFlags = VK_QUEUE_GRAPHICS_BIT;
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (!indices.GraphicsFamily.has_value() && queueFamily.queueFlags & desiredFlags)
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

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(device, &features);

		if (indices.IsComplete() && isSwapchainSuitable && features.samplerAnisotropy)
		{
			queueFamilyIndices = indices;
			return true;
		}

		return false;
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

	void CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
		VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkImage* image, VkDeviceMemory* deviceMemory)
	{
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;

		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;

		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usageFlags;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(device, &imageInfo, nullptr, image) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan image.");

		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(device, *image, &memoryRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memoryRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, memoryPropertyFlags);

		if (vkAllocateMemory(device, &allocInfo, nullptr, deviceMemory) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to allocate Vulkan memory.");

		vkBindImageMemory(device, *image, *deviceMemory, 0);
	}

	VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
	{
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;

		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan image view.");

		return imageView;
	}

	void CopyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer = BeginOneTimeCommandBuffer(device, commandPool);

		VkBufferImageCopy imageCopy = {};
		imageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopy.imageSubresource.mipLevel = 0;
		imageCopy.imageSubresource.baseArrayLayer = 0;
		imageCopy.imageSubresource.layerCount = 1;
		imageCopy.imageOffset = { 0, 0, 0 };
		imageCopy.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

		EndOneTimeCommandBuffer(device, commandPool, commandBuffer, graphicsQueue);
	}

	uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags flags)
	{
		VkPhysicalDeviceMemoryProperties properties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);

		for (uint32_t i = 0; i < properties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (properties.memoryTypes[i].propertyFlags & flags) == flags)
				return i;
		}

		SGE_DEBUG_BREAKM("Failed to find suitable memory type.");
	}

	VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
	{
		for (VkFormat format : formats)
		{
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

			if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
				return format;

			if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
				return format;
		}

		SGE_DEBUG_BREAKM("Failed to find supported format.");
	}

	VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice)
	{
		return FindSupportedFormat(physicalDevice, { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT },
			VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	bool HasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	VkCommandBuffer BeginOneTimeCommandBuffer(VkDevice device, VkCommandPool commandPool)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to allocate Vulkan command buffer.");

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void EndOneTimeCommandBuffer(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(queue, 1, &submitInfo, nullptr);
		vkQueueWaitIdle(queue);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	void TransitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkImage image,
		VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer commandBuffer = BeginOneTimeCommandBuffer(device, commandPool);

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = image;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.layerCount = 1;
		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (HasStencilComponent(format))
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		VkPipelineStageFlags srcStageFlags;
		VkPipelineStageFlags dstStageFlags;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			srcStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else
			SGE_DEBUG_BREAKM("Unsupported layout transition.");

		vkCmdPipelineBarrier(commandBuffer, srcStageFlags, dstStageFlags, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		EndOneTimeCommandBuffer(device, commandPool, commandBuffer, graphicsQueue);
	}

	glm::mat4 MakePerspective(float fovY, float aspect, float zNear, float zFar)
	{
		const float tan = std::tan(fovY / 2.0f);

		glm::mat4 perspective = {};
		perspective[0][0] = 1.0f / (aspect * tan);
		perspective[1][1] = 1.0f / tan;
		perspective[2][2] = zFar / (zFar - zNear);
		perspective[3][2] = (-zFar * zNear) / (zFar - zNear);
		perspective[2][3] = 1.0f;

		return perspective;
	}
} // namespace sge::vulkan