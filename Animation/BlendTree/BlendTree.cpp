#include "BlendTree.h"
#include "UtilityNodes.h"
#include "SkeletonBlendNodes.h"

namespace BlendTree
{
  BlendTree::BlendTree() : imguiNodeContext{ nullptr }, imguiNextPinMajor{0}
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
    AnimationGetterNode::registerClass();

    // Blend nodes
    AnimationNode::registerClass();
    ClipNode::registerClass();
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
      ne::BeginNode(node.first);

      ImGui::BeginHorizontal("hori");
      ImGui::Spring(1);
      ImGui::Text(node.second->getClassName().c_str());
      ImGui::Text(node.second->getName().c_str());
      ImGui::Spring(1);
      ImGui::EndHorizontal();

      node.second->render();

      ne::EndNode();
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

    nodePtr->setImguiPinStart(imguiToPinStart(imguiNextPinMajor));
    ++imguiNextPinMajor;
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
}
