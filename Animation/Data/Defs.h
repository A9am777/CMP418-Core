#pragma once

#include <string>

namespace Animation
{
	// Strings
	typedef const char* Literal;
	typedef const char* CString;
	typedef const std::string& Label;

	// Unsigned types
	typedef unsigned int UInt;

	// Control precision
	#ifdef Animation_DoublePrecision
	typedef double Float;
	#else
	typedef float Float;
	#endif

	static constexpr Literal fsp = "/"; // Path seperator
}