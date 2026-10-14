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
