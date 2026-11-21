// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_VECTOR_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_VECTOR_UTIL_H_

#include <optional>

#include "brave/components/gestures/core/common/directional/directional_gesture_types.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/vector2d_f.h"

namespace gestures {

// Resolves a screen-space displacement to the nearest of the 8 directions in
// `DirectionalGestureStrokeType`.
DirectionalGestureStrokeType DirectionFromDisplacement(float dx, float dy);

// Direction from `origin` to `location`, or `nullopt` if movement is
// smaller than `min_stroke_pixels`; resolves to the nearest of 8 directions.
std::optional<DirectionalGestureStrokeType> MaybeGetDirectionalGestureStroke(
    gfx::PointF origin,
    gfx::PointF location,
    double min_stroke_pixels);

// The unit-length screen-space vector for `stroke_type`, e.g. for
// `DirectionalGestureStrokesToCanonicalPath`'s idealized polyline.
gfx::Vector2dF UnitVectorForDirection(DirectionalGestureStrokeType stroke_type);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_VECTOR_UTIL_H_
