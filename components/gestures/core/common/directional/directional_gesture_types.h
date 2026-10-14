// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_TYPES_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_TYPES_H_

#include <vector>

namespace gestures {

// Physical screen directions, never remapped for RTL locales since gestures
// are physical movement, not logical direction.
enum class DirectionalGestureStrokeType {
  kUp,         // ↑
  kDown,       // ↓
  kLeft,       // ←
  kRight,      // →
  kUpLeft,     // ↖
  kUpRight,    // ↗
  kDownLeft,   // ↙
  kDownRight,  // ↘
};

// A gesture's full stroke sequence, in draw order.
using DirectionalGestureStrokes = std::vector<DirectionalGestureStrokeType>;

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_TYPES_H_
