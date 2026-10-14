// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_DEFAULTS_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_DEFAULTS_H_

#include "brave/components/gestures/core/common/directional/directional_gesture_default_commands_info.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_mapping.h"

namespace gestures {

// The built-in gesture-to-actions bindings, keyed by
// `DirectionalGestureStrokeToKeyCode`-coded stroke sequences.
DirectionalGestureMappings DefaultMappings(
    const DirectionalGestureDefaultCommandsInfo& commands);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_DEFAULTS_H_
