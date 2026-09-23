// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_PREFS_REGISTRATION_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_PREFS_REGISTRATION_H_

#include "base/version_info/channel.h"

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace containers {

// Registers container-related preferences with the profile's preference system.
void RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry,
    version_info::Channel channel = version_info::Channel::UNKNOWN);

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_PREFS_REGISTRATION_H_
