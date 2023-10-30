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
      AttributeType attribType = static_cast<AttributeType>(attributeID);
      if (subTrack.size())
      {
        // This track is in use, start build
        out->subTracks.emplace_back();
        auto& subProgress = out->subTracks.back();
        
        // Set attribute flags
        switch (attribType)
        {
          case AttributeFull:
            subProgress.hasRotation =
            subProgress.hasScale =
            subProgress.hasTranslation = true;
          case AttributeScale:
          case AttributeWidth:
          case AttributeHeight:
            subProgress.hasScale = true;
          break;
          case AttributeRotation:
            subProgress.hasRotation = true;
          break;
          case AttributeTranslation:
          case AttributeX:
          case AttributeY:
            subProgress.hasTranslation = true;
          break;
        }

        float durationProgress = .0f;
        for (auto& unrefinedKey : subTrack)
        {
          // Start to build the key
          subProgress.keyframes.emplace_back();
          auto& freshKey = subProgress.keyframes.back();
          freshKey.startTime = durationProgress;
          freshKey.easeIn = unrefinedKey.easeIn;
          freshKey.easeOut = unrefinedKey.easeOut;

          // Build and apply a transformation matrix for this key according to its type
          switch (attribType)
          {
            case AttributeTranslation:
            {
              freshKey.transform.translation = gef::Vector2(unrefinedKey.values[0], unrefinedKey.values[1]);
            }
            break;
            case AttributeRotation:
            {
              freshKey.transform.rotation = unrefinedKey.values[0];
            }
            break;
            default: throw; // TODO: implement
          }

          durationProgress += unrefinedKey.duration;
        }

        // We cannot have just 'one' key. One key implies a rest position therefore create an additional dummy!
        if (subProgress.keyframes.size() <= 1)
        {
          subProgress.keyframes.push_back(subProgress.keyframes.back());
          subProgress.keyframes.back().startTime = durationProgress;
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

  Maths::Transform2D DopeSheet2D::BakedTrack::applyTransform(float relativeTime, const Keyframe& first, const Keyframe& next, UInt subtrack)
  {
    const SubTrack& track = subTracks[subtrack];

    float span = next.startTime - first.startTime;
    float norm = std::min((relativeTime - first.startTime) / span, 1.f);

    // TODO: easing
    auto& in = first.easeIn;
    auto& out = next.easeOut;
    
    Maths::Transform2D transform;
    transform.scale = track.hasScale ? lerp(first.transform.scale, next.transform.scale, norm) : gef::Vector2::kOne;
    transform.translation = track.hasTranslation ? lerp(first.transform.translation, next.transform.translation, norm) : gef::Vector2::kZero;
    transform.rotation = track.hasRotation ? slerp(first.transform.rotation, next.transform.rotation, norm) : .0f;
    return transform;
  }

  float DopeSheet2D::BakedTrack::lerp(float a, float b, float t) const
  {
    return a * (1.f-t) + b * t;
  }

  gef::Vector2 DopeSheet2D::BakedTrack::lerp(const gef::Vector2& a, const gef::Vector2& b, float t) const
  {
    return a * (1.f - t) + b * t;
  }

  float DopeSheet2D::BakedTrack::slerp(float a, float b, float t) const
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

  Maths::Transform2D DopePlayer2D::getCurrentTransform(size_t trackID)
  {
    auto& track = tracks[trackID];
    
    Maths::Transform2D transform;
    transform.scale = gef::Vector2::kOne;
    transform.rotation = .0f;
    transform.translation = gef::Vector2::kZero;

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

      transform = transform + track.trackPtr->applyTransform(scaledTime, *currentKey, *nextKey, subID);
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