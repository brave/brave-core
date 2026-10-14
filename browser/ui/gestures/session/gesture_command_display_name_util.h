// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_COMMAND_DISPLAY_NAME_UTIL_H_
#define BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_COMMAND_DISPLAY_NAME_UTIL_H_

#include <string>
#include <string_view>

#include "brave/components/gestures/core/common/actions/gesture_action_info.h"

namespace gestures {

// Separator `ToGestureActionsDisplayName` joins chained actions' display names
// with.
inline constexpr std::u16string_view kActionsDisplayNameSeparator = u" \u203A ";

// Localized display name for `gesture_command_id`, e.g. for the gesture
// overlay label.
std::u16string ToGestureCommandDisplayName(int gesture_command_id);

// Every chained action's own display name, joined with a chevron, e.g. for
// the gesture overlay label.
std::u16string ToGestureActionsDisplayName(const GestureActions& actions);

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_COMMAND_DISPLAY_NAME_UTIL_H_
