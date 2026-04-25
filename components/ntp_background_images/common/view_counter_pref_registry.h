/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_COMMON_VIEW_COUNTER_PREF_REGISTRY_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_COMMON_VIEW_COUNTER_PREF_REGISTRY_H_

class PrefRegistrySimple;
class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace ntp_background_images {

void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);
void RegisterProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry);
void MigrateObsoleteProfilePrefs(PrefService* prefs);

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_COMMON_VIEW_COUNTER_PREF_REGISTRY_H_
