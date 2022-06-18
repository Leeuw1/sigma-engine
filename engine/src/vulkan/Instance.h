#pragma once

#include "Pipeline.h"
#include "Swapchain.h"
#include "Buffer.h"
#include "Texture.h"
#include "FrameGroup.h"
#include "base.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>

#include <imgui/imgui.h>

#include <vector>
#include <array>
#include <memory>

// TODO: Fix 'ReInitSwapchain'
// Maybe rename this class to 'GraphicsContext'

namespace sge::vulkan
{
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
		
		std::vector<Pipeline> m_Pipelines;
		
		//FrameGroup<UniformBuffer*> m_UniformBuffers;
		//glm::mat4x4 m_Rotation;

		// Use same layout for all the descriptor sets in the same framegroup
		VkDescriptorSetLayout m_DescriptorSetLayout;
		// In the future, make this an array of framegroups of descriptor sets
		FrameGroup<VkDescriptorSet> m_DescriptorSets;

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
		void InitCommandPool();

		void InitDepthResources();

		void InitCommandBuffers();
		void InitSyncObjects();
	public:
		Instance(GLFWwindow* window);
		~Instance();
		void BeginRenderPass(VkCommandBuffer commandBuffer, uint32_t imageIndex);
		void EndRenderPass(VkCommandBuffer commandBuffer);
		//void DrawFrame();
		void Present(uint32_t* imageIndex);
		void DrawIndexed(VkCommandBuffer commandBuffer, uint32_t pipelineIndex, VertexBuffer* vertexBuffer, IndexBuffer* indexBuffer, Shader* shader,
			uint32_t instanceCount);
		uint32_t CreatePipeline(Shader* shader, const BufferLayout* layout);
		void ReInitSwapchain();
		uint32_t AcquireNextSwapchainImage();

		// Descriptor set functions
		static void AddLayoutBindingUniformBuffer(std::vector<VkDescriptorSetLayoutBinding>& bindings);
		static void AddLayoutBindingTexture(std::vector<VkDescriptorSetLayoutBinding>& bindings);
		void AllocateDescriptorSets(std::vector<VkDescriptorSetLayoutBinding>& bindings);
		void AddDescriptorWrite(std::vector<VkWriteDescriptorSet>& descriptorWrites, VkDescriptorBufferInfo* bufferInfo, uint32_t frameIndex);
		void AddDescriptorWrite(std::vector<VkWriteDescriptorSet>& descriptorWrites, VkDescriptorImageInfo* imageInfo, uint32_t frameIndex);

		// Note: 'GetBufferInfo' and 'GetImageInfo' return pointers which must be freed with 'delete'
		VkDescriptorBufferInfo* GetBufferInfo(UniformBuffer* uniformBuffer);
		VkDescriptorImageInfo* GetImageInfo(Texture* texture);
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
		inline VkCommandBuffer GetCurrentCommandBuffer() const { return m_CommandBuffers[m_CurrentFrame]; }
		inline uint32_t GetCurrentFrame() const { return m_CurrentFrame; }
	};
} // namespace sge::vulkan