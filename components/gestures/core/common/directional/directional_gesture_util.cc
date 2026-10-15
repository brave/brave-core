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

std::vector<gfx::PointF> ResampleAtInterval(
    const std::vector<gfx::PointF>& path,
    float interval_pixels) {
  if (path.empty()) {
    return {};
  }
  std::vector<gfx::PointF> resampled = {path.front()};
  if (path.size() < 2 || interval_pixels <= 0) {
    return resampled;
  }

  // Carries over leftover distance from one original segment to the next, so
  // intervals stay consistent across the original points' own boundaries,
  // not just within each one.
  float accumulated = 0;
  gfx::PointF last_point = path.front();
  for (size_t i = 1; i < path.size(); ++i) {
    const gfx::PointF& current = path.at(i);
    float segment_length = (current - last_point).Length();

    while (accumulated + segment_length >= interval_pixels) {
      const float remaining = interval_pixels - accumulated;
      const float ratio = segment_length > 0 ? remaining / segment_length : 0;
      const gfx::PointF new_point(
          last_point.x() + ratio * (current.x() - last_point.x()),
          last_point.y() + ratio * (current.y() - last_point.y()));
      resampled.push_back(new_point);
      segment_length -= remaining;
      // Advances from the newly interpolated point, not the original
      // segment's start, so a single long original segment resamples into
      // several evenly-spaced points rather than repeatedly interpolating
      // from the same fixed start.
      last_point = new_point;
      accumulated = 0;
    }
    accumulated += segment_length;
    last_point = current;
  }
  if (resampled.back() != path.back()) {
    resampled.push_back(path.back());
  }
  return resampled;
}

std::vector<DirectionalGestureDirectionType> ClassifyDirectionalStrokes(
    const std::vector<gfx::PointF>& path,
    double min_stroke_pixels,
    size_t max_strokes) {
  std::vector<DirectionalGestureDirectionType> strokes;
  if (path.empty()) {
    return strokes;
  }

  // `origin` is always either the start of the current committed stroke or
  // the start of the current pending candidate -- it advances every time the
  // candidate direction changes, not just on commit. Measuring from a stale
  // anchor left over from several direction-changes ago (as a two-anchor
  // version of this would) reads a slowly-drifting aggregate bearing on a
  // curved path, which can disagree with the real local tangent for the
  // entire rest of the path and prevent any further stroke from ever
  // committing -- e.g. a hand-drawn oval collapsing to a single stroke.
  gfx::PointF origin = path.front();
  std::optional<DirectionalGestureDirectionType> pending_direction_type;

  for (size_t i = 1; i < path.size() && strokes.size() < max_strokes; ++i) {
    const gfx::PointF& location = path.at(i);
    const std::optional<DirectionalGestureDirectionType> direction_type =
        MaybeMovementDirection(origin, location, min_stroke_pixels);
    if (!direction_type) {
      continue;
    }

    if (!strokes.empty() && direction_type == strokes.back()) {
      // Continuing the current stroke; any pending candidate direction was a
      // false alarm.
      pending_direction_type.reset();
      origin = location;
      continue;
    }

    // `direction_type` differs from the stroke in progress, or there's no
    // stroke yet. Don't commit it until it's sustained on its own for
    // `min_stroke_pixels`.
    if (pending_direction_type != direction_type) {
      pending_direction_type = direction_type;
      origin = location;
      continue;
    }

    strokes.push_back(*direction_type);
    pending_direction_type.reset();
    origin = location;
  }

  return strokes;
}

}  // namespace gestures
