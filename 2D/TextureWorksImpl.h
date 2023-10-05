#include "TextureWorks.h"
namespace Textures
{
  template<typename T>
  inline TextureAtlas<T>::TextureAtlas()
  {
    regions = nullptr;
  }

  template<typename T>
  inline TextureAtlas<T>::~TextureAtlas()
  {
    if (regions) { delete[] regions; regions = nullptr; }
  }

  template<typename T>
  inline UInt TextureAtlas<T>::addDivision(const SubTextureDesc& division)
  {
    subDivisions.push_back(division);
    return UInt(subDivisions.size() - 1);
  }

  template<typename T>
  inline void TextureAtlas<T>::init(const TextureDesc& textureDesc)
  {
    desc = textureDesc;
  }

  template<typename T>
  inline bool Textures::TextureAtlas<T>::bake()
  {
    // Generate space for regions
    if (regions) { delete[] regions; }
    regions = new RegionPack[subDivisions.size()];

    // Copy over normalised information per division
    {
      // Pre-compute axes
      Number xNorm = 1 / Number(desc.width);
      Number yNorm = 1 / Number(desc.height);

      RegionPack* progress = regions;
      for (size_t i = 0; i < subDivisions.size(); ++i)
      {
        auto& div = subDivisions[i];
        progress->uv.left   = Number(div.x) * xNorm;
        progress->uv.bottom = Number(div.y) * yNorm;
        progress->uv.right  = progress->uv.left + Number(div.width) * xNorm;
        progress->uv.top    = progress->uv.bottom + Number(div.height) * yNorm;
        progress->origin    = { Number(div.offsetX) * xNorm, Number(div.offsetY) * yNorm };
        ++progress;
      }
    }

    return isBaked();
  }
}