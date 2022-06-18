#include "Application.h"
#include "event/KeyEvent.h"
#include "ImGuiLayer.h"

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

namespace sge
{
	Application::Application()
		: m_Rotation(glm::scale(glm::identity<glm::mat4>(), glm::vec3(5.0f, 5.0f, 5.0f)))
	{
		m_Window.SetEventCallback(std::bind(Application::OnEvent_Static, this, std::placeholders::_1));
		m_LayerStack.PushBack(new TestLayer("TEST LAYER 0"));
		m_LayerStack.PushBack(new TestLayer("TEST LAYER 1"));
		m_LayerStack.PushBack(new ImGuiLayer(m_Window.GetVulkanInstance()));
		m_Renderer = std::make_unique<Renderer>(m_Window.GetVulkanInstance());
		
		TestUniformBuffer uBuffer = {
			m_Rotation,
			glm::lookAtRH(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
			vulkan::MakePerspective(glm::half_pi<float>(), 800.0f / 600.0f, 0.1f, 10.0f),
		};
		for (uint32_t i = 0; i < vulkan::MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_UniformBuffers[i] = new vulkan::UniformBuffer(m_Window.GetVulkanInstance()->GetDevice(), m_Window.GetVulkanInstance()->GetPhysicalDevice(),
				&uBuffer, sizeof(TestUniformBuffer));
		}

		m_BunnyEntity = m_Scene.AddModel(m_Window.GetVulkanInstance(), "E:/C++/sigma-engine/engine/res/meshes/stanford_bunny",
			"E:/C++/sigma-engine/engine/materials/solidColor.mat");

		// Square mesh
		float vertices[] = {
			-0.3f, -0.4f, 0.0f,		0.0f, 1.0f,
			 0.3f, -0.4f, 0.0f,		1.0f, 1.0f,
			-0.3f,  0.4f, 0.0f,		0.0f, 0.0f,
			 0.3f,  0.4f, 0.0f,		1.0f, 0.0f
		};

		uint32_t indices[] = {
			0, 1, 3,
			3, 2, 0
		};

		m_SquareEntity = m_Scene.AddModel(m_Window.GetVulkanInstance(), vertices, sizeof(vertices), indices, sizeof(indices),
			"E:/C++/sigma-engine/engine/materials/texture.mat");

		m_Scene.InitDescriptorSets(m_Window.GetVulkanInstance(), m_UniformBuffers);
		m_Scene.InitPipelines(m_Window.GetVulkanInstance());
	}

	Application::~Application()
	{
		m_Scene.Destroy(m_Window.GetVulkanInstance());

		for (auto uniformBuffer : m_UniformBuffers)
		{
			uniformBuffer->Destroy(m_Window.GetVulkanInstance()->GetDevice());
			delete uniformBuffer;
		}
	}
	
	void Application::UpdateUniformBuffer(uint32_t index)
	{
		m_Rotation = glm::rotate(m_Rotation, 0.0002f, glm::vec3(0.0f, 1.0f, 0.0f));

		TestUniformBuffer uBuffer = {
			m_Rotation,
			glm::lookAtLH(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
			vulkan::MakePerspective(glm::half_pi<float>(), 800.0f / 600.0f, 0.1f, 10.0f),
		};
		m_UniformBuffers[index]->Upload(m_Window.GetVulkanInstance()->GetDevice(), &uBuffer, m_UniformBuffers[index]->GetSize());
	}

	int Application::Run()
	{
		uint32_t imageIndex;
		while (!m_Window.ShouldClose())
		{
			m_LayerStack.OnUpdate();
			
			imageIndex = m_Renderer->BeginFrame();
			UpdateUniformBuffer(imageIndex);
			m_Renderer->DrawScene(m_Scene);
			m_Renderer->EndFrame(imageIndex);

			m_Window.OnUpdate();
		}
		SGE_INFO("Exiting program...");

		return 0;
	}

	void Application::OnEvent_Static(Application* app, Event& event)
	{
		app->OnEvent(event);
	}

	void Application::OnEvent(Event& event)
	{
		m_LayerStack.OnEvent(event);

		switch (event.GetType())
		{
		case EventType::KeyPressed:
		{
			KeyPressedEvent& e = static_cast<KeyPressedEvent&>(event);
			std::cout << "Application: Key pressed\n";
			break;
		}
		}
	}
} // namespace sge