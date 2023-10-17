#pragma once

#include <string>

// Strings
typedef const char* Literal;
typedef const char* CString;
typedef const std::string& Label;
typedef const std::string& Path;

// Signed types
typedef int Int;

// Unsigned types
typedef unsigned int UInt;
static constexpr UInt SNULL = ~0; // Signed null in unsigned type

static constexpr Literal fsp = "/"; // Path seperator