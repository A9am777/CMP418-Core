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
      friend Skeleton2D;

      // Raw information
      std::string parentName;
      UInt length;
      BonePoseOffset restPose;

      // Managed
      private:
      UInt flattenedID;
      std::list<UInt> children;
    };

    // Bone traversal is optimised via a depth-first flattening of the skeleton tree
    struct FlatBone
    {
      UInt parent;
      Maths::Transform2D restTransform;
      Maths::Transform2D localTransform;
      gef::Matrix33 globalTransform;
    };

    // Simple container of a flattened bone list instance relative to a skeleton asset
    struct Instance
    {
      std::vector<FlatBone> boneList;
      Skeleton2D* baseSkeleton;
    };

    Skeleton2D();

    // Slow
    DetailedBone& addBone(Label name);
    bool bake(); // Build an optimised representation of the skeleton
    bool bindTo(Skeleton2D::Instance& inst); // Transfers the bone list to an instance for use
    UInt getBoneFlatID(gef::StringId nameID) const;
    //
    inline size_t getBoneCount() const { return boneList.size(); }
    inline bool isBaked() const { return !boneList.empty(); }
    
    static inline void setLocal(std::vector<FlatBone>& bones, UInt flatID, const Maths::Transform2D& localTransform) { bones[flatID].localTransform = localTransform; }
    // The root bone stores the transform for the entire rig!
    static void setWorldTransform(std::vector<FlatBone>& bones, const gef::Matrix33& worldMat) { bones[0].globalTransform = worldMat; }
    inline static const gef::Matrix33& getBoneTransform(const std::vector<FlatBone>& bones, UInt flatID) { return bones[flatID].globalTransform; }
    static Maths::Transform2D& getLocalPose(std::vector<FlatBone>& bones, UInt flatID); // Fetch the local pose of this bone so transforms can be applied
    static void resetPose(std::vector<FlatBone>& bones); // Reset local transforms so new ones can be applied
    static void forwardKinematics(std::vector<FlatBone>& bones); // Compute world transforms down a skeletal structure

    private:
    void linkDescriptor(); // Propagate skeletal structure

    NamedHeap<DetailedBone> boneCollection;
    UInt rootBoneID; // The ID of the presumed root bone
    std::vector<FlatBone> boneList; // Default 'bind' transforms to provide instances in a baked, flattened structure
  };

  class Skeleton2DSlots
  {
    public:
    Skeleton2DSlots() = default;

    bool bake(const Skeleton2D& skele);

    void addSlot(Label boneName, Label skinHook);
    gef::StringId getSlotBone(gef::StringId slotNameID) const;

    inline bool isBaked() const { return !bakedDrawOrder.empty(); }
    inline UInt getBoneID(UInt idxDraw) const { return bakedDrawOrder[idxDraw]; }

    private:
    struct DetailedSlotInfo
    {
      gef::StringId boneNameID;
      UInt priority; // Draw order
    };
    NamedHeap<DetailedSlotInfo> slotMap; // Slot name to info
    std::vector<UInt> bakedDrawOrder; // Draw order to bone list (lookup)
  };

  // Assigns a transform and subtexture to each bone part
  class Skeleton2DSkin
  {
    public:
    Skeleton2DSkin() = default;

    bool bake(const Skeleton2D& skele, const Skeleton2DSlots& slotMap, const Textures::TextureAtlas& atlas);

    void addLink(Label slotName, Label subTextureName, const Skeleton2D::BonePoseOffset& offset);

    inline bool isBaked() const { return !bakedSubtextureLinks.empty(); }
    inline UInt getSubtextureID(UInt boneFlatID) const { return bakedSubtextureLinks[boneFlatID].subtextureID; }
    inline const gef::Matrix33& getTransform(UInt boneFlatID) const { return bakedSubtextureLinks[boneFlatID].offsetTransform; }

    private:
    struct DetailedSkinPart
    {
      gef::StringId subName; // The name of the designated subtexture part
      Skeleton2D::BonePoseOffset offset;
    };
    struct SkinPart
    {
      gef::Matrix33 offsetTransform;
      UInt subtextureID;
    };

    std::unordered_map<gef::StringId, DetailedSkinPart> slots; // Slot to transform mapped to skin data
    std::vector<SkinPart> bakedSubtextureLinks; // Bone list to its subtexture and transform
  };

  // Full ensemble
  class SkinnedSkeleton2D
  {
    public:
    class Instance
    {
      friend SkinnedSkeleton2D;

      public:
      Instance();

      void update(float dt);
      void render(gef::SpriteRenderer* renderer, const Textures::TextureCollection& textures);
      void setAnimation(UInt animID);

      inline void setWorldTransform(const gef::Matrix33& worldMat) { Skeleton2D::setWorldTransform(skeleInst.boneList, worldMat); }
      inline void setPlaying(bool animationPlay) { animationPlayer.setPlaying(animationPlay); }
      inline SkinnedSkeleton2D* getSkinnedSkeleton() { return baseSkeleton; }
      inline void setSkin(UInt id) { currentSkin = id; }
      inline bool getPlaying() const { return animationPlayer.isPlaying(); }
      inline UInt getCurrentAnim() const { return currentAnimation; }

      private:
      SkinnedSkeleton2D* baseSkeleton;
      Skeleton2D::Instance skeleInst;
      UInt currentSkin;

      DopePlayer2D animationPlayer;
      UInt currentAnimation;
    };

    SkinnedSkeleton2D();
    ~SkinnedSkeleton2D();

    bool bake(Textures::TextureAtlas* atlas);
    bool bindTo(SkinnedSkeleton2D::Instance& inst); // Transfers to an instance for use
    void setAnimation(SkinnedSkeleton2D::Instance& inst, UInt anim);

    UInt addAnimation(Label name);
    inline DopeSheet2D& getAnimationData(UInt animID) { return detailedAnimationData.get(animID); }
    DopeSheet2D::DetailedTrack& getAnimationTrack(UInt animID, Label slotName);

    inline UInt addSkin() { skins.emplace_back(); return static_cast<UInt>(skins.size() - 1); }
    inline Skeleton2DSkin& getSkin(UInt id) { return skins[id]; }
    inline Skeleton2D& getSkeleton() { return skeleton; }
    inline Skeleton2DSlots& getSlots() { return slots; }
    inline Textures::TextureAtlas* getAtlas() { return atlas; }
    inline const NamedHeap<DopeSheet2D>& getAnimations() const { return detailedAnimationData; }
    inline size_t getAnimationCount() const { return detailedAnimationData.getHeapSize(); }
    inline bool isBaked() const { return baked; }

    private:
    void wipeBakedAnimations();

    Skeleton2DSlots slots;
    std::vector<Skeleton2DSkin> skins;

    Textures::TextureAtlas* atlas;
    Skeleton2D skeleton;

    NamedHeap<DopeSheet2D> detailedAnimationData;
    std::vector<std::vector<DopeSheet2D::BakedTrack*>> animations;

    bool baked;
  };
}
