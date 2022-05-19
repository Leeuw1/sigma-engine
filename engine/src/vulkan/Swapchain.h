#pragma once

#include "Util.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

struct GLFWwindow;

namespace sge::vulkan
{
	class Swapchain
	{
	private:
		VkSwapchainKHR m_SwapchainHandle;
		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;
		VkFormat m_ImageFormat;
		VkExtent2D m_Extent;
		std::vector<VkFramebuffer> m_Framebuffers;
		bool m_FramebufferResized;
#ifdef DEBUG
		bool m_CleanedUp = false;
#endif // DEBUG
	public:
		Swapchain(VkDevice device, VkSurfaceKHR surface, GLFWwindow* windowHandle,
			SwapchainSupportDetails&& supportDetails, const QueueFamilyIndices& queueFamilyIndices);
#ifdef DEBUG
		~Swapchain()
		{
			SGE_ASSERTM(m_CleanedUp, "Vulkan swap chain not cleaned up.");
		}
#endif // DEBUG
		void Destroy(VkDevice device);
		void InitImageViews(VkDevice device);
		void InitFramebuffers(VkDevice device, VkRenderPass renderPass, VkImageView depthImageView);
	public:
		inline VkSwapchainKHR GetSwapchainHandle() const { return m_SwapchainHandle; }
		inline bool GetFramebufferResized() const { return m_FramebufferResized; }
		inline VkFormat GetImageFormat() const { return m_ImageFormat; }
		inline VkExtent2D GetExtent() const { return m_Extent; }
		inline VkFramebuffer FramebufferAt(uint32_t index) const { return m_Framebuffers[index]; }
		inline void SetFramebufferResized(bool b) { m_FramebufferResized = b; }
		inline uint32_t GetImageCount() const { return static_cast<uint32_t>(m_Images.size()); }

		VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& availableModes);
		VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
	};
} // namespace sge::vulkan