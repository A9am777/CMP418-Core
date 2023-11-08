#include "Animation/BlendTree/UtilityNodes.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <ax/Widgets.h>
#include <ax/builders.h>
#include <imgui_stdlib.h>

namespace BlendTree
{
  // Implement the class, including declaring the static class descriptors
  #define ImplementGetterNode(Typename) NodeClassMeta Typename##GetterNode::get##Typename##ClassDescriptor;
  #define ImplementSetterNode(Typename) NodeClassMeta Typename##SetterNode::set##Typename##ClassDescriptor;

  ImplementGetterNode(Bool);
  ImplementGetterNode(Float);
  ImplementGetterNode(Int);
  ImplementGetterNode(String);
  ImplementGetterNode(Animation);

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
    ImGui::DragFloat("(Float)", &nodeValue);
    builder.Middle();
    renderStandardOutputPins(builder);
    builder.End();
  }

  void IntSetterNode::render()
  {
    ne::Utilities::BlueprintNodeBuilder builder;

    builder.Begin(imguiPinStart);
    renderStandardHeader(builder);
    ImGui::DragInt("(Int)", &nodeValue);
    builder.Middle();
    renderStandardOutputPins(builder);
    builder.End();
  }

  void StringSetterNode::render()
  {
    ne::Utilities::BlueprintNodeBuilder builder;

    builder.Begin(imguiPinStart);
    renderStandardHeader(builder);
    ImGui::InputText("(String)", &nodeValue);
    builder.Middle();
    renderStandardOutputPins(builder);
    builder.End();
  }
}