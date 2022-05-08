#include "Application.h"
#include "vulkan/Instance.h"
#include "base.h"

#include <iostream>
#include <vector>
#include <memory>

int main()
{
	sge::Application app;
	int exitCode = app.Run();

	return exitCode;
}