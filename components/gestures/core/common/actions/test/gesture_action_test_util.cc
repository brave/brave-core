// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/test/gesture_action_test_util.h"

#include <utility>

#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "brave/components/gestures/core/common/actions/int_param_info.h"
#include "brave/components/gestures/core/common/actions/text_param_info.h"
#include "brave/components/gestures/core/common/test/fake_gesture_commands.h"
#include "url/gurl.h"

namespace gestures::test {

GestureActionInfo MakeNavigateToUrlGestureAction(const GURL& url) {
  return {.command = kNavigateToUrl,
          .command_params = TextParamInfo{.value = url.spec()}};
}

GestureActionInfo MakeSwitchToTabGestureAction(std::string pattern) {
  return {.command = kSwitchToTab,
          .command_params = TextParamInfo{.value = std::move(pattern)}};
}

GestureActionInfo MakeOpenSettingsPageGestureAction(OpenSettingsPageType type) {
  return {.command = kIdcOptionsCommand,
          .command_params = IntParamInfo{.value = static_cast<int>(type)}};
}

GestureActionInfo MakeSearchSelectedTextGestureAction(
    SmartGestureSelectedTextSearchCommandParamType command_param_type) {
  return {.command = kSearchSelectedText,
          .command_params =
              IntParamInfo{.value = static_cast<int>(command_param_type)}};
}

GestureActionInfo MakeGestureActionOpenInCommandParam(
    int command,
    SmartGestureOpenInCommandParamType command_param_type) {
  return {.command = command,
          .command_params =
              IntParamInfo{.value = static_cast<int>(command_param_type)}};
}

}  // namespace gestures::test
