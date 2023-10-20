#pragma once
#include <maths/matrix33.h>
#include <maths/vector2.h>
#include <vector>
#include <array>
#include <list>
#include <map>
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
    enum AttributeType : Byte // Global transform, translation, etc. Specifically the operation. Ordered by transform application
    {
      AttributeFull = 0,
      AttributeTranslation,
      AttributeX,
      AttributeY,
      AttributeRotation,
      AttributeScale,
      AttributeWidth,
      AttributeHeight,
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
      gef::Matrix33 targetTransform; // Precomputed
      TweenPoint easeIn;
      TweenPoint easeOut;
      float endTime;
    };
    class BakedTrack
    {
      friend DopeSheet2D;
      public:

      private:
      // Different attributes can have different interpolations therefore cannot be merged into one track and must be
      // kept separate
      std::vector<std::vector<Keyframe>> subTracks;
    };

    DopeSheet2D();

    BakedTrack bakeTrack(DetailedTrack& track) const; // Bakes a track to be ready for use
    DetailedTrack& getTrack(Label name); // Finds or creates a track of name
    bool doesTrackExist(Label name) const;

    inline void addTranslationKeyframe(DetailedTrack& track, float duration, const gef::Vector2& offset, const TweenPoint& in = TweenPoint(), const TweenPoint& out = TweenPoint())
    { addBaseKeyframe(track, duration, {offset.x, offset.y}, AttributeTranslation, in, out); }
    inline void addRotationKeyframe(DetailedTrack& track, float duration, float angle, const TweenPoint& in = TweenPoint(), const TweenPoint& out = TweenPoint())
    { addBaseKeyframe(track, duration, { angle }, AttributeRotation, in, out); }
    inline void addScaleKeyframe(DetailedTrack& track, float duration, const gef::Vector2& scale, const TweenPoint& in = TweenPoint(), const TweenPoint& out = TweenPoint())
    { addBaseKeyframe(track, duration, { scale.x, scale.y}, AttributeScale, in, out); }

    inline void setRate(float rate) { sheetRate = rate; }
    inline void setDuration(float duration) { sheetDuration = duration; }

    private:
    struct DetailedSheet
    {
      std::map<std::string, UInt> trackNames; // Name to allocation
      std::vector<DetailedTrack> trackCollection; // Raw per-object track
    };

    void addBaseKeyframe(DetailedTrack& track, float duration, const std::initializer_list<float>& params, AttributeType keyType, const TweenPoint& in, const TweenPoint& out);

    DetailedSheet detailedSheet; // Sheet information prior to optimisation

    float sheetDuration;
    float sheetRate;
  };
}