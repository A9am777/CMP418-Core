#include "3D/Skeleton.h"

namespace Animation
{
  Skeleton3DInstance::Skeleton3DInstance() : baseSkeleton{ nullptr }, instance{nullptr}
  {

  }

  Skeleton3DInstance::~Skeleton3DInstance()
  {
    if (instance) { delete instance; instance = nullptr; }
  }

  void Skeleton3DInstance::setSkeleton(const Skeleton3D* newSkeleton)
  {
    baseSkeleton = newSkeleton;
    if (instance) { delete instance; }

    // Instance
    instance = new gef::SkinnedMeshInstance(*newSkeleton->getSkeleton());
    instance->set_mesh(newSkeleton->getMesh());
    {
      gef::Matrix44 identity;
      identity.SetIdentity();
      instance->set_transform(identity);
    }

    // Animation player
    player.Init(instance->bind_pose());

    setAnimation(0);
  }

  void Skeleton3DInstance::setWorldTransform(const gef::Matrix44& transform)
  {
    instance->set_transform(transform);
  }

  void Skeleton3DInstance::setAnimation(UInt animID)
  {
    player.set_clip(baseSkeleton->getAnimation(animID));
    player.set_looping(true);
    player.set_anim_time(0.0f);
  }

  void Skeleton3DInstance::update(float dt)
  {
    player.Update(dt, instance->bind_pose());
    instance->UpdateBoneMatrices(player.pose());
  }

  void Skeleton3DInstance::render(gef::Renderer3D* renderer) const
  {
    renderer->DrawSkinnedMesh(*instance, instance->bone_matrices());
  }

  Skeleton3D::Skeleton3D() : skeleton{nullptr}, mesh{nullptr}
  {
    
  }

  UInt Skeleton3D::addAnimation(gef::StringId labelID, const gef::Animation* animation)
  {
    return animations.add(labelID, animation).getHeapID();
  }

  UInt Skeleton3D::getAnimationID(Label label) const
  {
    return animations.getID(label);
  }

  UInt Skeleton3D::getAnimationID(gef::StringId labelID) const
  {
    return animations.getID(labelID);
  }

}
