// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_INPUT_TEST_WEB_TOUCH_EVENT_TEST_UTIL_H_
#define BRAVE_BROWSER_UI_GESTURES_INPUT_TEST_WEB_TOUCH_EVENT_TEST_UTIL_H_

#include <vector>

#include "third_party/blink/public/common/input/web_touch_event.h"

namespace gestures::test {

blink::WebTouchPoint MakeTouchPoint(float x,
                                    float y,
                                    blink::WebTouchPoint::State state);

blink::WebTouchEvent MakeWebTouchEvent(
    const std::vector<blink::WebTouchPoint>& touches);

}  // namespace gestures::test

#endif  // BRAVE_BROWSER_UI_GESTURES_INPUT_TEST_WEB_TOUCH_EVENT_TEST_UTIL_H_
