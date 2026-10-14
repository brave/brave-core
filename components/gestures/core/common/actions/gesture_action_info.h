// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_ACTION_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_ACTION_INFO_H_

#include <variant>

#include "brave/components/gestures/core/common/actions/gesture_command_types.h"
#include "brave/components/gestures/core/common/actions/navigate_to_url_params_info.h"
#include "brave/components/gestures/core/common/actions/switch_to_tab_params_info.h"

namespace gestures {

// One action run when a gesture matches. A gesture maps to a list of these, run
// in order, so a single gesture can trigger more than one thing (e.g. open a
// new tab, then navigate it to a URL).
struct GestureActionInfo {
  bool operator==(const GestureActionInfo&) const = default;

  // Which command this action runs. `kUnassigned` means no command has been
  // configured yet, e.g. a newly-added chain entry in the settings UI.
  GestureCommandType command = GestureCommandType::kUnassigned;

  // Parameters specific to `command`. `std::monostate` for commands that take
  // none. Adding a parameterized command means adding its own type here, not a
  // new field on this struct.
  std::variant<std::monostate, NavigateToUrlParamsInfo, SwitchToTabParamsInfo>
      command_params;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_ACTION_INFO_H_
