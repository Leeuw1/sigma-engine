#pragma once

#include "Pipeline.h"
#include "Swapchain.h"
#include "Buffer.h"
#include "base.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>

#include <imgui/imgui.h>

#include <vector>
#include <array>
#include <memory>

// TODO: Fix 'ReInitSwapchain'

namespace sge::vulkan
{
	constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

	struct TestUniformBuffer
	{
		glm::mat4 Projection;
		glm::mat4 Rotation;
	};
	constexpr size_t UNIFORM_BUFFER_SIZE = sizeof(TestUniformBuffer);

	struct PushConstant
	{
		float ks; // Specular reflection constant
		float kd; // Diffuse reflection constant
		float ka; // Ambient reflection constant
		float a;  // Shininess constant
		
		float color[3];
	};

	class Instance
	{
	private:
		VkInstance m_InstanceHandle;
		GLFWwindow* m_WindowHandle;
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
		
		VkRenderPass m_RenderPass;
		
		Pipeline* m_Pipeline;
		VertexBuffer* m_VertexBuffer;
		IndexBuffer* m_IndexBuffer;
		uint32_t m_VerticesCount; // TEMPORARY
		uint32_t m_IndicesCount; // TEMPORARY
		std::array<UniformBuffer*, MAX_FRAMES_IN_FLIGHT> m_UniformBuffers;
		std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> m_DescriptorSetLayouts;
		std::array<VkDescriptorSet, 2> m_DescriptorSets;
		VkDescriptorPool m_DescriptorPool;
		Swapchain* m_Swapchain;
		
		VkCommandPool m_CommandPool;
		std::vector<VkCommandBuffer> m_CommandBuffers;

		// Sync objects
		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;

		uint32_t m_CurrentFrame;

		// Depth resources
		VkImage m_DepthImage;
		VkDeviceMemory m_DepthImageMemory;
		VkImageView m_DepthImageView;

		glm::mat4 m_Rotation;
		PushConstant m_PushConstant;
	private:
		void InitInstance();
#ifdef SGE_USING_VALIDATION_LAYERS
		void InitDebugMessenger();
#endif // SGE_USING_VALIDATION_LAYERS
		void InitSurface();
		void InitPhysicalDevice();
		void InitLogicalDevice();
		void InitRenderPass();
		void InitGraphicsPipeline();
		void InitCommandPool();

		void InitDepthResources();

		void InitVertexBuffer();
		void InitUniformBuffers();
		void InitDescriptorSets();
		void InitCommandBuffers();
		void InitSyncObjects();
	private:
		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	public:
		Instance(GLFWwindow* window);
		~Instance();
		void DrawFrame();
		void UpdateUniformBuffer(uint32_t index);
		void ReInitSwapchain();

		VkShaderModule CreateShaderModule(const std::vector<char>& binary);
	public:
		inline VkInstance GetInstanceHandle() const { return m_InstanceHandle; }
#ifdef SGE_USING_VALIDATION_LAYERS
		inline VkDebugUtilsMessengerEXT GetDebugMessenger() const { return m_DebugMessenger; }
		inline const std::vector<const char*>& GetValidationLayers() const { return m_ValidationLayers; }
#endif // SGE_USING_VALIDATION_LAYERS
		inline void SetFramebufferResized() { m_Swapchain->SetFramebufferResized(true); }
		
		inline GLFWwindow* GetWindowHandle() const { return m_WindowHandle; }
		inline VkDevice GetDevice() const { return m_Device; }
		inline VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		inline VkExtent2D GetSwapchainExtent() const { return m_Swapchain->GetExtent(); }
		inline VkRenderPass GetRenderPass() const { return m_RenderPass; }
		inline VkSurfaceKHR GetSurface() const { return m_Surface; }
		inline VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
		inline const QueueFamilyIndices& GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }
		inline uint32_t GetSwapchainImageCount() const { return m_Swapchain->GetImageCount(); }
		inline VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }
		inline VkCommandPool GetCommandPool() const { return m_CommandPool; }
		inline PushConstant& GetPushConstant() { return m_PushConstant; }
	};
} // namespace sge::vulkan