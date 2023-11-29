#include <animation/animation.h>
#include "Animation/BlendTree/BlendTree.h"
#include "Animation/BlendTree/SkeletonBlendNodes.h"
#include "3D/Skeleton3D.h"

namespace bt = BlendTree;

namespace Animation
{
  Skeleton3DInstance::Skeleton3DInstance() : baseSkeleton{ nullptr }, instance{ nullptr }, blendTree{nullptr}
  {
    
  }

  Skeleton3DInstance::~Skeleton3DInstance()
  {
    if (blendTree) { delete blendTree; blendTree = nullptr; }
    if (instance) { delete instance; instance = nullptr; }
  }

  void Skeleton3DInstance::initBlendTree()
  {
    if (blendTree) { delete blendTree; }

    blendTree = new bt::BlendTree();
    blendTree->startRenderContext("Simple.json");
    {
      auto blendOutputNodeInterim = new bt::SkeletonOutputNode("Output");
      blendOutputNodeInterim->setSkeletonInstance(this);
      blendOutput = bt::BlendNodeWPtr(blendTree->setOutputNode(blendOutputNodeInterim));
    }

    for (size_t i = 0; i < baseSkeleton->getAnimationCount(); ++i)
    {
      if (auto animation = baseSkeleton->getAnimation(i))
      {
        blendTree->setReference(animation->name_id(), animation);
      }
    }

    blendTree->setBindPose(&instance->bind_pose());
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
  }

  void Skeleton3DInstance::setWorldTransform(const gef::Matrix44& transform)
  {
    instance->set_transform(transform);
  }

  void Skeleton3DInstance::setPose(const gef::SkeletonPose& newPose)
  {
    instance->UpdateBoneMatrices(newPose);
  }

  void Skeleton3DInstance::update(float dt)
  {
    instance->UpdateBoneMatrices(instance->bind_pose());
    if (blendTree)
    {
      blendTree->updateNodes(dt);
    }

    if (auto outputBlend = blendOutput.lock())
    {
      if (auto a = dynamic_cast<bt::SkeletonOutputNode*>(outputBlend.get()))
      {
        
        
      }
    }
    
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
