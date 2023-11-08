#pragma once
#include "Animation/BlendTree/BlendNode.h"

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
    ClipNode(Label name) : BlendNode(name, &clipClassDescriptor){}

    static void registerClass()
    {
      clipClassDescriptor.className = "SkeleClip";
      clipClassDescriptor.inputBlueprint = { {"BaseAnimation", Param_Animation}, { "SampleTime", Param_Float}};
      clipClassDescriptor.outputBlueprint = { { "SampledPose", Param_Pose } };
    };

    private:
    static NodeClassMeta clipClassDescriptor;
  };
}