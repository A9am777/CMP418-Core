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

  // Converts a mat44 -> mat33
	static gef::Matrix33 maskMat44(const gef::Matrix44& mat)
  {
    gef::Matrix33 result = gef::Matrix33::kIdentity;

    // This is highly unoptimal, then again gef should not be designed this way
    for (size_t x = 0; x < 3; ++x)
    {
      for (size_t y = 0; y < 3; ++y)
      {
        result.m[x][y] = mat.m(x, y);
      }
    }
    return result;
  }
}