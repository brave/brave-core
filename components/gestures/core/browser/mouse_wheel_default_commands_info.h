// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_MOUSE_WHEEL_DEFAULT_COMMANDS_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_MOUSE_WHEEL_DEFAULT_COMMANDS_INFO_H_

namespace gestures {

// Default commands (real `IDC_*` ids) for the mouse wheel gesture's up and
// down actions. Supplied by the caller rather than looked up internally,
// since this component must not depend on `IDC_*` headers.
struct MouseWheelDefaultCommandsInfo {
  int up = 0;
  int down = 0;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_MOUSE_WHEEL_DEFAULT_COMMANDS_INFO_H_
