// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_SWITCH_TO_TAB_PARAMS_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_SWITCH_TO_TAB_PARAMS_UTIL_H_

#include "brave/components/gestures/core/common/actions/switch_to_tab_params_info.h"

namespace base {
class DictValue;
}  // namespace base

namespace gestures {

// Parses the "switch_to_tab_pattern" field of a `kDirectionalGestures`-style
// action dict.
SwitchToTabParamsInfo ParseSwitchToTabParams(
    const base::DictValue& action_dict);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_SWITCH_TO_TAB_PARAMS_UTIL_H_
