#pragma once
#include <type_traits>
#include <unordered_map>

#include "Defs.h"
#include "Globals.h"

// Package of metadata related to a heaped item which should not occupy the heap itself
class NamedHeapInfo
{
  public:
  NamedHeapInfo();

  void setHeapID(UInt id);
  inline UInt getHeapID() const { return collectionID; }

  private:
  UInt collectionID; // The element in the heap this structure refers to
};

// A heap of data with associated string key metadata
template<typename Data, typename Indexer = NamedHeapInfo> class NamedHeap
{
  static_assert(std::is_base_of<NamedHeapInfo, Indexer>::value, "Indexer must derive from NamedHeapInfo");

  public:
  NamedHeap() = default;

  void clear();

  Indexer& add(gef::StringId nameHash, const Data& item);
  Data& add(gef::StringId nameHash);

  // Where possible use StringId counterparts
  Indexer& add(Label name, const Data& item);
  Data& add(Label name);
  Indexer* getMetaInfo(Label name);
  const Indexer* getMetaInfo(Label name) const;
  UInt getID(Label name) const;

  Data& get(UInt id) { return heapedData.at(id); }
  const Data& get(UInt id) const { return heapedData.at(id); }
  Indexer* getMetaInfo(gef::StringId nameHash);
  const Indexer* getMetaInfo(gef::StringId nameHash) const;
  UInt getID(gef::StringId nameHash) const;
  size_t getHeapSize() const { return heapedData.size(); }

  const std::unordered_map<gef::StringId, Indexer>& getNameMap() const { return metaMap; }

  private:
  std::unordered_map<gef::StringId, Indexer> metaMap;
  std::vector<Data> heapedData;
};

template<typename Data, typename Indexer>
inline void NamedHeap<Data, Indexer>::clear()
{
  heapedData.clear();
  metaMap.clear();
}

template<typename Data, typename Indexer>
inline Indexer& NamedHeap<Data, Indexer>::add(gef::StringId nameHash, const Data& item)
{
  auto& metaIt = this->metaMap.find(nameHash);
  if (metaIt == this->metaMap.end())
  {
    // Register new metadata
    metaIt = this->metaMap.insert({ nameHash, Indexer() }).first;

    // Link to a new element
    metaIt->second.setHeapID(UInt(heapedData.size()));
    heapedData.emplace_back(item);
  }

  return metaIt->second;
}
  
template<typename Data, typename Indexer>
inline Data& NamedHeap<Data, Indexer>::add(gef::StringId nameHash)
{
  auto metaIt = this->metaMap.find(nameHash);
  if (metaIt == this->metaMap.end())
  {
    // Register new metadata
    metaIt = this->metaMap.insert({ nameHash, Indexer() }).first;

    // Link to a new element
    metaIt->second.setHeapID(UInt(heapedData.size()));
    heapedData.emplace_back();
  }

  return heapedData[metaIt->second.getHeapID()];
}
template<typename Data, typename Indexer>
inline Indexer& NamedHeap<Data, Indexer>::add(Label name, const Data& item)
{
  return this->add(StringTable.Add(name), item);
}
template<typename Data, typename Indexer>
inline Data& NamedHeap<Data, Indexer>::add(Label name)
{
  return this->add(StringTable.Add(name));
}
template<typename Data, typename Indexer>
inline Indexer* NamedHeap<Data, Indexer>::getMetaInfo(Label name)
{
  return getMetaInfo(StringTable.Add(name));
}
template<typename Data, typename Indexer>
inline const Indexer* NamedHeap<Data, Indexer>::getMetaInfo(Label name) const
{
  return getMetaInfo(StringTable.Add(name));
}
template<typename Data, typename Indexer>
inline UInt NamedHeap<Data, Indexer>::getID(Label name) const
{
  return getID(StringTable.Add(name));
}
template<typename Data, typename Indexer>
inline Indexer* NamedHeap<Data, Indexer>::getMetaInfo(gef::StringId nameHash)
{
  auto& metaIt = this->metaMap.find(nameHash);
  return metaIt == this->metaMap.end() ? nullptr : &metaIt->second;
}
template<typename Data, typename Indexer>
inline const Indexer* NamedHeap<Data, Indexer>::getMetaInfo(gef::StringId nameHash) const
{
  return getMetaInfo(nameHash);
}
template<typename Data, typename Indexer>
inline UInt NamedHeap<Data, Indexer>::getID(gef::StringId nameHash) const
{
  auto& metaIt = this->metaMap.find(nameHash);
  return metaIt == this->metaMap.end() ? SNULL : metaIt->second.getHeapID();
}