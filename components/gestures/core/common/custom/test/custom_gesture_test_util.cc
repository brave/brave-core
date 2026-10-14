// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/test/custom_gesture_test_util.h"

#include <utility>

namespace gestures::test {

CustomGestureInfo MakeCustomGesture(const std::string& id,
                                    std::vector<gfx::PointF> stroke_points,
                                    std::vector<GestureActionInfo> actions) {
  CustomGestureInfo custom_gesture;
  custom_gesture.id = id;
  custom_gesture.stroke_points = std::move(stroke_points);
  custom_gesture.actions = std::move(actions);
  return custom_gesture;
}

std::vector<gfx::PointF> MakeVStrokePoints() {
  return {gfx::PointF(0.0, 50.0), gfx::PointF(20.0, 70.0),
          gfx::PointF(60.0, 0.0)};
}

std::vector<gfx::PointF> MakeTriangleStrokePoints(size_t start_vertex) {
  const std::vector<gfx::PointF> vertices = {
      gfx::PointF(0.0, 0.0), gfx::PointF(100.0, 0.0), gfx::PointF(50.0, 100.0)};
  std::vector<gfx::PointF> stroke_points;
  for (size_t i = 0; i <= vertices.size(); ++i) {
    stroke_points.push_back(vertices.at((start_vertex + i) % vertices.size()));
  }
  return stroke_points;
}

}  // namespace gestures::test
