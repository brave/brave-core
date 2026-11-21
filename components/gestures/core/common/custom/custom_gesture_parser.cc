// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/custom_gesture_parser.h"

#include <string_view>
#include <utility>

#include "brave/components/gestures/core/common/actions/gesture_action_parser.h"

namespace gestures {

namespace {

inline constexpr std::string_view kIdKey = "id";
inline constexpr std::string_view kPointsKey = "points";
inline constexpr std::string_view kActionsKey = "actions";
inline constexpr std::string_view kPointXKey = "x";
inline constexpr std::string_view kPointYKey = "y";

}  // namespace

std::optional<CustomGestureInfo> MaybeParseCustomGesture(
    const base::DictValue& dict) {
  const std::string* id = dict.FindString(kIdKey);
  if (!id) {
    return std::nullopt;
  }
  const base::ListValue* points_list = dict.FindList(kPointsKey);
  if (!points_list) {
    return std::nullopt;
  }
  const base::ListValue* actions_list = dict.FindList(kActionsKey);
  std::optional<GestureActions> actions =
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
    std::optional<double> x = point_dict->FindDouble(kPointXKey);
    if (!x) {
      continue;
    }
    std::optional<double> y = point_dict->FindDouble(kPointYKey);
    if (!y) {
      continue;
    }
    custom_gesture.gesture_path.emplace_back(*x, *y);
  }
  if (custom_gesture.gesture_path.size() < 2) {
    // A valid custom gesture must have at least 2 points.
    return std::nullopt;
  }
  return custom_gesture;
}

}  // namespace gestures
