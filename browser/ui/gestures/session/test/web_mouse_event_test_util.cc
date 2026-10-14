// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/session/test/web_mouse_event_test_util.h"

#include "third_party/blink/public/common/input/synthetic_web_input_event_builders.h"

namespace gestures::test {

blink::WebMouseEvent MakeWebMouseButtonEvent(
    blink::WebMouseEvent::Type type,
    blink::WebMouseEvent::Button button,
    int modifiers) {
  blink::WebMouseEvent mouse_event =
      blink::SyntheticWebMouseEventBuilder::Build(type, /*window_x=*/0,
                                                  /*window_y=*/0, modifiers);
  mouse_event.button = button;
  mouse_event.click_count = 1;
  return mouse_event;
}

blink::WebMouseWheelEvent MakeWebMouseWheelEvent(
    float delta_y,
    int modifiers,
    ui::ScrollGranularity delta_units) {
  return blink::SyntheticWebMouseWheelEventBuilder::Build(
      /*x=*/0, /*y=*/0, /*dx=*/0, delta_y, modifiers, delta_units);
}

}  // namespace gestures::test
