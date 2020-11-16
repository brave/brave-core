/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SEARCH_NTP_UTILS_H_
#define BRAVE_BROWSER_SEARCH_NTP_UTILS_H_

class Profile;

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace new_tab_page {

// APIs for prefs.
void MigrateNewTabPagePrefs(Profile* profile);
void RegisterNewTabPagePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry);

}  // namespace new_tab_page

#endif  // BRAVE_BROWSER_SEARCH_NTP_UTILS_H_
