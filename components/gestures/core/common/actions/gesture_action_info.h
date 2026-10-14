// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_ACTION_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_ACTION_INFO_H_

#include <variant>
#include <vector>

#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "brave/components/gestures/core/common/actions/int_param_info.h"
#include "brave/components/gestures/core/common/actions/text_param_info.h"

namespace gestures {

// One action run when a gesture matches; a gesture maps to a list of these,
// run in order, so one gesture can trigger more than one thing.
struct GestureActionInfo final {
  bool operator==(const GestureActionInfo&) const = default;

  // One of the sentinels in `gesture_command_constants.h`, or a raw
  // browser `IDC_*` command id. `kUnassigned` means none configured yet.
  int command = kUnassigned;

  // Parameters specific to `command`; `std::monostate` for commands that
  // take none. A smart gesture's captured content is runtime context, not here.
  std::variant<std::monostate, TextParamInfo, IntParamInfo> command_params;
};

// A gesture's full chain of actions, run in order.
using GestureActions = std::vector<GestureActionInfo>;

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_ACTION_INFO_H_
