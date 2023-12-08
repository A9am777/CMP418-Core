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
  // Denotes a constraint for the DOF for a joint
  class JointConstraint
  {
    public:
    JointConstraint();

    // Checks a configuration satisfies this constraint
    bool isValid(const gef::Vector4& jointForward, const gef::Vector4& boneDirection) const;
    // Reorients a configuration to satisfy this constraint
    void snapBack(const gef::Vector4& jointPos, const gef::Vector4& jointForward, gef::Vector4& boneDirection, gef::Vector4& childPos, const float boneLength) const;

    bool applyOrientation;
    float orientationalDOF; // Orientational constraint, in rads
  };

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

      void setInstance(const Skeleton3D::Instance* newInst);
      void setEffector(const gef::Vector4& local); // The effector in local space of the final bone

      // Resolves the bones this controller will solve for. From effector bone towards a specified root or for a set depth
      void bind(gef::StringId effectorJoint, gef::StringId rootJoint = 0, size_t maxDepth = SNULL);

      // Applies FABRIK from the end effector towards the target for a pose. Returns the number of iterations of the algorithm that occured
      UInt resolveFABRIK(const gef::Transform& target, gef::SkeletonPose& pose);

      float unreachableTolerance; // The distance threshold to bias against the fast pass for a nicer transition to full extension
      float reachTolerance; // The distance threshold deemed to terminate the algorithm
      UInt maxIterations; // Number of passes
      bool rightHanded = true; // Denotes whether to solve for a right handed or left handed coordinate system. A bit strange, but the skeletal visual data flips at points

      protected:
      // Composes a matrix from orthogonal basis vectors
      static void setOrientation(gef::Matrix44& mat, const gef::Vector4& forward, const gef::Vector4& right, const gef::Vector4& up);
      // Composes a matrix at a location oriented towards a target, with a right vector bias (can collapse rarely)
      static void setOrientationTowards(gef::Matrix44& mat, const gef::Vector4& location, const gef::Vector4& target, gef::Vector4 biasRight, bool rightHanded = true);

      private:
      const Skeleton3D::Instance* skeletonInstance; // The instance this controller is operating on

      std::vector<int> boneIndices; // Ordering of bones to iterate over
      std::vector<gef::Vector4> jointLocal; // The local bind position of the joint to its parent bone
      std::vector<float> jointLength; // The length of the bone connected to each joint (model space)
      float staticTotalLength; // The precomputed accumulated length of all bones (model space)

      float effectorLength; // Length to the end effector to resolve for
    };

    Skeleton3D();

    void bindTo(Skeleton3D::Instance& inst); // Transfers to an instance for use
    void setSkeleton(const gef::Skeleton* newSkeleton);
    inline void setMesh(const gef::Mesh* newMesh) { mesh = newMesh; }
    UInt addAnimation(gef::StringId labelID, const gef::Animation* animation);
    UInt getAnimationID(Label label) const;
    UInt getAnimationID(gef::StringId labelID) const;

    inline const NamedHeap<const gef::Animation*>& getAnimations() const { return animations; }
    inline const gef::Animation* getAnimation(UInt id) const { return animations.get(id); }
    inline const size_t getAnimationCount() const { return animations.getHeapSize(); }
    inline const gef::Skeleton* getSkeleton() const { return skeleton; }
    inline const gef::Mesh* getMesh() const { return mesh; }
    inline std::vector<JointConstraint>& getConstraints() { return constraints; }
    inline const std::vector<JointConstraint>& getConstraints() const { return constraints; }

    protected:
    const gef::Skeleton* skeleton;
    const gef::Mesh* mesh;
    std::vector<JointConstraint> constraints;

    NamedHeap<const gef::Animation*> animations;
  };
}