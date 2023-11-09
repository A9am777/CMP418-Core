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
typedef unsigned char Byte;
typedef unsigned int UInt;
static constexpr UInt SNULL = ~0; // Signed null in unsigned type

static constexpr Literal fsp = "/"; // Path seperator

// Bitwise operations
#define BIT(n) (1 << n)
template<typename T> T BitMask(const T state, const T mask) { return static_cast<T>(state & mask); }
template<typename T> T BitSet(const T state, const T mask) { return static_cast<T>(state | mask); }
template<typename T> T BitClear(const T state, const T mask) { return static_cast<T>(state & ~mask); }