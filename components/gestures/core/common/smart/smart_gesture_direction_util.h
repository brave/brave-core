// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_DIRECTION_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_DIRECTION_UTIL_H_

#include "brave/components/gestures/core/common/gesture_path_types.h"

namespace gestures {

// A smart gesture's single swipe direction. Unlike
// `DirectionalGestureStrokeType`, there's no diagonals and no multi-stroke
// sequence: a smart gesture is one short swipe over a link, image, or
// selection, not a drawn shape.
enum class SmartGestureDirectionType {
  kUp,
  kDown,
  kLeft,
  kRight,
};

// Classifies `path`'s overall direction from its first point to its last,
// picking whichever axis moved more; a tie resolves to `kRight`/`kDown`.
SmartGestureDirectionType ClassifySmartGestureDirection(
    const GesturePath& path);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_DIRECTION_UTIL_H_
