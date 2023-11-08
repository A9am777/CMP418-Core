#include "Animation/BlendTree/BlendNode.h"

namespace BlendTree
{
  NodeClassMeta BlendNode::baseClassDescriptor;

  BlendNode::BlendNode(Label name, const NodeClassMeta* descriptor)
  {
    nodeName = name;

    if (classDescriptor = descriptor)
    {
      inputs.resize(classDescriptor->inputBlueprint.size());
      outputs.resize(classDescriptor->outputBlueprint.size(), nullptr);
    }
  }

  void BlendNode::render()
  {
    UInt pinId = imguiPinStart;

    for (auto& outputPin : classDescriptor->outputBlueprint)
    {
      ImColor pinColour = getImguiTypeColour(outputPin.type);

      ne::BeginPin(pinId, ne::PinKind::Output);
      ImGui::Text(outputPin.name.c_str());
      ax::Widgets::Icon(ImVec2(static_cast<float>(24), static_cast<float>(24)), ax::Drawing::IconType::Circle, true, pinColour);
      ne::EndPin();

      ++pinId;
    }

    ImGui::SameLine();

    for (size_t inputIdx = 0; inputIdx < classDescriptor->inputBlueprint.size(); ++inputIdx)
    {
      auto& inputPin = classDescriptor->inputBlueprint[inputIdx];

      ImColor pinColour = getImguiTypeColour(inputPin.type);

      ne::BeginPin(pinId, ne::PinKind::Input);
      ImGui::Text(inputPin.name.c_str());
      ax::Widgets::Icon(ImVec2(static_cast<float>(24), static_cast<float>(24)), ax::Drawing::IconType::Circle, true, pinColour);
      ne::EndPin();

      if (auto connection = inputs[inputIdx].parentNode.lock())
      {
        // Inputs are 1:1 unlike outputs 1:M therefore use the link ID as the input ID

        UInt parentPinId = connection->imguiPinStart + inputs[inputIdx].slot;
        ne::Link(pinId, parentPinId, pinId, pinColour, 2.0f);
      }

      ++pinId;
    }

  }

  bool BlendNode::canLink(BlendNodePtr& parent, UInt outIdx, BlendNodePtr& child, UInt inIdx)
  {
    return parent && child // Pointers must be valid
      && parent->outputs.size() > outIdx && parent->inputs.size() > inIdx // Indices must be valid
      && parent->classDescriptor->outputBlueprint[outIdx].type == child->classDescriptor->inputBlueprint[inIdx].type; // Types must strictly match
  }

  bool BlendNode::tryLink(BlendNodePtr& parent, UInt outIdx, BlendNodePtr& child, UInt inIdx)
  {
    if (canLink(parent, outIdx, child, inIdx))
    {
      unsafeLink(parent, outIdx, child, inIdx);
      return true;
    }

    return false;
  }

  void BlendNode::unsafeLink(BlendNodePtr& parent, UInt outIdx, BlendNodePtr& child, UInt inIdx)
  {
    auto& inputSlot = child->inputs[inIdx];
    inputSlot.parentNode = BlendNodeWPtr(parent);
    inputSlot.slot = outIdx;
  }

  ImColor BlendNode::getImguiTypeColour(ParamType type)
  {
    Byte colourIndex = static_cast<Byte>(type);

    static ImColor colourOptions[Param_COUNT] = {
    ImColor(0xFFACE4AA),
    ImColor(0xFF635380),
    ImColor(0xFFF4A259),
    ImColor(0xFFBC4B51),
    //ImColor(0x5B8E7D),
    //ImColor(0x102E4A),
    //ImColor(0x5887FF),
    //ImColor(0xE0B1CB)
    };

    return colourOptions[colourIndex];
  }
}
