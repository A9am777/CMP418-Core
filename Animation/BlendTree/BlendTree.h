#pragma once

#include <unordered_map>

#include "BlendNode.h"

namespace BlendTree
{
  class BlendTree
  {
    public:
    BlendTree();
    ~BlendTree();

    static void registerAllNodes(); // Registers the classes of all nodes

    void startRenderContext(Path configFile);
    void renderGraph();
    void endRenderContext();

    BlendNodePtr addNode(BlendNode* node);

    BlendNodePtr findNode(Label name);
    BlendNodePtr findNode(gef::StringId nameID);

    private:
    // Converts from node unique ID to the start of pin unique IDs for a node
    UInt imguiToPinStart(UInt pinMajor) {
      return pinMajor << 5; // Give up to 32 pins for this node! (can be masked for retrieval)
    }

    ne::EditorContext* imguiNodeContext;
    UInt imguiNextPinMajor; // Unique id required for ImGui nodes

    std::unordered_map<gef::StringId, BlendNodePtr> nodeMap; // Collection of all kept nodes
  };
}