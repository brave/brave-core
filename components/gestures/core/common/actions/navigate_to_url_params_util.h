// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_NAVIGATE_TO_URL_PARAMS_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_NAVIGATE_TO_URL_PARAMS_UTIL_H_

#include "brave/components/gestures/core/common/actions/navigate_to_url_params_info.h"

namespace base {
class DictValue;
}  // namespace base

namespace gestures {

// Parses the "navigate_to_url" field of a `kDirectionalGestures`-style
// action dict.
NavigateToUrlParamsInfo ParseNavigateToUrlParams(
    const base::DictValue& action_dict);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_NAVIGATE_TO_URL_PARAMS_UTIL_H_
