/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/chromium_src/components/browsing_data/core/pref_names.h"

#include "components/pref_registry/pref_registry_syncable.h"

namespace {

void RegisterBraveUserPrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(
      browsing_data::prefs::kDeleteBrowsingHistoryOnExit, false);
  registry->RegisterBooleanPref(
      browsing_data::prefs::kDeleteDownloadHistoryOnExit, false);
  registry->RegisterBooleanPref(browsing_data::prefs::kDeleteCacheOnExit,
                                false);
  registry->RegisterBooleanPref(browsing_data::prefs::kDeleteCookiesOnExit,
                                false);
  registry->RegisterBooleanPref(browsing_data::prefs::kDeletePasswordsOnExit,
                                false);
  registry->RegisterBooleanPref(browsing_data::prefs::kDeleteFormDataOnExit,
                                false);
  registry->RegisterBooleanPref(
      browsing_data::prefs::kDeleteHostedAppsDataOnExit, false);
  registry->RegisterBooleanPref(browsing_data::prefs::kDeleteSiteSettingsOnExit,
                                false);
  registry->RegisterBooleanPref(browsing_data::prefs::kDeleteBraveLeoHistory,
                                false);
  registry->RegisterBooleanPref(
      browsing_data::prefs::kDeleteBraveLeoHistoryOnExit, false);
}

}  // namespace

#include <components/browsing_data/core/pref_names.cc>
