#include "Animation/BlendTree/UtilityNodes.h"

namespace BlendTree
{
  // Implement the class, including declaring the static class descriptors
  #define ImplementGetterNode(Typename) NodeClassMeta Typename##GetterNode::get##Typename##ClassDescriptor;

  ImplementGetterNode(Bool);
  ImplementGetterNode(Float);
  ImplementGetterNode(Animation);
}