// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/session/gesture_mouse_wheel_recognizer.h"

#include <cmath>
#include <utility>

#include "brave/browser/ui/gestures/dispatch/gesture_action_dispatcher.h"
#include "brave/browser/ui/gestures/session/gesture_config_tracker.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/common/input/web_mouse_wheel_event.h"
#include "ui/events/types/scroll_types.h"

namespace gestures {

GestureMouseWheelRecognizer::GestureMouseWheelRecognizer(
    const GestureConfigTracker& config_tracker,
    GestureActionDispatcher& action_dispatcher,
    base::RepeatingClosure on_right_button_fired)
    : config_tracker_(config_tracker),
      action_dispatcher_(action_dispatcher),
      on_right_button_fired_(std::move(on_right_button_fired)) {}

GestureMouseWheelRecognizer::~GestureMouseWheelRecognizer() = default;

void GestureMouseWheelRecognizer::Reset() {
  wheel_delta_accumulator_ = 0;
}

void GestureMouseWheelRecognizer::HandleEvent(
    const blink::WebMouseWheelEvent& web_mouse_wheel_event) {
  // Read live from the event's own modifiers, rather than a separately
  // tracked "is it held" bool, since a command this gesture dispatches
  // (e.g. Previous Tab) can switch the active tab or otherwise mean this
  // tab never sees the real mouse-up, which would leave a tracked bool
  // stuck true indefinitely.
  const int modifiers = web_mouse_wheel_event.GetModifiers();
  const MouseWheelModifierButtonType wheel_modifier_button =
      config_tracker_->MouseWheelModifierButton();
  const bool is_modifier_button_down =
      (wheel_modifier_button == MouseWheelModifierButtonType::kLeft &&
       (modifiers & blink::WebInputEvent::kLeftButtonDown) != 0) ||
      (wheel_modifier_button == MouseWheelModifierButtonType::kMiddle &&
       (modifiers & blink::WebInputEvent::kMiddleButtonDown) != 0) ||
      (wheel_modifier_button == MouseWheelModifierButtonType::kRight &&
       (modifiers & blink::WebInputEvent::kRightButtonDown) != 0);
  if (!is_modifier_button_down) {
    wheel_delta_accumulator_ = 0;
    return;
  }
  if (web_mouse_wheel_event.delta_y == 0) {
    return;
  }
  wheel_delta_accumulator_ += web_mouse_wheel_event.delta_y;
  // `MouseWheelSensitivityInPixels()` is calibrated in pixels, but
  // `delta_y` is
  // only in pixels when `delta_units` says so; a page-granularity delta (e.g.
  // Windows' "scroll one screen at a time" mouse setting) is already a
  // single deliberate action, not a magnitude comparable to a pixel
  // threshold, so it always counts rather than being measured against one.
  const bool is_pixel_granularity =
      web_mouse_wheel_event.delta_units ==
          ui::ScrollGranularity::kScrollByPixel ||
      web_mouse_wheel_event.delta_units ==
          ui::ScrollGranularity::kScrollByPrecisePixel;
  if (is_pixel_granularity &&
      std::abs(wheel_delta_accumulator_) <
          config_tracker_->MouseWheelSensitivityInPixels()) {
    return;
  }
  // The right mouse button also drives directional-gesture drawing and the
  // native context menu; without this, releasing it after a wheel action
  // would fall through to the owner's plain-click path and show the context
  // menu, the same reason rocker gestures suppress it.
  if (wheel_modifier_button == MouseWheelModifierButtonType::kRight) {
    on_right_button_fired_.Run();
  }
  const bool wheel_moved_up = wheel_delta_accumulator_ > 0;
  action_dispatcher_->DispatchActionsAsync(
      wheel_moved_up ? config_tracker_->mouse_wheel_up_actions()
                     : config_tracker_->mouse_wheel_down_actions(),
      /*smart_gesture_content=*/std::nullopt);
  wheel_delta_accumulator_ = 0;
}

}  // namespace gestures
