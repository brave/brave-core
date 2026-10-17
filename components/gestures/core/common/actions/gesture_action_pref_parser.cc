// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/gesture_action_pref_parser.h"

#include <utility>

#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "brave/components/gestures/core/common/actions/gesture_command_util.h"
#include "brave/components/gestures/core/common/actions/int_param_util.h"
#include "brave/components/gestures/core/common/actions/text_param_util.h"

namespace gestures {

namespace {

std::optional<GestureActionInfo> ParseGestureAction(const base::Value& value) {
  const base::DictValue* dict = value.GetIfDict();
  const std::optional<int> raw_gesture_command =
      dict ? dict->FindInt("command") : std::nullopt;
  if (!raw_gesture_command || !IsValidGestureCommand(*raw_gesture_command)) {
    return std::nullopt;
  }

  GestureActionInfo gesture_action;
  gesture_action.command = *raw_gesture_command;
  if (gesture_action.command == kNavigateToUrl ||
      gesture_action.command == kSwitchToTab) {
    gesture_action.command_params = ParseTextParam(*dict);
  } else if (dict->Find(kValueParamKey)) {
    // This component must not depend on `IDC_*` headers, so it can't
    // check against `IDC_OPTIONS` directly (see `gesture_command_constants.h`).
    gesture_action.command_params = ParseIntParam(*dict);
  }
  return gesture_action;
}

}  // namespace

std::optional<std::vector<GestureActionInfo>> MaybeParseGestureActions(
    const base::ListValue& list) {
  std::vector<GestureActionInfo> actions;
  for (const base::Value& value : list) {
    if (std::optional<GestureActionInfo> action = ParseGestureAction(value)) {
      actions.push_back(std::move(*action));
    }
  }
  if (actions.empty()) {
    return std::nullopt;
  }
  return actions;
}

}  // namespace gestures
