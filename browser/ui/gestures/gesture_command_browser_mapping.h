// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_GESTURE_COMMAND_BROWSER_MAPPING_H_
#define BRAVE_BROWSER_UI_GESTURES_GESTURE_COMMAND_BROWSER_MAPPING_H_

#include <optional>
#include <string>

#include "brave/components/gestures/core/common/actions/gesture_command_types.h"

namespace gestures {

// Maps the Brave-owned command enum to the browser's underlying `IDC_*`
// command id, defined in chrome/app or brave/app.
std::optional<int> MaybeToGestureCommandId(GestureCommandType gesture_command);

// Localized display name for `gesture_command`, e.g. for the gesture overlay
// label.
std::u16string ToDisplayName(GestureCommandType gesture_command);

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_GESTURE_COMMAND_BROWSER_MAPPING_H_
