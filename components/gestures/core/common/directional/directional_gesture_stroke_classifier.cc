// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_stroke_classifier.h"

#include "brave/components/gestures/core/common/directional/directional_gesture_stroke_util.h"

namespace gestures {

DirectionalGestureStrokeClassifier::DirectionalGestureStrokeClassifier(
    gfx::PointF start_location,
    double min_stroke_pixels,
    double min_stroke_straightness)
    : min_stroke_pixels_(min_stroke_pixels),
      min_stroke_straightness_(min_stroke_straightness),
      // `stroke_origin_` is the start of the current committed stroke,
      // advancing on every commit; `pending_origin_` is the start of the
      // current pending candidate, advancing whenever the candidate
      // direction changes. Measuring a new candidate from a stale
      // `stroke_origin_` left over from several direction-changes ago (as
      // a one-anchor version of this would) reads a slowly-drifting
      // aggregate bearing on a curved path, which can disagree with the
      // real local tangent for the entire rest of the path and prevent any
      // further stroke from ever committing, e.g. a hand-drawn oval
      // collapsing to a single stroke.
      stroke_origin_(start_location),
      last_point_(start_location) {}

std::optional<DirectionalGestureStrokeType>
DirectionalGestureStrokeClassifier::AddPoint(
    gfx::PointF location,
    const DirectionalGestureStrokes& strokes) {
  const float segment_length = (location - last_point_).Length();
  last_point_ = location;
  // Actual distance traveled since `stroke_origin_`, always running
  // (unlike `arc_length_since_departure_`, which only accumulates while
  // diverged). Gates the "continuing the current stroke" branch below,
  // since without this, any point that happens to land back within
  // `min_stroke_pixels_` of `stroke_origin_` on the committed octant is
  // accepted as a genuine continuation, even if it only got there by
  // looping or wandering back around, a coincidental geometric
  // realignment, not real continued travel in that direction.
  arc_length_since_stroke_origin_ += segment_length;

  const std::optional<DirectionalGestureStrokeType> stroke_type =
      MaybeGetDirectionalGestureStroke(stroke_origin_, location,
                                       min_stroke_pixels_);
  if (!stroke_type) {
    AccumulateDepartureDistance(segment_length);
    return std::nullopt;
  }

  if (!strokes.empty() && stroke_type == strokes.back()) {
    ContinueCommittedDirection(location, segment_length);
    return std::nullopt;
  }

  return MaybeCommitNewDirection(location, *stroke_type, segment_length);
}

void DirectionalGestureStrokeClassifier::AccumulateDepartureDistance(
    float segment_length) {
  if (has_departure_) {
    arc_length_since_departure_ += segment_length;
  }
}

void DirectionalGestureStrokeClassifier::ContinueCommittedDirection(
    gfx::PointF location,
    float segment_length) {
  // A straight-line displacement back onto the committed direction can
  // still come from a path that looped or wandered to get there; require
  // it to also dominate the actual distance traveled since
  // `stroke_origin_` before accepting it as a genuine continuation,
  // rather than a coincidental geometric realignment.
  const float continuing_straightness =
      arc_length_since_stroke_origin_ > 0
          ? (location - stroke_origin_).Length() /
                arc_length_since_stroke_origin_
          : 1.0F;
  if (continuing_straightness >= min_stroke_straightness_) {
    pending_stroke_type_.reset();
    stroke_origin_ = location;
    arc_length_since_stroke_origin_ = 0;
    has_departure_ = false;
    arc_length_since_departure_ = 0;
    return;
  }
  // Coincidental realignment only; charge the wander distance against
  // straightness like any other non-qualifying point, but leave the
  // pending candidate and departure state untouched.
  AccumulateDepartureDistance(segment_length);
}

std::optional<DirectionalGestureStrokeType>
DirectionalGestureStrokeClassifier::MaybeCommitNewDirection(
    gfx::PointF location,
    DirectionalGestureStrokeType stroke_type,
    float segment_length) {
  // Diverged from the committed direction, or there's no stroke yet.
  // Anchor straightness at the first point of this divergence; later
  // flips of `pending_stroke_type_` below do not move this anchor.
  if (!has_departure_) {
    has_departure_ = true;
    departure_origin_ = location;
    arc_length_since_departure_ = 0;
  } else {
    arc_length_since_departure_ += segment_length;
  }

  // Don't commit it until it's sustained on its own for
  // `min_stroke_pixels_`.
  if (pending_stroke_type_ != stroke_type) {
    pending_stroke_type_ = stroke_type;
    pending_origin_ = location;
    return std::nullopt;
  }
  if (MaybeGetDirectionalGestureStroke(pending_origin_, location,
                                       min_stroke_pixels_) != stroke_type) {
    return std::nullopt;
  }

  // A straight-line displacement that clears `min_stroke_pixels_` can
  // still come from a path that wandered to get there (e.g. a loop
  // briefly drifting in one direction); requiring the displacement to
  // also dominate the actual distance traveled since `departure_origin_`
  // rejects those, while a real swipe, which travels nearly straight to
  // where it ends up, passes easily.
  const float straightness = arc_length_since_departure_ > 0
                                 ? (location - departure_origin_).Length() /
                                       arc_length_since_departure_
                                 : 1.0F;
  if (straightness < min_stroke_straightness_) {
    return std::nullopt;
  }

  pending_stroke_type_.reset();
  stroke_origin_ = location;
  arc_length_since_stroke_origin_ = 0;
  has_departure_ = false;
  arc_length_since_departure_ = 0;
  return stroke_type;
}

}  // namespace gestures
