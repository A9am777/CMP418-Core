#include "3D/Skeleton3D.h"
#include <animation/animation.h>
#include "Animation/BlendTree/SkeletonBlendNodes.h"

namespace BlendTree
{
  NodeClassMeta AnimationNode::animationClassDescriptor;
  NodeClassMeta ClipNode::clipClassDescriptor;
  NodeClassMeta SkeletonOutputNode::skeleOutClassDescriptor;

  void SkeletonOutputNode::process(const BlendTree* tree, float dt)
  {
    if (instance)
    {
      if (auto pose = getInput<gef::SkeletonPose>(0))
      {
        instance->setPose(*pose);
      }
    }
  }

  ClipNode::ClipNode(Label name) : BlendNode(name, &clipClassDescriptor)
  {
    outputs[0] = &pose;
  }

  void ClipNode::setAnimation(const gef::Animation* newAnimation, const gef::SkeletonPose* newBindPose)
  {
    bindPose = newBindPose;

    pose = *bindPose;
  }

  void ClipNode::process(const BlendTree* tree, float dt)
  {
    auto animation = getInput<gef::Animation>(0);
    auto time = getInput<float>(1);
    if (animation)
    {
      pose.SetPoseFromAnim(*animation, *bindPose, time ? *time : .0f);
    }
    else
    {
      pose = *bindPose;
    }
    timeElapsed += dt;
  }
}