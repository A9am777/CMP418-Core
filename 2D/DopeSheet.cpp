#include "DopeSheet.h"

namespace Animation
{
  DopeSheet2D::DopeSheet2D() : sheetDuration{ .0f }, sheetRate{1.f}
  {

  }

  DopeSheet2D::BakedTrack DopeSheet2D::bakeTrack(DetailedTrack& track) const
  {
    BakedTrack out;

    // Bake the track for each attribute
    for (Byte attributeID = 0; attributeID < AttributeType::AttributeCount; ++attributeID)
    {
      auto& subTrack = track.attributeTracks[attributeID];
      if (subTrack.size())
      {
        // This track is in use, start build
        out.subTracks.emplace_back();
        auto& subProgress = out.subTracks.back();
        
        gef::Matrix33 transformProgress = gef::Matrix33::kIdentity;
        float durationProgress = .0f;
        for (auto& unrefinedKey : subTrack)
        {
          durationProgress += unrefinedKey.duration;

          // Build and apply a transformation matrix for this key according to its type
          switch (AttributeType attribType = static_cast<AttributeType>(attributeID))
          {
            case AttributeTranslation:
            {
              gef::Matrix33 translationMatrix = gef::Matrix33::kIdentity;
              translationMatrix.SetTranslation(gef::Vector2(unrefinedKey.values[0], unrefinedKey.values[1]));
              transformProgress = transformProgress * translationMatrix;
            }
            break;
            case AttributeRotation:
            {
              gef::Matrix33 rotationMatrix = gef::Matrix33::kIdentity;
              rotationMatrix.Rotate(unrefinedKey.values[0]);
              transformProgress = transformProgress * rotationMatrix;
            }
            break;
            default: throw; // TODO: implement
          }

          // Finally, build the key itself
          subProgress.emplace_back();
          {
            auto& freshKey = subProgress.back();
            freshKey.targetTransform = transformProgress;
            freshKey.endTime = durationProgress;
            freshKey.easeIn = unrefinedKey.easeIn;
            freshKey.easeOut = unrefinedKey.easeOut;
          }
        }
      }
    }

    return out;
  }

  DopeSheet2D::DetailedTrack& DopeSheet2D::getTrack(Label name)
  {
    auto it = detailedSheet.trackNames.find(name);
    if (it == detailedSheet.trackNames.end())
    {
      // Create new
      it = detailedSheet.trackNames.insert({ name,static_cast<UInt>(detailedSheet.trackCollection.size()) }).first;
      detailedSheet.trackCollection.emplace_back();
    }

    // Be careful that this is temporary
    return detailedSheet.trackCollection[it->second];
  }
  bool DopeSheet2D::doesTrackExist(Label name) const
  {
    auto it = detailedSheet.trackNames.find(name);
    return it != detailedSheet.trackNames.end();
  }
  void DopeSheet2D::addBaseKeyframe(DetailedTrack& track, float duration, const std::initializer_list<float>& params, AttributeType keyType, const TweenPoint& in, const TweenPoint& out)
  {
    track.attributeTracks[keyType].push_back({
      params,
      in,
      out,
      duration
    });
  }
}