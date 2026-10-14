// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_DEFAULT_COMMANDS_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_DEFAULT_COMMANDS_INFO_H_

namespace gestures {

// Default `IDC_*` commands for `DefaultMappings()`; caller-supplied since
// this component can't depend on `IDC_*` headers.
struct DirectionalGestureDefaultCommandsInfo final {
  int new_tab = 0;
  int back = 0;
  int forward = 0;
  int close_tab = 0;
  int new_window = 0;
  int previous_tab = 0;
  int duplicate_tab = 0;
  int next_tab = 0;
  int reload = 0;
  int reopen_closed_tab = 0;
  int scroll_tab_to_top = 0;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_DEFAULT_COMMANDS_INFO_H_
