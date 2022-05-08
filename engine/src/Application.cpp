#include "Application.h"

namespace sge
{
	Application::Application()
	{
	}

	Application::~Application()
	{
	}

	int Application::Run()
	{
		while (!m_Window.ShouldClose())
		{
			m_Window.OnUpdate();
		}

		return 0;
	}
}