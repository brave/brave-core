// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_INPUT_MOUSE_MOUSE_ROCKER_DEFAULT_COMMANDS_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_INPUT_MOUSE_MOUSE_ROCKER_DEFAULT_COMMANDS_INFO_H_

namespace gestures {

// Default `IDC_*` commands for the mouse rocker gesture's left/right
// actions; caller-supplied since this component can't use `IDC_*`.
struct MouseRockerDefaultCommandsInfo final {
  int left = 0;
  int right = 0;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_INPUT_MOUSE_MOUSE_ROCKER_DEFAULT_COMMANDS_INFO_H_
