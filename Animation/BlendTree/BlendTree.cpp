#include "BlendTree.h"
#include "UtilityNodes.h"
#include "SkeletonBlendNodes.h"

namespace BlendTree
{
  BlendTree::BlendTree() : imguiNodeContext{ nullptr }, imguiNextPinMajor{ 1 }, updateParity{ false }
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
  }

  BlendNodePtr BlendTree::setOutputNode(BlendNode* node)
  {
    outputNode = BlendNodePtr(node);
    addNodeInternal(outputNode);
    return outputNode;
  }

  void BlendTree::updateNodes(float dt)
  {
    // Flip flop the frame parity
    updateParity = !updateParity;

    outputNode->update(this, dt);
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
    ne::Begin("My Humble Test Editor");

    for (auto& node : nodeMap)
    {
      //ne::BeginNode(node.first);

      //ImGui::BeginHorizontal("hori");
      //ImGui::Spring(1);
      //ImGui::Text(node.second->getClassName().c_str());
      //ImGui::Text(node.second->getName().c_str());
      //ImGui::Spring(1);
      //ImGui::EndHorizontal();

      node.second->render();

      //ne::EndNode();
    }

    for (auto& node : nodeMap)
    {
      node.second->renderLinks();
    }

    ne::End();
    ne::SetCurrentEditor(nullptr);
  }

  void BlendTree::endRenderContext()
  {
    if (!imguiNodeContext) { return; }

    ne::DestroyEditor(imguiNodeContext);
    imguiNodeContext = nullptr;
  }

  BlendNodePtr BlendTree::addNode(BlendNode* node)
  {
    BlendNodePtr nodePtr(node);
    nodeMap.insert({
      StringTable.Add(node->getName()), nodePtr
    });

    addNodeInternal(nodePtr);
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

  void BlendTree::addNodeInternal(BlendNodePtr freshNodePtr)
  {
    freshNodePtr->acceptTree(this);

    freshNodePtr->setImguiPinStart(imguiToPinStart(imguiNextPinMajor));
    ++imguiNextPinMajor;
  }
}
