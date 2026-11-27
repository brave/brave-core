// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_TYPES_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_TYPES_H_

#include <vector>

#include "ui/gfx/geometry/point_f.h"

namespace gestures {

// A gesture's raw or processed point trail, in draw order.
using GesturePath = std::vector<gfx::PointF>;

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_TYPES_H_
