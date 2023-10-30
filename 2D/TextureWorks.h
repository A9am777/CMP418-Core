#pragma once

#include <maths/matrix44.h>
#include <maths/vector2.h>
#include <system/platform.h>

#include <vector>
#include <map>

#include "../Defs.h"
#include "../DataStructures.h"
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
    UInt getTextureDesc(gef::StringId path, TextureDesc*& out);

    inline const gef::Texture* getTextureData(UInt id) const { return resourceMap.get(id); }
    inline bool isBaked() const { return baked; }

    private:
    class DetailedTexture : public NamedHeapInfo
    {
      public:
      TextureDesc desc;
    };

    NamedHeap<gef::Texture*, DetailedTexture> resourceMap;
    bool baked = false;
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
    UInt getDivision(gef::StringId nameID) const;
    //
    
    inline bool isBaked() const { return regions != nullptr; }
    inline size_t getCount() const { return static_cast<UInt>(subDivisions.getHeapSize()); }
    inline const UInt getTextureID() const { return tex; }
    inline const RegionPack* getData(UInt id) const { return regions + id; }
    private:
    NamedHeap<SubTextureDesc> subDivisions; // Maps sub division by name to ID
    UInt tex; // Texture slot
    RegionPack* regions; // Baked position information
  };

}