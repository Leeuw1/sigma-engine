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

		if (indices.IsComplete() && isSwapchainSuitable)
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
} // namespace sge::vulkan