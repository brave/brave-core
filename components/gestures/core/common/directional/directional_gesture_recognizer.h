// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_RECOGNIZER_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_RECOGNIZER_H_

#include <optional>
#include <vector>

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
             double min_stroke_pixels = features::kMinStrokePixels.Get());

  // Feeds a new cursor position while the mouse button, trackpad button, or
  // touchscreen contact is held. May append a new stroke, collapse into the
  // current one, or be ignored if the sequence is already at `max_strokes`.
  void OnMouseMoved(gfx::PointF location, base::TimeTicks now);

  // Returns true if `now` is at least `features::kIdleHoldTimeout` past the
  // last time a qualifying stroke was recorded, or since `Reset` if none has
  // been.
  bool TimedOut(base::TimeTicks now) const;

  const std::vector<DirectionalGestureDirectionType>& strokes() const {
    return strokes_;
  }

 private:
  // Set from `Reset`'s `min_stroke_pixels` argument for the drawing in
  // progress.
  double min_stroke_pixels_ = features::kMinStrokePixels.Get();

  // State of the drawing in progress.
  std::vector<DirectionalGestureDirectionType> strokes_;

  // Origin of the stroke currently being measured. This is the last committed
  // stroke's endpoint, or the drawing start location if no stroke has
  // committed yet.
  gfx::PointF stroke_origin_;

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
  std::optional<DirectionalGestureDirectionType> pending_direction_type_;
  gfx::PointF pending_origin_;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_RECOGNIZER_H_
