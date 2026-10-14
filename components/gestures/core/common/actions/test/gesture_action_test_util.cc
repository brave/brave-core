// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/test/gesture_action_test_util.h"

#include <utility>

#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "brave/components/gestures/core/common/actions/int_param_info.h"
#include "brave/components/gestures/core/common/actions/text_param_info.h"

namespace gestures::test {

namespace {

// This component must not depend on `IDC_*` headers; mirrors the real
// `IDC_OPTIONS` value from chrome/app/chrome_command_ids.h.
constexpr int kIdcOptions = 40015;

}  // namespace

GestureActionInfo MakeNavigateToUrlGestureAction(std::string url_spec) {
  GestureActionInfo gesture_action;
  gesture_action.command = kNavigateToUrl;
  gesture_action.command_params = TextParamInfo{.value = std::move(url_spec)};
  return gesture_action;
}

GestureActionInfo MakeSwitchToTabGestureAction(std::string pattern) {
  GestureActionInfo gesture_action;
  gesture_action.command = kSwitchToTab;
  gesture_action.command_params = TextParamInfo{.value = std::move(pattern)};
  return gesture_action;
}

GestureActionInfo MakeOpenSettingsPageGestureAction(
    OpenSettingsPageType settings_page) {
  GestureActionInfo gesture_action;
  gesture_action.command = kIdcOptions;
  gesture_action.command_params =
      IntParamInfo{.value = static_cast<int>(settings_page)};
  return gesture_action;
}

GestureActionInfo MakeSearchSelectedTextGestureAction(
    SmartGestureSelectedTextSearchCommandParamType command_param_type) {
  GestureActionInfo gesture_action;
  gesture_action.command = kSearchSelectedText;
  gesture_action.command_params =
      IntParamInfo{.value = static_cast<int>(command_param_type)};
  return gesture_action;
}

GestureActionInfo MakeGestureActionOpenInCommandParam(
    int command,
    SmartGestureOpenInCommandParamType command_param_type) {
  return {.command = command,
          .command_params =
              IntParamInfo{.value = static_cast<int>(command_param_type)}};
}

}  // namespace gestures::test
