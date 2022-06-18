#include "Window.h"
#include "base.h"
#include "event/KeyEvent.h"
#include <vulkan/Util.h>

#include <imgui/imgui.h>

namespace sge
{
	constexpr uint32_t windowWidth = 1200, windowHeight = 900;
	
	Window::Window()
	{
		SGE_ASSERTM(glfwInit(), "Failed to initialize glfw.");

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		GLFWwindow* windowHandle = glfwCreateWindow(windowWidth, windowHeight, "Sigma Game Engine", nullptr, nullptr);
		SGE_ASSERTM(windowHandle, "Failed to create window.");
		SGE_INFO("Initializing Vulkan...");
		m_VulkanInstance = std::make_unique<vulkan::Instance>(windowHandle);
		
		glfwSetWindowUserPointer(windowHandle, this);
		InitEventCallbacks();
	}

	Window::~Window()
	{
		glfwDestroyWindow(m_VulkanInstance->GetWindowHandle());
		glfwTerminate();
	}

	void Window::InitEventCallbacks()
	{
		glfwSetKeyCallback(m_VulkanInstance->GetWindowHandle(),
		[](GLFWwindow* windowHandle, int key, int scancode, int action, int mods)
		{
			Window* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(windowHandle));
			if (action == GLFW_PRESS || action == GLFW_REPEAT)
			{
				KeyPressedEvent event(key);
				window->OnEvent(event);
			}
			else
			{
				KeyReleasedEvent event(key);
				window->OnEvent(event);
			}
		});

		glfwSetWindowCloseCallback(m_VulkanInstance->GetWindowHandle(),
		[](GLFWwindow* windowHandle)
		{
			Window* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(windowHandle));
			// Create window close event
			// ...
		}
		);

		glfwSetFramebufferSizeCallback(m_VulkanInstance->GetWindowHandle(),
		[](GLFWwindow* windowHandle, int width, int height)
		{
			Window* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(windowHandle));
			window->OnFrameBufferResize();
		});
	}

	void Window::ImGuiPresent()
	{
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	void Window::OnEvent(Event& event)
	{
		m_Eventcallback(event);
	}

	void Window::OnUpdate()
	{
		glfwPollEvents();
		ImGuiPresent(); // TODO: Move this to renderer
	}

	void Window::OnFrameBufferResize()
	{
		m_VulkanInstance->SetFramebufferResized();
	}
} // namespace sge