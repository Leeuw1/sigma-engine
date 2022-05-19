#pragma once

#include "Window.h"
#include "Layer.h"

namespace sge
{
	class SGE_API Application
	{
	private:
		Window m_Window;
		LayerStack m_LayerStack;
	public:
		Application();
		~Application();
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