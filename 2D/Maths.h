#pragma once
#include <maths/matrix33.h>
#include <maths/matrix44.h>

namespace Maths
{
	struct Region2D
	{
		float right, bottom, left, top;
	};

	struct Vector2D
	{
		float x, y;
	};
}