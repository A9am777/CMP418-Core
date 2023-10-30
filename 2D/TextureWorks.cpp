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
    return subDivisions.add(name, division).getHeapID();
  }

  UInt TextureAtlas::getDivision(Label name) const
  {
    return subDivisions.getID(name);
  }

  UInt TextureAtlas::getDivision(gef::StringId nameID) const
  {
    return subDivisions.getID(nameID);
  }

  bool Textures::TextureAtlas::bake(UInt textureID, const TextureDesc& desc)
  {
    tex = textureID;

    // Generate space for regions
    if (regions) { delete[] regions; }
    regions = new RegionPack[subDivisions.getHeapSize()];

    // Copy over normalised information per division
    {
      // Pre-compute axes
      float xNorm = 1 / float(desc.width);
      float yNorm = 1 / float(desc.height);

      RegionPack* progress = regions;
      for (size_t i = 0; i < subDivisions.getHeapSize(); ++i)
      {
        auto& div = subDivisions.get(i);

        // UV space
        progress->uv.left = float(div.x) * xNorm;
        progress->uv.bottom = float(div.y) * yNorm;
        progress->uv.right = progress->uv.left + float(div.width) * xNorm;
        progress->uv.top = progress->uv.bottom + float(div.height) * yNorm;
        
        // Sub sprite transform
        {
          gef::Matrix33 scaleMat = gef::Matrix33::kIdentity;
          scaleMat.Scale(gef::Vector2(float(div.width), float(div.height)));

          gef::Matrix33 translationMat = gef::Matrix33::kIdentity;
          translationMat.SetTranslation(gef::Vector2(float(div.width) * 0.5f - float(div.displayWidth) * 0.5f - float(div.displayX), float(div.height) * 0.5f - float(div.displayHeight) * 0.5f - float(div.displayY)));

          progress->transform = scaleMat * translationMat;
        }
        ++progress;
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

  void TextureCollection::loadAll(gef::Platform& platform)
  {
    // Load each texture by stored path into a slot
    for (const auto& namedResource : resourceMap.getNameMap())
    {
      auto& textureSlot = resourceMap.get(namedResource.second.getHeapID());
      if (textureSlot) { delete textureSlot; }

      std::string path;
      StringTable.Find(namedResource.first, path);
      textureSlot = CreateTextureFromPNG(path.c_str(), platform);
    }
    baked = true;
  }
}