// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_PREF_NAMES_H_
#define BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_PREF_NAMES_H_

namespace traffic_control::prefs {

// Toggle for applying traffic rules in this profile.
inline constexpr char kTrafficControlEnabled[] =
    "brave.traffic_control.enabled";

// Syncable list of profile-scoped traffic rules.
inline constexpr char kTrafficControlList[] = "brave.traffic_control.list";

}  // namespace traffic_control::prefs

#endif  // BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_PREF_NAMES_H_
