// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/browser/prefs_registration.h"

#include "base/values.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "brave/components/gestures/core/common/features.h"
#include "brave/components/gestures/core/common/mouse/wheel/mouse_wheel_modifier_button_types.h"
#include "components/pref_registry/pref_registry_syncable.h"

namespace gestures {

void RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry,
    const MouseRockerDefaultCommands& mouse_rocker_defaults,
    const MouseWheelDefaultCommands& mouse_wheel_defaults) {
  registry->RegisterBooleanPref(prefs::kEnabled, true);
  registry->RegisterBooleanPref(prefs::kShowTrail, true);
  registry->RegisterBooleanPref(prefs::kShowMatchedGesture, true);
  registry->RegisterDoublePref(prefs::kSensitivityInPixels,
                               features::kMinStrokePixels.Get());

  registry->RegisterDictionaryPref(
      prefs::kDirectionalGestures,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);

  registry->RegisterListPref(prefs::kCustomGestures,
                             user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);

  registry->RegisterBooleanPref(prefs::kMouseRockerGesturesEnabled, true);
  registry->RegisterListPref(prefs::kMouseRockerLeftAction,
                             base::ListValue().Append(base::DictValue().Set(
                                 "command", mouse_rocker_defaults.left)),
                             user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterListPref(prefs::kMouseRockerRightAction,
                             base::ListValue().Append(base::DictValue().Set(
                                 "command", mouse_rocker_defaults.right)),
                             user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);

  registry->RegisterBooleanPref(prefs::kMouseWheelGesturesEnabled, true);
  registry->RegisterIntegerPref(
      prefs::kMouseWheelModifierButton,
      static_cast<int>(MouseWheelModifierButtonType::kRight));
  registry->RegisterDoublePref(prefs::kMouseWheelSensitivityInPixels,
                               features::kMinWheelDelta.Get());
  registry->RegisterListPref(prefs::kMouseWheelUpAction,
                             base::ListValue().Append(base::DictValue().Set(
                                 "command", mouse_wheel_defaults.up)),
                             user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterListPref(prefs::kMouseWheelDownAction,
                             base::ListValue().Append(base::DictValue().Set(
                                 "command", mouse_wheel_defaults.down)),
                             user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);

  registry->RegisterDictionaryPref(
      prefs::kDisallowedSites, user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
}

}  // namespace gestures
