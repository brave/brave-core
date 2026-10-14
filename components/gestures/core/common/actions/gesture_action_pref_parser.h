// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_ACTION_PREF_PARSER_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_ACTION_PREF_PARSER_H_

#include <optional>
#include <vector>

#include "base/values.h"
#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_info.h"

namespace gestures {

// Parses an actions list such as `kDirectionalGestures`'s value or a custom
// gesture's "actions" field: a list of {"command": int, "navigate_to_url":
// string} dicts. Invalid entries are skipped rather than discarding the
// whole list. Returns `nullopt` if `actions_list` has no valid entry.
std::optional<std::vector<GestureActionInfo>> MaybeParseGestureActions(
    const base::ListValue& actions_list);

// Parses one `kCustomGestures` entry: {"id": string, "points": [{"x":
// number, "y": number}, ...], "actions": [...]}. Returns `nullopt` if the
// entry is missing a required field, or has fewer than 2 points:
// `CustomGestureRecognizer` requires at least 2 to measure a path's length
// and resample it, so a corrupted or truncated entry is dropped here rather
// than reaching the recognizer unusable.
std::optional<CustomGestureInfo> MaybeParseCustomGesture(
    const base::DictValue& custom_gesture_dict);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_ACTION_PREF_PARSER_H_
