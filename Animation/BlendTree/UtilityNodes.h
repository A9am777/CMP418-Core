#pragma once
#include <animation/animation.h>

#include "Animation/BlendTree/BlendNode.h"

namespace BlendTree
{
  class BlendTree; // Forward declare tree

  // Define a macro for automatically building getter classes
  #define DeclareGetterNode(Typename, ParamType) \
  class Typename##GetterNode : public BlendNode\
  {\
    public:\
    Typename##GetterNode(Label name) : BlendNode(name, &get##Typename##ClassDescriptor) { variableReferenceName = "undefined"; }\
\
    static void registerClass()\
    {\
      auto& descriptor = get##Typename##ClassDescriptor;\
      descriptor.className = "Get"#Typename;\
      descriptor.inputBlueprint = { };\
      descriptor.outputBlueprint = { { "Value", ParamType } };\
    };\
\
    virtual void process(const BlendTree* tree, float dt) override;\
    virtual void render() override;\
\
    private:\
    static NodeClassMeta get##Typename##ClassDescriptor;\
    std::string variableReferenceName;\
  };

  DeclareGetterNode(Bool, Param_Bool)
  DeclareGetterNode(Float, Param_Float)
  DeclareGetterNode(Int, Param_Int)
  DeclareGetterNode(String, Param_String)
  DeclareGetterNode(Animation, Param_Animation)
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

  // Define a macro for automatically building debug classes
  #define DeclareDebugNode(Typename, ParamType) \
  class Typename##DebugNode : public BlendNode\
  {\
    public:\
    Typename##DebugNode(Label name) : BlendNode(name, &debug##Typename##ClassDescriptor) {}\
\
    static void registerClass()\
    {\
      auto& descriptor = debug##Typename##ClassDescriptor;\
      descriptor.className = "Debug"#Typename;\
      descriptor.inputBlueprint = { { "Value", ParamType } };\
      descriptor.outputBlueprint = { { "Echo", ParamType } };\
    };\
\
    virtual void process(const BlendTree* tree, float dt) override;\
    virtual void render() override;\
\
    private:\
    static NodeClassMeta debug##Typename##ClassDescriptor;\
  };

  DeclareDebugNode(Bool, Param_Bool)
  DeclareDebugNode(Float, Param_Float)
  DeclareDebugNode(Int, Param_Int)
  DeclareDebugNode(String, Param_String)
  DeclareDebugNode(Animation, Param_Animation)
  #undef DeclareDebugNode
}