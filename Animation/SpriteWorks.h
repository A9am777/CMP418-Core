#pragma once
#include <graphics/sprite.h>
#include <graphics/sprite_renderer.h>

#include "2D/TextureWorks.h"

namespace Animation
{
  class SpriteSheet
  {
    public:
    struct DetailedAnimation
    {
      std::vector<gef::StringId> frameData;
      UInt bakedStartFrame;
      UInt bakedEndFrame; // +1 past the final frame
      float frametime;
    };

    SpriteSheet();

    // Slow
    bool bake();
    UInt addAnimation(Label name, const std::vector<gef::StringId>& frameNames, float fps = 60.f);
    UInt getAnimationID(Label name) const;
    //

    // Returns the current regionID of the frame representing this animation. Time will be reset to within range for precision
    UInt getAnimationFrameID(UInt animID, float& time) const;

    inline bool isBaked() const { return atlas.isBaked(); }
    inline Textures::TextureAtlas& getAtlas() { return atlas; }
    inline const Textures::TextureAtlas& getAtlas() const { return atlas; }
    inline float getAnimationFrametime(UInt animID) const { return detailedAnimations.get(animID).frametime; }
    inline UInt getAnimationCount() const { return detailedAnimations.getHeapSize(); }
    inline const NamedHeap<DetailedAnimation>& getAnimations() const { return detailedAnimations; }

    private:
    NamedHeap<DetailedAnimation> detailedAnimations;
    Textures::TextureAtlas atlas; // Frame data gets baked directly into the atlas
  };

  class SpriteInstance
  {
    public:
    SpriteInstance();

    inline void setSheet(SpriteSheet* spriteSheet) { sheet = spriteSheet; }

    void render(gef::SpriteRenderer* renderer);
    void update(float dt);

    void play(UInt animID);
    void setTexture(Textures::TextureCollection& texCollection);
    inline void setGlobalTransform(const gef::Matrix33& newTransform) { globalTransform = newTransform; }

    private:
    gef::Sprite sprite;
    gef::Matrix33 globalTransform;
    gef::Matrix33 spriteTransform;

    SpriteSheet* sheet;
    UInt currentAnimation;
    float elapsedTime;
    bool playing;
  };
}