// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_KEY_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_KEY_UTIL_H_

#include <string>

#include "brave/components/gestures/core/common/smart/smart_gesture_content_type.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_direction_util.h"

namespace gestures {

// A `SmartGestureMapping` key, e.g. `"link:right"`, combining which content
// type and which swipe direction a smart gesture's action is assigned to.
std::string SmartGestureKey(SmartGestureContentType content_type,
                            SmartGestureDirectionType direction_type);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_KEY_UTIL_H_
