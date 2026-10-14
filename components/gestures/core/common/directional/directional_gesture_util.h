// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_UTIL_H_

#include "brave/components/gestures/core/common/gesture_path_types.h"
#include "ui/gfx/geometry/point_f.h"

namespace gestures {

// Returns whether `candidate` is far enough from `last_accepted_point` to be
// worth recording, rather than jitter from a high-polling-rate mouse or
// trackpad. Unlike `MaybeGetDirectionalGestureStroke`'s `min_stroke_pixels`
// (see `directional_gesture_stroke_util.h`), which decides when accumulated
// movement is big enough to commit a directional stroke, this is a
// per-sample filter applied before a point is ever added to a gesture's
// traced path.
bool IsFarEnoughFromLastDrawnPoint(gfx::PointF last_accepted_point,
                                   gfx::PointF candidate,
                                   double min_point_distance_pixels);

// Resamples `path` to points evenly spaced `interval_pixels` apart by arc
// length, rather than a fixed point count, so a longer path gets more
// points, a shorter one gets fewer. Classifying a resampled path (see
// `ClassifyDirectionalGestureStrokes` in `directional_gesture_stroke_util.h`)
// makes the result depend only on the shape drawn, not on how unevenly the
// original points happened to be sampled (e.g. a brief slowdown mid-drag
// producing two unusually close samples).
GesturePath ResampleGesturePathAtInterval(const GesturePath& path,
                                          float interval_pixels);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_UTIL_H_
