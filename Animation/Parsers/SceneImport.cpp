#include "Animation/Parsers/SceneImport.h"

namespace IO
{
  using namespace Animation;

  bool SceneImporter::parseSkeleton(Animation::Skeleton3D& out, Animation::SceneCollection& scenes, Literal path, gef::Platform& platform, bool animationOnly)
  {
    gef::Scene* scene = scenes.getScene(path);
    if (scene == nullptr)
    {
      scene = new gef::Scene();
      scene->ReadSceneFromFile(platform, path);
      scenes.registerScene(path, scene);
    }

    if (!animationOnly)
    {
      // Ensure materials exist for this scene
      if (scene->materials.empty())
      {
        scene->CreateMaterials(platform);
      }

      // Ensure meshes exist for this scene
      if (scene->meshes.empty())
      {
        scene->CreateMeshes(platform);
      }

      // Create the skeleton
      if (!scene->skeletons.empty())
      {
        out.setSkeleton(scene->skeletons.front());
      }
      if (!scene->meshes.empty())
      {
        out.setMesh(scene->meshes.front());
      }
    }

    for (auto& it : scene->animations)
    {
      // Move the animation name into the table
      std::string name;
      scene->string_id_table.Find(it.first, name);
      StringTable.Add(name); // TODO: Could do with a hint system

      out.addAnimation(it.first, it.second);
    }

    return out.getSkeleton() && out.getMesh();
  }

}