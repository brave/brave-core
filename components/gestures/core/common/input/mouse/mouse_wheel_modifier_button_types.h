// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_INPUT_MOUSE_MOUSE_WHEEL_MODIFIER_BUTTON_TYPES_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_INPUT_MOUSE_MOUSE_WHEEL_MODIFIER_BUTTON_TYPES_H_

namespace gestures {

// Which mouse button, held while scrolling, turns the wheel into a
// gesture. Persisted directly in prefs; never renumber or reorder entries.
enum class MouseWheelModifierButtonType {
  kLeft = 0,
  kMiddle = 1,
  kRight = 2,
  kMaxValue = kRight,
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_INPUT_MOUSE_MOUSE_WHEEL_MODIFIER_BUTTON_TYPES_H_
