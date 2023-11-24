#include "3D/Skeleton3D.h"
#include <animation/animation.h>
#include "Animation/BlendTree/SkeletonBlendNodes.h"
#include "BlendTree.h"

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

  ClipNode::ClipNode(Label name) : BlendNode(name, &clipClassDescriptor), currentTime{ .0f }
  {
    outputs[OutSampledPoseIdx] = &pose;
  }

  void ClipNode::process(const BlendTree* tree, float dt)
  {
    // Fetch inputs
    auto animationRef = getInput<gef::Animation>(InBaseAnimationIdx);
    auto startTimeRef = getInput<float>(InStartTimeIdx);
    auto progressTimeRef = getInput<float>(InProgressionIdx);
    auto rateRef = getInput<float>(InRateIdx);
    auto isPlayingRef = getInput<bool>(InPlayingIdx);
    auto isLoopingRef = getInput<bool>(InLoopIdx);
    
    // Set the time reference to either my internal timer or the input
    progressTimeRef = progressTimeRef ? progressTimeRef : &currentTime;

    if (animationRef)
    {
      // Store a copy of time with the applied offset
      float offset = startTimeRef ? *startTimeRef : .0f;
      float accumulatedTime = *progressTimeRef + offset;
      
      // When looping, apply duration modulo to time. Wrap around if -ve
      if (!isLoopingRef || *isLoopingRef)
      {
        if (accumulatedTime > 0)
        {
          accumulatedTime = fmodf(accumulatedTime, animationRef->duration());
        }
        else
        {
          accumulatedTime += animationRef->duration();
        }

        // Undo offset to fetch back the looped time
        *progressTimeRef = accumulatedTime - offset;
      }

      // Annoying gef requirement. Initialise the pose to the bind pose if required
      if (pose.global_pose().size() < tree->getBindPose()->global_pose().size())
      {
        pose = *tree->getBindPose();
      }

      pose.SetPoseFromAnim(*animationRef, *tree->getBindPose(), accumulatedTime);

      // Update time
      if (!isPlayingRef || *isPlayingRef)
      {
        *progressTimeRef += dt * (rateRef ? *rateRef : 1.f);
      }

      outputs[OutSampledPoseIdx] = &pose;
    }
    else
    {
      // Use the bind pose if there is no animation
      outputs[OutSampledPoseIdx] = (void*)tree->getBindPose();
    }
  }
}