#pragma once

#include <maths/matrix44.h>
#include <maths/vector2.h>
#include <system/platform.h>

#include <vector>
#include <map>

#include "Defs.h"
#include "Maths.h"

namespace Textures
{
  struct TextureDesc
  {
    UInt width, height;
  };

  // Child region of a parent texture
  struct SubTextureDesc
  {
    UInt x, y;
    UInt width, height;
    Int displayX, displayY;
    Int displayWidth, displayHeight;
  };

  class TextureCollection
  {
    public:
    TextureCollection() = default;
    ~TextureCollection();

    UInt add(Path path, const TextureDesc& desc);
    void loadAll(gef::Platform& platform);
    UInt getTextureDesc(Path path, TextureDesc& out);

    inline const gef::Texture* getTextureData(UInt id) const { return id < textures.size() ? textures[id] : nullptr; }
    inline bool isBaked() const { return !textures.empty(); }

    private:
    struct DetailedTexture
    {
      TextureDesc desc;
      UInt id;
    };

    std::map<std::string, DetailedTexture> resourceMap; // Resource path to info
    std::vector<gef::Texture*> textures;
  };

  // Describes how a texture is subdivided into smaller textures
  class TextureAtlas
  {
    public:
    struct RegionPack
    {
      Maths::Region2D uv;
      gef::Matrix33 transform;
    };

    TextureAtlas();
    ~TextureAtlas();

    // SLOW //
    bool bake(UInt textureID, const TextureDesc& desc); // Generates parametrised data
    UInt addDivision(Label name, const SubTextureDesc& division);
    UInt getDivision(Label name) const;
    //
    
    inline bool isBaked() const { return regions != nullptr; }
    inline UInt getCount() const { return static_cast<UInt>(subDivisions.size()); }
    inline const UInt getTextureID() const { return tex; }
    inline const RegionPack* getData(UInt id) const { return regions + id; }
    private:
    std::vector<SubTextureDesc> subDivisions; // Describes sub-textures
    std::map<std::string, UInt> divisionNames; // Maps sub division by name to ID

    UInt tex; // Texture slot
    RegionPack* regions; // Baked position information
  };

}