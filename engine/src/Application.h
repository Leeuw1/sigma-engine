#pragma once

#include "Window.h"

namespace sge
{
	class Application
	{
	private:
		Window m_Window;
	public:
		Application();
		~Application();

		int Run();
	};
}