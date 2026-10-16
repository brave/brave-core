// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/custom_gesture_pref_parser.h"

#include <utility>

#include "brave/components/gestures/core/common/actions/gesture_action_pref_parser.h"

namespace gestures {

std::optional<CustomGestureInfo> MaybeParseCustomGesture(
    const base::DictValue& dict) {
  const std::string* id = dict.FindString("id");
  if (!id) {
    return std::nullopt;
  }
  const base::ListValue* points_list = dict.FindList("points");
  if (!points_list) {
    return std::nullopt;
  }
  const base::ListValue* actions_list = dict.FindList("actions");
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
