// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_vector_util.h"

#include <array>
#include <cmath>
#include <numbers>
#include <optional>

#include "base/numerics/safe_conversions.h"

namespace gestures {

namespace {

// Clockwise from `kRight` (angle 0), since screen `dy` grows downward, so
// `atan2(dy, dx)` sweeps clockwise here rather than counterclockwise.
constexpr std::array<DirectionalGestureStrokeType, 8> kDirectionsByAngle = {
    DirectionalGestureStrokeType::kRight,
    DirectionalGestureStrokeType::kDownRight,
    DirectionalGestureStrokeType::kDown,
    DirectionalGestureStrokeType::kDownLeft,
    DirectionalGestureStrokeType::kLeft,
    DirectionalGestureStrokeType::kUpLeft,
    DirectionalGestureStrokeType::kUp,
    DirectionalGestureStrokeType::kUpRight,
};

}  // namespace

DirectionalGestureStrokeType DirectionFromDisplacement(float dx, float dy) {
  float degrees = std::atan2(dy, dx) * 180.0F / std::numbers::pi_v<float>;
  if (degrees < 0) {
    degrees += 360.0F;
  }
  const auto nearest_direction = base::ClampRound<size_t>(degrees / 45.0F);
  const size_t octant = nearest_direction % kDirectionsByAngle.size();
  return kDirectionsByAngle.at(octant);
}

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
    // `atan2(0, 0)` resolves to 0 degrees, which would otherwise make a
    // zero-length displacement arbitrarily resolve to `kRight`.
    return std::nullopt;
  }
  return DirectionFromDisplacement(dx, dy);
}

gfx::Vector2dF UnitVectorForDirection(
    DirectionalGestureStrokeType stroke_type) {
  constexpr float kDiagonal = std::numbers::sqrt2_v<float> / 2.0F;
  switch (stroke_type) {
    case DirectionalGestureStrokeType::kRight:
      return {1, 0};
    case DirectionalGestureStrokeType::kDownRight:
      return {kDiagonal, kDiagonal};
    case DirectionalGestureStrokeType::kDown:
      return {0, 1};
    case DirectionalGestureStrokeType::kDownLeft:
      return {-kDiagonal, kDiagonal};
    case DirectionalGestureStrokeType::kLeft:
      return {-1, 0};
    case DirectionalGestureStrokeType::kUpLeft:
      return {-kDiagonal, -kDiagonal};
    case DirectionalGestureStrokeType::kUp:
      return {0, -1};
    case DirectionalGestureStrokeType::kUpRight:
      return {kDiagonal, -kDiagonal};
  }
}

}  // namespace gestures
