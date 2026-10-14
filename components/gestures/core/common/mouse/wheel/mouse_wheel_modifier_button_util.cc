// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/mouse/wheel/mouse_wheel_modifier_button_util.h"

#include <utility>

#include "brave/components/gestures/core/common/mouse/wheel/mouse_wheel_modifier_button_types.h"

namespace gestures {

bool IsValidMouseWheelModifierButton(int raw_mouse_wheel_modifier_button) {
  return raw_mouse_wheel_modifier_button >=
             std::to_underlying(MouseWheelModifierButtonType::kLeft) &&
         raw_mouse_wheel_modifier_button <=
             std::to_underlying(MouseWheelModifierButtonType::kMaxValue);
}

}  // namespace gestures
