// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_MOUSE_MODIFIER_CONFIG_TRACKER_H_
#define BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_MOUSE_MODIFIER_CONFIG_TRACKER_H_

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/input/mouse/mouse_wheel_modifier_button_types.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace gestures {

// Loads mouse rocker/wheel gesture settings from `prefs` and keeps them in
// sync with each relevant pref change.
class GestureMouseModifierConfigTracker final {
 public:
  // `prefs` is not owned, and must outlive this object; `GestureController`
  // gets it from its own tab's `BrowserContext`, which outlives it.
  explicit GestureMouseModifierConfigTracker(PrefService& prefs);

  GestureMouseModifierConfigTracker(const GestureMouseModifierConfigTracker&) =
      delete;
  GestureMouseModifierConfigTracker& operator=(
      const GestureMouseModifierConfigTracker&) = delete;

  ~GestureMouseModifierConfigTracker();

  bool IsMouseRockerGesturesEnabled() const;

  const GestureActions& mouse_rocker_left_actions() const {
    return mouse_rocker_left_actions_;
  }

  const GestureActions& mouse_rocker_right_actions() const {
    return mouse_rocker_right_actions_;
  }

  bool IsMouseWheelGesturesEnabled() const;

  MouseWheelModifierButtonType MouseWheelModifierButton() const;

  double MouseWheelSensitivityInPixels() const;

  const GestureActions& mouse_wheel_up_actions() const {
    return mouse_wheel_up_actions_;
  }

  const GestureActions& mouse_wheel_down_actions() const {
    return mouse_wheel_down_actions_;
  }

 private:
  void LoadMouseRockerGestures();
  void LoadMouseWheelGestures();

  const raw_ref<PrefService> prefs_;
  PrefChangeRegistrar pref_change_registrar_;

  GestureActions mouse_rocker_left_actions_;
  GestureActions mouse_rocker_right_actions_;

  GestureActions mouse_wheel_up_actions_;
  GestureActions mouse_wheel_down_actions_;

  base::WeakPtrFactory<GestureMouseModifierConfigTracker> weak_factory_{this};
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_MOUSE_MODIFIER_CONFIG_TRACKER_H_
