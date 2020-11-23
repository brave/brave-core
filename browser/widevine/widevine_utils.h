/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WIDEVINE_WIDEVINE_UTILS_H_
#define BRAVE_BROWSER_WIDEVINE_WIDEVINE_UTILS_H_

#include <string>

namespace content {
class WebContents;
}  // namespace content

namespace user_prefs {
class PrefRegistrySyncable;
}

class PrefRegistrySimple;
class Profile;

void EnableWidevineCdmComponent(content::WebContents* web_contents);
void RegisterWidevineProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry);
int GetWidevinePermissionRequestTextFrangmentResourceId(bool for_restart);
void RequestWidevinePermission(content::WebContents* web_contents,
                               bool for_restart);
void RegisterWidevineLocalstatePrefs(PrefRegistrySimple* registry);
void DontAskWidevineInstall(content::WebContents* web_contents, bool dont_ask);
bool IsWidevineOptedIn();
void SetWidevineOptedIn(bool opted_in);
void MigrateWidevinePrefs(Profile* profile);

#endif  // BRAVE_BROWSER_WIDEVINE_WIDEVINE_UTILS_H_
