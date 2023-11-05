#pragma once
#include <maths/matrix33.h>
#include <maths/vector2.h>
#include <vector>
#include <array>
#include <list>
#include <map>
#include <functional>

#include "Maths.h"
#include "DataStructures.h"
#include "../Defs.h"

namespace Animation
{
  class DopeSheet2D
  {
    public:
    enum class InterpolationType : Byte
    {
      TweenLinear
    };
    enum AttributeType : Byte // Global transform, translation, etc. Specifically the operation. Ordered by transform order
    {
      AttributeFull = 0,
      AttributeScale,
      AttributeWidth,
      AttributeHeight,
      AttributeRotation,
      AttributeTranslation,
      AttributeX,
      AttributeY,
      AttributeCount // Counts the possible number of attributes
    };
    struct TweenPoint // Interpolation at a point
    {
      InterpolationType tweenType = InterpolationType::TweenLinear;
      float tweenWeight = .0f;
    };
    struct DetailedKeyframe
    {
      std::vector<float> values; // Pack of all values for the attribute type
      TweenPoint easeIn;
      TweenPoint easeOut;
      float duration;
    };
    struct DetailedTrack
    {
      std::array<std::list<DetailedKeyframe>, AttributeCount> attributeTracks;
    };
    struct Keyframe
    {
      Maths::Transform2D transform;
      float startTime;
      TweenPoint easeIn;
      TweenPoint easeOut;
    };
    class BakedTrack
    {
      friend DopeSheet2D;
      public:

      Maths::Transform2D applyTransform(float relativeTime, const Keyframe& first, const Keyframe& next, UInt subtrack);
      const Keyframe& getKey(size_t subtrack, size_t idx) { return subTracks[subtrack].keyframes[idx]; }
      inline size_t getAttributeTrackCount() { return subTracks.size(); }
      inline size_t getKeyframeCount(size_t track) { return subTracks[track].keyframes.size(); }

      private:
      struct SubTrack
      {
        std::vector<Keyframe> keyframes;
        bool hasRotation = false;
        bool hasTranslation = false;
        bool hasScale = false;
      };

      float lerp(float a, float b, float t) const;
      gef::Vector2 lerp(const gef::Vector2& a, const gef::Vector2& b, float t) const;
      float slerp(float a, float b, float t) const;

      // Different attributes can have different interpolations therefore cannot be merged into one track and must be
      // kept separate
      std::vector<SubTrack> subTracks;
    };

    DopeSheet2D();

    BakedTrack* bakeTrack(const DetailedTrack& track) const; // Bakes a track to be ready for use
    void inspectTracks(const std::function<void(gef::StringId, const DetailedTrack&)>& itFunc); // Enables iteration of detailed tracks
    DetailedTrack& getTrack(Label name); // Finds or creates a track of name
    bool doesTrackExist(Label name) const;

    inline void addTranslationKeyframe(DetailedTrack& track, float duration, const gef::Vector2& offset, const TweenPoint& in = TweenPoint(), const TweenPoint& out = TweenPoint())
    { addBaseKeyframe(track, duration, {offset.x, offset.y}, AttributeTranslation, in, out); }
    inline void addRotationKeyframe(DetailedTrack& track, float duration, float angle, const TweenPoint& in = TweenPoint(), const TweenPoint& out = TweenPoint())
    { addBaseKeyframe(track, duration, { angle }, AttributeRotation, in, out); }
    inline void addScaleKeyframe(DetailedTrack& track, float duration, const gef::Vector2& scale, const TweenPoint& in = TweenPoint(), const TweenPoint& out = TweenPoint())
    { addBaseKeyframe(track, duration, { scale.x, scale.y}, AttributeScale, in, out); }

    inline void setRate(float rate) { sheetRate = rate; }
    inline void setDuration(float duration) { sheetDuration = duration; } // In frames or whatever unit that is offset by rate
    inline float getRate() const { return sheetRate; }
    inline float getDuration() const { return sheetDuration; }

    private:
    void addBaseKeyframe(DetailedTrack& track, float duration, const std::initializer_list<float>& params, AttributeType keyType, const TweenPoint& in, const TweenPoint& out);

    NamedHeap<DetailedTrack> detailedSheet; // Sheet information prior to optimisation

    float sheetDuration;
    float sheetRate;
  };

  class DopePlayer2D
  {
    public:
    DopePlayer2D();

    Maths::Transform2D getCurrentTransform(size_t trackID);
    inline void resizeTracks(size_t trackCount) { tracks.resize(trackCount); }
    inline void setPlaying(bool playState) { playing = playState; }
    void setSheet(DopeSheet2D* context) { sheet = context; }
    void setTrack(size_t idx, DopeSheet2D::BakedTrack* track);
    void reset();
    void update(float dt);

    inline bool isPlaying() const { return playing; }

    private:
    struct Tracker // Track and the current progress
    {
      DopeSheet2D::BakedTrack* trackPtr;
      std::vector<size_t> keyPointers;
    };

    DopeSheet2D* sheet;
    std::vector<Tracker> tracks;
    float time;
    float scaledTime; // Time relative to the sheet itself
    bool playing;
  };
}