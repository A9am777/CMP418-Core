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

    static void registerClass() // Define your class descriptor here
    {
      baseClassDescriptor.className = "Base";
      baseClassDescriptor.inputBlueprint = {};
      baseClassDescriptor.outputBlueprint = {};
    };

    virtual void render();
    void renderLinks(); // This is required due to silly ImGui reasons

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

    // ImGui
    static constexpr float imguiLinkThickness = 2.f;

    UInt getImGuiInputStartID() const { return imguiPinStart + classDescriptor->outputBlueprint.size(); }
    UInt getImGuiOutputStartID() const { return imguiPinStart; }
    //

    std::string nodeName;
    std::vector<InputSource> inputs; // Constrained by alive parents

    template<typename T> T* getOutput(UInt idx)
    {
      return reinterpret_cast<T*>(outputs[idx]);
    }
  };
}