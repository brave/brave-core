// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_TEST_GESTURE_ACTION_TEST_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_TEST_GESTURE_ACTION_TEST_UTIL_H_

#include <string>

#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/actions/open_settings_page_types.h"

namespace gestures::test {

GestureActionInfo MakeNavigateToUrlGestureAction(std::string url_spec);

GestureActionInfo MakeSwitchToTabGestureAction(std::string pattern);

GestureActionInfo MakeOpenSettingsGestureAction(OpenSettingsPage settings_page);

}  // namespace gestures::test

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_TEST_GESTURE_ACTION_TEST_UTIL_H_
