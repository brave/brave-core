// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_util.h"

namespace gestures {

bool IsFarEnoughFromLastDrawnPoint(gfx::PointF last_accepted_point,
                                   gfx::PointF candidate,
                                   double min_point_distance_pixels) {
  return (candidate - last_accepted_point).Length() >=
         min_point_distance_pixels;
}

GesturePath ResampleGesturePathAtInterval(const GesturePath& path,
                                          float interval_pixels) {
  if (path.empty()) {
    return {};
  }
  GesturePath resampled = {path.front()};
  if (path.size() < 2 || interval_pixels <= 0) {
    return resampled;
  }

  // Carries over leftover distance from one original segment to the next, so
  // intervals stay consistent across the original points' own boundaries,
  // not just within each one.
  float accumulated = 0;
  gfx::PointF last_point = path.front();
  for (size_t i = 1; i < path.size(); ++i) {
    const gfx::PointF& current = path.at(i);
    float segment_length = (current - last_point).Length();

    while (accumulated + segment_length >= interval_pixels) {
      const float remaining = interval_pixels - accumulated;
      const float ratio = segment_length > 0 ? remaining / segment_length : 0;
      const gfx::PointF new_point(
          last_point.x() + ratio * (current.x() - last_point.x()),
          last_point.y() + ratio * (current.y() - last_point.y()));
      resampled.push_back(new_point);
      segment_length -= remaining;
      // Advances from the newly interpolated point, not the original
      // segment's start, so a single long original segment resamples into
      // several evenly-spaced points rather than repeatedly interpolating
      // from the same fixed start.
      last_point = new_point;
      accumulated = 0;
    }
    accumulated += segment_length;
    last_point = current;
  }
  if (resampled.back() != path.back()) {
    resampled.push_back(path.back());
  }
  return resampled;
}

}  // namespace gestures
