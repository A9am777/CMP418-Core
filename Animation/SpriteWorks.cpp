#include "SpriteWorks.h"
#include "Data/Structs.h"

namespace Animation
{
  SpriteSheet::SpriteSheet()
  {

  }

  bool SpriteSheet::bake()
  {
    // For safety, set all regions to the back in case some frames are not matched
    for (size_t i = 0; i < atlas.getCount(); ++i)
    {
      atlas.setRegionID(static_cast<UInt>(i), atlas.getCount() - 1);
    }

    // Progressively build each frame in a consecutive order
    UInt regionID = 0;
    for (size_t animID = 0; animID < detailedAnimations.getHeapSize(); ++animID)
    {
      auto& detailedAnimation = detailedAnimations.get(static_cast<UInt>(animID));
      detailedAnimation.bakedStartFrame = regionID;
      for (auto& frameName : detailedAnimation.frameData)
      {
        // Per found division, assign the next region ID
        UInt foundDivisionID = atlas.getDivision(frameName);
        if (foundDivisionID != SNULL)
        {
          atlas.setRegionID(foundDivisionID, regionID);
          ++regionID;
        }
      }
      detailedAnimation.bakedEndFrame = regionID;
    }

    return atlas.bake(true);
  }

  UInt SpriteSheet::addAnimation(Label name, const std::vector<gef::StringId>& frameNames, float fps)
  {
    return detailedAnimations.add(name, {frameNames, 0, 0, 1.f / fps }).getHeapID();
  }

  UInt SpriteSheet::getAnimationID(Label name) const
  {
    auto indexer = detailedAnimations.getMetaInfo(name);
    return indexer ? indexer->getHeapID() : SNULL;
  }

  UInt SpriteSheet::getAnimationFrameID(UInt animID, float& time) const
  {
    auto& detailedAnimation = detailedAnimations.get(animID);
    Animation::FrameSignature frame(time, detailedAnimation.frametime);

    UInt regionID = detailedAnimation.bakedStartFrame + frame.major;
    if (regionID >= detailedAnimation.bakedEndFrame)
    {
      // Ensure the frame loops in the valid range
      UInt validID = frame.major % (detailedAnimation.bakedEndFrame - detailedAnimation.bakedStartFrame);
      validID += detailedAnimation.bakedStartFrame;

      // Reset time a bit
      time -= float(regionID - validID) * detailedAnimation.frametime;

      regionID = validID;
    }

    return regionID;
  }

  SpriteInstance::SpriteInstance() : sheet{ nullptr }, currentAnimation{ SNULL }, playing{false}
  {
    globalTransform.SetIdentity();
    spriteTransform.SetIdentity();
  }

  void SpriteInstance::render(gef::SpriteRenderer* renderer)
  {
    renderer->DrawSprite(sprite, spriteTransform * globalTransform);
  }

  void SpriteInstance::update(float dt)
  {
    if (playing && sheet && currentAnimation < sheet->getAnimationCount())
    {
      elapsedTime += dt;

      // Update the sprite transform and UV data based on animation cycle
      UInt frameRegionID = sheet->getAnimationFrameID(currentAnimation, elapsedTime);
      if (auto region = sheet->getAtlas().getData(frameRegionID))
      {
        spriteTransform = region->transform;
        sprite.set_uv_width(region->uv.right - region->uv.left);
        sprite.set_uv_height(region->uv.top - region->uv.bottom);
        sprite.set_uv_position({ region->uv.left, region->uv.bottom });
      }
    }
  }

  void SpriteInstance::play(UInt animID)
  {
    currentAnimation = animID;
    playing = true;
    elapsedTime = .0f;
  }

  void SpriteInstance::setTexture(Textures::TextureCollection& texCollection)
  {
    sprite.set_texture(texCollection.getTextureData(sheet->getAtlas().getTextureID()));
  }
}
