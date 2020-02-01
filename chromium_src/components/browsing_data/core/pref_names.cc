/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/chromium_src/components/browsing_data/core/pref_names.h"

#define RegisterBrowserUserPrefs RegisterBrowserUserPrefs_ChromiumImpl
#include "../../../../components/browsing_data/core/pref_names.cc"  // NOLINT
#undef RegisterBrowserUserPrefs

namespace browsing_data {
namespace prefs {

const char kDeleteBrowsingHistoryOnExit[] =
    "browser.clear_data.browsing_history_on_exit";
const char kDeleteDownloadHistoryOnExit[] =
    "browser.clear_data.download_history_on_exit";
const char kDeleteCacheOnExit[] = "browser.clear_data.cache_on_exit";
const char kDeleteCookiesOnExit[] = "browser.clear_data.cookies_on_exit";
const char kDeletePasswordsOnExit[] = "browser.clear_data.passwords_on_exit";
const char kDeleteFormDataOnExit[] = "browser.clear_data.form_data_on_exit";
const char kDeleteHostedAppsDataOnExit[] =
    "browser.clear_data.hosted_apps_data_on_exit";
const char kDeleteSiteSettingsOnExit[] =
    "browser.clear_data.site_settings_on_exit";
const char kDeleteShieldsSettingsOnExit[] =
    "browser.clear_data.shields_settings_on_exit";

const char kDeleteShieldsSettings[] = "browser.clear_data.shields_settings";

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
  registry->RegisterBooleanPref(kDeleteShieldsSettingsOnExit, false);

  registry->RegisterBooleanPref(kDeleteShieldsSettings, false);
}

}  // namespace prefs
}  // namespace browsing_data
