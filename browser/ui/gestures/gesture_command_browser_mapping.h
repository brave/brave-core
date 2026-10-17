// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_GESTURE_COMMAND_BROWSER_MAPPING_H_
#define BRAVE_BROWSER_UI_GESTURES_GESTURE_COMMAND_BROWSER_MAPPING_H_

#include <optional>
#include <string>

namespace gestures {

// Maps `command` to the browser's underlying `IDC_*` command id, or
// `nullopt` if `command` isn't dispatched through `chrome::ExecuteCommand`
// (one of the 4 sentinels in `gesture_command_constants.h`, each handled
// directly by `GestureActionDispatcher` instead).
std::optional<int> MaybeToGestureCommandId(int command);

// Localized display name for `command`, e.g. for the gesture overlay label.
std::u16string ToDisplayName(int command);

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_GESTURE_COMMAND_BROWSER_MAPPING_H_
