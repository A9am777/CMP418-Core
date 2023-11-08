#pragma once

#include "rapidjson/document.h"

#include "Animation/Parsers/Parser.h"
#include "2D/TextureWorks.h"
#include "2D/Skeleton2D.h"
#include "2D/DopeSheet.h"
#include "Animation/SpriteWorks.h"

namespace IO
{
  class DragonBonesImporter : public Parser
  {
    public:
    static bool parseSkeleton(Animation::Skeleton2D& out, const rapidjson::Value& node);
    static bool parseSlots(Animation::Skeleton2DSlots& out, const rapidjson::Value& node);
    static bool parseSkin(Animation::Skeleton2DSkin& out, const rapidjson::Value& node);
    static bool parseBoneAnimationKeyframes(Animation::DopeSheet2D& out, const rapidjson::Value& node);
    static bool parseSkinnedSkeleton(Animation::SkinnedSkeleton2D& out, const rapidjson::Document& json);
    static bool parseAnimationAtlas(Textures::TextureCollection& collection, Textures::TextureAtlas& out, const rapidjson::Document& json, bool prebake = true);
    static bool parseSpriteAnimation(Animation::SpriteSheet& out, const rapidjson::Document& json);
  };
}