#pragma once

#include <maths/matrix33.h>
#include <maths/vector2.h>
#include <graphics/sprite_renderer.h>
#include <vector>
#include <map>
#include <list>
#include "../Defs.h"
#include "TextureWorks.h"
#include "DopeSheet.h"

namespace Animation
{
  class Skeleton2D
  {
    public:
    struct BonePoseOffset
    {
      gef::Vector2 translation;
      gef::Vector2 skew;

      void assignTo(gef::Matrix33& transform) const;
    };

    // Expensive bone information to offload
    struct DetailedBone
    {
      // Raw information
      std::string parentName;
      UInt length;
      BonePoseOffset restPose;

      // Managed
      UInt heapID;
      std::list<UInt> children;
    };

    Skeleton2D();
    ~Skeleton2D();

    // Slow
    DetailedBone& addBone(Label name);
    bool bake(); // Build an optimised representation of the skeleton
    UInt getBoneHeapID(Label name) const;
    //

    // The root bone stores the transform for the entire rig!
    void setWorldTransform(const gef::Matrix33& worldMat) { boneHeap[0].globalTransform = worldMat; }
    inline void setLocal(UInt heapID, const gef::Matrix33& localMat) { boneHeap[heapID].localTransform = localMat; }
    inline size_t getBoneCount() const { return boneHeap.size(); }
    inline bool isBaked() const { return !boneHeap.empty(); }
    inline const gef::Matrix33& getBoneTransform(UInt heapID) const { return boneHeap[heapID].globalTransform; }
    
    gef::Matrix33& getLocalPose(UInt heapID); // Fetch the local pose of this bone so transforms can be applied
    void resetPose(); // Reset all local transforms so new ones can be applied
    void forwardKinematics(); // Compute world transforms down the skeletal structure

    private:
    void linkDescriptor(); // Propagate skeletal structure

    // COLD MEMORY //
    struct ColdDesc
    {
      std::vector<DetailedBone> boneCollection; // Collection of unoptimised bone information (by ID)
      std::map<std::string, UInt> names; // Exposes bone name to its ID
      UInt root; // The ID of the presumed root bone
    };
    ColdDesc* detailedDescriptor; // 
    //

    // HOT MEMORY //
    // Bone traversal is optimised via a depth-first flattening of the skeleton tree
    struct HeapedBone
    {
      UInt parent;
      gef::Matrix33 restTransform;
      gef::Matrix33 localTransform;
      gef::Matrix33 globalTransform;
    };
    std::vector<HeapedBone> boneHeap;
    //
  };

  class Skeleton2DSlots
  {
    public:
    Skeleton2DSlots() = default;

    bool bake(const Skeleton2D& skele);

    void addSlot(Label boneName, Label skinHook);
    std::string getSlotBone(Label slotName) const;

    inline bool isBaked() const { return !bakedDrawOrder.empty(); }
    inline UInt getBoneID(UInt idxDraw) const { return bakedDrawOrder[idxDraw]; }

    private:
    struct DetailedSlotInfo
    {
      std::string boneName;
      UInt priority; // Draw order
    };
    std::map<std::string, DetailedSlotInfo> slotMap; // Slot name to info
    std::vector<UInt> bakedDrawOrder; // Draw order to bone heap (lookup)
  };

  // Assigns a transform and subtexture to each bone part
  class Skeleton2DSkin
  {
    public:
    Skeleton2DSkin() = default;

    bool bake(const Skeleton2D& skele, const Skeleton2DSlots& slotMap, const Textures::TextureAtlas& atlas);

    void addLink(Label slotName, Label subTextureName, const Skeleton2D::BonePoseOffset& offset);

    inline bool isBaked() const { return !bakedSubtextureLinks.empty(); }
    inline UInt getSubtextureID(UInt boneHeapID) const { return bakedSubtextureLinks[boneHeapID].subtextureID; }
    inline const gef::Matrix33& getTransform(UInt boneHeapID) const { return bakedSubtextureLinks[boneHeapID].offsetTransform; }

    private:
    struct DetailedSkinPart
    {
      std::string subName; // The name of the designated subtexture part
      Skeleton2D::BonePoseOffset offset;
    };
    struct SkinPart
    {
      gef::Matrix33 offsetTransform;
      UInt subtextureID;
    };

    std::map<std::string, DetailedSkinPart> slots; // Slot to transform mapped to skin data
    std::vector<SkinPart> bakedSubtextureLinks; // Bone heap to its subtexture and transform
  };

  // Full ensemble
  class SkinnedSkeleton2D
  {
    public:
    SkinnedSkeleton2D();
    ~SkinnedSkeleton2D();

    bool bake(Textures::TextureAtlas* atlas);

    void update(float dt);
    void render(gef::SpriteRenderer* renderer, const Textures::TextureCollection& textures);

    UInt addAnimation(Label name);
    void setAnimation(UInt anim);
    void setPlay(bool animationPlay) { animationPlayer.setPlaying(animationPlay); }
    inline DopeSheet2D& getAnimationData(UInt animID) { return detailedAnimationData.dopeCollection[animID]; }
    DopeSheet2D::DetailedTrack& getAnimationTrack(UInt animID, Label slotName);

    inline UInt addSkin() { skins.emplace_back(); return static_cast<UInt>(skins.size() - 1); }
    inline void setSkin(UInt id) { currentSkin = id; }
    inline Skeleton2DSkin& getSkin(UInt id) { return skins[id]; }
    inline Skeleton2D& getSkeleton() { return skeleton; }
    inline Skeleton2DSlots& getSlots() { return slots; }
    inline bool isBaked() const { return baked; }

    private:
    struct DetailedAnimations
    {
      std::map<std::string, UInt> dopeNames;
      std::vector<DopeSheet2D> dopeCollection;
    };

    void wipeBakedAnimations();

    Skeleton2DSlots slots;
    std::vector<Skeleton2DSkin> skins;
    UInt currentSkin;

    Textures::TextureAtlas* atlas;
    Skeleton2D skeleton;

    DetailedAnimations detailedAnimationData;
    std::vector<std::vector<DopeSheet2D::BakedTrack*>> animations;
    DopePlayer2D animationPlayer;
    UInt currentAnimation;

    bool baked;
  };
}
