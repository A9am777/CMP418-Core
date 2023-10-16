#include "2D/TextureWorks.h"
namespace Textures
{
  TextureAtlas::TextureAtlas()
  {
    regions = nullptr;
  }

  TextureAtlas::~TextureAtlas()
  {
    if (regions) { delete[] regions; regions = nullptr; }
  }

  UInt TextureAtlas::addDivision(const SubTextureDesc& division)
  {
    subDivisions.push_back(division);
    return UInt(subDivisions.size() - 1);
  }

  void TextureAtlas::init(const TextureDesc& textureDesc)
  {
    desc = textureDesc;
  }

  bool Textures::TextureAtlas::bake()
  {
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

        progress->uv.left = float(div.x) * xNorm;
        progress->uv.bottom = float(div.y) * yNorm;
        progress->uv.right = progress->uv.left + float(div.width) * xNorm;
        progress->uv.top = progress->uv.bottom + float(div.height) * yNorm;
        progress->transform.SetIdentity();
        progress->transform.SetTranslation(gef::Vector4(float(div.width) * 0.5f - float(div.displayWidth) * 0.5f - float(div.displayX), float(div.height) * 0.5f - float(div.displayHeight) * 0.5f - float(div.displayY), 0));
        ++progress;
      }
    }

    return isBaked();
  }
}