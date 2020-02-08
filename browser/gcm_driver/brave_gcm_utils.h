/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_GCM_DRIVER_BRAVE_GCM_UTILS_H_
#define BRAVE_BROWSER_GCM_DRIVER_BRAVE_GCM_UTILS_H_

class Profile;

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace gcm {

void RegisterGCMProfilePrefs(user_prefs::PrefRegistrySyncable* registry);
void MigrateGCMPrefs(Profile* profile);

}  // namespace gcm

#endif  // BRAVE_BROWSER_GCM_DRIVER_BRAVE_GCM_UTILS_H_
