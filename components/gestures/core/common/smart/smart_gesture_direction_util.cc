// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/smart/smart_gesture_direction_util.h"

#include <cmath>

#include "ui/gfx/geometry/vector2d_f.h"

namespace gestures {

SmartGestureDirectionType ClassifySmartGestureDirection(
    const GesturePath& gesture_path) {
  const gfx::Vector2dF delta = gesture_path.back() - gesture_path.front();
  if (std::abs(delta.x()) >= std::abs(delta.y())) {
    return delta.x() >= 0 ? SmartGestureDirectionType::kRight
                          : SmartGestureDirectionType::kLeft;
  }
  return delta.y() >= 0 ? SmartGestureDirectionType::kDown
                        : SmartGestureDirectionType::kUp;
}

}  // namespace gestures
