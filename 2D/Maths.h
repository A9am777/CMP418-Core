#pragma once

namespace Maths
{
	template<typename T> struct Region2D
	{
		T right, bottom, left, top;
	};

	template<typename T> struct Vector2D
	{
		T x, y;
	};
}