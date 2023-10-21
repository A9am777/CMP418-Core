#pragma once
#include <maths/matrix33.h>
#include <maths/matrix44.h>

namespace Maths
{
	#define MATHS_PI 3.141592653589793
	#define MATHS_TAU 2. * 3.141592653589793

	struct Region2D
	{
		float right, bottom, left, top;
	};

	struct Vector2D
	{
		float x, y;
	};
}