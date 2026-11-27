// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/session/gesture_mouse_rocker_recognizer.h"

#include <utility>

#include "base/functional/callback_helpers.h"
#include "brave/browser/ui/gestures/dispatch/gesture_action_dispatcher.h"
#include "brave/browser/ui/gestures/session/gesture_mouse_modifier_config_tracker.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"

namespace gestures {

GestureMouseRockerRecognizer::GestureMouseRockerRecognizer(
    const GestureMouseModifierConfigTracker& config_tracker,
    GestureActionDispatcher& action_dispatcher,
    base::RepeatingCallback<bool()> is_allowed_for_site_callback,
    base::RepeatingClosure fired_callback)
    : config_tracker_(config_tracker),
      action_dispatcher_(action_dispatcher),
      is_allowed_for_site_callback_(std::move(is_allowed_for_site_callback)),
      fired_callback_(std::move(fired_callback)) {}

GestureMouseRockerRecognizer::~GestureMouseRockerRecognizer() = default;

bool GestureMouseRockerRecognizer::HandleMouseDownEvent(
    const blink::WebMouseEvent& web_mouse_event) {
  const bool enabled = config_tracker_->IsMouseRockerGesturesEnabled();
  const bool allowed = is_allowed_for_site_callback_.Run();
  const int modifiers = web_mouse_event.GetModifiers();
  const bool left_button_down =
      (modifiers & blink::WebInputEvent::kLeftButtonDown) != 0;
  const bool right_button_down =
      (modifiers & blink::WebInputEvent::kRightButtonDown) != 0;
  if (!enabled || !allowed) {
    return false;
  }

  if (web_mouse_event.button == blink::WebMouseEvent::Button::kRight &&
      left_button_down) {
    // Rocker gesture. Holding the left button and clicking the right one.
    fired_callback_.Run();
    action_dispatcher_->DispatchActionsAsync(
        config_tracker_->mouse_rocker_right_actions(),
        /*smart_gesture_content=*/std::nullopt, base::DoNothing());
    return true;
  }
  if (web_mouse_event.button == blink::WebMouseEvent::Button::kLeft &&
      right_button_down) {
    // Rocker gesture. Holding the right button and clicking the left one.
    fired_callback_.Run();
    action_dispatcher_->DispatchActionsAsync(
        config_tracker_->mouse_rocker_left_actions(),
        /*smart_gesture_content=*/std::nullopt, base::DoNothing());
    return true;
  }
  return false;
}

}  // namespace gestures
