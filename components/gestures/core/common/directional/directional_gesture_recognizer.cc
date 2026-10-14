// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_recognizer.h"

#include "brave/components/gestures/core/common/directional/directional_gesture_util.h"
#include "brave/components/gestures/core/common/features.h"

namespace gestures {

DirectionalGestureRecognizer::DirectionalGestureRecognizer() = default;

DirectionalGestureRecognizer::~DirectionalGestureRecognizer() = default;

void DirectionalGestureRecognizer::Reset(gfx::PointF start_location,
                                         base::TimeTicks now,
                                         double min_stroke_pixels) {
  strokes_.clear();
  stroke_origin_ = start_location;
  last_stroke_committed_at_ = now;
  min_stroke_pixels_ = min_stroke_pixels;
  pending_direction_type_.reset();
  pending_origin_ = gfx::PointF();
}

void DirectionalGestureRecognizer::OnMouseMoved(gfx::PointF location,
                                                base::TimeTicks now) {
  if (strokes_.size() >= features::kMaxDirectionalStrokes.Get()) {
    return;
  }

  const std::optional<DirectionalGestureDirectionType> direction_type =
      MaybeMovementDirection(stroke_origin_, location, min_stroke_pixels_);
  if (!direction_type) {
    return;
  }

  if (strokes_.empty() || direction_type == strokes_.back()) {
    // Continuing the current stroke (or starting the first one); any
    // pending candidate direction was a false alarm.
    pending_direction_type_.reset();
    if (strokes_.empty()) {
      strokes_.push_back(*direction_type);
    }
    // Update the origin and timestamp even when the direction is unchanged.
    // Otherwise, continuing to move the same way would still be measured
    // from the start of that stroke, and would eventually look idle and
    // time out.
    stroke_origin_ = location;
    last_stroke_committed_at_ = now;
    return;
  }

  // `direction_type` differs from the stroke in progress. Don't commit it
  // until it's sustained on its own for `min_stroke_pixels_`, so the diagonal
  // octant briefly crossed while rounding a 90-degree turn isn't mistaken
  // for a deliberate stroke.
  if (pending_direction_type_ != direction_type) {
    pending_direction_type_ = direction_type;
    pending_origin_ = location;
    return;
  }
  if (MaybeMovementDirection(pending_origin_, location, min_stroke_pixels_) !=
      direction_type) {
    return;
  }

  strokes_.push_back(*direction_type);
  pending_direction_type_.reset();
  stroke_origin_ = location;
  last_stroke_committed_at_ = now;
}

bool DirectionalGestureRecognizer::TimedOut(base::TimeTicks now) const {
  return now - last_stroke_committed_at_ >= features::kIdleHoldTimeout.Get();
}

}  // namespace gestures
