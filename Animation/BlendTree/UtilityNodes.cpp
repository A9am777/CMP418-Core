#include "Animation/BlendTree/UtilityNodes.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <ax/Widgets.h>
#include <ax/builders.h>
#include <imgui_stdlib.h>

#include "BlendTree.h"

namespace BlendTree
{
  // Implement the class, including declaring the static class descriptors
  #define ImplementGetterNode(Typename) NodeClassMeta Typename##GetterNode::get##Typename##ClassDescriptor; \
  void Typename##GetterNode::process(const BlendTree* tree, float dt)\
  {\
    outputs[OutValueIdx] = (void*)tree->get##Typename(variableReferenceName); /* A bit slow. Could be improved...*/\
  }\
  void Typename##GetterNode::render()\
  {\
    ne::Utilities::BlueprintNodeBuilder builder;\
    builder.Begin(imguiPinStart);\
    renderStandardHeader(builder);\
    if (outputs[OutValueIdx])\
    {\
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, .85f, .0f, 1.f));\
    }\
    ImGui::PushItemWidth(150.0f);\
      ImGui::InputText("Param Name", &variableReferenceName);\
    ImGui::PopItemWidth();\
    if (outputs[OutValueIdx])\
    {\
      ImGui::PopStyleColor();\
    }\
    builder.Middle();\
    renderStandardOutputPins(builder);\
    builder.End();\
  }

  ImplementGetterNode(Bool);
  ImplementGetterNode(Float);
  ImplementGetterNode(Int);
  ImplementGetterNode(String);
  ImplementGetterNode(Animation);

  #define ImplementSetterNode(Typename) NodeClassMeta Typename##SetterNode::set##Typename##ClassDescriptor;

  ImplementSetterNode(Bool);
  ImplementSetterNode(Float);
  ImplementSetterNode(Int);
  ImplementSetterNode(String);

  void BoolSetterNode::render()
  {
    ne::Utilities::BlueprintNodeBuilder builder;

    builder.Begin(imguiPinStart);
    renderStandardHeader(builder);
    ImGui::Checkbox("(Bool)", &nodeValue);
    builder.Middle();
    renderStandardOutputPins(builder);
    builder.End();
  }

  void FloatSetterNode::render()
  {
    ne::Utilities::BlueprintNodeBuilder builder;

    builder.Begin(imguiPinStart);
    renderStandardHeader(builder);
    ImGui::PushItemWidth(50.0f);
      ImGui::DragFloat("(Float)", &nodeValue);
    ImGui::PopItemWidth();
    builder.Middle();
    renderStandardOutputPins(builder);
    builder.End();
  }

  void IntSetterNode::render()
  {
    ne::Utilities::BlueprintNodeBuilder builder;
    builder.Begin(imguiPinStart);
    renderStandardHeader(builder);
    ImGui::PushItemWidth(50.0f);
      ImGui::DragInt("(Int)", &nodeValue);
    ImGui::PopItemWidth();
    builder.Middle();
    renderStandardOutputPins(builder);
    builder.End();
  }

  void StringSetterNode::render()
  {
    ne::Utilities::BlueprintNodeBuilder builder;

    builder.Begin(imguiPinStart);
    renderStandardHeader(builder);
    ImGui::PushItemWidth(150.0f);
      ImGui::InputText("(String)", &nodeValue);
    ImGui::PopItemWidth();
    builder.Middle();
    renderStandardOutputPins(builder);
    builder.End();
  }

  #define ImplementDebugNode(Typename) NodeClassMeta Typename##DebugNode::debug##Typename##ClassDescriptor;\
  void Typename##DebugNode::process(const BlendTree* tree, float dt)\
  {\
    outputs[0] = getInput<void>(0); /* Reflect input to output*/\
  }\

  ImplementDebugNode(Bool);
  ImplementDebugNode(Float);
  ImplementDebugNode(Int);
  ImplementDebugNode(String);
  ImplementDebugNode(Animation);

  void BoolDebugNode::render()
  {
    ne::Utilities::BlueprintNodeBuilder builder;
    builder.Begin(imguiPinStart);
    renderStandardHeader(builder);
    renderStandardInputPins(builder);
    builder.Middle();
    renderStandardOutputPins(builder);
    if (outputs[0])
    {
      ImGui::Text(outputs[0] ? "Value: true" : "Value: false");
    }
    else
    {
      ImGui::Text("Value: bad");
    }
    builder.End();
  }

  void FloatDebugNode::render()
  {
    ne::Utilities::BlueprintNodeBuilder builder;
    builder.Begin(imguiPinStart);
    renderStandardHeader(builder);
    renderStandardInputPins(builder);
    builder.Middle();
    renderStandardOutputPins(builder);
    if (outputs[0])
    {
      ImGui::Text("Value: %f", *(float*)outputs[0]);
    }
    else
    {
      ImGui::Text("Value: bad");
    }
    builder.End();
  }

  void IntDebugNode::render()
  {
    ne::Utilities::BlueprintNodeBuilder builder;
    builder.Begin(imguiPinStart);
    renderStandardHeader(builder);
    renderStandardInputPins(builder);
    builder.Middle();
    renderStandardOutputPins(builder);
    if (outputs[0])
    {
      ImGui::Text("Value: %u", *(int*)outputs[0]);
    }
    else
    {
      ImGui::Text("Value: bad");
    }
    builder.End();
  }

  void StringDebugNode::render()
  {
    ne::Utilities::BlueprintNodeBuilder builder;
    builder.Begin(imguiPinStart);
    renderStandardHeader(builder);
    renderStandardInputPins(builder);
    builder.Middle();
    renderStandardOutputPins(builder);
    if (outputs[0])
    {
      ImGui::Text("Value: %s", reinterpret_cast<std::string*>(outputs[0])->c_str());
    }
    else
    {
      ImGui::Text("Value: bad");
    }
    builder.End();
  }

  void AnimationDebugNode::render()
  {
    ne::Utilities::BlueprintNodeBuilder builder;
    builder.Begin(imguiPinStart);
    renderStandardHeader(builder);
    renderStandardInputPins(builder);
    builder.Middle();
    renderStandardOutputPins(builder);
    ImGui::Text(outputs[0] ? "Reference: good" : "Reference: bad");
    if (outputs[0])
    {
      auto animation = *reinterpret_cast<gef::Animation*>(outputs[0]);
      ImGui::Text("Duration: %f", animation.duration());
      ImGui::Text("Key count: %u", animation.anim_nodes().size());
    }
    builder.End();
  }
}