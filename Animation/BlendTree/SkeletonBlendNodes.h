#pragma once
#include "Animation/BlendTree/BlendNode.h"
#include "3D/Skeleton3D.h"

namespace gef
{
  class Animation;
}

namespace BlendTree
{
  // Interpolates between two poses using a linear parameter
  class BinaryInterpolatorNode : public BlendNode
  {
    public:
    BinaryInterpolatorNode(Label name);

    static void registerClass()
    {
      binaryInterpClassDescriptor.className = "BinaryInterp";
      binaryInterpClassDescriptor.inputBlueprint = {
        { "FirstPose", Param_Pose },
        { "SecondPose", Param_Pose },
        { "BlendParameter", Param_Float }
      };
      binaryInterpClassDescriptor.outputBlueprint = { { "BlendedPose", Param_Pose } };
    };

    enum InputIdx
    {
      InFirstPoseIdx = 0,
      InSecondPoseIdx,
      InBlendParameterIdx
    };

    enum OutputIdx
    {
      OutBlendedPoseIdx = 0
    };

    protected:
    gef::SkeletonPose pose;

    virtual void process(const BlendTree* tree, float dt) override;

    private:
    static NodeClassMeta binaryInterpClassDescriptor;
  };

  // Interpolates between four poses using two linear parameters
  // In theory a hierarchical blend tree implementation can represent this
  // as three "BinaryInterpolatorNode" nodes, however this saves on some intermediates
  class QuadInterpolatorNode : public BlendNode
  {
    public:
    QuadInterpolatorNode(Label name);

    static void registerClass()
    {
      quadInterpClassDescriptor.className = "QuadInterp";
      quadInterpClassDescriptor.inputBlueprint = {
        { "TopLeftPose", Param_Pose },
        { "TopRightPose", Param_Pose },
        { "BottomLeftPose", Param_Pose },
        { "BottomRightPose", Param_Pose },
        { "BlendParameter", Param_Vec2 }
      };
      quadInterpClassDescriptor.outputBlueprint = { { "LinearPose", Param_Pose }, { "BiLinearPose", Param_Pose } };
    };

    enum InputIdx
    {
      InTopLeftPoseIdx = 0,
      InTopRightPoseIdx,
      InBottomLeftPoseIdx,
      InBottomRightPoseIdx,
      InBlendParameterIdx
    };

    enum OutputIdx
    {
      OutLinearPoseIdx = 0,
      OutBilinearPoseIdx = 1
    };

    protected:
    gef::SkeletonPose linearPose; // Require two poses anyway, may as well reflect
    gef::SkeletonPose bilinearPose; // The proper output

    virtual void process(const BlendTree* tree, float dt) override;

    private:
    static NodeClassMeta quadInterpClassDescriptor;
  };

  // Samples from an animation
  class ClipNode : public BlendNode
  {
    public:
    ClipNode(Label name);

    static void registerClass()
    {
      clipClassDescriptor.className = "SkeleClip";
      clipClassDescriptor.inputBlueprint = { 
        { "BaseAnimation", Param_Animation }, 
        { "StartTime", Param_Float }, 
        { "Progression", Param_Float }, 
        { "Rate", Param_Float }, 
        { "Playing", Param_Bool }, 
        { "Loop", Param_Bool }
      };
      clipClassDescriptor.outputBlueprint = { { "SampledPose", Param_Pose } };
    };

    enum InputIdx
    {
      InBaseAnimationIdx = 0,
      InStartTimeIdx,
      InProgressionIdx,
      InRateIdx,
      InPlayingIdx,
      InLoopIdx
    };

    enum OutputIdx
    {
      OutSampledPoseIdx = 0
    };

    protected:
    gef::SkeletonPose pose;
    float currentTime;
    
    virtual void process(const BlendTree* tree, float dt) override;

    private:
    static NodeClassMeta clipClassDescriptor;
  };

  // Applies FABRIK to a pose
  class InverseKineNode : public BlendNode
  {
    public:
    InverseKineNode(Label name);

    static void registerClass()
    {
      ikClassDescriptor.className = "IK";
      ikClassDescriptor.inputBlueprint = {
        { "BasePose", Param_Pose },
        { "EffectorJoint", Param_String },
        { "RootJoint", Param_String },
        { "ChainDepth", Param_Int },
        { "Playing", Param_Bool },
        { "Resolve", Param_Bool }
      };
      ikClassDescriptor.outputBlueprint = { { "ResolvedPose", Param_Pose } };
    };

    enum InputIdx
    {
      InBasePoseIdx = 0,
      InStartTimeIdx,
      InProgressionIdx,
      InRateIdx,
      InPlayingIdx,
      InResolveIdx
    };

    enum OutputIdx
    {
      OutResolvedPoseIdx = 0
    };

    protected:
    gef::SkeletonPose pose;

    virtual void process(const BlendTree* tree, float dt) override;

    private:
    static NodeClassMeta ikClassDescriptor;
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

    inline void setSkeletonInstance(Animation::Skeleton3D::Instance* skeleInst) { instance = skeleInst; }

    gef::SkeletonPose* aaa = nullptr;
    protected:
    Animation::Skeleton3D::Instance* instance;

    virtual void process(const BlendTree* tree, float dt) override;

    private:
    static NodeClassMeta skeleOutClassDescriptor;
  };
}