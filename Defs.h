#pragma once

#include <string>

// Strings
typedef const char* Literal;
typedef const char* CString;
typedef const std::string& Label;

// Signed types
typedef int Int;

// Unsigned types
typedef unsigned int UInt;

static constexpr Literal fsp = "/"; // Path seperator

namespace Animation
{
	// Control precision
	#ifdef Animation_DoublePrecision
	typedef double Float;
	#else
	typedef float Float;
	#endif
}