#include "Animation/Parsers/DragonBonesImport.h"

namespace IO
{
  bool DragonBonesImporter::parseAnimationAtlas(Textures::TextureAtlas<Animation::Float>& out, const rapidjson::Document& json)
  {
    using namespace Textures;

    // TODO: recommendation - using component-based system here would streamline data

    // Texture desc
    {
      TextureDesc desc;
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

        out.addDivision(desc);
      }
    }

    return out.bake();
  }
}
