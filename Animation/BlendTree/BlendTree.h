#pragma once

#include <array>
#include <unordered_map>
#include <functional>

#include <maths/vector2.h>
#include <animation/animation.h>

#include "BlendNode.h"

namespace gef
{
  class Animation;
  class SkeletonPose;
}

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
    BlendNodePtr addNode(BlendNode* node, ImVec2 visualLocation = ImVec2(0, 0));

    // Returns a handle to a node of a name
    BlendNodePtr findNode(Label name);
    BlendNodePtr findNode(gef::StringId nameID);

    // Removes the node from the tree, destroying if the reference is not used
    BlendNodePtr removeNode(Label name);
    BlendNodePtr removeNode(gef::StringId nameID);

    inline void setBindPose(const gef::SkeletonPose* pose) { bindPoseContext = pose; }
    inline const gef::SkeletonPose* getBindPose() const { return bindPoseContext; }

    // Global variable setters
    inline void setReference(gef::StringId nameID, const bool* ref)            { setGlobalVariable(Param_Bool, nameID, ref); }
    inline void setReference(Label name, const bool* ref)                      { setGlobalVariable(Param_Bool, name, ref); }
    inline void setReference(gef::StringId nameID, const float* ref)           { setGlobalVariable(Param_Float, nameID, ref); }
    inline void setReference(Label name, const float* ref)                     { setGlobalVariable(Param_Float, name, ref); }
    inline void setReference(gef::StringId nameID, const gef::Vector2* ref)    { setGlobalVariable(Param_Vec2, nameID, ref); }
    inline void setReference(Label name, const gef::Vector2* ref)              { setGlobalVariable(Param_Vec2, name, ref); }
    inline void setReference(gef::StringId nameID, const int* ref)             { setGlobalVariable(Param_Int, nameID, ref); }
    inline void setReference(Label name, const int* ref)                       { setGlobalVariable(Param_Int, name, ref); }
    inline void setReference(gef::StringId nameID, const std::string* ref)     { setGlobalVariable(Param_String, nameID, ref); }
    inline void setReference(Label name, const std::string* ref)               { setGlobalVariable(Param_String, name, ref); }
    inline void setReference(gef::StringId nameID, const gef::Animation* ref)  { setGlobalVariable(Param_Animation, nameID, ref); }
    inline void setReference(Label name, const gef::Animation* ref)            { setGlobalVariable(Param_Animation, name, ref); }

    // Global variable getters
    inline const bool* getBool(gef::StringId nameID) const                 { return (bool*)getGlobalVariable(Param_Bool, nameID); }
    inline const bool* getBool(Label name) const                           { return (bool*)getGlobalVariable(Param_Bool, name); }
    inline const float* getFloat(gef::StringId nameID) const               { return (float*)getGlobalVariable(Param_Float, nameID); }
    inline const float* getFloat(Label name) const                         { return (float*)getGlobalVariable(Param_Float, name); }
    inline const gef::Vector2* getVector2(gef::StringId nameID) const      { return (gef::Vector2*)getGlobalVariable(Param_Vec2, nameID); }
    inline const gef::Vector2* getVector2(Label name) const                { return (gef::Vector2*)getGlobalVariable(Param_Vec2, name); }
    inline const int* getInt(gef::StringId nameID) const                   { return (int*)getGlobalVariable(Param_Int, nameID); }
    inline const int* getInt(Label name) const                             { return (int*)getGlobalVariable(Param_Int, name); }
    inline const std::string* getString(gef::StringId nameID) const        { return (std::string*)getGlobalVariable(Param_String, nameID); }
    inline const std::string* getString(Label name) const                  { return (std::string*)getGlobalVariable(Param_String, name); }
    inline const gef::Animation* getAnimation(gef::StringId nameID) const  { return (gef::Animation*)getGlobalVariable(Param_Animation, nameID); }
    inline const gef::Animation* getAnimation(Label name) const            { return (gef::Animation*)getGlobalVariable(Param_Animation, name); }

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

    void handleInput(); // Manages the imgui node editor input
    void renderContextMenus(); // Handles editor context menus

    void imguiPopup(); // Sets the environment for a new context menu popup

    // Sets a global variable reference for a specific parameter type
    inline void setGlobalVariable(ParamType type, gef::StringId nameID, const void* ref) { globalVariableMaps[type].insert_or_assign(nameID, ref); }
    inline void setGlobalVariable(ParamType type, Label name, const void* ref) { setGlobalVariable(type, gef::GetStringId(name), ref); }
    // Returns a global variable reference (if it exists) for a specific parameter type
    const void* getGlobalVariable(ParamType type, gef::StringId nameID) const;
    inline const void* getGlobalVariable(ParamType type, Label name) const { return getGlobalVariable(type, gef::GetStringId(name)); }

    // ImGui organisation of nodes
    typedef std::function<BlendNode* (BlendTree*, Label)> CreateNodeFunc;
    static void imguiRegisterAllNodes();
    static void imguiPushNodeClass(Literal name, const CreateNodeFunc& createFunc);
    static std::vector<Literal> imguiNodeTypeComboNameList;
    static std::vector<CreateNodeFunc> imguiNodeCreationFunctionList;

    ne::EditorContext* imguiNodeContext;
    ne::NodeId imguiNodeIdCtx; // Last menu selected node
    ne::PinId imguiNodePinIdCtx; // Last menu selected node pin
    ne::LinkId imguiNodeLinkIdCtx; // Last menu selected node link
    std::string imguiNextNodeName; // Node name entered by the user in a "Create..." menu
    size_t imguiContextComboCurrentID; // Current selected item in a unique context menu's combo box
    ImVec2 imguiPopupLocation;
    UInt imguiNextPinMajor; // Unique id required for ImGui nodes
    std::unordered_map<UInt, BlendNodeWPtr> nodeGUIDMap; // Collection of nodes to ImGui unique id. Weakptr to avoid the hassle of cleanup

    std::array<std::unordered_map<gef::StringId, const void*>, Param_COUNT> globalVariableMaps; // One map per variable type. Any destruction likely requires an extra tree traversal to notify
    std::unordered_map<gef::StringId, BlendNodePtr> nodeMap; // Collection of all kept nodes
    BlendNodePtr outputNode;
    bool updateParity; // Distinguishes between odd and even frames so nodes can automatically identify if they have been visited without multiple passes
    bool useTraversalCache; // Whether to optimise node update traversal
    std::list<BlendNodeWPtr> cachedNodeTraversal; // Caches the order of previously visited nodes to speed up next frames order

    // Due to bad gef design
    const gef::SkeletonPose* bindPoseContext;
  };
}