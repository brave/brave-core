// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_INPUT_MOUSE_MOUSE_WHEEL_DEFAULT_COMMANDS_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_INPUT_MOUSE_MOUSE_WHEEL_DEFAULT_COMMANDS_INFO_H_

namespace gestures {

// Default `IDC_*` commands for the mouse wheel gesture's up/down actions;
// caller-supplied since this component can't depend on `IDC_*`.
struct MouseWheelDefaultCommandsInfo final {
  int up = 0;
  int down = 0;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_INPUT_MOUSE_MOUSE_WHEEL_DEFAULT_COMMANDS_INFO_H_
