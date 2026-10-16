// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_COMMAND_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_COMMAND_UTIL_H_

namespace gestures {

// A gesture's command is persisted directly in prefs and synced across
// devices and Brave versions, so a raw int read back from a pref must be
// sanity-checked first. It must be one of the reserved sentinel values in
// `gesture_command_constants.h`, or look like a real `IDC_*` command id
// (which this component can't name directly, since it must not depend on
// `IDC_*` headers). An id that passes this check but isn't actually
// registered in the current browser version is handled later, at dispatch
// time, as a no-op.
bool IsValidGestureCommand(int gesture_command);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_COMMAND_UTIL_H_
