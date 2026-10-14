// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_MOUSE_WHEEL_RECOGNIZER_H_
#define BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_MOUSE_WHEEL_RECOGNIZER_H_

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"

namespace blink {
class WebMouseWheelEvent;
}  // namespace blink

namespace gestures {

class GestureActionDispatcher;
class GestureMouseModifierConfigTracker;

// Tracks accumulated wheel movement while the modifier button is held,
// firing the mapped actions once it crosses the sensitivity threshold.
class GestureMouseWheelRecognizer final {
 public:
  // Only fires for the right mouse button, since it also drives directional
  // drawing, letting the owner cancel any draw and suppress the context menu.
  GestureMouseWheelRecognizer(
      const GestureMouseModifierConfigTracker& config_tracker,
      GestureActionDispatcher& action_dispatcher,
      base::RepeatingClosure right_button_fired_callback);

  GestureMouseWheelRecognizer(const GestureMouseWheelRecognizer&) = delete;
  GestureMouseWheelRecognizer& operator=(const GestureMouseWheelRecognizer&) =
      delete;

  ~GestureMouseWheelRecognizer();

  // Resets accumulated scroll state; a threshold measured against one widget
  // shouldn't carry over to a new one.
  void Reset();

  void HandleMouseWheelEvent(
      const blink::WebMouseWheelEvent& web_mouse_wheel_event);

 private:
  const raw_ref<const GestureMouseModifierConfigTracker> config_tracker_;
  const raw_ref<GestureActionDispatcher> action_dispatcher_;
  base::RepeatingClosure right_button_fired_callback_;

  // Accumulated while the modifier button is held, so a gesture fires once
  // the total crosses the configured sensitivity, not on every tiny event.
  double wheel_delta_accumulator_ = 0.0;
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_MOUSE_WHEEL_RECOGNIZER_H_
