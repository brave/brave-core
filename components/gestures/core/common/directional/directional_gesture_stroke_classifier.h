// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_STROKE_CLASSIFIER_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_STROKE_CLASSIFIER_H_

#include <optional>

#include "brave/components/gestures/core/common/directional/directional_gesture_add_point_info.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_types.h"
#include "ui/gfx/geometry/point_f.h"

namespace gestures {

// Carries per-point state between `AddPoint` calls, so that loop body stays
// a flat dispatch instead of nesting the state transitions inline.
class DirectionalGestureStrokeClassifier final {
 public:
  DirectionalGestureStrokeClassifier(gfx::PointF start_location,
                                     double min_stroke_pixels,
                                     double min_stroke_straightness);

  // `location` is the next point in the gesture path; `strokes` is every
  // stroke committed so far.
  DirectionalGestureAddPointInfo AddPoint(
      gfx::PointF location,
      const DirectionalGestureStrokes& strokes);

 private:
  void AccumulateDivergenceDistance(float segment_length);

  void ContinueCommittedDirection(gfx::PointF location, float segment_length);

  std::optional<DirectionalGestureStrokeType> MaybeCommitNewDirection(
      gfx::PointF location,
      DirectionalGestureStrokeType stroke_type,
      float segment_length);

  const double min_stroke_pixels_;
  const double min_stroke_straightness_;
  gfx::PointF stroke_origin_;
  gfx::PointF last_point_;
  float arc_length_since_stroke_origin_ = 0.0F;
  std::optional<DirectionalGestureStrokeType> pending_divergent_stroke_type_;
  gfx::PointF pending_origin_;
  bool is_diverged_ = false;
  // Set once on first divergence and survives later candidate flips;
  // `stroke_origin_` can't double as this anchor since it lags the corner.
  gfx::PointF divergence_origin_;
  float arc_length_since_divergence_ = 0.0F;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_STROKE_CLASSIFIER_H_
