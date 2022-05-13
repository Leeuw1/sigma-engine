#pragma once

#include <iostream>

#ifdef DEBUG
	#define SGE_USING_VALIDATION_LAYERS

	#define SGE_DEBUG_BREAK() __debugbreak()
	#define SGE_DEBUG_BREAKM(msg) { fprintf(stderr, "Assertion Failed: %s\n", msg); SGE_DEBUG_BREAK(); }
	#define SGE_ASSERTM(expr, msg) if (!(expr)) SGE_DEBUG_BREAKM(msg)
	#define SGE_ASSERTF(expr, format, ...) if (!(expr)) { fprintf(stderr, "Assertion Failed: "format"\n", __VA_ARGS__); SGE_DEBUG_BREAK(); }
#endif // DEBUG