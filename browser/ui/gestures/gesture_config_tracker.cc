// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_config_tracker.h"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "brave/components/gestures/core/common/actions/gesture_action_pref_parser.h"
#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/actions/gesture_command_types.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_info.h"
#include "brave/components/gestures/core/common/mouse/wheel/mouse_wheel_modifier_button_util.h"
#include "components/prefs/pref_service.h"

namespace gestures {

GestureConfigTracker::GestureConfigTracker(PrefService& prefs) : prefs_(prefs) {
  LoadDirectionalGestures();
  LoadCustomGestures();
  LoadMouseRockerSettings();
  LoadMouseWheelSettings();

  pref_change_registrar_.Init(&*prefs_);
  pref_change_registrar_.Add(
      prefs::kDirectionalGestures,
      base::BindRepeating(&GestureConfigTracker::LoadDirectionalGestures,
                          weak_factory_.GetWeakPtr()));
  pref_change_registrar_.Add(
      prefs::kCustomGestures,
      base::BindRepeating(&GestureConfigTracker::LoadCustomGestures,
                          weak_factory_.GetWeakPtr()));
  for (const std::string_view pref_name :
       {prefs::kMouseRockerGesturesEnabled, prefs::kMouseRockerLeftAction,
        prefs::kMouseRockerRightAction}) {
    pref_change_registrar_.Add(
        pref_name,
        base::BindRepeating(&GestureConfigTracker::LoadMouseRockerSettings,
                            weak_factory_.GetWeakPtr()));
  }
  for (const std::string_view pref_name :
       {prefs::kMouseWheelGesturesEnabled, prefs::kMouseWheelModifierButton,
        prefs::kMouseWheelSensitivityInPixels, prefs::kMouseWheelUpAction,
        prefs::kMouseWheelDownAction}) {
    pref_change_registrar_.Add(
        pref_name,
        base::BindRepeating(&GestureConfigTracker::LoadMouseWheelSettings,
                            weak_factory_.GetWeakPtr()));
  }
}

GestureConfigTracker::~GestureConfigTracker() = default;

bool GestureConfigTracker::IsAllowedForHost(std::string_view host) const {
  if (!prefs_->GetBoolean(prefs::kEnabled)) {
    return false;
  }

  // `disallowed_host` comes straight from the stored pref, which syncs
  // across devices and isn't guaranteed to have gone through the settings
  // UI's own lowercasing; lowercase it here too so a mixed-case stored
  // value doesn't silently fail to match and fail open on the user's own
  // disallow-list.
  return std::ranges::none_of(
      prefs_->GetDict(prefs::kDisallowedSites), [&host](const auto& entry) {
        const auto& [disallowed_host_key, value] = entry;
        if (!value.GetIfBool().value_or(false)) {
          return false;
        }
        const std::string disallowed_host =
            base::ToLowerASCII(disallowed_host_key);
        return host == disallowed_host ||
               (host.size() > disallowed_host.size() &&
                host.ends_with(disallowed_host) &&
                host[host.size() - disallowed_host.size() - 1] == '.');
      });
}

void GestureConfigTracker::LoadDirectionalGestures() {
  directional_gesture_mapping_.ResetToDefaults();

  for (const auto [gesture_key, actions_value] :
       prefs_->GetDict(prefs::kDirectionalGestures)) {
    const base::ListValue* actions_list = actions_value.GetIfList();
    std::optional<std::vector<GestureActionInfo>> actions =
        actions_list ? MaybeParseGestureActions(*actions_list) : std::nullopt;
    if (actions) {
      directional_gesture_mapping_.SetChainedActions(gesture_key,
                                                     std::move(*actions));
    }
  }
}

void GestureConfigTracker::LoadCustomGestures() {
  custom_gesture_recognizer_.Clear();

  for (const base::Value& custom_gesture_value :
       prefs_->GetList(prefs::kCustomGestures)) {
    const base::DictValue* custom_gesture_dict =
        custom_gesture_value.GetIfDict();
    if (!custom_gesture_dict) {
      continue;
    }
    if (std::optional<CustomGestureInfo> custom_gesture =
            MaybeParseCustomGesture(*custom_gesture_dict)) {
      custom_gesture_recognizer_.Add(std::move(*custom_gesture));
    }
  }
}

bool GestureConfigTracker::IsMouseRockerGesturesEnabled() const {
  return prefs_->GetBoolean(prefs::kMouseRockerGesturesEnabled);
}

bool GestureConfigTracker::IsMouseWheelGesturesEnabled() const {
  return prefs_->GetBoolean(prefs::kMouseWheelGesturesEnabled);
}

MouseWheelModifierButtonType GestureConfigTracker::MouseWheelModifierButton()
    const {
  const int raw_wheel_modifier_button =
      prefs_->GetInteger(prefs::kMouseWheelModifierButton);
  return IsValidMouseWheelModifierButton(raw_wheel_modifier_button)
             ? static_cast<MouseWheelModifierButtonType>(
                   raw_wheel_modifier_button)
             : MouseWheelModifierButtonType::kRight;
}

double GestureConfigTracker::MouseWheelSensitivityInPixels() const {
  return prefs_->GetDouble(prefs::kMouseWheelSensitivityInPixels);
}

void GestureConfigTracker::LoadMouseRockerSettings() {
  mouse_rocker_left_actions_ =
      MaybeParseGestureActions(prefs_->GetList(prefs::kMouseRockerLeftAction))
          .value_or(MakeGestureActions(GestureCommandType::kBack));
  mouse_rocker_right_actions_ =
      MaybeParseGestureActions(prefs_->GetList(prefs::kMouseRockerRightAction))
          .value_or(MakeGestureActions(GestureCommandType::kForward));
}

void GestureConfigTracker::LoadMouseWheelSettings() {
  mouse_wheel_up_actions_ =
      MaybeParseGestureActions(prefs_->GetList(prefs::kMouseWheelUpAction))
          .value_or(MakeGestureActions(GestureCommandType::kPreviousTab));
  mouse_wheel_down_actions_ =
      MaybeParseGestureActions(prefs_->GetList(prefs::kMouseWheelDownAction))
          .value_or(MakeGestureActions(GestureCommandType::kNextTab));
}

}  // namespace gestures
