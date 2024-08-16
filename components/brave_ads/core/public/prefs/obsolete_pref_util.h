/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_OBSOLETE_PREF_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_OBSOLETE_PREF_UTIL_H_

class PrefService;

class PrefRegistrySimple;

namespace brave_ads {

void RegisterProfilePrefsForMigration(PrefRegistrySimple* registry);

void MigrateObsoleteProfilePrefs(PrefService* prefs);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_OBSOLETE_PREF_UTIL_H_
