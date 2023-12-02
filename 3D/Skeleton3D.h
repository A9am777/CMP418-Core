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
    Skeleton3D();

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

  class Skeleton3DInstance
  {
    public:
    Skeleton3DInstance();
    ~Skeleton3DInstance();

    void initBlendTree();
    void setSkeleton(const Skeleton3D* newSkeleton);
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
}