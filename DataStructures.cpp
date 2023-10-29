#include "DataStructures.h"

namespace Animation
{
  NamedHeapInfo::NamedHeapInfo() : collectionID{SNULL}
  {
  }

  void NamedHeapInfo::setHeapID(UInt id)
  {
    collectionID = id;
  }
}