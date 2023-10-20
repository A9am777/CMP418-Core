#include "Animation/Parsers/DragonBonesImport.h"

#include <map>
#include <list>
#include <maths/math_utils.h>

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

  using namespace Animation;

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

  bool DragonBonesImporter::parseSkeleton(Animation::Skeleton2D& out, const rapidjson::Value& node)
  {
    using namespace Animation;

    // Fetch all bones
    if (!node.HasMember("bone")) { return false; }
    {
      auto bonesNode = node["bone"].GetArray();
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

  bool DragonBonesImporter::parseSlots(Animation::Skeleton2DSlots& out, const rapidjson::Value& node)
  {
    if (!node.HasMember("slot")) { return false; }

    auto slotsNode = node["slot"].GetArray();
    for (auto& slotNode : slotsNode)
    {
      std::string name;
      std::string parent;
      if (getValue(slotNode, "name", name) && getValue(slotNode, "parent", parent))
      {
        out.addSlot(parent, name);
      }
    }

    return true;
  }

  bool DragonBonesImporter::parseSkin(Animation::Skeleton2DSkin& out, const rapidjson::Value& node)
  {
    if (!node.HasMember("slot")) { return false; }

    auto slotsNode = node["slot"].GetArray();
    for (auto& slotNode : slotsNode)
    {
      std::string slotName;
      if (!getValue(slotNode, "name", slotName)) { continue; }
      
      if (slotNode.HasMember("display"))
      {
        // TODO: why can display be an array
        auto displayNodes = slotNode["display"].GetArray();
        auto& displayNode = *displayNodes.Begin();

        std::string displayName;
        Skeleton2D::BonePoseOffset transform;
        if (!getValue(displayNode, "name", displayName)) { continue; }
        parseBoneTransform(transform, displayNode);

        out.addLink(slotName, displayName, transform);
      }
    }

    return true;
  }

  bool DragonBonesImporter::parseBoneAnimationKeyframes(Animation::DopeSheet2D& out, const rapidjson::Value& node)
  {
    if (!node.HasMember("bone") || !node["bone"].IsArray()) { return false; }

    auto boneNodes = node["bone"].GetArray();
    for (auto& boneNode : boneNodes)
    {
      std::string boneName;
      if (getValue(boneNode, "name", boneName))
      {
        auto& track = out.getTrack(boneName);

        // Check for translations
        if (boneNode.HasMember("translateFrame") && boneNode["translateFrame"].IsArray())
        {
          auto transNodes = boneNode["translateFrame"].GetArray();
          for (auto& transNode : transNodes)
          {
            float duration;
            float easing;
            gef::Vector2 offset;

            getValue(transNode, "duration", duration);
            getValue(transNode, "tweenEasing", easing);
            getValue(transNode, "x", offset.x);
            getValue(transNode, "y", offset.y);

            out.addTranslationKeyframe(track, duration, offset);
          }
        }

        // Check for rotations
        if (boneNode.HasMember("rotateFrame") && boneNode["rotateFrame"].IsArray())
        {
          auto rotNodes = boneNode["rotateFrame"].GetArray();
          for (auto& rotNode : rotNodes)
          {
            float duration;
            float easing;
            float rotOffset;

            getValue(rotNode, "duration", duration);
            getValue(rotNode, "tweenEasing", easing);
            getValue(rotNode, "rotate", rotOffset);

            out.addRotationKeyframe(track, duration, gef::DegToRad(rotOffset));
          }
        }
      }
    }

    return false;
  }

  bool DragonBonesImporter::parseSkinnedSkeleton(Animation::SkinnedSkeleton2D& out, const rapidjson::Document& json)
  {
    // TODO: why is armature an array
    if (!json.HasMember("armature") || !json["armature"].IsArray()) { return false; }

    // Fetch the root armature
    auto armaturesNode = json["armature"].GetArray();
    auto armatureRootNode = armaturesNode[0].GetObject();

    // Now for the bones
    parseSkeleton(out.getSkeleton(), armatureRootNode);

    // Slots!
    parseSlots(out.getSlots(), armatureRootNode);

    // Iterate all skins
    if (armatureRootNode.HasMember("skin"))
    {
      auto skinsNode = armatureRootNode["skin"].GetArray();
      for (auto& skinNode : skinsNode)
      {
        UInt skinID = out.addSkin();
        parseSkin(out.getSkin(skinID), skinNode);
      }
    }

    // Iterate all animations
    if (armatureRootNode.HasMember("animation") && armatureRootNode["animation"].IsArray())
    {
      float animFPS;
      getValue(armatureRootNode, "frameRate", animFPS, 24.0f);

      auto animationNodes = armatureRootNode["animation"].GetArray();
      for (auto& animationNode : animationNodes)
      {
        float animDuration;
        std::string animName;

        if (getValue(animationNode, "name", animName) && getValue(animationNode, "duration", animDuration))
        {
          UInt animID = out.addAnimation(animName);
          auto& sheet = out.getAnimationData(animID);
          sheet.setDuration(animDuration);
          sheet.setRate(animFPS);
          parseBoneAnimationKeyframes(sheet, animationNode);
        }
      }
    }

    return true;
  }

  bool DragonBonesImporter::parseAnimationAtlas(Textures::TextureCollection& collection, Textures::TextureAtlas& out, const rapidjson::Document& json)
  {
    using namespace Textures;

    TextureDesc desc;
    UInt textureID = SNULL;
    {
      std::string imagePath;
      getValue(json, "imagePath", imagePath);
      getValue(json, "width", desc.width);
      getValue(json, "height", desc.height);

      textureID = collection.add(imagePath, desc);
    }

    // Fetch all subtextures
    if(json.HasMember("SubTexture"))
    {
      auto subArrayNode = json["SubTexture"].GetArray();
      for (auto& subNode : subArrayNode)
      {
        SubTextureDesc subDesc;
        std::string name;
        getValue(subNode, "name", name, "undefined");
        getValue(subNode, "width", subDesc.width, desc.width);
        getValue(subNode, "height", subDesc.height, desc.height);
        getValue(subNode, "x", subDesc.x);
        getValue(subNode, "y", subDesc.y);
        getValue(subNode, "frameX", subDesc.displayX);
        getValue(subNode, "frameY", subDesc.displayY);
        getValue(subNode, "frameWidth", subDesc.displayWidth, subDesc.width);
        getValue(subNode, "frameHeight", subDesc.displayHeight, subDesc.height);
        
        out.addDivision(name, subDesc);
      }
    }

    return out.bake(textureID, desc);
  }
}
