#pragma once
#include <animation/skeleton.h>
#include <graphics/mesh.h>
#include <graphics/skinned_mesh_instance.h>
#include <maths/matrix44.h>
#include <graphics/renderer_3d.h>

#include <unordered_map>

#include "../Defs.h"
#include "../Globals.h"
#include "../DataStructures.h"

#include "Animation/BlendTree/BlendNode.h"

class MotionClipPlayer;
namespace gef
{
  class Animation;
}

namespace BlendTree
{
  class BlendTree;
}

namespace Animation
{
  class Skeleton3D
  {
    public:
    class Instance
    {
      friend Skeleton3D;

      public:
      Instance();
      ~Instance();

      void initBlendTree();
      void setWorldTransform(const gef::Matrix44& transform);
      void setPose(const gef::SkeletonPose& newPose);

      inline const Skeleton3D* getSkeleton() const { return baseSkeleton; }
      inline const gef::SkeletonPose* getBindPose() const { return &instance->bind_pose(); }

      //TEST
      gef::SkeletonPose* getPose();
      inline gef::SkinnedMeshInstance* getInstanceMeshUghWhy() { return instance; }

      void update(float dt);
      void render(gef::Renderer3D* renderer) const;

      inline BlendTree::BlendTree* getBlendTree() { return blendTree; }

      private:
      const Skeleton3D* baseSkeleton;
      gef::SkinnedMeshInstance* instance;
      MotionClipPlayer* player;

      BlendTree::BlendTree* blendTree;
      BlendTree::BlendNodeWPtr blendOutput;
    };

    class IKController
    {
      public:
      IKController();

      // Resolves the bones this controller will solve for. From effector bone towards a specified root or for a set depth
      void bind(const Skeleton3D* skeleton, gef::StringId effectorJoint, gef::StringId rootJoint = 0, size_t maxDepth = SNULL);

      // Applies FABRIK from the end effector towards the target for a pose. Returns the number of iterations of the algorithm that occured
      UInt resolveFABRIK(const gef::Transform& target, const gef::SkeletonPose& bindPose, gef::SkeletonPose& pose, const gef::SkinnedMeshInstance& animatedModel);

      float unreachableTolerance = 7.5f; // The distance threshold to bias against the fast pass for a nicer transition to full extension
      float reachTolerance = 0.001f; // The distance threshold deemed to terminate the algorithm
      UInt maxIterations = 25; // Number of passes
      float effectorLength = 1.f; // Length along the end effector forward to resolve for
      private:
      std::vector<int> boneIndices;
    };

    Skeleton3D();

    void bindTo(Skeleton3D::Instance& inst); // Transfers to an instance for use
    inline void setSkeleton(const gef::Skeleton* newSkeleton) { skeleton = newSkeleton; }
    inline void setMesh(const gef::Mesh* newMesh) { mesh = newMesh; }
    UInt addAnimation(gef::StringId labelID, const gef::Animation* animation);
    UInt getAnimationID(Label label) const;
    UInt getAnimationID(gef::StringId labelID) const;

    inline const NamedHeap<const gef::Animation*>& getAnimations() const { return animations; }
    inline const gef::Animation* getAnimation(UInt id) const { return animations.get(id); }
    inline const size_t getAnimationCount() const { return animations.getHeapSize(); }
    inline const gef::Skeleton* getSkeleton() const { return skeleton; }
    inline const gef::Mesh* getMesh() const { return mesh; }

    protected:
    const gef::Skeleton* skeleton;
    const gef::Mesh* mesh;

    NamedHeap<const gef::Animation*> animations;
  };
}