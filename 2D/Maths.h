#pragma once
#include <maths/matrix33.h>

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

	struct Transform2D
	{
		gef::Vector2 translation;
		gef::Vector2 scale;
		float rotation;

		void assignTo(gef::Matrix33& transform) const;
		Transform2D operator+(const Transform2D& other);
	};
}