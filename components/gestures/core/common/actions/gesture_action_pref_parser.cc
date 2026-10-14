// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/gesture_action_pref_parser.h"

#include <utility>

#include "brave/components/gestures/core/common/actions/gesture_command_util.h"
#include "brave/components/gestures/core/common/actions/navigate_to_url_params_util.h"
#include "brave/components/gestures/core/common/actions/switch_to_tab_params_util.h"

namespace gestures {

std::optional<std::vector<GestureActionInfo>> MaybeParseGestureActions(
    const base::ListValue& actions_list) {
  std::vector<GestureActionInfo> actions;
  for (const base::Value& action_value : actions_list) {
    const base::DictValue* action_dict = action_value.GetIfDict();
    const std::optional<int> raw_gesture_command =
        action_dict ? action_dict->FindInt("command") : std::nullopt;
    if (!raw_gesture_command || !IsValidGestureCommand(*raw_gesture_command)) {
      continue;
    }

    GestureActionInfo action;
    action.command = static_cast<GestureCommandType>(*raw_gesture_command);
    if (action.command == GestureCommandType::kNavigateToUrl) {
      action.command_params = ParseNavigateToUrlParams(*action_dict);
    } else if (action.command == GestureCommandType::kSwitchToTab) {
      action.command_params = ParseSwitchToTabParams(*action_dict);
    }

    actions.push_back(std::move(action));
  }
  if (actions.empty()) {
    return std::nullopt;
  }
  return actions;
}

std::optional<CustomGestureInfo> MaybeParseCustomGesture(
    const base::DictValue& custom_gesture_dict) {
  const std::string* id = custom_gesture_dict.FindString("id");
  if (!id) {
    return std::nullopt;
  }
  const base::ListValue* points_list = custom_gesture_dict.FindList("points");
  if (!points_list) {
    return std::nullopt;
  }
  const base::ListValue* actions_list = custom_gesture_dict.FindList("actions");
  std::optional<std::vector<GestureActionInfo>> actions =
      actions_list ? MaybeParseGestureActions(*actions_list) : std::nullopt;
  if (!actions) {
    return std::nullopt;
  }

  CustomGestureInfo custom_gesture;
  custom_gesture.id = *id;
  custom_gesture.actions = std::move(*actions);
  for (const base::Value& point_value : *points_list) {
    const base::DictValue* point_dict = point_value.GetIfDict();
    if (!point_dict) {
      continue;
    }
    const std::optional<double> x = point_dict->FindDouble("x");
    if (!x) {
      continue;
    }
    const std::optional<double> y = point_dict->FindDouble("y");
    if (!y) {
      continue;
    }
    custom_gesture.stroke_points.emplace_back(*x, *y);
  }
  if (custom_gesture.stroke_points.size() < 2) {
    return std::nullopt;
  }
  return custom_gesture;
}

}  // namespace gestures
