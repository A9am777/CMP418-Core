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
    UInt newID = static_cast<UInt>(subDivisions.size());
    subDivisions.push_back(division);
    divisionNames.insert({ name, newID });
    return newID;
  }

  UInt TextureAtlas::getDivision(Label name) const
  {
    auto divIt = divisionNames.find(name);
    if (divIt != divisionNames.end())
    {
      return divIt->second;
    }

    return SNULL;
  }

  bool Textures::TextureAtlas::bake(UInt textureID, const TextureDesc& desc)
  {
    tex = textureID;

    // Generate space for regions
    if (regions) { delete[] regions; }
    regions = new RegionPack[subDivisions.size()];

    // Copy over normalised information per division
    {
      // Pre-compute axes
      float xNorm = 1 / float(desc.width);
      float yNorm = 1 / float(desc.height);

      RegionPack* progress = regions;
      for (size_t i = 0; i < subDivisions.size(); ++i)
      {
        auto& div = subDivisions[i];

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
    for (auto tex : textures) { delete tex; }
    textures.clear();
    resourceMap.clear();
  }

  UInt TextureCollection::add(Path path, const TextureDesc& desc)
  {
    auto resourceIt = resourceMap.find(path);
    if (resourceIt == resourceMap.end())
    {
      resourceIt = resourceMap.insert({ path, DetailedTexture{desc, static_cast<UInt>(textures.size())} }).first;
      textures.push_back(nullptr);
    }
    return resourceIt->second.id;
  }
  
  UInt TextureCollection::getTextureDesc(Path path, TextureDesc& out)
  {
    auto resourceIt = resourceMap.find(path);
    if (resourceIt == resourceMap.end()) { return SNULL; }

    out = resourceIt->second.desc;
    return resourceIt->second.id;
  }

  void TextureCollection::loadAll(gef::Platform& platform)
  {
    // Load each texture by stored path into a slot
    for (auto& resource : resourceMap)
    {
      auto& textureSlot = textures[resource.second.id];
      if (textureSlot) { delete textureSlot; }

      textureSlot = CreateTextureFromPNG(resource.first.c_str(), platform);
    }
  }
}