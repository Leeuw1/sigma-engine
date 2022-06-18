#pragma once

#include "Log.h"

#ifdef DEBUG
	#define SGE_USING_VALIDATION_LAYERS

	#define SGE_DEBUG_BREAK() __debugbreak()
	#define SGE_DEBUG_BREAKM(msg) { SGE_FATALF("Assertion Failed: %s", msg); SGE_DEBUG_BREAK(); }
	#define SGE_ASSERTM(expr, msg) if (!(expr)) SGE_DEBUG_BREAKM(msg)
	#define SGE_ASSERTF(expr, format, ...) if (!(expr)) { SGE_FATALF("Assertion Failed: "format, __VA_ARGS__); SGE_DEBUG_BREAK(); }
#else
	#define SGE_DEBUG_BREAK() (void)0
	#define SGE_DEBUG_BREAKM(msg) (void)0
	#define SGE_ASSERTM(expr, msg) (void)(expr)
	#define SGE_ASSERTF(expr, format, ...) (void)(expr)
#endif // DEBUG

//#define SGE_BUILD_SHARED

#ifdef SGE_BUILD_SHARED
	#ifdef SGE_DLL_EXPORT
		#define SGE_API _declspec(dllexport)
	#else
		#define SGE_API _declspec(dllimport)
	#endif
#else
	#define SGE_API
#endif // SGE_BUILD_SHARED

/*
#ifndef SGE_SHADER_DIR
	#error Shader directory not specified
#endif // SGE_SHADER_DIR
*/

// Logging macros
#define SGE_TRACE(msg)		::sge::logger::Log(::sge::logger::Severity::Trace, msg)
#define SGE_INFO(msg)		::sge::logger::Log(::sge::logger::Severity::Info, msg)
#define SGE_WARN(msg)		::sge::logger::Log(::sge::logger::Severity::Warn, msg)
#define SGE_ERROR(msg)		::sge::logger::Log(::sge::logger::Severity::Error, msg)
#define SGE_FATAL(msg)		::sge::logger::Log(::sge::logger::Severity::Trace, msg)

#define SGE_TRACEF(format, ...)		::sge::logger::Logf(::sge::logger::Severity::Trace, format, __VA_ARGS__)
#define SGE_INFOF(format, ...)		::sge::logger::Logf(::sge::logger::Severity::Info, format, __VA_ARGS__)
#define SGE_WARNF(format, ...)		::sge::logger::Logf(::sge::logger::Severity::Warn, format, __VA_ARGS__)
#define SGE_ERRORF(format, ...)		::sge::logger::Logf(::sge::logger::Severity::Error, format, __VA_ARGS__)
#define SGE_FATALF(format, ...)		::sge::logger::Logf(::sge::logger::Severity::Trace, format, __VA_ARGS__)