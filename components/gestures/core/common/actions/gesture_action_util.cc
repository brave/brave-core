// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/gesture_action_util.h"

namespace gestures {

GestureActionInfo MakeGestureAction(GestureCommandType gesture_command_type) {
  GestureActionInfo action;
  action.command = gesture_command_type;
  return action;
}

std::vector<GestureActionInfo> MakeGestureActions(
    GestureCommandType gesture_command_type) {
  return {MakeGestureAction(gesture_command_type)};
}

}  // namespace gestures
