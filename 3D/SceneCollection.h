#pragma once
#include <graphics/scene.h>
#include "DataStructures.h"

namespace Animation
{
  // Stores scene references
  class SceneCollection
  {
    public:
    SceneCollection();
    ~SceneCollection();

    gef::Scene* getScene(Label name);
    void registerScene(Label name, gef::Scene* scene);

    private:
    NamedHeap<gef::Scene*> scenes;
  };

}