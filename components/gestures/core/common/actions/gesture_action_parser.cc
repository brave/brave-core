// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/gesture_action_parser.h"

#include <utility>

#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "brave/components/gestures/core/common/actions/gesture_command_util.h"
#include "brave/components/gestures/core/common/actions/int_param_parser.h"
#include "brave/components/gestures/core/common/actions/text_param_parser.h"

namespace gestures {

namespace {

std::optional<GestureActionInfo> MaybeParseGestureAction(
    const base::Value& value) {
  const base::DictValue* dict = value.GetIfDict();
  std::optional<int> gesture_command =
      dict ? dict->FindInt("command") : std::nullopt;
  if (!gesture_command || !IsValidGestureCommand(*gesture_command)) {
    return std::nullopt;
  }

  GestureActionInfo gesture_action;
  gesture_action.command = *gesture_command;
  if (gesture_action.command == kNavigateToUrl ||
      gesture_action.command == kSwitchToTab) {
    gesture_action.command_params = ParseTextParam(*dict);
  } else if (dict->Find(kValueParamKey)) {
    // Can't enumerate by command id, since one is a real `IDC_*` id from
    // `chrome/app/chrome_command_ids.h`; checks for the value key instead.
    gesture_action.command_params = ParseIntParam(*dict);
  }
  return gesture_action;
}

}  // namespace

std::optional<GestureActions> MaybeParseGestureActions(
    const base::ListValue& list) {
  GestureActions actions;
  for (const base::Value& value : list) {
    if (std::optional<GestureActionInfo> action =
            MaybeParseGestureAction(value)) {
      actions.push_back(std::move(*action));
    }
  }
  if (actions.empty()) {
    return std::nullopt;
  }
  return actions;
}

}  // namespace gestures
