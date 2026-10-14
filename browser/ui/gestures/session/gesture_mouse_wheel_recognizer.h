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
class GestureConfigTracker;

// Tracks accumulated wheel movement while the configured modifier button is
// held, firing the mapped chained actions once it crosses the configured
// sensitivity threshold.
class GestureMouseWheelRecognizer final {
 public:
  // `on_right_button_fired` runs just before dispatch, only when the
  // configured modifier button is the right mouse button, since that button
  // also drives directional-gesture drawing and the native context menu, so
  // the owner needs to cancel any in-progress draw and suppress the menu
  // show that would otherwise follow this button's release.
  GestureMouseWheelRecognizer(const GestureConfigTracker& config_tracker,
                              GestureActionDispatcher& action_dispatcher,
                              base::RepeatingClosure on_right_button_fired);

  GestureMouseWheelRecognizer(const GestureMouseWheelRecognizer&) = delete;
  GestureMouseWheelRecognizer& operator=(const GestureMouseWheelRecognizer&) =
      delete;

  ~GestureMouseWheelRecognizer();

  // Resets state tied to the previously-observed widget's scroll stream; a
  // pref-driven threshold accumulated against one widget shouldn't carry over
  // to a new one.
  void Reset();

  void HandleEvent(const blink::WebMouseWheelEvent& web_mouse_wheel_event);

 private:
  const raw_ref<const GestureConfigTracker> config_tracker_;
  const raw_ref<GestureActionDispatcher> action_dispatcher_;
  base::RepeatingClosure on_right_button_fired_;

  // Accumulates `WebMouseWheelEvent::delta_y` while the modifier button is
  // held, so a wheel gesture fires once the total crosses the configured
  // sensitivity rather than on every, possibly tiny, individual event a
  // precision trackpad or high-resolution wheel can produce. Reset to 0
  // whenever the modifier button isn't held.
  double wheel_delta_accumulator_ = 0;
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_MOUSE_WHEEL_RECOGNIZER_H_
