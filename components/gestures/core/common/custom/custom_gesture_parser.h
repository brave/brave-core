// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_PARSER_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_PARSER_H_

#include <optional>

#include "base/values.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_info.h"

namespace gestures {

// Parses one `kCustomGestures` entry; `nullopt` if a required field is
// missing or it has fewer than 2 points (the recognizer's own minimum).
std::optional<CustomGestureInfo> MaybeParseCustomGesture(
    const base::DictValue& custom_gesture_dict);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_PARSER_H_
