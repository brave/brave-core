// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_recognizer.h"

#include "brave/components/gestures/core/common/directional/directional_gesture_add_point_info.h"
#include "brave/components/gestures/core/common/features.h"

namespace gestures {

DirectionalGestureRecognizer::DirectionalGestureRecognizer() = default;

DirectionalGestureRecognizer::~DirectionalGestureRecognizer() = default;

void DirectionalGestureRecognizer::Reset(gfx::PointF start_location,
                                         base::TimeTicks now,
                                         double min_stroke_pixels,
                                         double min_stroke_straightness) {
  strokes_.clear();
  stroke_classifier_.emplace(start_location, min_stroke_pixels,
                             min_stroke_straightness);
  last_stroke_committed_at_ = now;
}

void DirectionalGestureRecognizer::OnMouseMoved(gfx::PointF location,
                                                base::TimeTicks now) {
  if (strokes_.size() >= features::kMaxDirectionalGestureStrokes.Get()) {
    return;
  }

  const DirectionalGestureAddPointInfo add_point =
      stroke_classifier_->AddPoint(location, strokes_);
  // Counts against the idle timeout even without a commit, since the
  // timeout exists to catch a motionless hold, not to require a commit.
  if (add_point.had_qualifying_movement) {
    last_stroke_committed_at_ = now;
  }
  if (add_point.committed_stroke_type) {
    strokes_.push_back(*add_point.committed_stroke_type);
  }
}

bool DirectionalGestureRecognizer::TimedOut(base::TimeTicks now) const {
  return now - last_stroke_committed_at_ >=
         features::kGestureSessionIdleHoldTimeout.Get();
}

}  // namespace gestures
