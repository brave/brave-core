// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_DEFAULT_COMMANDS_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_DEFAULT_COMMANDS_H_

namespace gestures {

// Default commands (real `IDC_*` ids) for the built-in directional gesture
// mappings in `DefaultMappings()`. Supplied by the caller rather than looked
// up internally, since this component must not depend on `IDC_*` headers.
// `kScrollToTop`'s default ("↑") isn't here. It's one of the reserved
// sentinels in `gesture_command_constants.h`, not a real browser command.
struct DirectionalGestureDefaultCommands {
  int new_tab;
  int back;
  int forward;
  int close_tab;
  int new_window;
  int previous_tab;
  int duplicate_tab;
  int next_tab;
  int reload;
  int reopen_closed_tab;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_DEFAULT_COMMANDS_H_
