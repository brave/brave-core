// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_stroke_classifier.h"

#include "brave/components/gestures/core/common/directional/directional_gesture_vector_util.h"

namespace gestures {

DirectionalGestureStrokeClassifier::DirectionalGestureStrokeClassifier(
    gfx::PointF start_location,
    double min_stroke_pixels,
    double min_stroke_straightness)
    : min_stroke_pixels_(min_stroke_pixels),
      min_stroke_straightness_(min_stroke_straightness),
      // Two separate anchors, not one, so a new candidate is measured from
      // its own recent start, not a stale aggregate bearing on a curve.
      stroke_origin_(start_location),
      last_point_(start_location) {}

DirectionalGestureAddPointInfo DirectionalGestureStrokeClassifier::AddPoint(
    gfx::PointF location,
    const DirectionalGestureStrokes& strokes) {
  const float segment_length = (location - last_point_).Length();
  last_point_ = location;
  // Always running, unlike `arc_length_since_divergence_`, which only
  // accumulates while diverged; gates against a coincidental realignment.
  arc_length_since_stroke_origin_ += segment_length;

  std::optional<DirectionalGestureStrokeType> stroke_type =
      MaybeGetDirectionalGestureStroke(stroke_origin_, location,
                                       min_stroke_pixels_);
  if (!stroke_type) {
    AccumulateDivergenceDistance(segment_length);
    return {.had_qualifying_movement = false};
  }

  if (!strokes.empty() && stroke_type == strokes.back()) {
    ContinueCommittedDirection(location, segment_length);
    return {.had_qualifying_movement = true};
  }

  return {.had_qualifying_movement = true,
          .committed_stroke_type =
              MaybeCommitNewDirection(location, *stroke_type, segment_length)};
}

void DirectionalGestureStrokeClassifier::AccumulateDivergenceDistance(
    float segment_length) {
  if (is_diverged_) {
    arc_length_since_divergence_ += segment_length;
  }
}

void DirectionalGestureStrokeClassifier::ContinueCommittedDirection(
    gfx::PointF location,
    float segment_length) {
  // A straight-line displacement back onto the committed direction can
  // come from a wandering gesture_path; require it to dominate distance.
  const float continuing_straightness =
      arc_length_since_stroke_origin_ > 0
          ? (location - stroke_origin_).Length() /
                arc_length_since_stroke_origin_
          : 1.0F;
  if (continuing_straightness >= min_stroke_straightness_) {
    pending_divergent_stroke_type_.reset();
    stroke_origin_ = location;
    arc_length_since_stroke_origin_ = 0;
    is_diverged_ = false;
    arc_length_since_divergence_ = 0;
    return;
  }
  // Coincidental realignment only; charge the wander distance against
  // straightness, leaving the pending candidate and divergence untouched.
  AccumulateDivergenceDistance(segment_length);
}

std::optional<DirectionalGestureStrokeType>
DirectionalGestureStrokeClassifier::MaybeCommitNewDirection(
    gfx::PointF location,
    DirectionalGestureStrokeType stroke_type,
    float segment_length) {
  // Anchors straightness at the first point of this divergence; later
  // flips of `pending_divergent_stroke_type_` below do not move this anchor.
  if (!is_diverged_) {
    is_diverged_ = true;
    divergence_origin_ = location;
    arc_length_since_divergence_ = 0;
  } else {
    arc_length_since_divergence_ += segment_length;
  }

  // Don't commit it until it's sustained on its own for
  // `min_stroke_pixels_`.
  if (pending_divergent_stroke_type_ != stroke_type) {
    pending_divergent_stroke_type_ = stroke_type;
    pending_origin_ = location;
    return std::nullopt;
  }
  if (MaybeGetDirectionalGestureStroke(pending_origin_, location,
                                       min_stroke_pixels_) != stroke_type) {
    return std::nullopt;
  }

  // A displacement that clears `min_stroke_pixels_` can still come from a
  // wandering gesture_path; requiring it to dominate distance rejects those.
  const float straightness = arc_length_since_divergence_ > 0
                                 ? (location - divergence_origin_).Length() /
                                       arc_length_since_divergence_
                                 : 1.0F;
  if (straightness < min_stroke_straightness_) {
    return std::nullopt;
  }

  pending_divergent_stroke_type_.reset();
  stroke_origin_ = location;
  arc_length_since_stroke_origin_ = 0;
  is_diverged_ = false;
  arc_length_since_divergence_ = 0;
  return stroke_type;
}

}  // namespace gestures
