#pragma once
#include "Animation/BlendTree/BlendNode.h"
#include <animation/skeleton.h>

namespace gef
{
  class Animation;
}

namespace Animation
{
  class Skeleton3DInstance;
}

namespace BlendTree
{
  class AnimationNode : public BlendNode
  {
    public:
    AnimationNode(Label name) : BlendNode(name, &animationClassDescriptor) {}

    static void registerClass()
    {
      animationClassDescriptor.className = "GetSkeleAnim";
      animationClassDescriptor.inputBlueprint = {};
      animationClassDescriptor.outputBlueprint = { {"Animation", Param_Animation} };
    };

    private:
    static NodeClassMeta animationClassDescriptor;
  };

  class ClipNode : public BlendNode
  {
    public:
    ClipNode(Label name);

    static void registerClass()
    {
      clipClassDescriptor.className = "SkeleClip";
      clipClassDescriptor.inputBlueprint = { {"BaseAnimation", Param_Animation}, { "SampleTime", Param_Float}};
      clipClassDescriptor.outputBlueprint = { { "SampledPose", Param_Pose } };
    };

    void setAnimation(const gef::Animation* newAnimation, const gef::SkeletonPose* newBindPose);

    enum InputIdx
    {
      InBaseAnimationIdx = 0,
      InSampleTimeIdx
    };

    enum OutputIdx
    {
      OutSampledPoseIdx = 0
    };

    protected:
    // TODO: need to clean this up by exporting to other nodes
    const gef::SkeletonPose* bindPose;
    gef::SkeletonPose pose;
    float timeElapsed = .0f;
    virtual void process(const BlendTree* tree, float dt) override;

    private:
    static NodeClassMeta clipClassDescriptor;
  };

  class SkeletonOutputNode : public BlendNode
  {
    public:
    SkeletonOutputNode(Label name) : BlendNode(name, &skeleOutClassDescriptor), instance{ nullptr } {  }

    static void registerClass()
    {
      skeleOutClassDescriptor.className = "SkeleOut";
      skeleOutClassDescriptor.inputBlueprint = { { "Pose", Param_Pose } };
      skeleOutClassDescriptor.outputBlueprint = { };
    };

    inline void setSkeletonInstance(Animation::Skeleton3DInstance* skeleInst) { instance = skeleInst; }

    protected:
    Animation::Skeleton3DInstance* instance;

    virtual void process(const BlendTree* tree, float dt) override;

    private:
    static NodeClassMeta skeleOutClassDescriptor;
  };
}