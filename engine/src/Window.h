#pragma once

#include "vulkan/Instance.h"
#include "event/Event.h"

#include <memory>
#include <functional>

namespace sge
{
	using EventCallback = std::function<void(Event&)>;
	
	class Window
	{
	private:
		std::unique_ptr<vulkan::Instance> m_VulkanInstance;
		EventCallback m_Eventcallback;
	private:
		void InitEventCallbacks();
	public:
		Window();
		~Window();
		void OnUpdate();
		void OnEvent(Event& event);
		void OnFrameBufferResize();

		inline bool ShouldClose() const { return glfwWindowShouldClose(m_VulkanInstance->GetWindowHandle()); }
		inline void SetEventCallback(const EventCallback& callback) { m_Eventcallback = callback; }
	};
} // namespace sge