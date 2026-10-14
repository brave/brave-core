// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_SMART_GESTURE_SELECTED_TEXT_SEARCH_COMMAND_PARAM_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_SMART_GESTURE_SELECTED_TEXT_SEARCH_COMMAND_PARAM_UTIL_H_

#include <string_view>

#include "brave/components/gestures/core/common/actions/smart_gesture_selected_text_search_command_param_types.h"

namespace gestures {

std::string_view ToSearchBaseUrl(
    SmartGestureSelectedTextSearchCommandParamType command_param_type);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_SMART_GESTURE_SELECTED_TEXT_SEARCH_COMMAND_PARAM_UTIL_H_
