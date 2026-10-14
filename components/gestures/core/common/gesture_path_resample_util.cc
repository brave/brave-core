// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_path_resample_util.h"

#include <limits>

#include "base/numerics/safe_conversions.h"
#include "brave/components/gestures/core/common/gesture_path_distance_util.h"

namespace gestures {

namespace {

// Walks `gesture_path`'s segments, appending an interpolated point every
// `interval` of arc length, up to `max_point_count`.
void AppendResampledPoints(const GesturePath& gesture_path,
                           float interval,
                           size_t max_point_count,
                           GesturePath& resampled_gesture_path) {
  float accumulated = 0.0F;
  gfx::PointF last_point = gesture_path.front();
  for (size_t i = 1; i < gesture_path.size() &&
                     resampled_gesture_path.size() < max_point_count;
       ++i) {
    const gfx::PointF& current = gesture_path.at(i);
    float segment_length = (current - last_point).Length();

    while (accumulated + segment_length >= interval &&
           resampled_gesture_path.size() < max_point_count) {
      const float remaining = interval - accumulated;
      const float ratio = segment_length > 0 ? remaining / segment_length : 0;
      const gfx::PointF new_point(
          last_point.x() + ratio * (current.x() - last_point.x()),
          last_point.y() + ratio * (current.y() - last_point.y()));
      resampled_gesture_path.push_back(new_point);
      segment_length -= remaining;
      accumulated = 0;
      // Advances from the newly interpolated point, not the original
      // segment's start, so one long segment resamples into several points.
      last_point = new_point;
    }
    accumulated += segment_length;
    last_point = current;
  }
}

}  // namespace

GesturePath ResampleGesturePathToPointCount(const GesturePath& gesture_path,
                                            size_t point_count) {
  if (gesture_path.empty()) {
    return {};
  }

  const float path_length = GesturePathLength(gesture_path);
  if (path_length == 0) {
    return {point_count, gesture_path.front()};
  }

  const float interval =
      path_length / base::saturated_cast<float>(point_count - 1);
  GesturePath resampled_gesture_path;
  resampled_gesture_path.reserve(point_count);
  resampled_gesture_path.push_back(gesture_path.front());
  AppendResampledPoints(gesture_path, interval, point_count,
                        resampled_gesture_path);

  while (resampled_gesture_path.size() < point_count) {
    resampled_gesture_path.push_back(gesture_path.back());
  }
  return resampled_gesture_path;
}

GesturePath ResampleGesturePathAtInterval(const GesturePath& gesture_path,
                                          float interval_pixels) {
  if (gesture_path.empty()) {
    return {};
  }
  GesturePath resampled_gesture_path = {gesture_path.front()};
  if (gesture_path.size() < 2 || interval_pixels <= 0) {
    return resampled_gesture_path;
  }
  AppendResampledPoints(gesture_path, interval_pixels,
                        std::numeric_limits<size_t>::max(),
                        resampled_gesture_path);
  if (resampled_gesture_path.back() != gesture_path.back()) {
    resampled_gesture_path.push_back(gesture_path.back());
  }
  return resampled_gesture_path;
}

}  // namespace gestures
