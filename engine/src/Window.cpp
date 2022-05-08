#include "Window.h"
#include "base.h"
#include "vulkan/Instance.h"

namespace sge
{
	Window::Window()
		: m_WindowHandle(nullptr)
	{
		SGE_ASSERTM(glfwInit(), "Failed to initialize glfw.");

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		constexpr uint32_t windowWidth = 800, windowHeight = 600;
		m_WindowHandle = glfwCreateWindow(windowWidth, windowHeight, "Sigma Game Engine", nullptr, nullptr);

		m_VulkanInstance = std::make_unique<vulkan::Instance>(m_WindowHandle);
	}

	Window::~Window()
	{
		Destroy();
	}

	void Window::Destroy() const
	{
		glfwDestroyWindow(m_WindowHandle);
		glfwTerminate();
	}

	void Window::OnUpdate()
	{
		m_VulkanInstance->OnUpdate();
		glfwPollEvents();
	}

} // namespace sge