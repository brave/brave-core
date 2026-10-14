// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMOOTHED_PATH_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMOOTHED_PATH_UTIL_H_

#include "brave/components/gestures/core/common/gesture_path_types.h"
#include "third_party/skia/include/core/SkPath.h"

namespace gestures {

// Builds a smooth path through `gesture_path` with a Catmull-Rom-derived
// cubic Bezier between each pair, matching the curve drawn for a saved
// custom gesture's preview so the two look consistent. `gesture_path` must
// have at least 2 points.
SkPath SmoothedPath(const GesturePath& gesture_path);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMOOTHED_PATH_UTIL_H_
