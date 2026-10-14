// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_RESAMPLE_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_RESAMPLE_UTIL_H_

#include "brave/components/gestures/core/common/gesture_path_types.h"

namespace gestures {

// Resamples `gesture_path` to exactly `point_count` evenly-spaced points,
// preserving its overall shape, so a fixed point count can be compared
// regardless of how many points the original was drawn with.
GesturePath ResampleGesturePathToCount(const GesturePath& gesture_path,
                                       size_t point_count);

// Translates and scales `gesture_path` so its bounding box starts at the
// origin and its longer side spans exactly 1, removing position and size
// as factors in comparing it against another path.
GesturePath NormalizeGesturePathScaleAndTranslation(
    const GesturePath& gesture_path);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_RESAMPLE_UTIL_H_
