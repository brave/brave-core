// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_STROKE_CLASSIFIER_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_STROKE_CLASSIFIER_H_

#include <optional>

#include "brave/components/gestures/core/common/directional/directional_gesture_types.h"
#include "ui/gfx/geometry/point_f.h"

namespace gestures {

// Carries the per-point state `ClassifyDirectionalGestureStrokes` needs between
// calls to `AddPoint`, so that function's loop body stays a flat dispatch
// to one of 3 outcomes instead of nesting all the state transitions inline.
class DirectionalGestureStrokeClassifier final {
 public:
  DirectionalGestureStrokeClassifier(gfx::PointF start_location,
                                     double min_stroke_pixels,
                                     double min_stroke_straightness);

  // Returns the direction a stroke just committed on, given `location` is
  // the next point in the path and `strokes` is every stroke committed so
  // far.
  std::optional<DirectionalGestureStrokeType> AddPoint(
      gfx::PointF location,
      const DirectionalGestureStrokes& strokes);

 private:
  // Straightness-only bookkeeping, independent of `stroke_origin_`/
  // `pending_origin_`. `departure_origin_` is set once, the first time
  // movement diverges from the committed direction (or there's no stroke
  // yet), and is only cleared by a return to that direction or by a commit,
  // never by a later candidate flip of `pending_stroke_type_`, so
  // wandering through several directions before settling is still charged
  // against straightness instead of being forgotten. Measuring from
  // `stroke_origin_` instead would double as the straightness anchor too,
  // but `stroke_origin_` only advances on points that independently
  // reclear `min_stroke_pixels_` on their own, so it can lag the real
  // corner by almost a full `min_stroke_pixels_` and contaminate the next
  // stroke's straightness with leftover travel that geometrically belongs
  // to the stroke before it.
  void AccumulateDepartureDistance(float segment_length);

  void ContinueCommittedDirection(gfx::PointF location, float segment_length);

  std::optional<DirectionalGestureStrokeType> MaybeCommitNewDirection(
      gfx::PointF location,
      DirectionalGestureStrokeType stroke_type,
      float segment_length);

  const double min_stroke_pixels_;
  const double min_stroke_straightness_;
  gfx::PointF stroke_origin_;
  gfx::PointF last_point_;
  float arc_length_since_stroke_origin_ = 0;
  std::optional<DirectionalGestureStrokeType> pending_stroke_type_;
  gfx::PointF pending_origin_;
  bool has_departure_ = false;
  gfx::PointF departure_origin_;
  float arc_length_since_departure_ = 0;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_STROKE_CLASSIFIER_H_
