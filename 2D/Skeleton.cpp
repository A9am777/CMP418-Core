#include "Skeleton.h"

#include "maths/math_utils.h"
#include <functional>

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

  void Skeleton2DSlots::bake(const Skeleton2D& skele)
  {
    // Build the draw order
    {
      UInt order = 0;
      bakedDrawOrder.resize(skele.getBoneCount());
      for (auto& slot : slotMap)
      {
        bakedDrawOrder[skele.getBoneHeapID(slot.first)] = order;
        ++order;
      }
    }
  }

  void Skeleton2DSlots::addSlot(Label boneName, Label skinHook)
  {
    slotMap.insert({ boneName, skinHook });
  }

  std::string Skeleton2DSlots::getSkinSlot(Label boneName) const
  {
    auto slotIt = slotMap.find(boneName);
    if (slotIt == slotMap.end()) { return ""; }
    return slotIt->second;
  }

  SkinnedSkeleton2D::SkinnedSkeleton2D()
  {
    currentSkin = 0;
  }

  void SkinnedSkeleton2D::update(float dt)
  {
  }

  void SkinnedSkeleton2D::render(gef::SpriteRenderer& renderer)
  {
  }

  void Skeleton2DSkin::bake(const Skeleton2D& skele, const Skeleton2DSlots& slots, const Textures::TextureAtlas& atlas)
  {

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
