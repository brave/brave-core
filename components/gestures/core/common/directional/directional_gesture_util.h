// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_UTIL_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "brave/components/gestures/core/common/directional/directional_gesture_types.h"
#include "ui/gfx/geometry/point_f.h"

namespace gestures {

// Returns the direction from `origin` to `location`, or `nullopt` if the
// movement is smaller than `min_stroke_pixels`. Resolves to the nearest of
// the 8 directions in `DirectionalGestureDirectionType`.
std::optional<DirectionalGestureDirectionType> MaybeMovementDirection(
    gfx::PointF origin,
    gfx::PointF location,
    double min_stroke_pixels);

// Resamples `path` to points evenly spaced `interval_pixels` apart by arc
// length, rather than a fixed point count: a longer path gets more points, a
// shorter one gets fewer. Classifying a resampled path (see
// `ClassifyDirectionalStrokes`) makes the result depend only on the shape
// drawn, not on how unevenly the original points happened to be sampled
// (e.g. a brief slowdown mid-drag producing two unusually close samples).
std::vector<gfx::PointF> ResampleAtInterval(
    const std::vector<gfx::PointF>& path,
    float interval_pixels);

// Classifies `path` into a sequence of directional strokes, capped at
// `max_strokes`: the same rules `DirectionalGestureRecognizer` applies
// incrementally during a live drag (each candidate direction, including the
// first, must sustain on its own for `min_stroke_pixels` before committing),
// but as a one-shot batch pass over an already-finished path. `path` should
// already be resampled (see `ResampleAtInterval`) for a result that doesn't
// depend on the original points' sampling rate.
std::vector<DirectionalGestureDirectionType> ClassifyDirectionalStrokes(
    const std::vector<gfx::PointF>& path,
    double min_stroke_pixels,
    size_t max_strokes);

// Per-direction arrow glyph, concatenated by `StrokesToGestureKey` into a
// gesture key. For instance, `↓→` means a down stroke followed by a right
// stroke, distinct from the single diagonal `↘` stroke. Also used to prefix
// the overlay label with the direction just drawn, before the matched
// command's name.
std::string_view DirectionToGestureKeyCode(
    DirectionalGestureDirectionType direction_type);

// Concatenates each stroke's `DirectionToGestureKeyCode` into the gesture key
// `DirectionalGestureMapping` stores and looks up actions by, e.g. in
// `kDirectionalGestures`.
std::string StrokesToGestureKey(
    const std::vector<DirectionalGestureDirectionType>& strokes);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_UTIL_H_
