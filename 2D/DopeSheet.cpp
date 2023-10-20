#include "DopeSheet.h"

namespace Animation
{
  DopeSheet2D::DopeSheet2D() : sheetDuration{ .0f }, sheetRate{60.f}
  {

  }

  DopeSheet2D::BakedTrack* DopeSheet2D::bakeTrack(const DetailedTrack& track) const
  {
    BakedTrack* out = new BakedTrack();

    // Bake the track for each attribute
    for (Byte attributeID = 0; attributeID < AttributeType::AttributeCount; ++attributeID)
    {
      auto& subTrack = track.attributeTracks[attributeID];
      if (subTrack.size())
      {
        // This track is in use, start build
        out->subTracks.emplace_back();
        auto& subProgress = out->subTracks.back();
        
        gef::Matrix33 transformProgress = gef::Matrix33::kIdentity;
        float rotationProgress = .0f;
        float durationProgress = .0f;

        for (auto& unrefinedKey : subTrack)
        {
          transformProgress = gef::Matrix33::kIdentity;
          rotationProgress = .0f;

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
              rotationProgress += unrefinedKey.values[0]; // Note that this is lerp compliant!
            }
            break;
            default: throw; // TODO: implement
          }

          // Finally, build the key itself
          subProgress.emplace_back();
          {
            auto& freshKey = subProgress.back();
            freshKey.targetTransform = transformProgress;
            freshKey.angle = rotationProgress;
            freshKey.endTime = durationProgress;
            freshKey.easeIn = unrefinedKey.easeIn;
            freshKey.easeOut = unrefinedKey.easeOut;
          }
        }
      }
    }

    if (!out->getAttributeTrackCount())
    {
      delete out;
      return nullptr;
    }

    return out;
  }

  void DopeSheet2D::inspectTracks(const std::function<void(Label, const DetailedTrack&)>& itFunc)
  {
    for (auto& trackIt : detailedSheet.trackNames)
    {
      itFunc(trackIt.first, detailedSheet.trackCollection[trackIt.second]); // Reflect the track
    }
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

  gef::Matrix33 DopeSheet2D::BakedTrack::applyTransform(float relativeTime, const Keyframe& first, const Keyframe& next)
  {
    float span = next.endTime - first.endTime;
    float norm = std::max((next.endTime - relativeTime) / span, .0f);

    // TODO: easing
    auto& in = first.easeIn;
    auto& out = next.easeOut;

    gef::Matrix33 uniformTransform = gef::Matrix33::kIdentity;
    for (size_t i = 0; i < 9; ++i)
    {
      reinterpret_cast<float*>(uniformTransform.m)[i] = lerp(reinterpret_cast<const float*>(&first.targetTransform.m)[i], reinterpret_cast<const float*>(&next.targetTransform.m)[i], norm);
    }

    // TODO: surely there is a better way to do this
    gef::Matrix33 rotationTransform = gef::Matrix33::kIdentity;
    rotationTransform.Rotate(lerp(first.angle, next.angle, norm)); // Using accumulated angle therefore lerp is valid!

    return rotationTransform * uniformTransform;
  }

  float DopeSheet2D::BakedTrack::lerp(float a, float b, float t)
  {
    return a * (1.f-t) + b * t;
  }

  DopePlayer2D::DopePlayer2D() : sheet{ nullptr }, time{ .0f }, scaledTime{ .0f }, playing{ false }
  {

  }

  gef::Matrix33 DopePlayer2D::getCurrentTransform(size_t trackID)
  {
    auto& track = tracks[trackID];
    
    gef::Matrix33 transform = gef::Matrix33::kIdentity;
    if (track.trackPtr == nullptr) { return transform; }

    for (size_t subID = 0; subID < track.keyPointers.size(); ++subID)
    {
      auto& currentKeyID = track.keyPointers[subID];

      // Key target
      const DopeSheet2D::Keyframe* currentKey = &track.trackPtr->getKey(subID, currentKeyID);
      // Validate target key according to the current time
      if (currentKey->endTime < scaledTime && currentKeyID + 1 < track.trackPtr->getKeyframeCount(subID))
      {
        currentKey = &track.trackPtr->getKey(subID, ++currentKeyID);
      }

      const DopeSheet2D::Keyframe* nextKey = &track.trackPtr->getKey(subID, currentKeyID + 1);

      transform = transform * track.trackPtr->applyTransform(scaledTime, *currentKey, *nextKey);
    }

    return transform;
  }

  void DopePlayer2D::setTrack(size_t idx, DopeSheet2D::BakedTrack* trackPtr)
  {
    auto& tracker = tracks[idx];
    if (tracker.trackPtr = trackPtr)
    {
      tracker.keyPointers.resize(trackPtr->getAttributeTrackCount());
      std::fill(tracker.keyPointers.begin(), tracker.keyPointers.end(), 0);
    }
  }

  void DopePlayer2D::reset()
  {
    for (auto& ptr : tracks) { std::fill(ptr.keyPointers.begin(), ptr.keyPointers.end(), 0); }
  }

  void DopePlayer2D::update(float dt)
  {
    if (!isPlaying()) { return; }

    time += dt;
    scaledTime = time * sheet->getRate();
    if (scaledTime > sheet->getDuration())
    {
      reset();
      scaledTime = fmodf(scaledTime, sheet->getDuration());
      time = scaledTime / sheet->getRate();
    }
  }
}