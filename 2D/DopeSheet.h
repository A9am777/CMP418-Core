#pragma once
#include <maths/matrix33.h>
#include <maths/vector2.h>
#include <vector>
#include <array>
#include <list>
#include "../Defs.h"

namespace Animation
{
  class DopeSheet2D
  {
    public:

    private:
    enum InterpolationType : Byte
    {
      TweenLinear
    };
    enum AttributeType : Byte // Global transform, translation, etc. Specifically the operation
    {
      AttributeFull = 0,
      AttributeTranslation,
      AttributeX,
      AttributeY,
      AttributeRotation,
      AttributeScale,
      AttributeWidth,
      AttributeHeight,
      AttributeCount // Counts the possible number of attributes
    };
    struct TweenPoint // Interpolation at a point
    {
      InterpolationType tweenType;
      float tweenWeight;
    };
    struct DetailedKeyframe
    {
      std::vector<float> values; // Pack of all values for the attribute type
      TweenPoint easeIn;
      TweenPoint easeOut;
    };
    struct DetailedTrack
    {
      std::string objectName;
      std::array<std::list<DetailedKeyframe>, AttributeCount> attributeTracks;
    };
    struct DetailedSheet
    {
      std::vector<DetailedTrack> 
    };

    struct Keyframe
    {
      gef::Matrix33 targetTransform; // Precomputed
      float endTime;
    };

    float duration;
    float framerate;
  };
}