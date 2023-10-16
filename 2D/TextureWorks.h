#pragma once

#include <vector>

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
    Int offsetX, offsetY;
  };

  // Describes how a texture is subdivided into smaller textures
  class TextureAtlas
  {
    public:
    struct RegionPack
    {
      Maths::Region2D uv;
      Maths::Vector2D origin;
    };

    TextureAtlas();
    ~TextureAtlas();

    void init(const TextureDesc& textureDesc);
    bool bake(); // Generates parametrised data
    UInt addDivision(const SubTextureDesc& division);
    
    inline bool isBaked() const { return regions != nullptr; }
    inline UInt getCount() const { return static_cast<UInt>(subDivisions.size()); }
    inline const TextureDesc* getDescriptor() const { return &desc; }
    inline const RegionPack* getData(UInt id) const { return regions + id; }
    private:
    TextureDesc desc; // Atlas desc
    std::vector<SubTextureDesc> subDivisions; // Describes sub-textures
    RegionPack* regions; // Baked position information
  };

}