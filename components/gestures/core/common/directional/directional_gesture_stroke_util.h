// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_STROKE_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_STROKE_UTIL_H_

#include <optional>
#include <string>
#include <string_view>

#include "brave/components/gestures/core/common/directional/directional_gesture_types.h"
#include "brave/components/gestures/core/common/gesture_path_types.h"
#include "ui/gfx/geometry/point_f.h"

namespace gestures {

// Returns the direction from `origin` to `location`, or `nullopt` if the
// movement is smaller than `min_stroke_pixels`. Resolves to the nearest of
// the 8 directions in `DirectionalGestureStrokeType`.
std::optional<DirectionalGestureStrokeType> MaybeGetDirectionalGestureStroke(
    gfx::PointF origin,
    gfx::PointF location,
    double min_stroke_pixels);

// Classifies `path` into a sequence of directional strokes, capped at
// `max_strokes`, applying the same rules `DirectionalGestureRecognizer` does
// incrementally during a live drag (each candidate direction, including the
// first, must sustain on its own for `min_stroke_pixels`, and dominate the
// actual distance traveled to reach it by at least `min_stroke_straightness`,
// before committing), but as a one-shot batch pass over an already-finished
// path. `path` should already be resampled (see
// `ResampleGesturePathAtInterval` in `directional_gesture_util.h`) for a
// result that doesn't depend on the original points' sampling rate.
DirectionalGestureStrokes ClassifyDirectionalGestureStrokes(
    const GesturePath& path,
    double min_stroke_pixels,
    double min_stroke_straightness,
    size_t max_strokes);

// Makes an idealized polyline for `strokes` (each stroke a unit-length
// segment chained from the origin), so a directional match can be compared
// against custom gestures on the same point-sequence distance scale
// (`ResampleAndNormalizePath`/`DistanceToCandidate`), rather than always
// winning just because some stroke sequence classified.
GesturePath DirectionalGestureStrokesToCanonicalPath(
    const DirectionalGestureStrokes& strokes);

// Per-direction arrow glyph, concatenated by `DirectionalGestureStrokesToKey`
// into a gesture key. For instance, `↓→` means a down stroke followed by a
// right stroke, distinct from the single diagonal `↘` stroke. Also used to
// prefix the overlay label with the direction just drawn, before the matched
// command's name.
std::string_view DirectionalGestureStrokeToKeyCode(
    DirectionalGestureStrokeType stroke_type);

// Concatenates each stroke's `DirectionalGestureStrokeToKeyCode` into the
// gesture key `DirectionalGestureMapping` stores and looks up actions by, e.g.
// in `kDirectionalGestures`.
std::string DirectionalGestureStrokesToKey(
    const DirectionalGestureStrokes& strokes);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_STROKE_UTIL_H_
