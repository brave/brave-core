// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_INPUT_TOUCH_CENTROID_UTIL_H_
#define BRAVE_BROWSER_UI_GESTURES_INPUT_TOUCH_CENTROID_UTIL_H_

#include "third_party/blink/public/common/input/web_touch_event.h"
#include "ui/gfx/geometry/point_f.h"

namespace gestures {

// Returns the centroid (the average position, i.e. the geometric center) of
// `web_touch_event`'s currently-active touches, so a multi-finger gesture has
// one location to start/continue from. Excludes touches that have already
// lifted. Falls back to the first touch's position if none are active, or the
// origin if there are no touches at all.
gfx::PointF TouchCentroid(const blink::WebTouchEvent& web_touch_event);

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_INPUT_TOUCH_CENTROID_UTIL_H_
