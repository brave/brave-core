// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_util.h"

#include <array>
#include <cmath>
#include <numbers>

#include "base/numerics/safe_conversions.h"

namespace gestures {

namespace {

// The 8 directions in clockwise angular order starting from `kRight` (screen
// angle 0), matching the sector math in `DirectionFromDisplacement` below. This
// order assumes `dy` grows downward, as in screen/widget coordinates.
// `atan2(dy, dx)` then sweeps clockwise on screen (e.g. straight down is +90
// degrees), not counterclockwise as it would in standard math coordinates where
// `y` grows upward.
constexpr std::array<DirectionalGestureDirectionType, 8> kDirectionsByAngle = {
    DirectionalGestureDirectionType::kRight,
    DirectionalGestureDirectionType::kDownRight,
    DirectionalGestureDirectionType::kDown,
    DirectionalGestureDirectionType::kDownLeft,
    DirectionalGestureDirectionType::kLeft,
    DirectionalGestureDirectionType::kUpLeft,
    DirectionalGestureDirectionType::kUp,
    DirectionalGestureDirectionType::kUpRight,
};

DirectionalGestureDirectionType DirectionFromDisplacement(float dx, float dy) {
  float degrees = std::atan2(dy, dx) * 180.0F / std::numbers::pi_v<float>;
  if (degrees < 0) {
    degrees += 360.0F;
  }
  const auto nearest_direction = base::ClampRound<size_t>(degrees / 45.0F);
  const size_t octant = nearest_direction % kDirectionsByAngle.size();
  return kDirectionsByAngle.at(octant);
}

}  // namespace

std::optional<DirectionalGestureDirectionType> MaybeMovementDirection(
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

std::string_view DirectionToGestureKeyCode(
    DirectionalGestureDirectionType direction_type) {
  switch (direction_type) {
    case DirectionalGestureDirectionType::kUp:
      return "↑";
    case DirectionalGestureDirectionType::kDown:
      return "↓";
    case DirectionalGestureDirectionType::kLeft:
      return "←";
    case DirectionalGestureDirectionType::kRight:
      return "→";
    case DirectionalGestureDirectionType::kUpLeft:
      return "↖";
    case DirectionalGestureDirectionType::kUpRight:
      return "↗";
    case DirectionalGestureDirectionType::kDownLeft:
      return "↙";
    case DirectionalGestureDirectionType::kDownRight:
      return "↘";
  }
}

std::string StrokesToGestureKey(
    const std::vector<DirectionalGestureDirectionType>& strokes) {
  std::string key;
  for (DirectionalGestureDirectionType direction_type : strokes) {
    key.append(DirectionToGestureKeyCode(direction_type));
  }
  return key;
}

}  // namespace gestures
