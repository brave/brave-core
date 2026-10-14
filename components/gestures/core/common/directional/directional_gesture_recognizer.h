// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_RECOGNIZER_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_RECOGNIZER_H_

#include <optional>

#include "base/time/time.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_types.h"
#include "brave/components/gestures/core/common/features.h"
#include "ui/gfx/geometry/point_f.h"

namespace gestures {

// Converts a stream of locations sampled while a gesture is being recorded
// into a sequence of directional strokes. Has no dependency on real input
// events or platform code.
class DirectionalGestureRecognizer final {
 public:
  DirectionalGestureRecognizer();

  DirectionalGestureRecognizer(const DirectionalGestureRecognizer&) = delete;
  DirectionalGestureRecognizer& operator=(const DirectionalGestureRecognizer&) =
      delete;

  ~DirectionalGestureRecognizer();

  // Starts a new drawing, clearing any previously accumulated strokes.
  // `min_stroke_pixels` defaults to the feature param but is passed in
  // explicitly so callers can honor the user's sensitivity setting instead.
  void Reset(gfx::PointF start_location,
             base::TimeTicks now,
             double min_stroke_pixels =
                 features::kDirectionalGestureMinStrokePixels.Get(),
             double min_stroke_straightness =
                 features::kDirectionalGestureMinStrokeStraightness.Get());

  // Feeds a new cursor position while the mouse button, trackpad button, or
  // touchscreen contact is held. May append a new stroke, collapse into the
  // current one, or be ignored if the sequence is already at `max_strokes`.
  void OnMouseMoved(gfx::PointF location, base::TimeTicks now);

  // True if `now` is at least `kGestureSessionIdleHoldTimeout` past the last
  // qualifying stroke, or since `Reset` if none has been recorded yet.
  bool TimedOut(base::TimeTicks now) const;

  const DirectionalGestureStrokes& strokes() const { return strokes_; }

 private:
  // Set from `Reset`'s `min_stroke_pixels` argument for the drawing in
  // progress.
  double min_stroke_pixels_ =
      features::kDirectionalGestureMinStrokePixels.Get();

  // Set from `Reset`'s `min_stroke_straightness` argument for the drawing in
  // progress. See `kDirectionalGestureMinStrokeStraightness`.
  double min_stroke_straightness_ =
      features::kDirectionalGestureMinStrokeStraightness.Get();

  // State of the drawing in progress.
  DirectionalGestureStrokes strokes_;

  // Origin of the stroke currently being measured. This is the last committed
  // stroke's endpoint, or the drawing start location if no stroke has
  // committed yet.
  gfx::PointF stroke_origin_;

  // The most recent location fed to `OnMouseMoved`, or the drawing start
  // location if none yet; used to accumulate `arc_length_since_departure_`
  // one segment at a time.
  gfx::PointF last_point_;

  // Actual distance traveled since `stroke_origin_`, always running (unlike
  // `arc_length_since_departure_` below, which only accumulates while
  // diverged). Gates the "continuing the current stroke" branch, since
  // without this, any point that happens to land back within
  // `min_stroke_pixels_` of `stroke_origin_` on the committed octant is
  // accepted as a genuine continuation, even if it only got there by
  // looping or wandering back around, a coincidental geometric realignment,
  // not real continued travel in that direction.
  float arc_length_since_stroke_origin_ = 0;

  // Straightness-only state, independent of `stroke_origin_`/
  // `pending_origin_`. `departure_origin_` is set once, the first
  // time movement diverges from the committed direction (or there's no
  // stroke yet), and is only cleared by a return to that direction or by a
  // commit, never by a later candidate flip of `pending_direction_type_`,
  // so wandering through several directions before settling is still
  // charged against straightness instead of being forgotten. Measuring
  // from `stroke_origin_` instead would double it as the straightness
  // anchor too, but `stroke_origin_` only advances on points that
  // independently reclear `min_stroke_pixels_` on their own, so it can lag
  // the real corner by almost a full `min_stroke_pixels_` and contaminate
  // the next stroke's straightness with leftover travel that geometrically
  // belongs to the stroke before it.
  bool has_departure_ = false;
  gfx::PointF departure_origin_;

  // Actual distance traveled since `departure_origin_`, as opposed to
  // `(location - departure_origin_).Length()`'s straight-line distance
  // between just the two endpoints. Used to compute a candidate stroke's
  // straightness.
  float arc_length_since_departure_ = 0;

  // When a qualifying stroke was last committed, or the drawing start time if
  // none has been.
  base::TimeTicks last_stroke_committed_at_;

  // A direction change not yet committed as a stroke. Drawing a 90-degree
  // turn (e.g. up then right) in one smooth motion briefly moves through the
  // diagonal octant between the two; without this, that single instant would
  // get committed as its own spurious stroke. A candidate direction is only
  // committed once it's sustained for `min_stroke_pixels_` on its own,
  // measured from `pending_origin_`; it's discarded if movement reverts to
  // the current stroke's direction or diverges to a different one first.
  std::optional<DirectionalGestureStrokeType> pending_stroke_type_;
  gfx::PointF pending_origin_;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_RECOGNIZER_H_
