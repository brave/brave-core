// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_ACTION_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_ACTION_UTIL_H_

#include "brave/components/gestures/core/common/actions/gesture_action_info.h"

namespace gestures {

// Makes a `GestureActionInfo` for a command that takes no parameters. For a
// single-action list, wrap the call in braces, `{MakeGestureAction(command)}`.
GestureActionInfo MakeGestureAction(int command);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_ACTION_UTIL_H_
