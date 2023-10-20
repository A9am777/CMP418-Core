#include "DopeSheet.h"

namespace Animation
{
  DopeSheet2D::DopeSheet2D() : sheetDuration{ .0f }, sheetRate{1.f}
  {

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
  bool DopeSheet2D::doesTrackExist(Label name)
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