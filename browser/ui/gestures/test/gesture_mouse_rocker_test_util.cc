// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/test/gesture_mouse_rocker_test_util.h"

#include "brave/browser/ui/gestures/session/gesture_mouse_rocker_recognizer.h"

namespace gestures::test {

bool HandleMouseDown(GestureMouseRockerRecognizer& mouse_rocker_recognizer,
                     const blink::WebMouseEvent& web_mouse_event) {
  return mouse_rocker_recognizer.HandleMouseDown(web_mouse_event);
}

}  // namespace gestures::test
