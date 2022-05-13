#include "Application.h"
#include "event/KeyEvent.h"

#include <iostream>

namespace sge
{
	Application::Application()
	{
		m_Window.SetEventCallback(std::bind(Application::OnEvent_Static, this, std::placeholders::_1));
		m_LayerStack.PushBack(new TestLayer("TEST LAYER 0"));
		m_LayerStack.PushBack(new TestLayer("TEST LAYER 1"));
	}

	Application::~Application()
	{
	}

	int Application::Run()
	{
		while (!m_Window.ShouldClose())
		{
			m_LayerStack.OnUpdate();
			m_Window.OnUpdate();
		}

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