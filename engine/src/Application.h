#pragma once

#include "Window.h"
#include "Layer.h"
#include "renderer/Renderer.h"
#include "renderer/Scene.h"
#include "ecs/Registry.h"

#include <glm/mat4x4.hpp>

namespace sge
{
	struct TestUniformBuffer
	{
		glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Projection;
	};
	
	class SGE_API Application
	{
	private:
		Window m_Window;
		LayerStack m_LayerStack;
		std::unique_ptr<Renderer> m_Renderer;
		ecs::EntityID m_BunnyEntity;
		ecs::EntityID m_SquareEntity;
		vulkan::FrameGroup<vulkan::UniformBuffer*> m_UniformBuffers;

		glm::mat4 m_Rotation;

		Scene m_Scene;
	public:
		Application();
		~Application();
		void UpdateUniformBuffer(uint32_t index);
		void OnEvent(Event& event);
		static void OnEvent_Static(Application* app, Event& event);

		int Run();
	};

	class TestLayer : public Layer
	{
	private:
		std::string m_Name;
	public:
		TestLayer(std::string&& name)
			: m_Name(std::move(name))
		{
		}
		virtual ~TestLayer() override {}

		virtual void OnUpdate() override {}
		virtual bool OnEvent(Event& event) override
		{
			switch (event.GetType())
			{
			case EventType::KeyReleased:
				std::cout << m_Name << ": Key released\n";
				break;
			}

			return true;
		}
	};
} // namespace sge