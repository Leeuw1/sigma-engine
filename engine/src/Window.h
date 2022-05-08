#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <memory>

namespace sge
{
	namespace vulkan
	{
		class Instance;
	}
	
	class Window
	{
	private:
		GLFWwindow* m_WindowHandle;
		std::unique_ptr<vulkan::Instance> m_VulkanInstance;
	public:
		Window();
		~Window();
		void Destroy() const;
		void OnUpdate();

		inline GLFWwindow* GetWindowHandle() const { return m_WindowHandle; }
		inline bool ShouldClose() const { return glfwWindowShouldClose(m_WindowHandle); }
	};
} // namespace sge