// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_path_resample_util.h"

#include <algorithm>
#include <limits>

#include "base/numerics/safe_conversions.h"
#include "brave/components/gestures/core/common/gesture_geometry_util.h"

namespace gestures {

GesturePath ResampleGesturePathToCount(const GesturePath& gesture_path,
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
  GesturePath resampled_path;
  resampled_path.reserve(point_count);
  resampled_path.push_back(gesture_path.front());

  float accumulated = 0;
  for (size_t i = 1;
       i < gesture_path.size() && resampled_path.size() < point_count; ++i) {
    gfx::PointF previous = gesture_path.at(i - 1);
    const gfx::PointF& current = gesture_path.at(i);
    float segment_length = (current - previous).Length();

    while (accumulated + segment_length >= interval &&
           resampled_path.size() < point_count) {
      const float remaining = interval - accumulated;
      const float ratio = segment_length > 0 ? remaining / segment_length : 0;
      const gfx::PointF new_point(
          previous.x() + ratio * (current.x() - previous.x()),
          previous.y() + ratio * (current.y() - previous.y()));
      resampled_path.push_back(new_point);
      segment_length -= remaining;
      accumulated = 0;
      // Advances from the newly interpolated point, not the original
      // segment's start, so one long segment resamples into several points.
      previous = new_point;
    }
    accumulated += segment_length;
  }

  while (resampled_path.size() < point_count) {
    resampled_path.push_back(gesture_path.back());
  }
  return resampled_path;
}

GesturePath NormalizeGesturePathScaleAndTranslation(
    const GesturePath& gesture_path) {
  float min_x = std::numeric_limits<float>::max();
  float min_y = std::numeric_limits<float>::max();
  float max_x = std::numeric_limits<float>::lowest();
  float max_y = std::numeric_limits<float>::lowest();
  for (const gfx::PointF& point : gesture_path) {
    min_x = std::min(min_x, point.x());
    min_y = std::min(min_y, point.y());
    max_x = std::max(max_x, point.x());
    max_y = std::max(max_y, point.y());
  }

  const float scale = std::max(max_x - min_x, max_y - min_y);

  GesturePath normalized_path;
  normalized_path.reserve(gesture_path.size());
  for (const gfx::PointF& point : gesture_path) {
    if (scale == 0) {
      normalized_path.emplace_back(0, 0);
    } else {
      normalized_path.emplace_back((point.x() - min_x) / scale,
                                   (point.y() - min_y) / scale);
    }
  }
  return normalized_path;
}

}  // namespace gestures
