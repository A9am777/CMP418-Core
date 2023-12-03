#include <maths/math_utils.h>
#include <graphics/sprite.h>
#include <maths/matrix44.h>
#include <functional>

#include "Skeleton2D.h"

namespace Animation
{
  Skeleton2D::Skeleton2D() : rootBoneID{NULL}
  {
    
  }

  Skeleton2D::DetailedBone& Skeleton2D::addBone(Label name)
  {
    return boneCollection.add(name).first;
  }

  bool Skeleton2D::bake()
  {
    // Ensure information is accurate
    linkDescriptor();

    // Traverse tree by depth, ensuring bones are packed sequentially
    {
      boneList.resize(boneCollection.getHeapSize()); // May overallocate where bone data is bad, this is not an issue
      // The recursive step
      std::function<void(DetailedBone&, UInt&)> nodeTraversal = [&](DetailedBone& parent, UInt& allocProgress)
      {
        // First, build self at the designated location
        {
          auto& flatParent = boneList[allocProgress];
          flatParent.restTransform = { parent.restPose.translation, gef::Vector2::kOne, std::abs(parent.restPose.skew.x) > std::abs(parent.restPose.skew.y) ? parent.restPose.skew.x : parent.restPose.skew.y };
          flatParent.localTransform = { gef::Vector2::kZero, gef::Vector2::kOne, .0f };
        }

        // Now explore children
        for(auto& childID : parent.children)
        {
          auto& childBone = boneCollection.get(childID);
          childBone.flattenedID = ++allocProgress;

          auto& flatBone = boneList[allocProgress];
          flatBone.parent = parent.flattenedID;

          nodeTraversal(childBone, allocProgress);
        }

      };

      // Start from the root and build the flattened skeletal structure
      auto& rootBone = boneCollection.get(rootBoneID);
      rootBone.flattenedID = 0;
      UInt progress = 0;
      nodeTraversal(rootBone, progress);

      // Set the global transform to identity
      boneList[0].globalTransform.SetIdentity();
    }

    // Generate global transforms for reference
    forwardKinematics(boneList);

    return isBaked();
  }

  bool Skeleton2D::bindTo(Skeleton2D::Instance& inst)
  {
    if (!isBaked()) { return false; }

    inst.baseSkeleton = this;
    inst.boneList = boneList;

    return true;
  }

  UInt Skeleton2D::getBoneFlatID(gef::StringId nameID) const
  {
    if (!isBaked()) { return SNULL; }
    
    auto boneID = boneCollection.getID(nameID);
    
    return boneID == SNULL ? SNULL : boneCollection.get(boneID).flattenedID;
  }

  Maths::Transform2D& Skeleton2D::getLocalPose(std::vector<FlatBone>& bones, UInt flatID)
  {
    return bones[flatID].localTransform;
  }

  void Skeleton2D::resetPose(std::vector<FlatBone>& bones)
  {
    for (size_t i = 1; i < bones.size(); ++i)
    {
      auto& bone = bones[i];
      bone.localTransform.scale = gef::Vector2::kOne;
      bone.localTransform.translation = gef::Vector2::kZero;
      bone.localTransform.rotation = 0.f;
    }
  }

  void Skeleton2D::forwardKinematics(std::vector<FlatBone>& bones)
  {
    // Start after the root and traverse the tree linearly
    for (size_t i = 1; i < bones.size(); ++i)
    {
      auto& thisBone = bones[i];
      auto& thisParent = bones[thisBone.parent];

      gef::Matrix33 localMat;
      Maths::Transform2D combinedTransform = thisBone.restTransform + thisBone.localTransform;
      combinedTransform.assignTo(localMat);

      thisBone.globalTransform = localMat * thisParent.globalTransform;
    }
  }

  void Skeleton2D::linkDescriptor()
  {
    // Clean previous attempts and search for the root
    for (size_t id = 0; id < boneCollection.getHeapSize(); ++id)
    {
      auto& bone = boneCollection.get(id);

      bone.children.clear();
      if(bone.parentName.empty()) // Assume root has no parent
      {
        rootBoneID = static_cast<UInt>(id);
      }
    }

    // Link children to their parent
    for (size_t childID = 0; childID < boneCollection.getHeapSize(); ++childID)
    {
      auto& childBone = boneCollection.get(childID);

      UInt parentHeapID = boneCollection.getID(childBone.parentName);
      if(parentHeapID != SNULL)
      {
        boneCollection.get(parentHeapID)
          .children.push_back(childID);
      }
    }
  }

  bool Skeleton2DSlots::bake(const Skeleton2D& skele)
  {
    // Build a lookup from the draw order to optimised bone order
    {
      bakedDrawOrder.resize(skele.getBoneCount());
      for (UInt slotID = 0; slotID < slotMap.getHeapSize(); ++slotID)
      {
        auto& slotInfo = slotMap.get(slotID);

        // Search for the heap bone
        UInt boneFlatID = skele.getBoneFlatID(slotInfo.boneNameID);
        // Use the root bone on failure as a precaution as it typically isn't visible
        bakedDrawOrder[slotInfo.priority] = (boneFlatID == SNULL) ? 0 : boneFlatID;
      }
    }

    return isBaked();
  }

  void Skeleton2DSlots::addSlot(Label boneName, Label skinHook)
  {
    slotMap.add(skinHook, { StringTable.Add(boneName), static_cast<UInt>(slotMap.getHeapSize())});
  }

  gef::StringId Skeleton2DSlots::getSlotBone(gef::StringId slotNameID) const
  {
    if (auto indexer = slotMap.getMetaInfo(slotNameID))
    {
      return slotMap.get(indexer->getHeapID()).boneNameID;
    }
    return NULL;
  }

  SkinnedSkeleton2D::SkinnedSkeleton2D()
  {
    currentSkin = currentAnimation = 0;
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
      animations.resize(detailedAnimationData.getHeapSize());
      for (UInt animID = 0; animID < animations.size(); ++animID)
      {
        auto& detailedSheet = detailedAnimationData.get(animID);

        auto& slotTracks = animations[animID];
        slotTracks.resize(skeleton.getBoneCount(), nullptr);

        detailedSheet.inspectTracks([&](gef::StringId slotNameID, const DopeSheet2D::DetailedTrack& detailedTrack) {
          UInt boneHeapID = skeleton.getBoneFlatID(slots.getSlotBone(slotNameID));
          if (boneHeapID == SNULL) { return; }

          // Place the baked track in the same location as the bone
          slotTracks[boneHeapID] = detailedSheet.bakeTrack(detailedTrack);
        });
      }
      
    }

    return baked;
  }

  bool SkinnedSkeleton2D::bindTo(SkinnedSkeleton2D::Instance& inst)
  {
    if (!isBaked()) { return false; }
    inst.currentSkin = 0;
    inst.baseSkeleton = this;
    skeleton.bindTo(inst.skeleInst);
    setAnimation(inst, 0);
    return true;
  }

  void SkinnedSkeleton2D::update(SkinnedSkeleton2D::Instance& inst, float dt) const
  {
    Skeleton2D::resetPose(inst.skeleInst.boneList);
    inst.animationPlayer.update(dt);

    for (size_t i = 0; i < skeleton.getBoneCount(); ++i)
    {
      // Convert to the optimised bone index
      if (UInt boneHeapID = slots.getBoneID(i)) // We don't need to draw the root
      {
        // Apply the current animation
        auto& localPose = skeleton.getLocalPose(inst.skeleInst.boneList, boneHeapID);
        localPose = localPose + inst.animationPlayer.getCurrentTransform(boneHeapID);
      }
    }

    Skeleton2D::forwardKinematics(inst.skeleInst.boneList);
  }

  void SkinnedSkeleton2D::render(SkinnedSkeleton2D::Instance& inst, gef::SpriteRenderer* renderer, const Textures::TextureCollection& textures) const
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
        finalTransform = skeleton.getBoneTransform(inst.skeleInst.boneList, boneHeapID) * finalTransform; // Apply world
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
    return detailedAnimationData.add(name).second;
  }

  void SkinnedSkeleton2D::setAnimation(SkinnedSkeleton2D::Instance& inst, UInt animID)
  {
    inst.currentAnimation = animID;
    if (isBaked())
    {
      // Copy animation data over to the player
      inst.animationPlayer.setSheet(&detailedAnimationData.get(animID));
      inst.animationPlayer.resizeTracks(skeleton.getBoneCount());
      auto& bakedTracks = animations[animID];
      for (size_t trackID = 0; trackID < bakedTracks.size(); ++trackID)
      {
        inst.animationPlayer.setTrack(trackID, bakedTracks[trackID]);
      }

      inst.animationPlayer.reset();
      inst.setPlaying(true);
    }
  }

  DopeSheet2D::DetailedTrack& SkinnedSkeleton2D::getAnimationTrack(UInt animID, Label slotName)
  {
    return detailedAnimationData.get(animID).getTrack(slotName);
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
      gef::StringId boneNameID = slotMap.getSlotBone(slot.first);
      UInt boneFlatID = skele.getBoneFlatID(boneNameID);
      if (boneFlatID == SNULL) { continue; }

      // Build the offset transform
      auto& bakedLink = bakedSubtextureLinks[boneFlatID];
      slot.second.offset.assignTo(bakedLink.offsetTransform);
      
      // Assign the atlas ID
      UInt atlasRegionID = atlas.getRegionID(atlas.getDivision(slot.second.subName));
      bakedLink.subtextureID = atlasRegionID == SNULL ? 0 : atlasRegionID;
    }

    return isBaked();
  }

  void Skeleton2DSkin::addLink(Label slotName, Label subTextureName, const Skeleton2D::BonePoseOffset& offset)
  {
    slots.insert({ StringTable.Add(slotName), {StringTable.Add(subTextureName), offset} });
  }

  void Skeleton2D::BonePoseOffset::assignTo(gef::Matrix33& transform) const
  {
    gef::Matrix33 rotMat = gef::Matrix33::kIdentity;
    rotMat.Rotate(std::abs(skew.x) > std::abs(skew.y) ? skew.x : skew.y);

    gef::Matrix33 transMat = gef::Matrix33::kIdentity;
    transMat.SetTranslation(gef::Vector2(translation.x, translation.y));

    transform = rotMat * transMat;
  }

  SkinnedSkeleton2D::Instance::Instance() : baseSkeleton{ nullptr }
  {

  }
  void SkinnedSkeleton2D::Instance::update(float dt)
  {
    if (baseSkeleton)
    {
      baseSkeleton->update(*this, dt);
    }
  }

  void SkinnedSkeleton2D::Instance::render(gef::SpriteRenderer* renderer, const Textures::TextureCollection& textures)
  {
    if (baseSkeleton)
    {
      baseSkeleton->render(*this, renderer, textures);
    }
  }

  void SkinnedSkeleton2D::Instance::setAnimation(UInt animID)
  {
    if (baseSkeleton)
    {
      baseSkeleton->setAnimation(*this, animID);
    }
  }
}