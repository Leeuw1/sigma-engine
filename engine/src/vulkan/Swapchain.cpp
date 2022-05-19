#include "Swapchain.h"
#include "base.h"

#include <algorithm>

namespace sge::vulkan
{
	Swapchain::Swapchain(VkDevice device, VkSurfaceKHR surface, GLFWwindow* windowHandle,
		SwapchainSupportDetails&& supportDetails, const QueueFamilyIndices& queueFamilyIndices)
		: m_SwapchainHandle(nullptr), m_FramebufferResized(false)
	{
		VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(supportDetails.Formats);
		VkPresentModeKHR presentMode = ChoosePresentMode(supportDetails.PresentModes);
		VkExtent2D extent = ChooseExtent(supportDetails.Capabilities, windowHandle);

		uint32_t imageCount = supportDetails.Capabilities.minImageCount + 1;
		if (supportDetails.Capabilities.maxImageCount > 0 && imageCount > supportDetails.Capabilities.maxImageCount)
			imageCount = supportDetails.Capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.presentMode = presentMode;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (queueFamilyIndices.GraphicsFamily != queueFamilyIndices.PresentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
		}
		else
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

		createInfo.preTransform = supportDetails.Capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_SwapchainHandle) != VK_SUCCESS)
			SGE_DEBUG_BREAKM("Failed to create Vulkan swap chain.");

		vkGetSwapchainImagesKHR(device, m_SwapchainHandle, &imageCount, nullptr);
		m_Images.resize(imageCount);
		vkGetSwapchainImagesKHR(device, m_SwapchainHandle, &imageCount, m_Images.data());
		m_ImageFormat = surfaceFormat.format;
		m_Extent = extent;
	}

	void Swapchain::Destroy(VkDevice device)
	{
		for (auto framebuffer : m_Framebuffers)
			vkDestroyFramebuffer(device, framebuffer, nullptr);

		//m_Pipeline->Destroy(this);

		//vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

		vkDestroySwapchainKHR(device, m_SwapchainHandle, nullptr);

		for (auto imageView : m_ImageViews)
			vkDestroyImageView(device, imageView, nullptr);

#ifdef DEBUG
		m_CleanedUp = true;
#endif
	}

	void Swapchain::InitImageViews(VkDevice device)
	{
		m_ImageViews.resize(m_Images.size());

		for (size_t i = 0; i < m_Images.size(); i++)
			m_ImageViews[i] = CreateImageView(device, m_Images[i], m_ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void Swapchain::InitFramebuffers(VkDevice device, VkRenderPass renderPass, VkImageView depthImageView)
	{
		m_Framebuffers.resize(m_ImageViews.size());

		for (size_t i = 0; i < m_ImageViews.size(); i++)
		{
			VkImageView attachments[2] = { m_ImageViews[i], depthImageView };

			VkFramebufferCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			createInfo.renderPass = renderPass;
			createInfo.attachmentCount = 2;
			createInfo.pAttachments = attachments;
			createInfo.width = m_Extent.width;
			createInfo.height = m_Extent.height;
			createInfo.layers = 1;

			if (vkCreateFramebuffer(device, &createInfo, nullptr, &m_Framebuffers[i]))
				SGE_DEBUG_BREAKM("Failed to create Vulkan framebuffer.");
		}
	}

	VkSurfaceFormatKHR Swapchain::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& format : availableFormats)
		{
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return format;
		}

		return availableFormats[0];
	}

	VkPresentModeKHR Swapchain::ChoosePresentMode(const std::vector<VkPresentModeKHR>& availableModes)
	{
		for (const auto& mode : availableModes)
		{
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
				return mode;
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D Swapchain::ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
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
} // namespace sge::vulkan