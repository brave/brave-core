// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_ROTATION_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_ROTATION_UTIL_H_

#include "brave/components/gestures/core/common/gesture_path_types.h"

namespace gestures {

// Returns `gesture_path` with its points rotated so the point at `offset`
// comes first, preserving order, letting a closed shape be compared from
// any start.
GesturePath RotateGesturePath(const GesturePath& gesture_path, size_t offset);

// Mean per-point distance between `first_path` and `second_path`, which
// must already have the same point count.
float AverageGesturePathDistance(const GesturePath& first_path,
                                 const GesturePath& second_path);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_ROTATION_UTIL_H_
