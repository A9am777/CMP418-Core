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

    BlendNodePtr setOutputNode(BlendNode* node);

    void updateNodes(float dt);

    void startRenderContext(Path configFile);
    void renderGraph();
    void endRenderContext();

    // Constructs a traversal order during update
    void notifyTraversal(BlendNodePtr& nodePtr);

    // Registers a node
    BlendNodePtr addNode(BlendNode* node);

    // Returns a handle to a node of a name
    BlendNodePtr findNode(Label name);
    BlendNodePtr findNode(gef::StringId nameID);

    // Removes the node from the tree, destroying if the reference is not used
    BlendNodePtr removeNode(Label name);
    BlendNodePtr removeNode(gef::StringId nameID);

    inline bool hasTraversalCache() const { return useTraversalCache; }
    inline bool getUpdateParity() const { return updateParity; }

    private:
    // Converts from node unique ID to the start of pin unique IDs for a node
    UInt imguiToPinStart(UInt pinMajor) {
      return pinMajor << 5; // Give up to 32 pins for this node! (can be masked for retrieval)
    }
    UInt imguiFromPinStart(UInt pinMajor) {
      return pinMajor >> 5;
    }

    void handleInput();
    void renderContextMenus();

    ne::EditorContext* imguiNodeContext;
    ne::NodeId imguiNodeIdCtx; // Last menu selected node
    ne::PinId imguiNodePinIdCtx; // Last menu selected node pin
    ne::LinkId imguiNodeLinkIdCtx; // Last menu selected node link
    std::string imguiNextNodeName; // Node name entered by the user in a "Create..." menu
    UInt imguiNextPinMajor; // Unique id required for ImGui nodes
    std::unordered_map<UInt, BlendNodeWPtr> nodeGUIDMap; // Collection of nodes to ImGui unique id. Weakptr to avoid the hassle of cleanup

    std::unordered_map<gef::StringId, BlendNodePtr> nodeMap; // Collection of all kept nodes
    BlendNodePtr outputNode;
    bool updateParity; // Distinguishes between odd and even frames so nodes can automatically identify if they have been visited without multiple passes
    bool useTraversalCache; // Whether to optimise node update traversal
    std::list<BlendNodeWPtr> cachedNodeTraversal; // Caches the order of previously visited nodes to speed up next frames order
  };
}