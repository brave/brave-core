// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_SMOOTHING_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_SMOOTHING_UTIL_H_

#include "brave/components/gestures/core/common/gesture_path_types.h"
#include "third_party/skia/include/core/SkPath.h"

namespace gestures {

// A smooth Catmull-Rom-derived Bezier curve through `gesture_path`, matching
// a saved custom gesture's preview. Needs at least 2 points.
SkPath SmoothedPath(const GesturePath& gesture_path);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_SMOOTHING_UTIL_H_
