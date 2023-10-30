#include <maths/math_utils.h>
#include <graphics/sprite.h>
#include <maths/matrix44.h>
#include <functional>

#include "Skeleton.h"

namespace Animation
{
  Skeleton2D::Skeleton2D()
  {
    detailedDescriptor = new ColdDesc();
  }
  Skeleton2D::~Skeleton2D()
  {
    delete detailedDescriptor; detailedDescriptor = nullptr;
  }

  Skeleton2D::DetailedBone& Skeleton2D::addBone(Label name)
  {
    auto& desc = *detailedDescriptor;

    // Return the previous instance of the bone (if it exists)
    auto it = desc.names.find(name);
    if(it != desc.names.end())
    {
      return desc.boneCollection[it->second];
    }

    // Register the new bone, at the back of the collection
    it = desc.names.insert({ name, static_cast<UInt>(desc.boneCollection.size()) }).first;
    desc.boneCollection.emplace_back();
    return desc.boneCollection.back();
  }

  bool Skeleton2D::bake()
  {
    // Ensure information is accurate
    linkDescriptor();

    auto& desc = *detailedDescriptor;
    // Traverse tree by depth, ensuring bones are packed sequentially
    {
      boneHeap.resize(desc.boneCollection.size()); // May overallocate where bone data is bad, this is not an issue
      // The recursive step
      std::function<void(DetailedBone&, UInt&)> nodeTraversal = [&](DetailedBone& parent, UInt& allocProgress)
      {
        // First, build self at the designated location
        {
          auto& heapParent = boneHeap[allocProgress];
          heapParent.restTransform = { parent.restPose.translation, gef::Vector2::kOne, std::abs(parent.restPose.skew.x) > std::abs(parent.restPose.skew.y) ? parent.restPose.skew.x : parent.restPose.skew.y };
          heapParent.localTransform = { gef::Vector2::kZero, gef::Vector2::kOne, .0f };
        }

        // Now explore children
        for(auto& childID : parent.children)
        {
          auto& childBone = desc.boneCollection[childID];
          childBone.heapID = ++allocProgress;

          auto& heapBone = boneHeap[allocProgress];
          heapBone.parent = parent.heapID;

          nodeTraversal(childBone, allocProgress);
        }

      };

      // Start from the root and build the flattened skeletal structure
      auto& rootBone = desc.boneCollection[desc.root];
      rootBone.heapID = 0;
      UInt progress = 0;
      nodeTraversal(rootBone, progress);

      // Set the global transform to identity
      boneHeap[0].globalTransform.SetIdentity();
    }

    return isBaked();
  }

  UInt Skeleton2D::getBoneHeapID(Label name) const
  {
    if (!isBaked()) { return SNULL; }
    
    auto& desc = *detailedDescriptor;
    auto boneDescIt = desc.names.find(name);
    if (boneDescIt == desc.names.end()) { return SNULL; }
    
    return desc.boneCollection[boneDescIt->second].heapID;
  }

  Maths::Transform2D& Skeleton2D::getLocalPose(UInt heapID)
  {
    return boneHeap[heapID].localTransform;
  }

  void Skeleton2D::resetPose()
  {
    for (size_t i = 1; i < boneHeap.size(); ++i)
    {
      boneHeap[i].localTransform.scale = gef::Vector2::kOne;
      boneHeap[i].localTransform.translation = gef::Vector2::kZero;
      boneHeap[i].localTransform.rotation = 0.f;
    }
  }

  void Skeleton2D::forwardKinematics()
  {
    // Start after the root and traverse the tree linearly
    for (size_t i = 1; i < boneHeap.size(); ++i)
    {
      auto& thisBone = boneHeap[i];
      auto& thisParent = boneHeap[thisBone.parent];

      gef::Matrix33 localMat;
      Maths::Transform2D combinedTransform = thisBone.restTransform + thisBone.localTransform;
      combinedTransform.assignTo(localMat);

      thisBone.globalTransform = localMat * thisParent.globalTransform;
    }
  }

  void Skeleton2D::linkDescriptor()
  {
    auto& desc = *detailedDescriptor;

    // Clean previous attempts and search for the root
    for (size_t id = 0; id < desc.boneCollection.size(); ++id)
    {
      auto& bone = desc.boneCollection[id];

      bone.children.clear();
      if(bone.parentName.empty()) // Assume root has no parent
      {
        desc.root = static_cast<UInt>(id);
      }
    }

    // Link children to their parent
    for (size_t childID = 0; childID < desc.boneCollection.size(); ++childID)
    {
      auto& childBone = desc.boneCollection[childID];

      auto parentIt = desc.names.find(childBone.parentName);
      if (parentIt != desc.names.end())
      {
        auto& parentBone = desc.boneCollection[parentIt->second];
        parentBone.children.push_back(static_cast<UInt>(childID));
      }
    }
  }

  bool Skeleton2DSlots::bake(const Skeleton2D& skele)
  {
    // Build a lookup from the draw order to optimised bone order
    {
      bakedDrawOrder.resize(skele.getBoneCount());
      for (auto& slot : slotMap)
      {
        // Search for the heap bone
        UInt boneHeapID = skele.getBoneHeapID(slot.second.boneName);
        // Use the root bone on failure as a precaution as it typically isn't visible
        bakedDrawOrder[slot.second.priority] = (boneHeapID == SNULL) ? 0 : boneHeapID;
      }
    }

    return isBaked();
  }

  void Skeleton2DSlots::addSlot(Label boneName, Label skinHook)
  {
    auto slotIt = slotMap.find(skinHook);
    if (slotIt == slotMap.end())
    {
      slotMap.insert({ skinHook, 
        {boneName, static_cast<UInt>(slotMap.size())}
      });
    }
  }

  std::string Skeleton2DSlots::getSlotBone(Label slotName) const
  {
    auto slotIt = slotMap.find(slotName);
    if (slotIt == slotMap.end()) { return ""; }
    return slotIt->second.boneName;
  }

  SkinnedSkeleton2D::SkinnedSkeleton2D()
  {
    currentSkin = 0;
    atlas = nullptr;
    baked = false;
  }

  SkinnedSkeleton2D::~SkinnedSkeleton2D()
  {
    wipeBakedAnimations();
  }

  bool SkinnedSkeleton2D::bake(Textures::TextureAtlas* atlasTextures)
  {
    baked = true;
    baked = baked && skeleton.bake();
    baked = baked && atlasTextures->isBaked();
    atlas = atlasTextures;

    slots.bake(skeleton);
    for (auto& skin : skins)
    {
      baked = baked && skin.bake(skeleton, slots, *atlas);
    }

    // Animations
    {
      wipeBakedAnimations();
      animations.resize(detailedAnimationData.dopeCollection.size());
      for (UInt animID = 0; animID < animations.size(); ++animID)
      {
        auto& detailedSheet = detailedAnimationData.dopeCollection[animID];

        auto& slotTracks = animations[animID];
        slotTracks.resize(skeleton.getBoneCount(), nullptr);

        detailedSheet.inspectTracks([&](Label slotName, const DopeSheet2D::DetailedTrack& detailedTrack) {
          UInt boneHeapID = skeleton.getBoneHeapID(slotName); //TODO: is this the slot or bone name???
          if (boneHeapID == SNULL) { return; }

          // Place the baked track in the same location as the bone
          slotTracks[boneHeapID] = detailedSheet.bakeTrack(detailedTrack);
        });
      }
      setAnimation(0);
    }

    return baked;
  }

  void SkinnedSkeleton2D::update(float dt)
  {
    skeleton.resetPose();
    animationPlayer.update(dt);

    for (size_t i = 0; i < skeleton.getBoneCount(); ++i)
    {
      // Convert to the optimised bone index
      if (UInt boneHeapID = slots.getBoneID(i)) // We don't need to draw the root
      {
        // Apply the current animation
        auto& localPose = skeleton.getLocalPose(boneHeapID);
        localPose = localPose + animationPlayer.getCurrentTransform(boneHeapID);
      }
    }

    skeleton.forwardKinematics();
  }

  void SkinnedSkeleton2D::render(gef::SpriteRenderer* renderer, const Textures::TextureCollection& textures)
  {
    if (!atlas || !baked) { return; }

    // Hold a sprite with the texture atlas
    gef::Sprite sprite;
    if (textures.isBaked())
    {
      sprite.set_texture(textures.getTextureData(atlas->getTextureID()));
    }

    auto& skin = skins[currentSkin];

    // Traverse the draw list
    for (size_t i = 0; i < skeleton.getBoneCount(); ++i)
    {
      // Convert to the optimised bone index
      if (UInt boneHeapID = slots.getBoneID(i)) // We don't need to draw the root
      {
        auto divData = atlas->getData(skin.getSubtextureID(boneHeapID));

        // Assign texture region
        sprite.set_uv_width(divData->uv.right - divData->uv.left);
        sprite.set_uv_height(divData->uv.top - divData->uv.bottom);
        sprite.set_uv_position({ divData->uv.left, divData->uv.bottom });

        // Build the sprite transform
        gef::Matrix33 finalTransform = gef::Matrix33::kIdentity;
        finalTransform = skeleton.getBoneTransform(boneHeapID) * finalTransform; // Apply world
        finalTransform = skin.getTransform(boneHeapID) * finalTransform; // Apply sprite offset
        finalTransform = divData->transform * finalTransform; // Apply subtexture offset
        
        // TODO: sprite offset and subtexture transforms can be precomputed on skin swap!

        // Render!
        renderer->DrawSprite(sprite, finalTransform);
      }
    }
  }

  UInt SkinnedSkeleton2D::addAnimation(Label name)
  {
    auto it = detailedAnimationData.dopeNames.find(name);
    if (it == detailedAnimationData.dopeNames.end())
    {
      it = detailedAnimationData.dopeNames.insert({name, detailedAnimationData.dopeCollection.size()}).first;
      detailedAnimationData.dopeCollection.emplace_back();
    }

    return it->second;
  }

  void SkinnedSkeleton2D::setAnimation(UInt animID)
  {
    currentAnimation = animID;
    if (isBaked())
    {
      // Copy animation data over to the player
      animationPlayer.setSheet(&detailedAnimationData.dopeCollection[animID]);
      animationPlayer.resizeTracks(skeleton.getBoneCount());
      auto& bakedTracks = animations[animID];
      for (size_t trackID = 0; trackID < bakedTracks.size(); ++trackID)
      {
        animationPlayer.setTrack(trackID, bakedTracks[trackID]);
      }

      animationPlayer.reset();
      setPlay(true);
    }
  }

  DopeSheet2D::DetailedTrack& SkinnedSkeleton2D::getAnimationTrack(UInt animID, Label slotName)
  {
    return detailedAnimationData.dopeCollection[animID].getTrack(slotName);
  }

  void SkinnedSkeleton2D::wipeBakedAnimations()
  {
    for (auto& bakedAnimation : animations)
    {
      for (auto& bakedTrack : bakedAnimation)
      {
        delete bakedTrack;
      }
    }
    animations.clear();
    currentAnimation = 0;
  }

  bool Skeleton2DSkin::bake(const Skeleton2D& skele, const Skeleton2DSlots& slotMap, const Textures::TextureAtlas& atlas)
  {
    if (!skele.isBaked() || !atlas.isBaked()) { return false; }

    // Build the subtexture info as it relates to the optimised bone order
    bakedSubtextureLinks.resize(skele.getBoneCount());
    for (auto& slot : slots)
    {
      std::string boneName = slotMap.getSlotBone(slot.first);
      UInt boneHeapID = skele.getBoneHeapID(boneName);
      if (boneHeapID == SNULL) { continue; }

      // Build the offset transform
      auto& bakedLink = bakedSubtextureLinks[boneHeapID];
      slot.second.offset.assignTo(bakedLink.offsetTransform);
      
      // Assign the atlas ID
      UInt atlasDivisionID = atlas.getDivision(slot.second.subName);
      bakedLink.subtextureID = atlasDivisionID == SNULL ? 0 : atlasDivisionID;
    }

    return isBaked();
  }

  void Skeleton2DSkin::addLink(Label slotName, Label subTextureName, const Skeleton2D::BonePoseOffset& offset)
  {
    slots.insert({ slotName, {subTextureName, offset} });
  }

  void Skeleton2D::BonePoseOffset::assignTo(gef::Matrix33& transform) const
  {
    gef::Matrix33 rotMat = gef::Matrix33::kIdentity;
    rotMat.Rotate(std::abs(skew.x) > std::abs(skew.y) ? skew.x : skew.y);

    gef::Matrix33 transMat = gef::Matrix33::kIdentity;
    transMat.SetTranslation(gef::Vector2(translation.x, translation.y));

    transform = rotMat * transMat;
  }
}