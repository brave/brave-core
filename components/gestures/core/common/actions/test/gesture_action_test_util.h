// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_TEST_GESTURE_ACTION_TEST_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_TEST_GESTURE_ACTION_TEST_UTIL_H_

#include <string>

#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/actions/open_settings_page_types.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_open_in_command_param_types.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_selected_text_search_command_param_types.h"

namespace gestures::test {

GestureActionInfo MakeNavigateToUrlGestureAction(std::string url_spec);

GestureActionInfo MakeSwitchToTabGestureAction(std::string pattern);

GestureActionInfo MakeOpenSettingsPageGestureAction(
    OpenSettingsPageType settings_page);

GestureActionInfo MakeSearchSelectedTextGestureAction(
    SmartGestureSelectedTextSearchCommandParamType command_param_type);

// `command` is `kOpenLinkIn`/`kOpenImageIn`; kept as a plain `int` so this
// doesn't need to depend on `gesture_command_constants.h`'s exact set.
GestureActionInfo MakeGestureActionOpenInCommandParam(
    int command,
    SmartGestureOpenInCommandParamType command_param_type);

}  // namespace gestures::test

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_TEST_GESTURE_ACTION_TEST_UTIL_H_
