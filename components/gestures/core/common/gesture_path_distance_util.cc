// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_path_distance_util.h"

#include <optional>

#include "brave/components/gestures/core/common/features.h"

namespace gestures {

float GesturePathLength(const GesturePath& gesture_path) {
  float length = 0.0F;
  std::optional<gfx::PointF> previous_point;
  for (const gfx::PointF& point : gesture_path) {
    if (previous_point) {
      length += (point - *previous_point).Length();
    }
    previous_point = point;
  }
  return length;
}

bool IsFarEnoughFromLastDrawnPoint(gfx::PointF last_drawn_point,
                                   gfx::PointF candidate,
                                   double min_point_distance_pixels) {
  return (candidate - last_drawn_point).Length() >= min_point_distance_pixels;
}

bool IsClosedGesturePath(const GesturePath& gesture_path) {
  const float length = GesturePathLength(gesture_path);
  if (length == 0) {
    return false;
  }
  const float end_to_start_distance =
      (gesture_path.back() - gesture_path.front()).Length();
  return end_to_start_distance / length <
         features::kCustomGestureClosedPathGapRatio.Get();
}

}  // namespace gestures
