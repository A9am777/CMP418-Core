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
    out.translation.x = boneTransformNode.HasMember("x") ? boneTransformNode["x"].GetFloat() : .0f;
    out.translation.y = boneTransformNode.HasMember("y") ? boneTransformNode["y"].GetFloat() : .0f;
    out.skew.x = boneTransformNode.HasMember("skX") ? boneTransformNode["skX"].GetFloat() : .0f;
    out.skew.y = boneTransformNode.HasMember("skY") ? boneTransformNode["skY"].GetFloat() : .0f;

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

        boneInfo.parentName = boneNode.HasMember("parent") ? boneNode["parent"].GetString() : "";
        boneInfo.length = boneNode.HasMember("length") ? boneNode["length"].GetUint() : 0;

        parseBoneTransform(boneInfo.restPose, boneNode);
      }
    }

    return out.bake();
  }
  bool DragonBonesImporter::parseAnimationAtlas(Textures::TextureAtlas<Animation::Float>& out, const rapidjson::Document& json)
  {
    using namespace Textures;

    TextureDesc desc;
    {
      desc.width = json["width"].GetUint();
      desc.height = json["height"].GetUint();
      out.init(desc);
    }

    // Fetch all subtextures
    {
      auto subArray = json["SubTexture"].GetArray();
      for (auto& sub : subArray)
      {
        SubTextureDesc desc;
        desc.x = sub["x"].GetUint();
        desc.y = sub["y"].GetUint();
        desc.width = sub["width"].GetUint();
        desc.height = sub["height"].GetUint();
        desc.offsetX = sub["frameX"].GetInt();
        desc.offsetY = sub["frameY"].GetInt();
        desc.offsetX = sub["frameX"].GetInt();
        desc.offsetY = sub["frameY"].GetInt();

        out.addDivision(desc);
      }
    }

    return out.bake();
  }
}
