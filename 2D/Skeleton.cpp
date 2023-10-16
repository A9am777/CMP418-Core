#include "Skeleton.h"

#include "maths/math_utils.h"

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
    // Traverse tree by depth, ensuring children are packed sequentially
    {
      boneHeap.resize(desc.boneCollection.size()); // May overallocate where bone data is bad, this is not an issue
      // The recursive step
      auto nodeTraversal = [&](DetailedBone& parent, UInt& allocProgress)
      {
        // First, build self at the designated location
        {
          auto& heapParent = boneHeap[allocProgress];
          heapParent.localTransform.SetIdentity();
          heapParent.localTransform.Rotate(gef::DegToRad(parent.restPose.skew.x));
          heapParent.localTransform.SetTranslation(parent.restPose.translation);
        }

        // Now explore children
        for(auto& childID : parent.children)
        {
          auto& childBone = desc.boneCollection[childID];
          childBone.heapID = allocProgress;
          //childBone.

          auto& heapBone = boneHeap[allocProgress];
          heapBone.parent = ++allocProgress;
          
        }

      };
    }

    return isBaked();
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
}
