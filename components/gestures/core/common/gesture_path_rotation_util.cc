// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_path_rotation_util.h"

#include <algorithm>
#include <iterator>

#include "base/numerics/safe_conversions.h"

namespace gestures {

GesturePath RotateGesturePath(const GesturePath& gesture_path, size_t offset) {
  GesturePath rotated_path;
  rotated_path.reserve(gesture_path.size());
  std::ranges::rotate_copy(
      gesture_path,
      gesture_path.cbegin() + static_cast<GesturePath::difference_type>(offset),
      std::back_inserter(rotated_path));
  return rotated_path;
}

float AverageGesturePathDistance(const GesturePath& first_path,
                                 const GesturePath& second_path) {
  float total = 0;
  for (size_t i = 0; i < first_path.size(); ++i) {
    total += (first_path.at(i) - second_path.at(i)).Length();
  }
  return total / base::saturated_cast<float>(first_path.size());
}

}  // namespace gestures
