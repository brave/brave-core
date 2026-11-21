// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_MOUSE_ROCKER_RECOGNIZER_H_
#define BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_MOUSE_ROCKER_RECOGNIZER_H_

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"

namespace blink {
class WebMouseEvent;
}  // namespace blink

namespace gestures {

class GestureActionDispatcher;
class GestureMouseModifierConfigTracker;

// Detects rocker gestures, pressing the other mouse button while one is
// already held.
class GestureMouseRockerRecognizer final {
 public:
  // `fired_callback` runs just before dispatch, so the owner can cancel
  // any in-progress draw and suppress the held button's context menu.
  GestureMouseRockerRecognizer(
      const GestureMouseModifierConfigTracker& config_tracker,
      GestureActionDispatcher& action_dispatcher,
      base::RepeatingCallback<bool()> is_allowed_for_site_callback,
      base::RepeatingClosure fired_callback);

  GestureMouseRockerRecognizer(const GestureMouseRockerRecognizer&) = delete;
  GestureMouseRockerRecognizer& operator=(const GestureMouseRockerRecognizer&) =
      delete;

  ~GestureMouseRockerRecognizer();

  // Returns true if `web_mouse_event` was consumed as a rocker gesture.
  bool HandleMouseDownEvent(const blink::WebMouseEvent& web_mouse_event);

 private:
  const raw_ref<const GestureMouseModifierConfigTracker> config_tracker_;
  const raw_ref<GestureActionDispatcher> action_dispatcher_;
  base::RepeatingCallback<bool()> is_allowed_for_site_callback_;
  base::RepeatingClosure fired_callback_;
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_MOUSE_ROCKER_RECOGNIZER_H_
