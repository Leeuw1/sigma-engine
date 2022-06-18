#include "Log.h"

namespace sge::logger
{
	void SetLogColor(Severity severity)
	{
		switch (severity)
		{
		case Severity::Trace:
			std::cout << "\x1b[0;32m";
			break;
		case Severity::Info:
			std::cout << "\x1b[0m";
			break;
		case Severity::Warn:
			std::cout << "\x1b[0;33m";
			break;
		case Severity::Error:
			std::cout << "\x1b[0;31m";
			break;
		case Severity::Fatal:
			std::cout << "\x1b[1;31m";
			break;
		}
	}

	void Log(Severity severity, const std::string& message)
	{
		SetLogColor(severity);
		std::cout << "[Engine] " << message << '\n';
	}
} // namespace sge::logger