// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_DISTANCE_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_DISTANCE_UTIL_H_

#include "brave/components/gestures/core/common/gesture_path_types.h"
#include "ui/gfx/geometry/point_f.h"

namespace gestures {

// Total distance traveled along `gesture_path`; unlike net displacement,
// doesn't shrink as the gesture_path curves back near its own start.
float GesturePathLength(const GesturePath& gesture_path);

// Whether `candidate` is far enough from `last_drawn_point` to add,
// filtering jitter before a point is ever added to a gesture_path.
bool IsFarEnoughFromLastDrawnPoint(gfx::PointF last_drawn_point,
                                   gfx::PointF candidate,
                                   double min_point_distance_pixels);

// True if `gesture_path`'s start and end are close relative to its own
// length (e.g. a circle), meaning its start point is arbitrary.
bool IsClosedGesturePath(const GesturePath& gesture_path);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_DISTANCE_UTIL_H_
