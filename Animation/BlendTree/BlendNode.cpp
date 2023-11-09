#include "Animation/BlendTree/BlendNode.h"
#include "Animation/BlendTree/BlendTree.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <ax/Widgets.h>
#include <ax/builders.h>

namespace BlendTree
{
  NodeClassMeta BlendNode::baseClassDescriptor;

  BlendNode::BlendNode(Label name, const NodeClassMeta* descriptor) : nodeFlags{ NodeInitMask }
  {
    nodeName = name;

    if (classDescriptor = descriptor)
    {
      inputs.resize(classDescriptor->inputBlueprint.size());
      outputs.resize(classDescriptor->outputBlueprint.size(), nullptr);
    }
  }

  void BlendNode::acceptTree(BlendTree* tree)
  {
    // Copy over the update parity to be ready for next frame
    nodeFlags = tree->getUpdateParity() ? BitSet(nodeFlags, NodeUpdateParityFlag) : BitClear(nodeFlags, NodeUpdateParityFlag);
  }

  void BlendNode::update(BlendTree* tree, float dt)
  {
    // Work is not required if already visited
    if (BitMask(nodeFlags, NodeUpdateParityFlag) == tree->getUpdateParity()) { return; }

    // Set my update parity now (avoids accidental recursion)
    nodeFlags = tree->getUpdateParity() ? BitSet(nodeFlags, NodeUpdateParityFlag) : BitClear(nodeFlags, NodeUpdateParityFlag);

    for (auto& parent : inputs)
    {
      // Per real node input
      if (auto parentPtr = parent.parentNode.lock())
      {
        // Recursively update parent inputs
        parentPtr->update(tree, dt);
      }
    }

    // TODO: idea to cache the order of visited nodes for faster traversal next frame

    // All parent inputs are current, can now update properly
    process(dt);
  }

  void BlendNode::render()
  {
    ne::Utilities::BlueprintNodeBuilder builder;

    builder.Begin(imguiPinStart);
    renderStandardHeader(builder);
    renderStandardInputPins(builder);
    builder.Middle();
    renderStandardOutputPins(builder);
    builder.End();
  }

  void BlendNode::renderLinks()
  {
    UInt inputPinId = getImGuiInputStartID();

    for (size_t inputIdx = 0; inputIdx < classDescriptor->inputBlueprint.size(); ++inputIdx)
    {
      auto& inputPin = classDescriptor->inputBlueprint[inputIdx];

      ImColor pinColour = getImguiTypeColour(inputPin.type);
      if (auto connection = inputs[inputIdx].parentNode.lock())
      {
        // Inputs are 1:1 unlike outputs 1:M therefore use the link ID as the input ID

        UInt parentPinId = connection->imguiPinStart + inputs[inputIdx].slot;
        ne::Link(inputPinId, parentPinId, inputPinId, pinColour, imguiLinkThickness);
      }

      ++inputPinId;
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

  void BlendNode::renderStandardHeader(ne::Utilities::BlueprintNodeBuilder& builder)
  {
    builder.Header();
      ImGui::Text((getClassName() + " \"" + getName() + "\"").c_str());
    builder.EndHeader();
  }

  void BlendNode::renderStandardInputPins(ne::Utilities::BlueprintNodeBuilder& builder)
  {
    UInt inputPinId = getImGuiInputStartID();
    for (size_t inputPinIdx = 0; inputPinIdx < classDescriptor->inputBlueprint.size(); ++inputPinIdx)
    {
      auto& inputPin = classDescriptor->inputBlueprint[inputPinIdx];
      ImColor pinColour = getImguiTypeColour(inputPin.type);

      builder.Input(inputPinId);
      ax::Widgets::Icon(ImVec2(static_cast<float>(16), static_cast<float>(16)), ax::Drawing::IconType::Circle, !inputs[inputPinIdx].parentNode.expired(), pinColour);

      ImGui::TextUnformatted(inputPin.name.c_str());
      ImGui::Spring(0);
      builder.EndInput();

      ++inputPinId;
    }
  }

  void BlendNode::renderStandardOutputPins(ne::Utilities::BlueprintNodeBuilder& builder)
  {
    UInt outputPinId = getImGuiOutputStartID();
    for (auto& outputPin : classDescriptor->outputBlueprint)
    {
      ImColor pinColour = getImguiTypeColour(outputPin.type);

      builder.Output(outputPinId);
        ImGui::TextUnformatted(outputPin.name.c_str());
        ImGui::Spring(0);

        ax::Widgets::Icon(ImVec2(static_cast<float>(16), static_cast<float>(16)), ax::Drawing::IconType::Circle, true, pinColour);
      builder.EndOutput();

      ++outputPinId;
    }
  }

  ImColor BlendNode::getImguiTypeColour(ParamType type)
  {
    Byte colourIndex = static_cast<Byte>(type);

    static ImColor colourOptions[Param_COUNT] = {
    ImColor(0xFFACE4AA),
    ImColor(0xFF635380),
    ImColor(0xFFF4A259),
    ImColor(0xFFBC4B51),
    ImColor(0xFF5B8E7D),
    ImColor(0xFF102E4A)
    //ImColor(0x5887FF),
    //ImColor(0xE0B1CB)
    };

    return colourOptions[colourIndex];
  }
}
