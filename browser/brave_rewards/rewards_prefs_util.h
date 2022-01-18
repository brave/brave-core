/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_PREFS_UTIL_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_PREFS_UTIL_H_

class PrefService;

namespace brave_rewards {

void MigrateObsoleteProfilePrefs(PrefService* prefs);

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_PREFS_UTIL_H_
