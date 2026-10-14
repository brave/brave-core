// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_ACTION_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_ACTION_UTIL_H_

#include <vector>

#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/actions/gesture_command_types.h"

namespace gestures {

// Builds a `GestureActionInfo` for a command that takes no parameters.
GestureActionInfo MakeGestureAction(GestureCommandType gesture_command_type);

// Builds a single-action list for a command that takes no parameters, for
// gesture mappings that trigger just the one command.
std::vector<GestureActionInfo> MakeGestureActions(
    GestureCommandType gesture_command_type);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_ACTION_UTIL_H_
