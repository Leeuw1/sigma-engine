#pragma once

#include "base.h"

#include <glm/mat4x4.hpp>
#include <vulkan/vulkan.h>

#include <vector>
#include <optional>

namespace sge::vulkan
{
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> GraphicsFamily;
		std::optional<uint32_t> PresentFamily;

		inline bool IsComplete()
		{
			return GraphicsFamily.has_value() && PresentFamily.has_value();
		}
	};

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;
	};

	std::vector<const char*> GetRequiredExtensions();

#ifdef SGE_USING_VALIDATION_LAYERS
	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
		void* userData);
	VkResult CreateDebugUtilsMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
		const VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT* debugMessenger);
	void DestroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* allocator);
	bool ValidationLayersSupported(const std::vector<const char*>& validationLayers);
#endif // SGE_USING_VALIDATION_LAYERS
	void ListExtensions();
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& extensions);

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
	bool IsSuitablePhysicalDevice(VkPhysicalDevice device, VkSurfaceKHR surface,
		const std::vector<const char*>& requiredExtensions, QueueFamilyIndices& queueFamilyIndices);
	SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
	void CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
		VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkImage* image, VkDeviceMemory* deviceMemory);
	VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void CopyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	std::vector<char> LoadShaderBinary(const std::string& filepath);
	uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags flags);
	VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
	VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice);
	bool HasStencilComponent(VkFormat format);
	void TransitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkImage image,
		VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	VkCommandBuffer BeginOneTimeCommandBuffer(VkDevice device, VkCommandPool commandPool);
	void EndOneTimeCommandBuffer(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue);

	glm::mat4 MakePerspective(float fovY, float aspect, float zNear, float zFar);
} // namespace sge::vulkan