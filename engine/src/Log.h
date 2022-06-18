#pragma once

#include <iostream>

namespace sge::logger
{
	enum class Severity
	{
		Trace,
		Info,
		Warn,
		Error,
		Fatal
	};

	void SetLogColor(Severity severity);
	void Log(Severity severity, const std::string& message);

	template<typename... Args>
	void Logf(Severity severity, const std::string& format, Args... args)
	{
		SetLogColor(severity);
		std::cout << "[Engine] ";
		printf(format.c_str(), args...);
		std::cout << '\n';
	}
} // namespace sge::logger