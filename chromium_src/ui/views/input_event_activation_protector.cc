/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ui/views/input_event_activation_protector.h"

#define OnWindowStationaryStateChanged(...) \
  OnWindowStationaryStateChanged_ChromiumImpl(__VA_ARGS__)

#include "src/ui/views/input_event_activation_protector.cc"

#undef OnWindowStationaryStateChanged

namespace views {

void InputEventActivationProtector::IgnoreNextWindowStationaryStateChanged() {
  ignore_next_window_stationary_state_changed_ = true;
}

void InputEventActivationProtector::OnWindowStationaryStateChanged() {
  if (ignore_next_window_stationary_state_changed_) {
    ignore_next_window_stationary_state_changed_ = false;
    return;
  }
  OnWindowStationaryStateChanged_ChromiumImpl();
}

}  // namespace views
