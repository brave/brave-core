// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/input/touch_centroid_util.h"

#include "base/containers/span.h"
#include "brave/browser/ui/gestures/input/touch_point_state_util.h"

namespace gestures {

gfx::PointF TouchCentroid(const blink::WebTouchEvent& web_touch_event) {
  // Excludes touches that have already lifted, such as one finger of a
  // two-finger drag releasing before the other on `TouchEnd`. Their last known
  // position no longer reflects where the user is currently pointing, which
  // would otherwise pull the centroid toward a stale location.
  float x = 0;
  float y = 0;
  float active_count = 0;
  for (const blink::WebTouchPoint& touch :
       base::span(web_touch_event.touches)
           .first(web_touch_event.touches_length)) {
    if (!IsActiveTouchState(touch.state)) {
      continue;
    }
    x += touch.PositionInWidget().x();
    y += touch.PositionInWidget().y();
    ++active_count;
  }
  // TouchStart/TouchMove should always carry at least one active touch; this is
  // only a defensive fallback against dividing by zero.
  if (active_count == 0) {
    return web_touch_event.touches_length > 0
               ? web_touch_event.touches.at(0).PositionInWidget()
               : gfx::PointF();
  }
  return {x / active_count, y / active_count};
}

}  // namespace gestures
