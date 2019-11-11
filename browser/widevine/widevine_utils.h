/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WIDEVINE_WIDEVINE_UTILS_H_
#define BRAVE_BROWSER_WIDEVINE_WIDEVINE_UTILS_H_

#include <string>

#include "third_party/widevine/cdm/buildflags.h"

namespace content {
class WebContents;
}  // namespace content

namespace user_prefs {
class PrefRegistrySyncable;
}

class PrefRegistrySimple;
class Profile;

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
void InstallBundleOrRestartBrowser();
void SetWidevineInstalledVersion(const std::string& version);
std::string GetWidevineInstalledVersion();
#endif

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
void EnableWidevineCdmComponent(content::WebContents* web_contents);
#endif

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT) || BUILDFLAG(BUNDLE_WIDEVINE_CDM)
void RegisterWidevineProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry);
int GetWidevinePermissionRequestTextFrangmentResourceId();
void RequestWidevinePermission(content::WebContents* web_contents);
void RegisterWidevineLocalstatePrefs(PrefRegistrySimple* registry);
void DontAskWidevineInstall(content::WebContents* web_contents, bool dont_ask);
bool IsWidevineOptedIn();
void SetWidevineOptedIn(bool opted_in);
void MigrateWidevinePrefs(Profile* profile);
#endif

#endif  // BRAVE_BROWSER_WIDEVINE_WIDEVINE_UTILS_H_
