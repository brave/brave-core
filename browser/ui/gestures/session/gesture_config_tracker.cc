// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/session/gesture_config_tracker.h"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/browser/ui/gestures/session/default_directional_gesture_commands_util.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "brave/components/gestures/core/common/actions/gesture_action_pref_parser.h"
#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_info.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_pref_parser.h"
#include "brave/components/gestures/core/common/mouse/wheel/mouse_wheel_modifier_button_util.h"
#include "chrome/app/chrome_command_ids.h"
#include "components/prefs/pref_service.h"

namespace gestures {

GestureConfigTracker::GestureConfigTracker(PrefService& prefs)
    : prefs_(prefs),
      directional_gesture_mapping_(DefaultDirectionalGestureCommands()) {
  LoadDirectionalGestures();
  LoadCustomGestures();
  LoadSmartGestures();
  LoadMouseRockerGestures();
  LoadMouseWheelGestures();

  pref_change_registrar_.Init(&*prefs_);
  pref_change_registrar_.Add(
      prefs::kDirectionalGestures,
      base::BindRepeating(&GestureConfigTracker::LoadDirectionalGestures,
                          weak_factory_.GetWeakPtr()));
  pref_change_registrar_.Add(
      prefs::kCustomGestures,
      base::BindRepeating(&GestureConfigTracker::LoadCustomGestures,
                          weak_factory_.GetWeakPtr()));
  pref_change_registrar_.Add(
      prefs::kSmartGestures,
      base::BindRepeating(&GestureConfigTracker::LoadSmartGestures,
                          weak_factory_.GetWeakPtr()));
  for (const std::string_view pref_name :
       {prefs::kMouseRockerGesturesEnabled, prefs::kMouseRockerLeftAction,
        prefs::kMouseRockerRightAction}) {
    pref_change_registrar_.Add(
        pref_name,
        base::BindRepeating(&GestureConfigTracker::LoadMouseRockerGestures,
                            weak_factory_.GetWeakPtr()));
  }
  for (const std::string_view pref_name :
       {prefs::kMouseWheelGesturesEnabled, prefs::kMouseWheelModifierButton,
        prefs::kMouseWheelSensitivityInPixels, prefs::kMouseWheelUpAction,
        prefs::kMouseWheelDownAction}) {
    pref_change_registrar_.Add(
        pref_name,
        base::BindRepeating(&GestureConfigTracker::LoadMouseWheelGestures,
                            weak_factory_.GetWeakPtr()));
  }
}

GestureConfigTracker::~GestureConfigTracker() = default;

bool GestureConfigTracker::IsAllowedForHost(std::string_view scheme,
                                            std::string_view host) const {
  if (!prefs_->GetBoolean(prefs::kEnabled)) {
    return false;
  }

  // The synced pref value isn't guaranteed to be lowercased already, so
  // lowercase it here too, or a mixed-case entry could silently fail open.
  return std::ranges::none_of(
      prefs_->GetDict(prefs::kDisallowedSites),
      [&scheme, &host](const auto& entry) {
        const auto& [disallowed_key, value] = entry;
        if (!value.GetIfBool().value_or(false)) {
          return false;
        }
        const std::string lower_key = base::ToLowerASCII(disallowed_key);
        const size_t scheme_separator = lower_key.find("://");
        if (scheme_separator != std::string::npos) {
          return scheme ==
                     std::string_view(lower_key).substr(0, scheme_separator) &&
                 host ==
                     std::string_view(lower_key).substr(scheme_separator + 3);
        }
        return host == lower_key ||
               (host.size() > lower_key.size() && host.ends_with(lower_key) &&
                host[host.size() - lower_key.size() - 1] == '.');
      });
}

void GestureConfigTracker::LoadDirectionalGestures() {
  directional_gesture_mapping_.ResetToDefaults();

  for (const auto [gesture_key, actions_value] :
       prefs_->GetDict(prefs::kDirectionalGestures)) {
    const base::ListValue* list = actions_value.GetIfList();
    std::optional<GestureActions> actions =
        list ? MaybeParseGestureActions(*list) : std::nullopt;
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

void GestureConfigTracker::LoadSmartGestures() {
  smart_gesture_mapping_.ResetToDefaults();

  for (const auto [key, actions_value] :
       prefs_->GetDict(prefs::kSmartGestures)) {
    const base::ListValue* list = actions_value.GetIfList();
    std::optional<GestureActions> actions =
        list ? MaybeParseGestureActions(*list) : std::nullopt;
    if (actions) {
      smart_gesture_mapping_.SetChainedActions(key, std::move(*actions));
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

void GestureConfigTracker::LoadMouseRockerGestures() {
  mouse_rocker_left_actions_ =
      MaybeParseGestureActions(prefs_->GetList(prefs::kMouseRockerLeftAction))
          .value_or({MakeGestureAction(IDC_BACK)});
  mouse_rocker_right_actions_ =
      MaybeParseGestureActions(prefs_->GetList(prefs::kMouseRockerRightAction))
          .value_or({MakeGestureAction(IDC_FORWARD)});
}

void GestureConfigTracker::LoadMouseWheelGestures() {
  mouse_wheel_up_actions_ =
      MaybeParseGestureActions(prefs_->GetList(prefs::kMouseWheelUpAction))
          .value_or({MakeGestureAction(IDC_SELECT_PREVIOUS_TAB)});
  mouse_wheel_down_actions_ =
      MaybeParseGestureActions(prefs_->GetList(prefs::kMouseWheelDownAction))
          .value_or({MakeGestureAction(IDC_SELECT_NEXT_TAB)});
}

}  // namespace gestures
