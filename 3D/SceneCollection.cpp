#include "3D/SceneCollection.h"

namespace Animation
{
  SceneCollection::SceneCollection()
  {
  }

  SceneCollection::~SceneCollection()
  {
    for (size_t i = 0; i < scenes.getHeapSize(); ++i)
    {
      delete scenes.get(i);
    }
    scenes.clear();
  }

  gef::Scene* SceneCollection::getScene(Label name)
  {
    if (auto indexer = scenes.getMetaInfo(name))
    {
      return scenes.get(indexer->getHeapID());
    }

    return nullptr;
  }

  void Animation::SceneCollection::registerScene(Label name, gef::Scene* scene)
  {
    scenes.add(name, scene);
  }
}
