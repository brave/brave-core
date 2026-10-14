// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_CANONICAL_PATH_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_CANONICAL_PATH_UTIL_H_

#include "brave/components/gestures/core/common/directional/directional_gesture_types.h"
#include "brave/components/gestures/core/common/gesture_path_types.h"

namespace gestures {

// A straight-line path for `strokes`, one unit per stroke, so a directional
// match can be compared against custom gestures on the same distance scale.
GesturePath DirectionalGestureStrokesToCanonicalPath(
    const DirectionalGestureStrokes& strokes);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_CANONICAL_PATH_UTIL_H_
