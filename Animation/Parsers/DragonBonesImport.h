#pragma once

#include "rapidjson/document.h"

#include "Animation/Parsers/Parser.h"
#include "2D/TextureWorks.h"
#include "2D/Skeleton.h"

namespace IO
{
  class DragonBonesImporter : public Parser
  {
    public:
    static bool parseSkeleton(Animation::Skeleton2D& out, const rapidjson::Document& json);
    static bool parseAnimationAtlas(Textures::TextureAtlas<Animation::Float>& out, const rapidjson::Document& json);
  };
}