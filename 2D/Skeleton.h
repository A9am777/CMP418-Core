#pragma once
#include "../Defs.h"
#include "maths/matrix33.h"
#include <vector>
#include <map>
#include <list>

namespace Animation
{
  class Skeleton2D
  {
    public:
    struct BonePoseOffset
    {
      gef::Vector2 translation;
      gef::Vector2 skew;
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
    //

    inline bool isBaked() const { return !boneHeap.empty(); }

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
      gef::Matrix33 localTransform;
      gef::Matrix33 globalTransform;
    };
    std::vector<HeapedBone> boneHeap;
    //
  };
}
