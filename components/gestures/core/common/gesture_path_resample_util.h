// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_RESAMPLE_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_RESAMPLE_UTIL_H_

#include "brave/components/gestures/core/common/gesture_path_types.h"

namespace gestures {

// Resamples `gesture_path` to exactly `point_count` evenly-spaced points,
// so a fixed point count can be compared regardless of how it was drawn.
GesturePath ResampleGesturePathToPointCount(const GesturePath& gesture_path,
                                            size_t point_count);

// Resamples `gesture_path` to points evenly spaced `interval_pixels` apart by
// arc length, so classification depends only on shape, not sampling rate.
GesturePath ResampleGesturePathAtInterval(const GesturePath& gesture_path,
                                          float interval_pixels);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_RESAMPLE_UTIL_H_
