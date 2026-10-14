// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_ADD_POINT_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_ADD_POINT_INFO_H_

#include <optional>

#include "brave/components/gestures/core/common/directional/directional_gesture_types.h"

namespace gestures {

// The outcome of feeding one point to `DirectionalGestureStrokeClassifier`.
struct DirectionalGestureAddPointInfo final {
  // Whether the point cleared `min_stroke_pixels` from the stroke origin in
  // some direction, even if no stroke actually committed.
  bool had_qualifying_movement = false;
  std::optional<DirectionalGestureStrokeType> committed_stroke_type;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_ADD_POINT_INFO_H_
