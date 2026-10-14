// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_stroke_util.h"

#include <cmath>

#include "brave/components/gestures/core/common/directional/directional_gesture_stroke_classifier.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_vector_util.h"

namespace gestures {

std::optional<DirectionalGestureStrokeType> MaybeGetDirectionalGestureStroke(
    gfx::PointF origin,
    gfx::PointF location,
    double min_stroke_pixels) {
  const float dx = location.x() - origin.x();
  const float dy = location.y() - origin.y();
  if (std::hypot(dx, dy) < min_stroke_pixels) {
    // Movement too small to count as a deliberate stroke; not a direction
    // change, just noise or an incidental jitter.
    return std::nullopt;
  }
  if (dx == 0 && dy == 0) {
    // `atan2(0, 0)` resolves to 0 degrees rather than an undefined angle, which
    // would otherwise make a zero-length displacement arbitrarily resolve to
    // `kRight` instead of correctly reporting no direction. In practice
    // `min_stroke_pixels` is never configured to exactly 0, so the check above
    // already excludes this case; checked explicitly anyway for correctness.
    return std::nullopt;
  }
  return DirectionFromDisplacement(dx, dy);
}

std::string_view DirectionalGestureStrokeToKeyCode(
    DirectionalGestureStrokeType stroke_type) {
  switch (stroke_type) {
    case DirectionalGestureStrokeType::kUp:
      return "↑";
    case DirectionalGestureStrokeType::kDown:
      return "↓";
    case DirectionalGestureStrokeType::kLeft:
      return "←";
    case DirectionalGestureStrokeType::kRight:
      return "→";
    case DirectionalGestureStrokeType::kUpLeft:
      return "↖";
    case DirectionalGestureStrokeType::kUpRight:
      return "↗";
    case DirectionalGestureStrokeType::kDownLeft:
      return "↙";
    case DirectionalGestureStrokeType::kDownRight:
      return "↘";
  }
}

std::string DirectionalGestureStrokesToKey(
    const DirectionalGestureStrokes& strokes) {
  std::string key;
  for (DirectionalGestureStrokeType stroke_type : strokes) {
    key.append(DirectionalGestureStrokeToKeyCode(stroke_type));
  }
  return key;
}

DirectionalGestureStrokes ClassifyDirectionalGestureStrokes(
    const GesturePath& path,
    double min_stroke_pixels,
    double min_stroke_straightness,
    size_t max_strokes) {
  DirectionalGestureStrokes strokes;
  if (path.empty()) {
    return strokes;
  }

  DirectionalGestureStrokeClassifier classifier(path.front(), min_stroke_pixels,
                                                min_stroke_straightness);
  for (size_t i = 1; i < path.size() && strokes.size() < max_strokes; ++i) {
    if (const std::optional<DirectionalGestureStrokeType> committed =
            classifier.AddPoint(path.at(i), strokes)) {
      strokes.push_back(*committed);
    }
  }
  return strokes;
}

GesturePath DirectionalGestureStrokesToCanonicalPath(
    const DirectionalGestureStrokes& strokes) {
  GesturePath path = {gfx::PointF()};
  gfx::PointF current;
  for (DirectionalGestureStrokeType stroke_type : strokes) {
    current += UnitVectorForDirection(stroke_type);
    path.push_back(current);
  }
  return path;
}

}  // namespace gestures
