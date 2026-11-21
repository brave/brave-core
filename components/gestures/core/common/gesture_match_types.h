// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_MATCH_TYPES_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_MATCH_TYPES_H_

namespace gestures {

// What kind of gesture a finished draw resolved to, if any.
enum class GestureMatchType { kUnmatched, kDirectional, kCustom, kSmart };

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_MATCH_TYPES_H_
