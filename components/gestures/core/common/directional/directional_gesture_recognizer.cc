// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_recognizer.h"

#include "brave/components/gestures/core/common/directional/directional_gesture_stroke_util.h"
#include "brave/components/gestures/core/common/features.h"

namespace gestures {

DirectionalGestureRecognizer::DirectionalGestureRecognizer() = default;

DirectionalGestureRecognizer::~DirectionalGestureRecognizer() = default;

void DirectionalGestureRecognizer::Reset(gfx::PointF start_location,
                                         base::TimeTicks now,
                                         double min_stroke_pixels,
                                         double min_stroke_straightness) {
  strokes_.clear();
  stroke_origin_ = start_location;
  last_point_ = start_location;
  arc_length_since_stroke_origin_ = 0;
  has_departure_ = false;
  arc_length_since_departure_ = 0;
  last_stroke_committed_at_ = now;
  min_stroke_pixels_ = min_stroke_pixels;
  min_stroke_straightness_ = min_stroke_straightness;
  pending_stroke_type_.reset();
  pending_origin_ = gfx::PointF();
}

void DirectionalGestureRecognizer::OnMouseMoved(gfx::PointF location,
                                                base::TimeTicks now) {
  if (strokes_.size() >= features::kMaxDirectionalGestureStrokes.Get()) {
    return;
  }

  const float segment_length = (location - last_point_).Length();
  last_point_ = location;
  arc_length_since_stroke_origin_ += segment_length;

  std::optional<DirectionalGestureStrokeType> stroke_type =
      MaybeGetDirectionalGestureStroke(stroke_origin_, location,
                                       min_stroke_pixels_);
  if (!stroke_type) {
    if (has_departure_) {
      arc_length_since_departure_ += segment_length;
    }
    return;
  }
  // Any movement that clears `min_stroke_pixels_` counts against the idle
  // timeout, even if it only proposes a pending candidate direction rather
  // than committing a stroke outright, since the timeout exists to catch a
  // genuinely motionless hold, not to require a stroke commit specifically.
  last_stroke_committed_at_ = now;

  if (!strokes_.empty() && stroke_type == strokes_.back()) {
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
      // Update the origin even when the direction is unchanged. Otherwise,
      // continuing to move the same way would still be measured from the
      // start of that stroke.
      stroke_origin_ = location;
      arc_length_since_stroke_origin_ = 0;
      has_departure_ = false;
      arc_length_since_departure_ = 0;
      return;
    }
    // Coincidental realignment only; charge the wander distance against
    // straightness like any other non-qualifying point, but leave the
    // pending candidate and departure state untouched.
    if (has_departure_) {
      arc_length_since_departure_ += segment_length;
    }
    return;
  }

  // Diverged from the committed direction, or there's no stroke yet. Anchor
  // straightness at the first point of this divergence; later flips of
  // `pending_stroke_type_` below do not move this anchor.
  if (!has_departure_) {
    has_departure_ = true;
    departure_origin_ = location;
    arc_length_since_departure_ = 0;
  } else {
    arc_length_since_departure_ += segment_length;
  }

  // `stroke_type` differs from the stroke in progress, or there's no
  // stroke yet. Don't commit it until it's sustained on its own for
  // `min_stroke_pixels_`, so the diagonal octant briefly crossed while
  // rounding a 90 degree turn, or while tracing a continuously curving
  // shape like a circle, isn't mistaken for a deliberate stroke. Applying
  // this to the first stroke too (not just subsequent ones) is what stops a
  // hand-drawn oval from collapsing to a single false stroke, since
  // previously the very first qualifying direction committed
  // unconditionally, with no sustain check at all.
  if (pending_stroke_type_ != stroke_type) {
    pending_stroke_type_ = stroke_type;
    pending_origin_ = location;
    return;
  }
  if (MaybeGetDirectionalGestureStroke(pending_origin_, location,
                                       min_stroke_pixels_) != stroke_type) {
    return;
  }

  // A straight-line displacement that clears `min_stroke_pixels_` can still
  // come from a path that wandered to get there (e.g. a loop briefly
  // drifting in one direction); requiring the displacement to also dominate
  // the actual distance traveled since `departure_origin_` rejects those,
  // while a real swipe, which travels nearly straight to where it ends up,
  // passes easily.
  const float straightness = arc_length_since_departure_ > 0
                                 ? (location - departure_origin_).Length() /
                                       arc_length_since_departure_
                                 : 1.0F;
  if (straightness < min_stroke_straightness_) {
    return;
  }

  strokes_.push_back(*stroke_type);
  pending_stroke_type_.reset();
  stroke_origin_ = location;
  arc_length_since_stroke_origin_ = 0;
  has_departure_ = false;
  arc_length_since_departure_ = 0;
  last_stroke_committed_at_ = now;
}

bool DirectionalGestureRecognizer::TimedOut(base::TimeTicks now) const {
  return now - last_stroke_committed_at_ >=
         features::kGestureSessionIdleHoldTimeout.Get();
}

}  // namespace gestures
