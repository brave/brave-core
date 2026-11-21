// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_INPUT_MOUSE_MOUSE_WHEEL_MODIFIER_BUTTON_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_INPUT_MOUSE_MOUSE_WHEEL_MODIFIER_BUTTON_UTIL_H_

namespace gestures {

// Whether `raw_mouse_wheel_modifier_button` is within
// `MouseWheelModifierButtonType`'s range before casting an older/newer value.
bool IsValidMouseWheelModifierButton(int raw_mouse_wheel_modifier_button);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_INPUT_MOUSE_MOUSE_WHEEL_MODIFIER_BUTTON_UTIL_H_
