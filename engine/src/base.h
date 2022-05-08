#pragma once

#include <iostream>

#ifdef DEBUG
	#define SGE_USING_VALIDATION_LAYERS

	#define SGE_DEBUG_BREAK() __debugbreak()
	#define SGE_ASSERTM(expr, msg) if (!(expr)) { std::cerr << "Assertion Failed: " << msg << '\n'; SGE_DEBUG_BREAK(); }
	#define SGE_ASSERTF(expr, format, ...) if (!(expr)) { fprintf(stderr, format"\n", __VA_ARGS__); SGE_DEBUG_BREAK(); }
#endif // DEBUG