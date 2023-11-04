#pragma once
#include <animation/skeleton.h>
#include <graphics/mesh.h>
#include <graphics/skinned_mesh_instance.h>
#include <animation/animation.h>
#include <motion_clip_player.h>
#include <maths/matrix44.h>
#include <graphics/renderer_3d.h>

#include <unordered_map>

#include "../Defs.h"
#include "../Globals.h"
#include "../DataStructures.h"

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

    void setSkeleton(const Skeleton3D* newSkeleton);
    void setWorldTransform(const gef::Matrix44& transform);

    void setAnimation(UInt animID);
    void setAnimationBlend(UInt animID, float time = 0.f);

    void update(float dt);
    void render(gef::Renderer3D* renderer) const;

    private:
    const Skeleton3D* baseSkeleton;
    gef::SkinnedMeshInstance* instance;
    gef::SkeletonPose pose;
    MotionClipPlayer player;
    MotionClipPlayer oldPlayer;
    float blendValue = 0;
    float blendTime = 0.f;
    float blendTotalTime = 0.f;
  };
}