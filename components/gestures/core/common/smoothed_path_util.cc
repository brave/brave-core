// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/smoothed_path_util.h"

#include <algorithm>

#include "third_party/skia/include/core/SkPathBuilder.h"

namespace gestures {

namespace {

// Scales a Catmull-Rom tangent into a Bezier control-point offset.
constexpr float kCatmullRomTangentScale = 6;

}  // namespace

SkPath SmoothedPath(const std::vector<gfx::PointF>& points) {
  SkPathBuilder path_builder;
  path_builder.moveTo(points.front().x(), points.front().y());
  if (points.size() == 2) {
    path_builder.lineTo(points.back().x(), points.back().y());
    return path_builder.detach();
  }
  const size_t last = points.size() - 1;
  for (size_t i = 0; i < last; ++i) {
    const gfx::PointF& previous_point = points.at(i == 0 ? 0 : i - 1);
    const gfx::PointF& segment_start = points.at(i);
    const gfx::PointF& segment_end = points.at(i + 1);
    const gfx::PointF& next_point = points.at(std::min(i + 2, last));
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
