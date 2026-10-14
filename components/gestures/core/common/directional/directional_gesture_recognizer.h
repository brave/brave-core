// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_RECOGNIZER_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_RECOGNIZER_H_

#include <optional>

#include "base/time/time.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_stroke_classifier.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_types.h"
#include "brave/components/gestures/core/common/features.h"
#include "ui/gfx/geometry/point_f.h"

namespace gestures {

// Converts sampled locations into a sequence of directional strokes. No
// dependency on input events or platform code.
class DirectionalGestureRecognizer final {
 public:
  DirectionalGestureRecognizer();

  DirectionalGestureRecognizer(const DirectionalGestureRecognizer&) = delete;
  DirectionalGestureRecognizer& operator=(const DirectionalGestureRecognizer&) =
      delete;

  ~DirectionalGestureRecognizer();

  // Starts a new drawing, clearing any previously accumulated strokes;
  // `min_stroke_pixels` is passed in so callers can honor user sensitivity.
  void Reset(gfx::PointF start_location,
             base::TimeTicks now,
             double min_stroke_pixels =
                 features::kDirectionalGestureMinStrokePixels.Get(),
             double min_stroke_straightness =
                 features::kDirectionalGestureMinStrokeStraightness.Get());

  // Feeds a new mouse position while held; may append a new stroke,
  // collapse into the current one, or be ignored at `max_strokes`.
  void OnMouseMoved(gfx::PointF location, base::TimeTicks now);

  // True if `now` is at least `kGestureSessionIdleHoldTimeout` past the last
  // qualifying stroke, or since `Reset` if none has committed yet.
  bool TimedOut(base::TimeTicks now) const;

  const DirectionalGestureStrokes& strokes() const { return strokes_; }

 private:
  DirectionalGestureStrokes strokes_;

  // Reconstructed on each `Reset`, since its sensitivity is fixed at
  // construction; `nullopt` only before the first `Reset`.
  std::optional<DirectionalGestureStrokeClassifier> stroke_classifier_;

  // When a qualifying stroke was last committed, or the drawing start time if
  // none has been.
  base::TimeTicks last_stroke_committed_at_;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_RECOGNIZER_H_
