// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_MOUSE_ROCKER_DEFAULT_COMMANDS_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_MOUSE_ROCKER_DEFAULT_COMMANDS_H_

namespace gestures {

// Default commands (real `IDC_*` ids) for the mouse rocker gesture's left
// and right click actions. Supplied by the caller rather than looked up
// internally, since this component must not depend on `IDC_*` headers.
struct MouseRockerDefaultCommands {
  int left;
  int right;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_MOUSE_ROCKER_DEFAULT_COMMANDS_H_
