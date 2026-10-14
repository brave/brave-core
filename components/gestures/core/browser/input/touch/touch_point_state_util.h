// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_INPUT_TOUCH_TOUCH_POINT_STATE_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_INPUT_TOUCH_TOUCH_POINT_STATE_UTIL_H_

#include "third_party/blink/public/common/input/web_touch_point.h"

namespace gestures {

// True while a touch point is still down (pressed, moved, or stationary),
// false once it has lifted, cancelled, or was never set.
bool IsActiveTouchState(blink::WebTouchPoint::State state);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_INPUT_TOUCH_TOUCH_POINT_STATE_UTIL_H_
