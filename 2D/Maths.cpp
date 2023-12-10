#include "2D/Maths.h"
#include <maths/math_utils.h>
#include <math.h>

namespace Maths
{
    void Transform2D::assignTo(gef::Matrix33& transform) const
    {
      transform.m[0][0] = cosf(rotation) * scale.x;
      transform.m[0][1] = sinf(rotation);
      transform.m[0][2] = 0.0f;

      transform.m[1][0] = -sinf(rotation);
      transform.m[1][1] = cosf(rotation) * scale.y;
      transform.m[1][2] = 0.0f;

      transform.m[2][0] = translation.x;
      transform.m[2][1] = translation.y;
      transform.m[2][2] = 1.0f;
    }

    Transform2D Transform2D::operator+(const Transform2D& other)
    {
      return Transform2D 
      {
        translation + other.translation,
        gef::Vector2(scale.x * other.scale.x, scale.y * other.scale.y),
        rotation + other.rotation,
      };
    }
}
