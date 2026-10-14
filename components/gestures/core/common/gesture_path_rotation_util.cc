// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_path_rotation_util.h"

namespace gestures {

GesturePath RotateGesturePath(const GesturePath& gesture_path, size_t offset) {
  GesturePath rotated_gesture_path;
  rotated_gesture_path.reserve(gesture_path.size());
  for (size_t i = 0; i < gesture_path.size(); ++i) {
    rotated_gesture_path.push_back(
        gesture_path.at((offset + i) % gesture_path.size()));
  }
  return rotated_gesture_path;
}

}  // namespace gestures
