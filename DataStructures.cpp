#include "DataStructures.h"


NamedHeapInfo::NamedHeapInfo() : collectionID{SNULL}
{
}

void NamedHeapInfo::setHeapID(UInt id)
{
  collectionID = id;
}