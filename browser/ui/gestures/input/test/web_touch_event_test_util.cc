// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/input/test/web_touch_event_test_util.h"

namespace gestures::test {

blink::WebTouchPoint MakeTouchPoint(float x,
                                    float y,
                                    blink::WebTouchPoint::State state) {
  blink::WebTouchPoint touch_point;
  touch_point.SetPositionInWidget(x, y);
  touch_point.state = state;
  return touch_point;
}

blink::WebTouchEvent MakeWebTouchEvent(
    const std::vector<blink::WebTouchPoint>& touches) {
  blink::WebTouchEvent web_touch_event;
  web_touch_event.touches_length = touches.size();
  for (size_t i = 0; i < touches.size(); ++i) {
    web_touch_event.touches[i] = touches[i];
  }
  return web_touch_event;
}

}  // namespace gestures::test
