// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_PREF_PARSER_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_PREF_PARSER_H_

#include <optional>

#include "base/values.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_info.h"

namespace gestures {

// Parses one `kCustomGestures` entry: {"id": string, "points": [{"x":
// number, "y": number}, ...], "actions": [...]}. Returns `nullopt` if the
// entry is missing a required field, or has fewer than 2 points:
// `CustomGestureRecognizer` requires at least 2 to measure a path's length
// and resample it, so a corrupted or truncated entry is dropped here rather
// than reaching the recognizer unusable.
std::optional<CustomGestureInfo> MaybeParseCustomGesture(
    const base::DictValue& custom_gesture_dict);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_PREF_PARSER_H_
