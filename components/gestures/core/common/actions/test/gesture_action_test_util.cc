// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/test/gesture_action_test_util.h"

#include <utility>

#include "brave/components/gestures/core/common/actions/navigate_to_url_params_info.h"
#include "brave/components/gestures/core/common/actions/switch_to_tab_params_info.h"

namespace gestures::test {

GestureActionInfo MakeNavigateToUrlGestureAction(std::string url_spec) {
  GestureActionInfo action;
  action.command = GestureCommandType::kNavigateToUrl;
  action.command_params =
      NavigateToUrlParamsInfo{.url_spec = std::move(url_spec)};
  return action;
}

GestureActionInfo MakeSwitchToTabGestureAction(std::string pattern) {
  GestureActionInfo action;
  action.command = GestureCommandType::kSwitchToTab;
  action.command_params = SwitchToTabParamsInfo{.pattern = std::move(pattern)};
  return action;
}

}  // namespace gestures::test
