#pragma once
#include "Animation/Parsers/Parser.h"
#include "3D/SceneCollection.h"
#include "3D/Skeleton3D.h"

namespace IO
{
  class SceneImporter : public Parser
  {
    public:
    static bool parseSkeleton(Animation::Skeleton3D& out, Animation::SceneCollection& scenes, Literal path, gef::Platform& platform, bool animationOnly = false);
    // For unnamed, single animations
    static bool parseSkeletonAppendAnimation(Animation::Skeleton3D& out, Animation::SceneCollection& scenes, Literal path, Literal name, gef::Platform& platform);
  };
}