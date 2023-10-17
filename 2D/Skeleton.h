#pragma once
#include "../Defs.h"
#include <maths/matrix44.h>
#include <vector>
#include <map>
#include <list>
#include <maths/vector2.h>
#include <graphics/sprite_renderer.h>
#include "TextureWorks.h"

namespace Animation
{
  class Skeleton2D
  {
    public:
    struct BonePoseOffset
    {
      gef::Vector2 translation;
      gef::Vector2 skew;

      void assignTo(gef::Matrix44& transform) const;
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

    inline void setLocal(UInt heapID, const gef::Matrix44& localMat) { boneHeap[heapID].localTransform = localMat; }
    inline size_t getBoneCount() const { return boneHeap.size(); }
    inline bool isBaked() const { return !boneHeap.empty(); }
    inline const gef::Matrix44& getBoneTransform(UInt heapID) const { return boneHeap[heapID].globalTransform; }
    void forwardKinematics();

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
      gef::Matrix44 localTransform;
      gef::Matrix44 globalTransform;
    };
    std::vector<HeapedBone> boneHeap;
    //
  };

  class Skeleton2DSlots
  {
    public:

    bool bake(const Skeleton2D& skele);

    void addSlot(Label boneName, Label skinHook);
    std::string getSlotBone(Label slotName) const;

    inline bool isBaked() const { return !bakedDrawOrder.empty(); }

    private:
    struct DetailedSlotInfo
    {
      std::string boneName;
      UInt priority; // Draw order
    };
    std::map<std::string, DetailedSlotInfo> slotMap; // Slot name to info
    std::vector<UInt> bakedDrawOrder; // Bone heap to draw order
  };

  // Assigns a transform and subtexture to each bone part
  class Skeleton2DSkin
  {
    public:

    bool bake(const Skeleton2D& skele, const Skeleton2DSlots& slotMap, const Textures::TextureAtlas& atlas);

    void addLink(Label slotName, Label subTextureName, const Skeleton2D::BonePoseOffset& offset);

    inline bool isBaked() const { return !bakedSubtextureLinks.empty(); }
    inline UInt getSubtextureID(UInt boneHeapID) const { return bakedSubtextureLinks[boneHeapID].subtextureID; }
    inline const gef::Matrix44& getTransform(UInt boneHeapID) const { return bakedSubtextureLinks[boneHeapID].offsetTransform; }

    private:
    struct DetailedSkinPart
    {
      std::string subName; // The name of the designated subtexture part
      Skeleton2D::BonePoseOffset offset;
    };
    struct SkinPart
    {
      gef::Matrix44 offsetTransform;
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

    bool bake(Textures::TextureAtlas* atlas);

    void update(float dt);
    void render(gef::SpriteRenderer* renderer, const Textures::TextureCollection& textures);

    inline UInt addSkin() { skins.emplace_back(); return static_cast<UInt>(skins.size() - 1); }
    inline void setSkin(UInt id) { currentSkin = id; }
    inline Skeleton2DSkin& getSkin(UInt id) { return skins[id]; }
    inline Skeleton2D& getSkeleton() { return skeleton; }
    inline Skeleton2DSlots& getSlots() { return slots; }
    inline bool isBaked() const { return baked; }

    private:
    Skeleton2DSlots slots;
    std::vector<Skeleton2DSkin> skins;
    UInt currentSkin;
    Textures::TextureAtlas* atlas;
    Skeleton2D skeleton;
    bool baked;
  };
}
