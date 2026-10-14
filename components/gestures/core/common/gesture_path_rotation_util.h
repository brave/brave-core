// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_ROTATION_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_ROTATION_UTIL_H_

#include <algorithm>
#include <iterator>
#include <ranges>  // IWYU pragma: keep

#include "brave/components/gestures/core/common/gesture_path_types.h"

namespace gestures {

// `gesture_path` rotated so the point at `offset` comes first, preserving
// order, letting a closed shape be compared from any start.
GesturePath RotateGesturePath(const GesturePath& gesture_path, size_t offset);

// Invokes `on_rotation` with every rotation of `candidate_gesture_path` and
// its reversed winding direction, since a closed shape has neither.
template <typename T>
void ForEachClosedGesturePathRotation(const GesturePath& candidate_gesture_path,
                                      const T& callback) {
  GesturePath reversed_candidate_gesture_path;
  reversed_candidate_gesture_path.reserve(candidate_gesture_path.size());
  std::ranges::reverse_copy(
      candidate_gesture_path,
      std::back_inserter(reversed_candidate_gesture_path));

  for (const GesturePath& gesture_path :
       {candidate_gesture_path, reversed_candidate_gesture_path}) {
    for (size_t offset = 0; offset < gesture_path.size(); ++offset) {
      callback(RotateGesturePath(gesture_path, offset));
    }
  }
}

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_ROTATION_UTIL_H_
