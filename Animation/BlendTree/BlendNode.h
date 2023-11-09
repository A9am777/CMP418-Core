#pragma once

#include <memory>
#include <vector>

#include "Globals.h"
#include "Defs.h"

#include <imgui_node_editor.h>
#include <imgui_internal.h>

// ImGui node editor forward declarations
namespace ax {
  namespace NodeEditor {
    namespace Utilities {
      struct BlueprintNodeBuilder;
    }
  }
}

// ImGui node editor namespace
namespace ne = ax::NodeEditor;

namespace BlendTree
{
  class BlendTree; // Forward declaration

  // Type protection of passed variables in the tree
  enum ParamType : Byte
  {
    Param_Bool = 0,
    Param_Float,
    Param_Int,
    Param_String,
    Param_Animation,
    Param_Pose,
    Param_COUNT
  };

  // Describes a parameter
  struct ParamDescriptor
  {
    std::string name;
    ParamType type;
  };

  struct NodeClassMeta
  {
    std::vector<ParamDescriptor> inputBlueprint;
    std::vector<ParamDescriptor> outputBlueprint;
    std::string className;
  };

  class BlendNode;
  typedef std::shared_ptr<BlendNode> BlendNodePtr;
  typedef std::weak_ptr<BlendNode> BlendNodeWPtr;

  class BlendNode
  {
    public:
    BlendNode(Label name, const NodeClassMeta* descriptor = &BlendNode::baseClassDescriptor);
    virtual ~BlendNode() = default;

    static void registerClass() // Define your class descriptor here
    {
      baseClassDescriptor.className = "Base";
      baseClassDescriptor.inputBlueprint = {};
      baseClassDescriptor.outputBlueprint = {};
    };

    // Called by a tree when this node has been linked to it
    void acceptTree(BlendTree* tree);

    void update(BlendTree* tree, float dt);
    virtual void render();
    // This is required due to silly ImGui reasons. There is no reason for child classes to implement this
    void renderLinks();

    // Returns if two nodes can safely be linked
    static bool canLink(BlendNodePtr& parent, UInt outIdx, BlendNodePtr& child, UInt inIdx);
    // Safely attempts to link two nodes. Returns if successful
    static bool tryLink(BlendNodePtr& parent, UInt outIdx, BlendNodePtr& child, UInt inIdx);
    // Quickly links two nodes
    static void unsafeLink(BlendNodePtr& parent, UInt outIdx, BlendNodePtr& child, UInt inIdx);

    inline void setImguiPinStart(UInt newStart) { imguiPinStart = newStart; }

    inline const std::string& getName() const { return nodeName; }
    inline const std::string& getClassName() const { return classDescriptor->className; }
    inline UInt getConnectionCount() const { return static_cast<UInt>(classDescriptor->inputBlueprint.size() + classDescriptor->outputBlueprint.size()); }

    protected:
    struct InputSource
    {
      BlendNodeWPtr parentNode; // Dead or alive node reference
      UInt slot; // Static ID of output to fetch from
    };

    template<typename T> T* getInput(UInt idx)
    {
      auto nodeInput = inputs[idx].parentNode.lock(); // Fetch the node reference
      return nodeInput ? nodeInput->getOutput<T>(inputs[idx].slot) : nullptr; // Return its data if it exists
    }

    // Derived classes can implement this to process any updates - knowing all input nodes have already been visited
    virtual void process(float dt) {}

    // ImGui
    void renderStandardHeader(ne::Utilities::BlueprintNodeBuilder& builder);
    void renderStandardInputPins(ne::Utilities::BlueprintNodeBuilder& builder);
    void renderStandardOutputPins(ne::Utilities::BlueprintNodeBuilder& builder);
    // Returns an ImGui colour for a parameter type
    static ImColor getImguiTypeColour(ParamType type);
    //

    UInt imguiPinStart; // ID used for managing pins. Starts with outputs before inputs for nicer lookup
    std::vector<void*> outputs; // Can be freely managed by the underlying derived class

    private:
    static NodeClassMeta baseClassDescriptor; // The default node class params
    const NodeClassMeta* classDescriptor; // Describes the static node implementation by the underlying class. Technically a dynamic node class can implement this


    enum NodeFlags : UInt
    {
      // Used to track if this node has been visited and refreshed. Flip flops each frame to avoid multiple traversals
      NodeUpdateParityFlag = BIT(0),

      NodeInitMask = NodeUpdateParityFlag
    };

    // ImGui
    static constexpr float imguiLinkThickness = 2.f;

    UInt getImGuiInputStartID() const { return imguiPinStart + classDescriptor->outputBlueprint.size(); }
    UInt getImGuiOutputStartID() const { return imguiPinStart; }
    //

    std::string nodeName;
    std::vector<InputSource> inputs; // Constrained by alive parents
    NodeFlags nodeFlags;

    template<typename T> T* getOutput(UInt idx)
    {
      return reinterpret_cast<T*>(outputs[idx]);
    }
  };
}