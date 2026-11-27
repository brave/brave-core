// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_path_smoothing_util.h"

#include <algorithm>

#include "third_party/skia/include/core/SkPathBuilder.h"

namespace gestures {

namespace {
constexpr float kCatmullRomTangentScale = 6.0F;
}  // namespace

SkPath SmoothedPath(const GesturePath& gesture_path) {
  SkPathBuilder path_builder;
  path_builder.moveTo(gesture_path.front().x(), gesture_path.front().y());
  if (gesture_path.size() == 2) {
    path_builder.lineTo(gesture_path.back().x(), gesture_path.back().y());
    return path_builder.detach();
  }
  const size_t last = gesture_path.size() - 1;
  for (size_t i = 0; i < last; ++i) {
    const gfx::PointF& previous_point = gesture_path.at(i == 0 ? 0 : i - 1);
    const gfx::PointF& segment_start = gesture_path.at(i);
    const gfx::PointF& segment_end = gesture_path.at(i + 1);
    const gfx::PointF& next_point = gesture_path.at(std::min(i + 2, last));
    const gfx::PointF control_point_1(
        segment_start.x() +
            (segment_end.x() - previous_point.x()) / kCatmullRomTangentScale,
        segment_start.y() +
            (segment_end.y() - previous_point.y()) / kCatmullRomTangentScale);
    const gfx::PointF control_point_2(
        segment_end.x() -
            (next_point.x() - segment_start.x()) / kCatmullRomTangentScale,
        segment_end.y() -
            (next_point.y() - segment_start.y()) / kCatmullRomTangentScale);
    path_builder.cubicTo(control_point_1.x(), control_point_1.y(),
                         control_point_2.x(), control_point_2.y(),
                         segment_end.x(), segment_end.y());
  }
  return path_builder.detach();
}

}  // namespace gestures
