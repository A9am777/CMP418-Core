#include "3D/Skeleton3D.h"
#include <animation/animation.h>
#include "Animation/BlendTree/SkeletonBlendNodes.h"

namespace BlendTree
{
  NodeClassMeta AnimationNode::animationClassDescriptor;
  NodeClassMeta ClipNode::clipClassDescriptor;
  NodeClassMeta SkeletonOutputNode::skeleOutClassDescriptor;

  void SkeletonOutputNode::process(float dt)
  {
    if (instance)
    {
      if (auto pose = getInput<gef::SkeletonPose>(0))
      {
        instance->setPose(*pose);
      }
    }
  }

  ClipNode::ClipNode(Label name) : BlendNode(name, &clipClassDescriptor), animation{nullptr}
  {
    outputs[0] = &pose;
  }

  void ClipNode::setAnimation(const gef::Animation* newAnimation, const gef::SkeletonPose* newBindPose)
  {
    animation = newAnimation;
    bindPose = newBindPose;

    pose = *bindPose;
  }

  void ClipNode::process(float dt)
  {
    if (animation)
    {
      pose.SetPoseFromAnim(*animation, *bindPose, timeElapsed);
    }
    timeElapsed += dt;
  }
}