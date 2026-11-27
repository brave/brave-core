// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_COMMAND_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_COMMAND_UTIL_H_

namespace gestures {

// Whether `gesture_command` is a known sentinel or a plausible `IDC_*` id; a
// `gesture_command` that passes but isn't registered is a no-op at dispatch.
bool IsValidGestureCommand(int gesture_command);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_COMMAND_UTIL_H_
