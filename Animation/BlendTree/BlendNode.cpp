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

  void BlendNode::update(BlendTree* tree, BlendNodePtr& context, float dt)
  {
    // Work is not required if already visited
    if (!requiresUpdate(tree->getUpdateParity())) { return; }

    // Set my update parity now (avoids accidental recursion)
    nodeFlags = tree->getUpdateParity() ? BitSet(nodeFlags, NodeUpdateParityFlag) : BitClear(nodeFlags, NodeUpdateParityFlag);

    // Iterate over parent nodes. This step is culled if the tree has cached a previous traversal and that traversal is guaranteed to be equivocal
    if (requiresTraversal() || !tree->hasTraversalCache())
    {
      for (auto& parent : inputs)
      {
        // Per real node input
        auto parentPtr = parent.parentNode.lock();

        // Requires a real reference and a pending update
        if (parentPtr && parentPtr->requiresUpdate(tree->getUpdateParity()))
        {
          // Recursively update parent inputs
          parentPtr->update(tree, parentPtr, dt);
        }
      }
    }

    // Clear some flags
    nodeFlags = BitClear(nodeFlags, NodeLinkUpdateFlag);
    // Set some flags
    nodeFlags = BitSet(nodeFlags, NodeVisualVisited);

    // Notify this node has been visited for faster tree traversal
    tree->notifyTraversal(context);

    // All parent inputs are current, can now update properly
    process(tree, dt);
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
    bool nodeIsUsed = BitMask(nodeFlags, NodeVisualVisited);
    UInt inputPinId = getImguiInputStartID();

    for (size_t inputIdx = 0; inputIdx < classDescriptor->inputBlueprint.size(); ++inputIdx)
    {
      auto& inputPin = classDescriptor->inputBlueprint[inputIdx];

      ImColor pinColour = getImguiTypeColour(inputPin.type);
      if (!nodeIsUsed) { pinColour.Value.w *= .75f; }

      if (auto connection = inputs[inputIdx].parentNode.lock())
      {
        // Inputs are 1:1 unlike outputs 1:M therefore use the link ID as the input ID

        UInt parentPinId = connection->getImguiOutputStartID() + inputs[inputIdx].slot;
        ne::Link(inputPinId, parentPinId, inputPinId, pinColour, imguiLinkThickness);
        if (nodeIsUsed)
        {
          ne::Flow(inputPinId);
        }
      }

      ++inputPinId;
    }

    // Let next frame decide if this node has been visited
    nodeFlags = BitClear(nodeFlags, NodeVisualVisited);
  }

  bool BlendNode::canLink(BlendNodePtr& parent, UInt outIdx, BlendNodePtr& child, UInt inIdx)
  {
    return parent && child // Pointers must be valid
      && parent->outputs.size() > outIdx && child->inputs.size() > inIdx // Indices must be valid
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

    
    // It is unlikely this will be required for the parent
    //parent->nodeFlags = BitSet(parent->nodeFlags, NodeLinkUpdateFlag);

    // Used to signal the tree needs partially rebuilt
    child->nodeFlags = BitSet(child->nodeFlags, NodeLinkUpdateFlag);
  }

  void BlendNode::clearLink(BlendNodePtr& node, UInt inIdx)
  {
    auto& inputSlot = node->inputs[inIdx];
    inputSlot.parentNode.reset();
    inputSlot.slot = 0;

    // Used to signal the tree needs partially rebuilt
    node->nodeFlags = BitSet(node->nodeFlags, NodeLinkUpdateFlag);
  }

  std::string BlendNode::imguiPinToName(UInt id) const
  {
    bool isInput = isImguiInputPin(id);
    UInt idx = imguiPinToIdx(isInput, id);

    return isInput ? classDescriptor->inputBlueprint[idx].name : classDescriptor->outputBlueprint[idx].name;
  }

  ParamType BlendNode::imguiPinToType(UInt id) const
  {
    bool isInput = isImguiInputPin(id);
    UInt idx = imguiPinToIdx(isInput, id);

    return isInput ? classDescriptor->inputBlueprint[idx].type : classDescriptor->outputBlueprint[idx].type;
  }

  void BlendNode::renderStandardHeader(ne::Utilities::BlueprintNodeBuilder& builder)
  {
    builder.Header();
      ImGui::Text((getClassName() + " \"" + getName() + "\"").c_str());
    builder.EndHeader();
  }

  void BlendNode::renderStandardInputPins(ne::Utilities::BlueprintNodeBuilder& builder)
  {
    UInt inputPinId = getImguiInputStartID();
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
    UInt outputPinId = getImguiOutputStartID();
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
    ImColor(0xFF102E4A),
    ImColor(0xFF5887FF),
    ImColor(0xFFE0B1CB)
    };

    return colourOptions[colourIndex];
  }
  bool BlendNode::requiresUpdate(bool parity) const
  {
    return BitMask(nodeFlags, NodeUpdateParityFlag) != parity || BitMask(nodeFlags, NodeUpdateDirtyMask);
  }
  bool BlendNode::requiresTraversal() const
  {
    return BitMask(nodeFlags, NodeUpdateDirtyMask);
  }
}
