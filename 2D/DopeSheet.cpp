#include "DopeSheet.h"
#include "Maths.h"

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
        
        float durationProgress = .0f;
        for (auto& unrefinedKey : subTrack)
        {
          // Start to build the key
          subProgress.emplace_back();
          auto& freshKey = subProgress.back();
          freshKey.targetTransform = gef::Matrix33::kIdentity;
          freshKey.angle = .0f;
          freshKey.startTime = durationProgress;
          freshKey.easeIn = unrefinedKey.easeIn;
          freshKey.easeOut = unrefinedKey.easeOut;

          // Build and apply a transformation matrix for this key according to its type
          switch (AttributeType attribType = static_cast<AttributeType>(attributeID))
          {
            case AttributeTranslation:
            {
              freshKey.targetTransform.SetTranslation(gef::Vector2(unrefinedKey.values[0], unrefinedKey.values[1]));
            }
            break;
            case AttributeRotation:
            {
              freshKey.angle = unrefinedKey.values[0];
            }
            break;
            default: throw; // TODO: implement
          }

          durationProgress += unrefinedKey.duration;
        }

        // We cannot have just 'one' key. One key implies a rest position therefore create an additional dummy!
        if (subProgress.size() <= 1)
        {
          subProgress.push_back(subProgress.back());
          subProgress.back().startTime = durationProgress;
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
    float span = next.startTime - first.startTime;
    float norm = std::min((relativeTime - first.startTime) / span, 1.f);

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
    rotationTransform.Rotate(slerp(first.angle, next.angle, norm));

    return rotationTransform * uniformTransform;
  }

  float DopeSheet2D::BakedTrack::lerp(float a, float b, float t)
  {
    return a * (1.f-t) + b * t;
  }

  float DopeSheet2D::BakedTrack::slerp(float a, float b, float t)
  {
    float diff = b - a;
    float sign = copysignf(1.f, diff);
    diff *= sign; // Strip the sign for a single comparison
    if (diff > MATHS_PI) // Take the shortest path on the circle
    {
      diff -= MATHS_TAU;
    }
    diff *= sign; // Add the sign back!

    return a + diff * t; // Lerp as normal now
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
      size_t nextKeyID = currentKeyID + 1;

      // Key target
      const DopeSheet2D::Keyframe* nextKey = &track.trackPtr->getKey(subID, nextKeyID);
      
      // Validate target key according to the current time
      ++nextKeyID; // Next AGAIN
      if (nextKey->startTime < scaledTime && nextKeyID < track.trackPtr->getKeyframeCount(subID))
      {
        ++currentKeyID;
        nextKey = &track.trackPtr->getKey(subID, nextKeyID);
      }

      const DopeSheet2D::Keyframe* currentKey = &track.trackPtr->getKey(subID, currentKeyID);

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