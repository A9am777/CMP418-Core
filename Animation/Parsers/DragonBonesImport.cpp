#include "Animation/Parsers/DragonBonesImport.h"

#include <map>
#include <list>

namespace IO
{
  // Semantic workaround to get rapidjson values regardless of existing data
  bool getValue(const rapidjson::Value& node, Literal key, bool& out, bool defaultValue = false)
  {
    if (node.HasMember(key))
    {
      out = node[key].GetBool();
      return true;
    }
    else
    {
      out = defaultValue;
      return false;
    }
  }
  // Same workaround but generalised to all types
  #define ImplGetImmediate(Type, TypeGetter)\
  bool getValue(const rapidjson::Value& node, const char* key, Type& out, Type defaultValue = Type())\
  {\
    if (node.HasMember(key))\
    {\
      out = node[key].TypeGetter();\
      return true;\
    }\
    else\
    {\
      out = defaultValue;\
      return false;\
    }\
  }

  ImplGetImmediate(float, GetFloat)
  ImplGetImmediate(std::string, GetString)
  ImplGetImmediate(int, GetInt)
  ImplGetImmediate(UInt, GetUint)

  bool parseBoneTransform(Animation::Skeleton2D::BonePoseOffset& out, const rapidjson::Value& node)
  {
    if (!node.HasMember("transform")) { return false; }
    auto boneTransformNode = node["transform"].GetObject();
    getValue(boneTransformNode, "x", out.translation.x);
    getValue(boneTransformNode, "y", out.translation.y);
    getValue(boneTransformNode, "skX", out.skew.x);
    getValue(boneTransformNode, "skY", out.skew.y);

    return true;
  }

  bool DragonBonesImporter::parseSkeleton(Animation::Skeleton2D& out, const rapidjson::Document& json)
  {
    using namespace Animation;

    if (!json.HasMember("armature") || !json["armature"].IsArray()) { return false; }
    
    // Fetch all bones
    auto armatureNode = json["armature"].GetArray();
    auto armatureRootNode = armatureNode[0].GetObject();
    if (!armatureRootNode.HasMember("bone")) { return false; }
    {
      auto bonesNode = armatureRootNode["bone"].GetArray();
      for (auto& boneNode : bonesNode)
      {
        if (!boneNode.HasMember("name")) { continue; }
        auto& boneInfo = out.addBone(boneNode["name"].GetString());

        getValue(boneNode, "parent", boneInfo.parentName, "");
        getValue(boneNode, "length", boneInfo.length);

        parseBoneTransform(boneInfo.restPose, boneNode);
      }
    }

    return out.bake();
  }
  bool DragonBonesImporter::parseAnimationAtlas(Textures::TextureAtlas& out, const rapidjson::Document& json)
  {
    using namespace Textures;

    TextureDesc desc;
    {
      getValue(json, "width", desc.width);
      getValue(json, "height", desc.height);
      out.init(desc);
    }

    // Fetch all subtextures
    if(json.HasMember("SubTexture"))
    {
      auto subArrayNode = json["SubTexture"].GetArray();
      for (auto& subNode : subArrayNode)
      {
        SubTextureDesc desc;
        getValue(subNode, "frameX", desc.displayX);
        getValue(subNode, "frameY", desc.displayY);
        getValue(subNode, "frameWidth", desc.displayWidth);
        getValue(subNode, "frameHeight", desc.displayHeight);
        getValue(subNode, "x", desc.x);
        getValue(subNode, "y", desc.y);
        getValue(subNode, "width", desc.width, desc.displayWidth);
        getValue(subNode, "height", desc.height, desc.displayHeight);
        out.addDivision(desc);
      }
    }

    return out.bake();
  }
}
