// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_INPUT_TOUCH_TOUCH_CENTROID_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_INPUT_TOUCH_TOUCH_CENTROID_UTIL_H_

#include "third_party/blink/public/common/input/web_touch_event.h"
#include "ui/gfx/geometry/point_f.h"

namespace gestures {

// Centroid of `web_touch_event`'s active touches; falls back to the first
// touch's position, or the origin, if none are active.
gfx::PointF TouchCentroid(const blink::WebTouchEvent& web_touch_event);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_INPUT_TOUCH_TOUCH_CENTROID_UTIL_H_
