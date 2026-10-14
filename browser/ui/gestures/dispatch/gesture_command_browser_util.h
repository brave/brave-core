// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_DISPATCH_GESTURE_COMMAND_BROWSER_UTIL_H_
#define BRAVE_BROWSER_UI_GESTURES_DISPATCH_GESTURE_COMMAND_BROWSER_UTIL_H_

#include <optional>
#include <string>

#include "brave/components/gestures/core/common/actions/gesture_action_info.h"

namespace gestures {

// Separator `ToActionsDisplayName` joins chained actions' display names with.
inline constexpr char16_t kActionsDisplayNameSeparator[] = u" › ";

// Maps `gesture_command_id` to its `IDC_*` id, or `nullopt` for one of the 4
// sentinels `GestureActionDispatcher` handles directly instead of via
// `ExecuteCommand`.
std::optional<int> MaybeToGestureCommandId(int gesture_command_id);

// Localized display name for `gesture_command_id`, e.g. for the gesture
// overlay label.
std::u16string ToCommandDisplayName(int gesture_command_id);

// Every chained action's own display name, joined with a chevron, e.g. for
// the gesture overlay label.
std::u16string ToActionsDisplayName(const GestureActions& actions);

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_DISPATCH_GESTURE_COMMAND_BROWSER_UTIL_H_
