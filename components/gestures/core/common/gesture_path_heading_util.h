// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_HEADING_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_HEADING_UTIL_H_

#include <optional>

#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/vector2d_f.h"

namespace gestures {

// Starts a new heading from `heading_origin` toward `point`, leaving
// `heading` unset if they coincide. `max_forward` is the farthest point
// reached so far.
void StartGesturePathHeading(gfx::PointF heading_origin,
                             gfx::PointF point,
                             std::optional<gfx::Vector2dF>& heading,
                             float& max_forward);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_HEADING_UTIL_H_
