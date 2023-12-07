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
  NodeClassMeta InverseKineNode::ikClassDescriptor;
  NodeClassMeta SkeletonOutputNode::skeleOutClassDescriptor;

  void SkeletonOutputNode::process(const BlendTree* tree, float dt)
  {
    if (auto instance = const_cast<Animation::Skeleton3D::Instance*>(tree->getInstanceContext()))
    {
      if (auto pose = getInput<gef::SkeletonPose>(0))
      {
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

  InverseKineNode::InverseKineNode(Label name) : BlendNode(name, &ikClassDescriptor), previousIterations{ -1 }
  {
    setOutput(OutResolvedPoseIdx, &pose);
    setOutput(OutIterationsResolvedIdx, &previousIterations);
  }

  void InverseKineNode::process(const BlendTree* tree, float dt)
  {
    // Fetch inputs
    auto basePoseRef = getInput<const gef::SkeletonPose>(InBasePoseIdx);
    auto resolveRef = getInput<bool>(InResolveIdx);
    
    // Mirror the input base pose if not resolving IK
    if (resolveRef && !*resolveRef)
    {
      setOutput(OutResolvedPoseIdx, basePoseRef);
      return;
    }
    setOutput(OutResolvedPoseIdx, &pose);
    
    auto targetRef = getInput<const gef::Transform>(InTargetIdx);
    auto effectorLocalRef = getInput<const gef::Transform>(InEffectorLocalIdx);
    auto effectorJointNameRef = getInput<const std::string>(InEffectorJointIdx);
    auto rootJointNameRef = getInput<const std::string>(InRootJointIdx);
    auto chainDepthRef = getInput<int>(InChainDepthIdx);
    auto rightHandedRef = getInput<bool>(InRightHandedIdx);
    auto maxIterationsRef = getInput<int>(InMaxIterationsIdx);
    auto reachToleranceRef = getInput<float>(InReachToleranceIdx);
    auto unreachToleranceRef = getInput<float>(InUnreachableToleranceIdx);

    auto skele = tree->getInstanceContext();
    
    // Copy over the relevant pose
    pose = basePoseRef ? *basePoseRef : *tree->getBindPose();

    // Build the IK controller (could be cached)
    Animation::Skeleton3D::IKController ikController;
    ikController.setInstance(skele);
    ikController.setEffector(effectorLocalRef ? effectorLocalRef->translation() : gef::Vector4(-1.f, .0f, .0f));
    ikController.rightHanded = rightHandedRef ? *rightHandedRef : true; // Important!!!
    if (maxIterationsRef) { ikController.maxIterations = UInt(*maxIterationsRef); }
    if (reachToleranceRef) { ikController.reachTolerance = *reachToleranceRef; }
    if (unreachToleranceRef) { ikController.unreachableTolerance = *unreachToleranceRef; }

    // Cache the bone chain (could be lazily cached)
    ikController.bind(
      effectorJointNameRef ? gef::GetStringId(*effectorJointNameRef) : NULL,
      rootJointNameRef ? gef::GetStringId(*rootJointNameRef) : NULL,
      chainDepthRef ? *chainDepthRef : SNULL
    );

    // Run FABRIK
    gef::Matrix44 identityMat;
    identityMat.SetIdentity();
    previousIterations = ikController.resolveFABRIK(targetRef ? *targetRef : gef::Transform(identityMat), pose);
  }
}