#pragma once
#include <animation/animation.h>

#include "Animation/BlendTree/BlendNode.h"

namespace BlendTree
{
  class BlendTree; // Forward declare tree

  // Define a macro for automatically building getter classes
  #define DeclareGetterNode(Typename, ParamType, RealType) \
  class Typename##GetterNode : public BlendNode\
  {\
    public:\
    Typename##GetterNode(Label name) : BlendNode(name, &get##Typename##ClassDescriptor) {}\
\
    static void registerClass()\
    {\
      auto& descriptor = get##Typename##ClassDescriptor;\
      descriptor.className = "Get"#Typename;\
      descriptor.inputBlueprint = { };\
      descriptor.outputBlueprint = { { "Value", ParamType } };\
    };\
\
    private:\
    static NodeClassMeta get##Typename##ClassDescriptor;\
  };

  DeclareGetterNode(Bool, Param_Bool, bool)
  DeclareGetterNode(Float, Param_Float, float)
  DeclareGetterNode(Animation, Param_Animation, gef::Animation)
  #undef DeclareGetterNode
}