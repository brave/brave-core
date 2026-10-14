// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_GESTURE_PROFILE_PREFS_H_
#define BRAVE_BROWSER_UI_GESTURES_GESTURE_PROFILE_PREFS_H_

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace gestures {

// Registers gestures' profile prefs, including the mouse rocker/wheel
// defaults as real `IDC_*` ids. Delegates to the 3-argument overload in
// `components/gestures/core/browser/prefs_registration.h`, which can't
// name those ids itself.
void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_GESTURE_PROFILE_PREFS_H_
