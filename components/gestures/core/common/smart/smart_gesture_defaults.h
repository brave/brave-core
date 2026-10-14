// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_DEFAULTS_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_DEFAULTS_H_

#include "brave/components/gestures/core/common/smart/smart_gesture_mapping.h"

namespace gestures {

// The out-of-the-box `SmartGestureMapping` table, keyed by `ToSmartGestureKey`.
// Every slot not listed here defaults to unassigned.
SmartGestureMappings DefaultSmartGestureMappings();

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_DEFAULTS_H_
