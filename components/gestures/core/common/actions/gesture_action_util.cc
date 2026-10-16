// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/gesture_action_util.h"

namespace gestures {

GestureActionInfo MakeGestureAction(int command) {
  GestureActionInfo gesture_action;
  gesture_action.command = command;
  return gesture_action;
}

}  // namespace gestures
