// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_COMMAND_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_COMMAND_UTIL_H_

namespace gestures {

// `GestureCommandType` is persisted directly in prefs and synced across
// devices and Brave versions, so a raw int read back from a pref must be
// checked against the enum's current range before it is cast. An older
// Brave version has no notion of commands appended since, and a newer one
// may have appended commands this version doesn't know about yet.
bool IsValidGestureCommand(int raw_gesture_command);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_COMMAND_UTIL_H_
