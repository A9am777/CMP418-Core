#include "BlendTree.h"
#include "UtilityNodes.h"
#include "SkeletonBlendNodes.h"

namespace BlendTree
{
  BlendTree::BlendTree() : imguiNodeContext{ nullptr }, imguiNextPinMajor{ 1 }, updateParity{ false }, useTraversalCache{ true }
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
    IntGetterNode::registerClass();
    StringGetterNode::registerClass();
    AnimationGetterNode::registerClass();

    // Setters
    BoolSetterNode::registerClass();
    FloatSetterNode::registerClass();
    IntSetterNode::registerClass();
    StringSetterNode::registerClass();

    // Blend nodes
    AnimationNode::registerClass();
    ClipNode::registerClass();

    // Output
    SkeletonOutputNode::registerClass();
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
            // idk what this is
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
      if (ne::ShowNodeContextMenu(&imguiNodeIdCtx)) { ImGui::OpenPopup("Node Context Menu"); }
      else if (ne::ShowPinContextMenu(&imguiNodePinIdCtx)) { ImGui::OpenPopup("Pin Context Menu"); }
      else if (ne::ShowLinkContextMenu(&imguiNodeLinkIdCtx)) { ImGui::OpenPopup("Link Context Menu"); }
      else if (ne::ShowBackgroundContextMenu()) { ImGui::OpenPopup("Create New Node"); }
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

  BlendNodePtr BlendTree::addNode(BlendNode* node)
  {
    BlendNodePtr nodePtr(node);
    nodeMap.insert({
      StringTable.Add(node->getName()), nodePtr
      });

    nodePtr->acceptTree(this);

    {
      nodePtr->setImguiPinStart(imguiToPinStart(imguiNextPinMajor));
      nodeGUIDMap.insert_or_assign(imguiNextPinMajor, nodePtr);
      ++imguiNextPinMajor;
    }
    return nodePtr;
  }

  BlendNodePtr BlendTree::findNode(Label name)
  {
    return findNode(StringTable.Add(name));
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

    ne::Resume();
  }
}