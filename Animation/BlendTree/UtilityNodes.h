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
    DeclareGetterNode(Int, Param_Int, int)
    DeclareGetterNode(String, Param_String, std::string)
    DeclareGetterNode(Animation, Param_Animation, gef::Animation)
    #undef DeclareGetterNode

    // Define a macro for automatically building setter classes (with their own underlying storage)
    #define DeclareSetterNode(Typename, ParamType, RealType) \
  class Typename##SetterNode : public BlendNode\
  {\
    public:\
    Typename##SetterNode(Label name, RealType defaultVal = RealType()) : BlendNode(name, &set##Typename##ClassDescriptor)\
    {\
      nodeValue = defaultVal;\
      outputs[0] = &nodeValue;\
    }\
\
    static void registerClass()\
    {\
      auto& descriptor = set##Typename##ClassDescriptor;\
      descriptor.className = "Set"#Typename;\
      descriptor.inputBlueprint = { };\
      descriptor.outputBlueprint = { { "Value", ParamType } };\
    };\
\
    virtual void render() override;\
\
    protected:\
    RealType nodeValue;\
    private:\
    static NodeClassMeta set##Typename##ClassDescriptor;\
  };

  DeclareSetterNode(Bool, Param_Bool, bool)
  DeclareSetterNode(Float, Param_Float, float)
  DeclareSetterNode(Int, Param_Int, int)
  DeclareSetterNode(String, Param_String, std::string)
  #undef DeclareSetterNode
}