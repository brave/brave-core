// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/session/gesture_mouse_modifier_config_tracker.h"

#include <string_view>

#include "base/functional/bind.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "brave/components/gestures/core/common/actions/gesture_action_parser.h"
#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/input/mouse/mouse_wheel_modifier_button_util.h"
#include "chrome/app/chrome_command_ids.h"
#include "components/prefs/pref_service.h"

namespace gestures {

GestureMouseModifierConfigTracker::GestureMouseModifierConfigTracker(
    PrefService& prefs)
    : prefs_(prefs) {
  LoadMouseRockerGestures();
  LoadMouseWheelGestures();

  pref_change_registrar_.Init(&*prefs_);
  for (std::string_view pref_name :
       {prefs::kMouseRockerGesturesEnabled, prefs::kMouseRockerLeftAction,
        prefs::kMouseRockerRightAction}) {
    pref_change_registrar_.Add(
        pref_name,
        base::BindRepeating(
            &GestureMouseModifierConfigTracker::LoadMouseRockerGestures,
            weak_factory_.GetWeakPtr()));
  }
  for (std::string_view pref_name :
       {prefs::kMouseWheelGesturesEnabled, prefs::kMouseWheelModifierButton,
        prefs::kMouseWheelSensitivityInPixels, prefs::kMouseWheelUpAction,
        prefs::kMouseWheelDownAction}) {
    pref_change_registrar_.Add(
        pref_name,
        base::BindRepeating(
            &GestureMouseModifierConfigTracker::LoadMouseWheelGestures,
            weak_factory_.GetWeakPtr()));
  }
}

GestureMouseModifierConfigTracker::~GestureMouseModifierConfigTracker() =
    default;

bool GestureMouseModifierConfigTracker::IsMouseRockerGesturesEnabled() const {
  return prefs_->GetBoolean(prefs::kMouseRockerGesturesEnabled);
}

bool GestureMouseModifierConfigTracker::IsMouseWheelGesturesEnabled() const {
  return prefs_->GetBoolean(prefs::kMouseWheelGesturesEnabled);
}

MouseWheelModifierButtonType
GestureMouseModifierConfigTracker::MouseWheelModifierButton() const {
  const int raw_wheel_modifier_button =
      prefs_->GetInteger(prefs::kMouseWheelModifierButton);
  return IsValidMouseWheelModifierButton(raw_wheel_modifier_button)
             ? static_cast<MouseWheelModifierButtonType>(
                   raw_wheel_modifier_button)
             : MouseWheelModifierButtonType::kRight;
}

double GestureMouseModifierConfigTracker::MouseWheelSensitivityInPixels()
    const {
  return prefs_->GetDouble(prefs::kMouseWheelSensitivityInPixels);
}

void GestureMouseModifierConfigTracker::LoadMouseRockerGestures() {
  mouse_rocker_left_actions_ =
      MaybeParseGestureActions(prefs_->GetList(prefs::kMouseRockerLeftAction))
          .value_or({MakeGestureAction(IDC_BACK)});
  mouse_rocker_right_actions_ =
      MaybeParseGestureActions(prefs_->GetList(prefs::kMouseRockerRightAction))
          .value_or({MakeGestureAction(IDC_FORWARD)});
}

void GestureMouseModifierConfigTracker::LoadMouseWheelGestures() {
  mouse_wheel_up_actions_ =
      MaybeParseGestureActions(prefs_->GetList(prefs::kMouseWheelUpAction))
          .value_or({MakeGestureAction(IDC_SELECT_PREVIOUS_TAB)});
  mouse_wheel_down_actions_ =
      MaybeParseGestureActions(prefs_->GetList(prefs::kMouseWheelDownAction))
          .value_or({MakeGestureAction(IDC_SELECT_NEXT_TAB)});
}

}  // namespace gestures
