// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_TEST_GESTURE_MOUSE_ROCKER_TEST_UTIL_H_
#define BRAVE_BROWSER_UI_GESTURES_TEST_GESTURE_MOUSE_ROCKER_TEST_UTIL_H_

#include "third_party/blink/public/common/input/web_mouse_event.h"

namespace gestures {

class GestureMouseRockerRecognizer;

namespace test {

bool HandleMouseDown(GestureMouseRockerRecognizer& mouse_rocker_recognizer,
                     const blink::WebMouseEvent& web_mouse_event);

}  // namespace test

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_TEST_GESTURE_MOUSE_ROCKER_TEST_UTIL_H_
