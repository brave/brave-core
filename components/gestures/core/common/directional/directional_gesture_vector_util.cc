// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_vector_util.h"

#include <array>
#include <cmath>
#include <numbers>

#include "base/numerics/safe_conversions.h"

namespace gestures {

namespace {

// The 8 directions in clockwise angular order starting from `kRight` (screen
// angle 0), matching the sector math in `DirectionFromDisplacement`. This
// order assumes `dy` grows downward, as in screen/widget coordinates.
// `atan2(dy, dx)` then sweeps clockwise on screen (e.g. straight down is +90
// degrees), not counterclockwise as it would in standard math coordinates where
// `y` grows upward.
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
