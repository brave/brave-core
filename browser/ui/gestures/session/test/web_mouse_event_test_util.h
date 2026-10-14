// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_SESSION_TEST_WEB_MOUSE_EVENT_TEST_UTIL_H_
#define BRAVE_BROWSER_UI_GESTURES_SESSION_TEST_WEB_MOUSE_EVENT_TEST_UTIL_H_

#include "third_party/blink/public/common/input/web_mouse_event.h"
#include "third_party/blink/public/common/input/web_mouse_wheel_event.h"
#include "ui/events/types/scroll_types.h"

namespace gestures::test {

blink::WebMouseEvent MakeWebMouseButtonEvent(
    blink::WebMouseEvent::Type type,
    blink::WebMouseEvent::Button button,
    int modifiers);

blink::WebMouseWheelEvent MakeWebMouseWheelEvent(
    float delta_y,
    int modifiers,
    ui::ScrollGranularity delta_units = ui::ScrollGranularity::kScrollByPixel);

}  // namespace gestures::test

#endif  // BRAVE_BROWSER_UI_GESTURES_SESSION_TEST_WEB_MOUSE_EVENT_TEST_UTIL_H_
