#include "BlendTree.h"
#include "UtilityNodes.h"
#include "SkeletonBlendNodes.h"
#include <imgui_stdlib.h>

namespace BlendTree
{
  std::vector<Literal> BlendTree::imguiNodeTypeComboNameList;
  std::vector<BlendTree::CreateNodeFunc> BlendTree::imguiNodeCreationFunctionList;

  BlendTree::BlendTree() : imguiNodeContext{ nullptr }, imguiNextPinMajor{ 1 }, updateParity{ false }, useTraversalCache{ true }, forceVisitNodes{ false }, imguiContextComboCurrentID{ 0 }
  {
    
  }

  BlendTree::~BlendTree()
  {
    endRenderContext();
  }

  void BlendTree::registerAllNodes()
  {
    BlendNode::registerClass();

    // Getters
    BoolGetterNode::registerClass();
    FloatGetterNode::registerClass();
    Vector2GetterNode::registerClass();
    IntGetterNode::registerClass();
    StringGetterNode::registerClass();
    AnimationGetterNode::registerClass();
    TransformGetterNode::registerClass();
    
    // Setters
    BoolSetterNode::registerClass();
    FloatSetterNode::registerClass();
    Vector2SetterNode::registerClass();
    IntSetterNode::registerClass();
    StringSetterNode::registerClass();
    TransformSetterNode::registerClass();

    // Debuggers
    BoolDebugNode::registerClass();
    FloatDebugNode::registerClass();
    Vector2DebugNode::registerClass();
    IntDebugNode::registerClass();
    StringDebugNode::registerClass();
    AnimationDebugNode::registerClass();
    TransformDebugNode::registerClass();

    // Blend nodes
    ClipNode::registerClass();
    CrossFadeControllerNode::registerClass();
    BinaryInterpolatorNode::registerClass();
    QuadInterpolatorNode::registerClass();
    InverseKineNode::registerClass();

    // Output
    SkeletonOutputNode::registerClass();

    imguiRegisterAllNodes();
  }

  BlendNodePtr BlendTree::setOutputNode(BlendNode* node)
  {
    return outputNode = addNode(node);
  }

  void BlendTree::handleInput()
  {
    if (ne::BeginCreate(ImColor(255, 255, 255), 2.0f))
    {
      // Manage links
      {
        ne::PinId startPinId = 0;
        ne::PinId endPinId = 0;
        if (ne::QueryNewLink(&startPinId, &endPinId))
        {
          bool linkPerformed = false;

          // Fetch the nodes
          UInt firstNodeId = imguiFromPinStart(startPinId.Get());
          UInt secondNodeId = imguiFromPinStart(endPinId.Get());
          auto& firstNodeIt = nodeGUIDMap.find(firstNodeId);
          auto& secondNodeIt = nodeGUIDMap.find(secondNodeId);
          auto firstNodeWPtr = (firstNodeIt == nodeGUIDMap.end()) ? BlendNodeWPtr() : firstNodeIt->second;
          auto secondNodeWPtr = (secondNodeIt == nodeGUIDMap.end()) ? BlendNodeWPtr() : secondNodeIt->second;

          auto firstNodePtr = firstNodeWPtr.lock();
          auto secondNodePtr = secondNodeWPtr.lock();

          if (firstNodePtr && secondNodePtr)
          {
            // Determine the nature of the nodes
            bool firstNodeLinkIsInput = firstNodePtr->isImguiInputPin(startPinId.Get());
            bool secondNodeLinkIsInput = secondNodePtr->isImguiInputPin(endPinId.Get());
            if (firstNodeLinkIsInput != secondNodeLinkIsInput) // Strictly input<->output only
            {
              // Swap such that the first node is always output
              if (firstNodeLinkIsInput)
              {
                std::swap(firstNodePtr, secondNodePtr);
                std::swap(startPinId, endPinId);
              }

              // Get the node pins to connect
              UInt firstNodePinIdx = firstNodePtr->imguiPinToIdx(false, startPinId.Get());
              UInt secondNodePinIdx = secondNodePtr->imguiPinToIdx(true, endPinId.Get());

              // Check this link is allowed
              if (BlendNode::canLink(firstNodePtr, firstNodePinIdx, secondNodePtr, secondNodePinIdx))
              {
                // Final call
                if (ne::AcceptNewItem(ImColor(255, 255, 128), 4.0f))
                {
                  BlendNode::unsafeLink(firstNodePtr, firstNodePinIdx, secondNodePtr, secondNodePinIdx);
                }
                linkPerformed = true; // For some reason imgui double checks before accepting a link
              }
            }
          }

          if (!linkPerformed)
          {
            ne::RejectNewItem(ImColor(128, 128, 128, 128), 1.0f);
          }
        }
      }

      {
        ne::PinId pinId = 0;
        if (ne::QueryNewNode(&pinId))
        {


          if (ne::AcceptNewItem())
          {
            ne::Suspend();
            ImGui::OpenPopup("Create New Param Node");
            ne::Resume();
            pinId = 1;
          }
        }
      }
    }
    ne::EndCreate();

    if (ne::BeginDelete())
    {
      ne::NodeId nodeId = 0;
      while (ne::QueryDeletedNode(&nodeId))
      {
        auto nodeIt = nodeGUIDMap.find(imguiFromPinStart(nodeId.Get()));
        if (nodeIt != nodeGUIDMap.end())
        {
          if (auto nodePtr = nodeIt->second.lock())
          {
            if (nodePtr != outputNode)
            {
              if (ne::AcceptDeletedItem())
              {
                removeNode(nodePtr->getName());
              }
            }
          }
        }
      }

      ne::LinkId linkId = 0;
      while (ne::QueryDeletedLink(&linkId))
      {
        auto nodeIt = nodeGUIDMap.find(imguiFromPinStart(linkId.Get()));
        if (nodeIt != nodeGUIDMap.end())
        {
          if (auto nodePtr = nodeIt->second.lock())
          {
            if (nodePtr->isImguiInputPin(linkId.Get()))
            {
              if (ne::AcceptDeletedItem())
              {
                BlendNode::clearLink(nodePtr, nodePtr->imguiPinToIdx(true, linkId.Get()));
              }
            }
          }
        }
      }
    }
    ne::EndDelete();

    // Context menus
    {
      ne::Suspend();
      if (ne::ShowNodeContextMenu(&imguiNodeIdCtx)) { ImGui::OpenPopup("Node Context Menu"); imguiPopup(); }
      else if (ne::ShowPinContextMenu(&imguiNodePinIdCtx)) { ImGui::OpenPopup("Pin Context Menu"); imguiPopup(); }
      else if (ne::ShowLinkContextMenu(&imguiNodeLinkIdCtx)) { ImGui::OpenPopup("Link Context Menu"); imguiPopup(); }
      else if (ne::ShowBackgroundContextMenu()) { ImGui::OpenPopup("Create New Node"); imguiPopup(); }
      ne::Resume();
    }
  }

  void BlendTree::updateNodes(float dt)
  {
    // Flip flop the frame parity
    updateParity = !updateParity;

    // Traverse using the prior order
    if (useTraversalCache)
    {
      // Grab the list, freeing the old one
      auto cacheList(std::move(cachedNodeTraversal));
      for (auto& nodeWPtr : cacheList)
      {
        if (auto nodePtr = nodeWPtr.lock())
        {
          nodePtr->update(this, nodePtr, dt);
        }
      }
    }

    if (forceVisitNodes)
    {
      for (auto& node : nodeMap)
      {
        node.second->update(this, node.second, dt);
      }
    }

    // Regardless of cache, the output node must be visited to ensure all nodes are up-to-date
    outputNode->update(this, outputNode, dt);
  }

  void BlendTree::startRenderContext(Path configFile)
  {
    endRenderContext();

    ne::Config config;
    config.SettingsFile = configFile.c_str();
    imguiNodeContext = ne::CreateEditor(&config);
  }

  void BlendTree::renderGraph()
  {
    if (!imguiNodeContext) { return; }
    ne::SetCurrentEditor(imguiNodeContext);
    ne::Begin("BlendNode Tree Editor");

    for (auto& node : nodeMap)
    {
      node.second->render();
    }

    for (auto& node : nodeMap)
    {
      node.second->renderLinks();
    }

    handleInput();
    renderContextMenus();

    ne::End();
  }

  void BlendTree::endRenderContext()
  {
    if (!imguiNodeContext) { return; }

    ne::DestroyEditor(imguiNodeContext);
    imguiNodeContext = nullptr;
  }

  void BlendTree::notifyTraversal(BlendNodePtr& nodePtr)
  {
    cachedNodeTraversal.push_back(BlendNodeWPtr(nodePtr));
  }

  BlendNodePtr BlendTree::addNode(BlendNode* node, ImVec2 visualLocation)
  {
    BlendNodePtr nodePtr(node);
    nodeMap.insert({
      StringTable.Add(node->getName()), nodePtr
      });

    nodePtr->acceptTree(this);

    {
      UInt nodeID = imguiToPinStart(imguiNextPinMajor);
      nodeGUIDMap.insert_or_assign(imguiNextPinMajor, nodePtr);
      nodePtr->setImguiPinStart(nodeID);
      ne::SetCurrentEditor(imguiNodeContext);
      ne::SetNodePosition(nodeID, visualLocation);
      ++imguiNextPinMajor;
    }
    return nodePtr;
  }

  BlendNodePtr BlendTree::findNode(Label name)
  {
    return findNode(gef::GetStringId(name));
  }

  BlendNodePtr BlendTree::findNode(gef::StringId nameID)
  {
    auto& it = nodeMap.find(nameID);
    return it == nodeMap.end() ? BlendNodePtr() : it->second;
  }

  BlendNodePtr BlendTree::removeNode(Label name)
  {
    return removeNode(StringTable.Add(name));;
  }

  BlendNodePtr BlendTree::removeNode(gef::StringId nameID)
  {
    auto& it = nodeMap.find(nameID);
    if (it != nodeMap.end())
    {
      BlendNodePtr caughtNode = it->second;
      nodeMap.erase(it);
      return caughtNode;
    }

    return BlendNodePtr();
  }

  void BlendTree::renderContextMenus()
  {
    ne::Suspend();

    if (ImGui::BeginPopup("Node Context Menu"))
    {
      UInt nodeId = imguiFromPinStart(imguiNodeIdCtx.Get());
      auto nodeIt = nodeGUIDMap.find(nodeId);

      if (nodeIt != nodeGUIDMap.end())
      {
        ImGui::TextUnformatted("Node Context Menu");
        ImGui::Separator();
        if (auto nodePtr = nodeIt->second.lock())
        {
          ImGui::Text("%s", nodePtr->getName().c_str());
          ImGui::Text("ID: %d", nodeId);
          ImGui::Text("Type: %s", nodePtr->getClassName().c_str());
          ImGui::Text("Inputs: %d", nodePtr->getInputCount());
          ImGui::Text("Outputs: %d", nodePtr->getOutputCount());

          if (nodePtr != outputNode)
          {
            ImGui::Separator();
            if (ImGui::MenuItem("Delete"))
            {
              ne::DeleteNode(imguiNodeIdCtx);
            }
          }
        }
      }
      ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Pin Context Menu"))
    {
      UInt nodeId = imguiFromPinStart(imguiNodePinIdCtx.Get());
      auto nodeIt = nodeGUIDMap.find(nodeId);

      if (nodeIt != nodeGUIDMap.end())
      {
        ImGui::TextUnformatted("Pin Context Menu");
        ImGui::Separator();

        if (auto nodePtr = nodeIt->second.lock())
        {
          bool isInput = nodePtr->isImguiInputPin(imguiNodePinIdCtx.Get());

          ImGui::Text("%s", nodePtr->imguiPinToName(imguiNodePinIdCtx.Get()).c_str());
          ImGui::Text("ID: %d", imguiNodePinIdCtx.Get());
          ImGui::Text("Type: %s", paramTypeToString(nodePtr->imguiPinToType(imguiNodePinIdCtx.Get())).c_str());

          if (isInput)
          {
            ImGui::Separator();
            if (ImGui::MenuItem("Break Link"))
            {
              ne::DeleteLink(imguiNodePinIdCtx.Get());
            }
          }
        }
      }
      ImGui::EndPopup();
    }

    
    if (ImGui::BeginPopup("Link Context Menu"))
    {
      UInt nodeId = imguiFromPinStart(imguiNodeLinkIdCtx.Get());
      auto nodeIt = nodeGUIDMap.find(nodeId);

      if (nodeIt != nodeGUIDMap.end())
      {
        ImGui::TextUnformatted("Link Context Menu");
        ImGui::Separator();

        if (auto nodePtr = nodeIt->second.lock())
        {
          bool isInput = nodePtr->isImguiInputPin(imguiNodeLinkIdCtx.Get());

          ImGui::Text("%s", nodePtr->imguiPinToName(imguiNodeLinkIdCtx.Get()).c_str());
          ImGui::Text("ID: %d", imguiNodeLinkIdCtx.Get());
          ImGui::Text("Type: %s", paramTypeToString(nodePtr->imguiPinToType(imguiNodeLinkIdCtx.Get())).c_str());

          if (isInput)
          {
            ImGui::Separator();
            if (ImGui::MenuItem("Delete"))
            {
              ne::DeleteLink(imguiNodeLinkIdCtx);
            }
          }
        }
      }
      ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Create New Node"))
    {
      ImGui::InputText("[Node name]", &imguiNextNodeName);

      // Node type combo selection box
      if (ImGui::BeginCombo("Type", imguiNodeTypeComboNameList[imguiContextComboCurrentID]))
      {
        for (size_t i = 0; i < imguiNodeTypeComboNameList.size(); ++i)
        {
          if (ImGui::Selectable(imguiNodeTypeComboNameList[i], i == imguiContextComboCurrentID))
          {
            imguiContextComboCurrentID = i;
          }
        }
        ImGui::EndCombo();
      }

      // Display the create new node button only if the entered name is unique
      bool isNameUsed = findNode(imguiNextNodeName).get();

      if (isNameUsed)
      {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);

        // Simulate button transparency
        ImVec4 initialColour = ImGui::GetStyleColorVec4(ImGuiCol_Button);
        initialColour.w *= .5f;
        ImGui::PushStyleColor(ImGuiCol_Button, initialColour);
      }

      if (ImGui::Button("Create Node"))
      {
        // Attempt to create and add the selected node type
        if (imguiContextComboCurrentID < imguiNodeCreationFunctionList.size())
        {
          if (BlendNode* createdNode = imguiNodeCreationFunctionList[imguiContextComboCurrentID](this, imguiNextNodeName))
          {
            if (BlendNodePtr nodePtr = addNode(createdNode))
            {
              ne::SetNodePosition(nodePtr->getImguiPinStart(), imguiPopupLocation);
              ImGui::CloseCurrentPopup();
            }
          }
        }
      }

      // End disable button
      if (isNameUsed)
      {
        ImGui::PopItemFlag();
        ImGui::PopStyleColor();
      }
      
      ImGui::EndPopup();
    }

    ne::Resume();
  }

  void BlendTree::imguiPopup()
  {
    imguiPopupLocation = ne::ScreenToCanvas(ImGui::GetMousePosOnOpeningCurrentPopup());
    imguiContextComboCurrentID = 0;
  }

  const void* BlendTree::getGlobalVariable(ParamType type, gef::StringId nameID) const
  {
    auto& map = globalVariableMaps[type];
    auto variableIt = map.find(nameID);
    return variableIt == map.end() ? nullptr : variableIt->second;
  }

  void BlendTree::imguiRegisterAllNodes()
  {
    imguiNodeTypeComboNameList.clear();
    imguiNodeCreationFunctionList.clear();

    // Sorts out abstract, multi-type nodes with no construction requirements
    #define PushAbstractNode(type, classification, nameprefix) imguiPushNodeClass(#nameprefix" "#type, [&](BlendTree* tree, Label name) -> BlendNode* { return new type##classification(name); });

    // Getters
    PushAbstractNode(Bool, GetterNode, Get);
    PushAbstractNode(Float, GetterNode, Get);
    PushAbstractNode(Vector2, GetterNode, Get);
    PushAbstractNode(Int, GetterNode, Get);
    PushAbstractNode(String, GetterNode, Get);
    PushAbstractNode(Animation, GetterNode, Get);
    PushAbstractNode(Transform, GetterNode, Get);

    // Setters
    PushAbstractNode(Bool, SetterNode, Set);
    PushAbstractNode(Float, SetterNode, Set);
    PushAbstractNode(Vector2, SetterNode, Set);
    PushAbstractNode(Int, SetterNode, Set);
    PushAbstractNode(String, SetterNode, Set);
    PushAbstractNode(Transform, SetterNode, Set);

    // Debuggers
    PushAbstractNode(Bool, DebugNode, Debug);
    PushAbstractNode(Float, DebugNode, Debug);
    PushAbstractNode(Vector2, DebugNode, Debug);
    PushAbstractNode(Int, DebugNode, Debug);
    PushAbstractNode(String, DebugNode, Debug);
    PushAbstractNode(Animation, DebugNode, Debug);
    PushAbstractNode(Transform, DebugNode, Debug);

    #undef PushAbstractNode

    // Sorts out normal nodes with no construction requirements
    #define PushVanillaNode(implName, className) imguiPushNodeClass(#className, [&](BlendTree* tree, Label name) -> BlendNode* { return new implName(name); });

    // Blend nodes
    PushVanillaNode(ClipNode, SkeleClip);
    PushVanillaNode(CrossFadeControllerNode, CrossFader);
    PushVanillaNode(BinaryInterpolatorNode, BinaryInterp);
    PushVanillaNode(QuadInterpolatorNode, QuadInterp);
    PushVanillaNode(InverseKineNode, FABRIK);

    // Output
    //SkeletonOutputNode::registerClass();

    #undef PushVanillaNode
  }
  void BlendTree::imguiPushNodeClass(Literal name, const CreateNodeFunc& createFunc)
  {
    BlendTree::imguiNodeTypeComboNameList.push_back(name);
    BlendTree::imguiNodeCreationFunctionList.push_back(createFunc);
  }
}