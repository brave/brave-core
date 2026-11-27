// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_path_normalize_util.h"

#include <algorithm>
#include <limits>

namespace gestures {

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

  GesturePath normalized_gesture_path;
  normalized_gesture_path.reserve(gesture_path.size());
  for (const gfx::PointF& point : gesture_path) {
    if (scale == 0) {
      normalized_gesture_path.emplace_back(0, 0);
    } else {
      normalized_gesture_path.emplace_back((point.x() - min_x) / scale,
                                           (point.y() - min_y) / scale);
    }
  }
  return normalized_gesture_path;
}

}  // namespace gestures
