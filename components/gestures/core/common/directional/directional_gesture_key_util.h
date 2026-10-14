// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_KEY_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_KEY_UTIL_H_

#include <string>
#include <string_view>

#include "brave/components/gestures/core/common/directional/directional_gesture_types.h"

namespace gestures {

// Per-direction arrow glyph, concatenated by `ToDirectionalGestureStrokesKey`
// into a gesture key, e.g. `↓→` for down then right.
std::string_view ToDirectionalGestureStrokeGlyph(
    DirectionalGestureStrokeType stroke_type);

// Concatenates each stroke's glyph into the gesture key
// `DirectionalGestureMapping` looks actions up by.
std::string ToDirectionalGestureStrokesKey(
    const DirectionalGestureStrokes& strokes);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_KEY_UTIL_H_
