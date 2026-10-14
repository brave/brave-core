// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_VECTOR_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_VECTOR_UTIL_H_

#include "brave/components/gestures/core/common/directional/directional_gesture_types.h"
#include "ui/gfx/geometry/vector2d_f.h"

namespace gestures {

// Resolves a screen-space displacement to the nearest of the 8 directions in
// `DirectionalGestureStrokeType`.
DirectionalGestureStrokeType DirectionFromDisplacement(float dx, float dy);

// The unit-length screen-space vector for `stroke_type`, e.g. for
// `DirectionalGestureStrokesToCanonicalPath` to chain into an idealized
// polyline.
gfx::Vector2dF UnitVectorForDirection(DirectionalGestureStrokeType stroke_type);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_VECTOR_UTIL_H_
