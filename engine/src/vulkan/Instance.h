#pragma once

#include "base.h"
#include "Window.h"

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

	class Instance
	{
	private:
		VkInstance m_InstanceHandle;
#ifdef SGE_USING_VALIDATION_LAYERS
		VkDebugUtilsMessengerEXT m_DebugMessenger;
		std::vector<const char*> m_ValidationLayers;
#endif // SGE_USING_VALIDATION_LAYERS
		VkSurfaceKHR m_Surface;
		VkPhysicalDevice m_PhysicalDevice;
		QueueFamilyIndices m_QueueFamilyIndices;
		std::vector<const char*> m_DeviceExtensions;
		VkDevice m_Device;
		VkQueue m_GraphicsQueue;
		VkQueue m_PresentQueue;

		VkSwapchainKHR m_Swapchain;
		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImageViews;
		VkFormat m_SwapchainImageFormat;
		VkExtent2D m_SwapchainExtent;
		std::vector<VkFramebuffer> m_Framebuffers;
		
		VkRenderPass m_RenderPass;
		
		VkPipeline m_Pipeline;
		VkPipelineLayout m_PipelineLayout;
		
		VkCommandPool m_CommandPool;
		VkCommandBuffer m_CommandBuffer;

		VkSemaphore m_ImageAvailableSemaphore;
		VkSemaphore m_RenderFinishedSemaphore;
		VkFence m_InFlightFence;
	private:
		void InitInstance();
#ifdef SGE_USING_VALIDATION_LAYERS
		void InitDebugMessenger();
#endif // SGE_USING_VALIDATION_LAYERS
		void InitSurface(GLFWwindow* window);
		void InitPhysicalDevice();
		void InitLogicalDevice();
		void InitSwapchain(GLFWwindow* window);
		void InitImageViews();
		void InitRenderPass();
		void InitGraphicsPipeline();
		void InitFramebuffers();
		void InitCommandPool();
		void InitCommandBuffer();
		void InitSyncObjects();
	private:
		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	public:
		Instance(GLFWwindow* window);
		~Instance();
		void Destroy();
		void OnUpdate();

		VkShaderModule CreateShaderModule(const std::vector<char>& binary);
	public:
		inline VkInstance GetInstanceHandle() const { return m_InstanceHandle; }
#ifdef SGE_USING_VALIDATION_LAYERS
		inline VkDebugUtilsMessengerEXT GetDebugMessenger() const { return m_DebugMessenger; }
		inline const std::vector<const char*>& GetValidationLayers() const { return m_ValidationLayers; }
#endif // SGE_USING_VALIDATION_LAYERS
	};

	std::vector<const char*> GetRequiredExtensions();

#ifdef SGE_USING_VALIDATION_LAYERS
	static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
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
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
	bool IsSuitablePhysicalDevice(VkPhysicalDevice device, VkSurfaceKHR surface,
		const std::vector<const char*>& requiredExtensions, QueueFamilyIndices& queueFamilyIndices);
	void ListExtensions();
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& extensions);

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;
	};

	SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availableModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);

	std::vector<char> LoadShaderBinary(const std::string& filepath);
} // namespace sge::vulkan