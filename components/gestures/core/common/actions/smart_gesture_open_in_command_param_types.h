// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_SMART_GESTURE_OPEN_IN_COMMAND_PARAM_TYPES_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_SMART_GESTURE_OPEN_IN_COMMAND_PARAM_TYPES_H_

namespace gestures {

// Where to open a smart-gesture link/image. Persisted directly in prefs, so
// never renumber or reorder existing entries, only append.
enum class SmartGestureOpenInCommandParamType {
  kNewBackgroundTab = 0,  // The default.
  kNewForegroundTab = 1,
  kNewWindow = 2,
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_SMART_GESTURE_OPEN_IN_COMMAND_PARAM_TYPES_H_
