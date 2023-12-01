#include "2D/TextureWorks.h"

#include "load_texture.h"
namespace Textures
{
  TextureAtlas::TextureAtlas()
  {
    regions = nullptr;
    tex = SNULL;
  }

  TextureAtlas::~TextureAtlas()
  {
    if (regions) { delete[] regions; regions = nullptr; }
  }

  UInt TextureAtlas::addDivision(Label name, const SubTextureDesc& division)
  {
    return subDivisions.add(name, { 0, division }).getHeapID();
  }

  UInt TextureAtlas::getDivision(Label name) const
  {
    return subDivisions.getID(name);
  }

  UInt TextureAtlas::getDivision(gef::StringId nameID) const
  {
    return subDivisions.getID(nameID);
  }

  TextureAtlas& TextureAtlas::operator=(TextureAtlas& other)
  {
    subDivisions = other.subDivisions;
    texDesc = other.texDesc;
    tex = other.tex;
    if (regions) { delete[] regions; regions = nullptr; }
    if (other.regions)
    {
      regions = new RegionPack[subDivisions.getHeapSize()];
      std::memcpy(regions, other.regions, subDivisions.getHeapSize() * sizeof(RegionPack));
    }

    return *this;
  }

  void TextureAtlas::setTexture(UInt textureID, const TextureDesc& desc)
  {
    tex = textureID;
    texDesc = desc;
  }

  bool Textures::TextureAtlas::bake(const bool isSwizzled)
  {
    // Generate space for regions
    if (regions) { delete[] regions; }
    regions = new RegionPack[subDivisions.getHeapSize()];

    // Copy over normalised information per division
    {
      // Pre-compute axes
      float xNorm = 1 / float(texDesc.width);
      float yNorm = 1 / float(texDesc.height);

      for (size_t i = 0; i < subDivisions.getHeapSize(); ++i)
      {
        auto& detailedDiv = subDivisions.get(i);
        auto& subDiv = detailedDiv.subDesc;

        if (!isSwizzled)
        {
          // Use the division order when lacking a preset order
          detailedDiv.regionID = i;
        }

        RegionPack* progress = regions + detailedDiv.regionID;

        // UV space
        progress->uv.left = float(subDiv.x) * xNorm;
        progress->uv.bottom = float(subDiv.y) * yNorm;
        progress->uv.right = progress->uv.left + float(subDiv.width) * xNorm;
        progress->uv.top = progress->uv.bottom + float(subDiv.height) * yNorm;
        
        // Sub sprite transform
        {
          gef::Matrix33 scaleMat = gef::Matrix33::kIdentity;
          scaleMat.Scale(gef::Vector2(float(subDiv.width), float(subDiv.height)));

          gef::Matrix33 translationMat = gef::Matrix33::kIdentity;
          translationMat.SetTranslation(gef::Vector2(float(subDiv.width) * 0.5f - float(subDiv.displayWidth) * 0.5f - float(subDiv.displayX), float(subDiv.height) * 0.5f - float(subDiv.displayHeight) * 0.5f - float(subDiv.displayY)));

          progress->transform = scaleMat * translationMat;
        }
      }
    }
    return isBaked();
  }

  TextureCollection::~TextureCollection()
  {
    for (size_t i = 0; i < resourceMap.getHeapSize(); ++i) 
    {
      delete resourceMap.get(i);
    }
    resourceMap.clear();
  }

  UInt TextureCollection::add(Path path, const TextureDesc& desc)
  {
    auto& resourceDesc = resourceMap.add(path, nullptr);
    resourceDesc.desc = desc;
    return resourceDesc.getHeapID();
  }
  
  UInt TextureCollection::getTextureDesc(gef::StringId path, TextureDesc*& out)
  {
    if (auto resourceDesc = resourceMap.getMetaInfo(path))
    {
      out = &resourceDesc->desc;
      return resourceDesc->getHeapID();
    }

    return SNULL;
  }

  void TextureCollection::loadAll(Path rootPath, gef::Platform& platform)
  {
    // Load each texture by stored path into a slot
    for (const auto& namedResource : resourceMap.getNameMap())
    {
      auto& textureSlot = resourceMap.get(namedResource.second.getHeapID());
      if (textureSlot) { delete textureSlot; }

      std::string path;
      StringTable.Find(namedResource.first, path);
      path = rootPath + fsp + path;
      textureSlot = CreateTextureFromPNG(path.c_str(), platform);
    }
    baked = true;
  }
}