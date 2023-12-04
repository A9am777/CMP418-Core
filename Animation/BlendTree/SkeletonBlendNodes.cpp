#include <algorithm>

#include <animation/animation.h>

#include "3D/Skeleton3D.h"
#include "Animation/BlendTree/SkeletonBlendNodes.h"
#include "BlendTree.h"

namespace BlendTree
{
  NodeClassMeta BinaryInterpolatorNode::binaryInterpClassDescriptor;
  NodeClassMeta QuadInterpolatorNode::quadInterpClassDescriptor;
  NodeClassMeta ClipNode::clipClassDescriptor;
  NodeClassMeta SkeletonOutputNode::skeleOutClassDescriptor;

  void SkeletonOutputNode::process(const BlendTree* tree, float dt)
  {
    if (instance)
    {
      if (auto pose = getInput<gef::SkeletonPose>(0))
      {
        aaa = pose;
        instance->setPose(*pose);
      }
    }
  }

  ClipNode::ClipNode(Label name) : BlendNode(name, &clipClassDescriptor), currentTime{ .0f }
  {
    setOutput(OutSampledPoseIdx, &pose);
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

  BinaryInterpolatorNode::BinaryInterpolatorNode(Label name) : BlendNode(name, &binaryInterpClassDescriptor)
  {
    setOutput(OutBlendedPoseIdx, &pose);
  }

  void BinaryInterpolatorNode::process(const BlendTree* tree, float dt)
  {
    // Fetch inputs
    auto firstRef = getInput<gef::SkeletonPose>(InFirstPoseIdx);
    auto secondRef = getInput<gef::SkeletonPose>(InSecondPoseIdx);
    auto lerpRef = getInput<float>(InBlendParameterIdx);

    // Make a best guess when poses are not linked
    if (!firstRef) { setOutput(OutBlendedPoseIdx, tree->getBindPose()); return; }
    if (!secondRef) { setOutput(OutBlendedPoseIdx, firstRef); return; }

    // Annoying gef requirement. Initialise the pose to the bind pose if required
    if (pose.global_pose().size() < tree->getBindPose()->global_pose().size())
    {
      pose = *tree->getBindPose();
    }

    // Simple lerp
    pose.Linear2PoseBlend(*firstRef, *secondRef, std::clamp(lerpRef ? *lerpRef : .0f, .0f, 1.f));
    setOutput(OutBlendedPoseIdx, &pose);
  }


  QuadInterpolatorNode::QuadInterpolatorNode(Label name) : BlendNode(name, &quadInterpClassDescriptor)
  {
    setOutput(OutLinearPoseIdx, &linearPose);
    setOutput(OutBilinearPoseIdx, &bilinearPose);
  }

  void QuadInterpolatorNode::process(const BlendTree* tree, float dt)
  {
    // Fetch inputs
    auto TLRef = getInput<const gef::SkeletonPose>(InTopLeftPoseIdx);
    auto TRRef = getInput<const gef::SkeletonPose>(InTopRightPoseIdx);
    auto BLRef = getInput<const gef::SkeletonPose>(InBottomLeftPoseIdx);
    auto BRRef = getInput<const gef::SkeletonPose>(InBottomRightPoseIdx);
    auto lerpRef = getInput<gef::Vector2>(InBlendParameterIdx);

    // Set all invalid references to the bind pose!
    auto bindPose = tree->getBindPose();
    TLRef = TLRef ? TLRef : bindPose;
    TRRef = TRRef ? TRRef : bindPose;
    BLRef = BLRef ? BLRef : bindPose;
    BRRef = BRRef ? BRRef : bindPose;

    // Annoying gef requirement. Initialise the poses to the bind pose if required
    if (linearPose.global_pose().size() < tree->getBindPose()->global_pose().size())
    {
      linearPose = bilinearPose = *tree->getBindPose();
    }
    
    gef::Vector2 blendParams = lerpRef ? *lerpRef : gef::Vector2(0, 0);
    blendParams.x = std::clamp(blendParams.x, .0f, 1.f);
    blendParams.y = std::clamp(blendParams.y, .0f, 1.f);

    // Blend between the horizontal poses
    linearPose.Linear2PoseBlend(*TLRef, *TRRef, blendParams.x);
    bilinearPose.Linear2PoseBlend(*BLRef, *BRRef, blendParams.x);

    // Now combine each vertically!
    bilinearPose.Linear2PoseBlend(linearPose, bilinearPose, blendParams.y);
  }

}