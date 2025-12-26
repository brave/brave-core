/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_STATS_BRAVE_STATS_PREFS_H_
#define BRAVE_IOS_BROWSER_BRAVE_STATS_BRAVE_STATS_PREFS_H_

class PrefRegistrySimple;
class PrefService;

namespace brave_stats {

void RegisterLocalStatePrefs(PrefRegistrySimple* registry);
void RegisterLocalStatePrefsForMigration(PrefRegistrySimple* registry);
void MigrateObsoleteLocalStatePrefs(PrefService* local_state);

}  // namespace brave_stats

#endif  // BRAVE_IOS_BROWSER_BRAVE_STATS_BRAVE_STATS_PREFS_H_
