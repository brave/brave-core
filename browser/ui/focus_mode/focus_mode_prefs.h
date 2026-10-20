/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_PREFS_H_
#define BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_PREFS_H_

class PrefRegistrySimple;

namespace focus_mode {

inline constexpr char kFocusModeEnabled[] = "brave.focus_mode.enabled";

// Registers profile preferences for Focus Mode.
void RegisterProfilePrefs(PrefRegistrySimple* registry);

}  // namespace focus_mode

#endif  // BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_PREFS_H_
