/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/chromium_src/components/browsing_data/core/pref_names.h"

#define RegisterBrowserUserPrefs RegisterBrowserUserPrefs_ChromiumImpl
#include "src/components/browsing_data/core/pref_names.cc"
#undef RegisterBrowserUserPrefs

namespace browsing_data::prefs {

void RegisterBrowserUserPrefs(user_prefs::PrefRegistrySyncable* registry) {
  RegisterBrowserUserPrefs_ChromiumImpl(registry);

  registry->RegisterBooleanPref(kDeleteBrowsingHistoryOnExit, false);
  registry->RegisterBooleanPref(kDeleteDownloadHistoryOnExit, false);
  registry->RegisterBooleanPref(kDeleteCacheOnExit, false);
  registry->RegisterBooleanPref(kDeleteCookiesOnExit, false);
  registry->RegisterBooleanPref(kDeletePasswordsOnExit, false);
  registry->RegisterBooleanPref(kDeleteFormDataOnExit, false);
  registry->RegisterBooleanPref(kDeleteHostedAppsDataOnExit, false);
  registry->RegisterBooleanPref(kDeleteSiteSettingsOnExit, false);
  registry->RegisterBooleanPref(kDeleteBraveLeoHistory, false);
  registry->RegisterBooleanPref(kDeleteBraveLeoHistoryOnExit, false);
}

}  // namespace browsing_data::prefs
