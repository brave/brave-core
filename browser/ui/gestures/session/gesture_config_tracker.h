// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_CONFIG_TRACKER_H_
#define BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_CONFIG_TRACKER_H_

#include <string_view>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_recognizer.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_mapping.h"
#include "brave/components/gestures/core/common/mouse/wheel/mouse_wheel_modifier_button_types.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_mapping.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace gestures {

// Loads gesture settings from `prefs` on construction and keeps them in sync
// with each relevant pref change, for the lifetime of this object. Owned by
// `GestureController`, one instance per tab.
class GestureConfigTracker final {
 public:
  // `prefs` is not owned, and must outlive this object; `GestureController`
  // gets it from its own tab's `BrowserContext`, which outlives it.
  explicit GestureConfigTracker(PrefService& prefs);

  GestureConfigTracker(const GestureConfigTracker&) = delete;
  GestureConfigTracker& operator=(const GestureConfigTracker&) = delete;

  ~GestureConfigTracker();

  // Whether gestures may fire on `scheme`/`host` (both already lowercase),
  // checked against the user's disallowed-sites pref.
  bool IsAllowedForHost(std::string_view scheme, std::string_view host) const;

  const DirectionalGestureMapping& directional_gesture_mapping() const {
    return directional_gesture_mapping_;
  }

  const CustomGestureRecognizer& custom_gesture_recognizer() const {
    return custom_gesture_recognizer_;
  }

  const SmartGestureMapping& smart_gesture_mapping() const {
    return smart_gesture_mapping_;
  }

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
  void LoadDirectionalGestures();
  void LoadCustomGestures();
  void LoadSmartGestures();
  void LoadMouseRockerGestures();
  void LoadMouseWheelGestures();

  const raw_ref<PrefService> prefs_;
  PrefChangeRegistrar pref_change_registrar_;

  DirectionalGestureMapping directional_gesture_mapping_;
  CustomGestureRecognizer custom_gesture_recognizer_;
  SmartGestureMapping smart_gesture_mapping_;

  GestureActions mouse_rocker_left_actions_;
  GestureActions mouse_rocker_right_actions_;

  GestureActions mouse_wheel_up_actions_;
  GestureActions mouse_wheel_down_actions_;

  base::WeakPtrFactory<GestureConfigTracker> weak_factory_{this};
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_CONFIG_TRACKER_H_
