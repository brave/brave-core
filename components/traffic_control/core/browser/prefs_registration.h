// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_PREFS_REGISTRATION_H_
#define BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_PREFS_REGISTRATION_H_

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace traffic_control {

// Registers traffic-control preferences with the profile's preference system.
void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

}  // namespace traffic_control

#endif  // BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_PREFS_REGISTRATION_H_
