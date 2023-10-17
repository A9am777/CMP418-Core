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
          parent.restPose.assignTo(heapParent.localTransform);
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

  void Skeleton2D::forwardKinematics()
  {
    // Start after the root and traverse the tree linearly
    for (size_t i = 1; i < boneHeap.size(); ++i)
    {
      auto& thisBone = boneHeap[i];
      thisBone.globalTransform = boneHeap[thisBone.parent].globalTransform * thisBone.localTransform;
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
    // Build the draw order as it relates to the optimised bone order
    {
      bakedDrawOrder.resize(skele.getBoneCount());
      for (auto& slot : slotMap)
      {
        bakedDrawOrder[skele.getBoneHeapID(slot.second.boneName)] = slot.second.priority;
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
    baked = false;
  }

  bool SkinnedSkeleton2D::bake(Textures::TextureAtlas* atlas)
  {
    baked = true;
    baked = baked && skeleton.bake();
    baked = baked && atlas->isBaked();

    slots.bake(skeleton);
    for (auto& skin : skins)
    {
      baked = baked && skin.bake(skeleton, slots, *atlas);
    }

    return baked;
  }

  void SkinnedSkeleton2D::update(float dt)
  {
    skeleton.forwardKinematics();
  }

  void SkinnedSkeleton2D::render(gef::SpriteRenderer* renderer, const Textures::TextureCollection& textures)
  {
    if (!atlas || !baked) { return; }

    // Hold a sprite with the texture atlas
    gef::Sprite sprite;
    sprite.set_texture(textures.getTextureData(atlas->getTextureID()));

    auto& skin = skins[currentSkin];

    // Traverse the linear bone list
    renderer->Begin();
    for (UInt boneID = 0; boneID < skeleton.getBoneCount(); ++boneID)
    {
      auto divData = atlas->getData(skin.getSubtextureID(boneID));

      // Assign texture region
      sprite.set_uv_width(divData->uv.right - divData->uv.left);
      sprite.set_uv_height(divData->uv.top - divData->uv.bottom);
      sprite.set_uv_position({ divData->uv.left, divData->uv.bottom });
      
      // Resize
      sprite.set_width(divData->size.x);
      sprite.set_height(divData->size.y);
      
      // Compute the subtexture transform
      gef::Matrix44 transform = skeleton.getBoneTransform(boneID); // World
      transform = divData->transform * transform; // Subtexture offset transform

      {
        gef::Matrix33 worldTransform = Maths::maskMat44(transform);
        renderer->DrawSprite(sprite, worldTransform);
      }
    }
    renderer->End();
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

  }

  void Skeleton2DSkin::addLink(Label slotName, Label subTextureName, const Skeleton2D::BonePoseOffset& offset)
  {
    slots.insert({ slotName, {subTextureName, offset} });
  }

  void Skeleton2D::BonePoseOffset::assignTo(gef::Matrix44& transform) const
  {
    transform.SetIdentity();
    transform.RotationZ(gef::DegToRad(skew.x));
    transform.SetTranslation(gef::Vector4(translation.x, translation.y, .0f));
  }
}
