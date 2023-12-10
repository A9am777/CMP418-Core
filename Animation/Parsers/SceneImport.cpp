#include "Animation/Parsers/SceneImport.h"
#include <animation/animation.h>

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
      StringTable.Add(name);

      out.addAnimation(it.first, it.second);
    }

    return out.getSkeleton() && out.getMesh();
  }

  bool SceneImporter::parseSkeletonAppendAnimation(Animation::Skeleton3D& out, Animation::SceneCollection& scenes, Literal path, Literal name, gef::Platform& platform)
  {
    gef::Scene* scene = scenes.getScene(path);
    if (scene == nullptr)
    {
      scene = new gef::Scene();
      scene->ReadSceneFromFile(platform, path);
      scenes.registerScene(path, scene);
    }

    if (!scene->animations.empty())
    {
      if (auto animation = scene->animations.begin()->second)
      {
        animation->set_name_id(StringTable.Add(name));
        out.addAnimation(animation->name_id(), animation);
      }
      return true;
    }

    return false;
  }

}