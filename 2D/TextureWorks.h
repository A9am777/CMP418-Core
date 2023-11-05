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
    struct DetailedTexture : public NamedHeapInfo
    {
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
    bool bake(const bool isSwizzled = false); // Generates parametrised data, optionally to a prescribed "regionID"
    UInt addDivision(Label name, const SubTextureDesc& division);
    UInt getDivision(Label name) const;
    UInt getDivision(gef::StringId nameID) const;
    //
    
    void setTexture(UInt textureID, const TextureDesc& desc);

    // This can be used to ascribe a more cache friendly ordering before baking!
    inline void setRegionID(UInt divID, UInt regionID) { subDivisions.get(divID).regionID = regionID; }

    inline UInt getRegionID(UInt divID) const { return divID >= subDivisions.getHeapSize() ? SNULL : subDivisions.get(divID).regionID; }
    inline bool isBaked() const { return regions != nullptr; }
    inline size_t getCount() const { return static_cast<UInt>(subDivisions.getHeapSize()); }
    inline UInt getTextureID() const { return tex; }
    inline const RegionPack* getData(UInt id) const { return regions + id; }
    private:
    struct DetailedDivision
    {
      UInt regionID; // Used for sorting regions externally
      SubTextureDesc subDesc;
    };

    NamedHeap<DetailedDivision> subDivisions; // Maps sub division by name to ID
    TextureDesc texDesc;
    UInt tex; // Texture slot
    RegionPack* regions; // Baked position information
  };
}