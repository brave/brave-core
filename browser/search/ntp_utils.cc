/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search/ntp_utils.h"

#include "brave/browser/profiles/profile_util.h"
#include "brave/common/pref_names.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace {

void ClearNewTabPageProfilePrefs(Profile* profile) {
  PrefService* prefs = profile->GetPrefs();
  prefs->ClearPref(kNewTabPageShowTopSites);
}

}  // namespace

namespace new_tab_page {

void MigrateNewTabPagePrefs(Profile* profile) {
  // Migrate over to the Chromium setting for shortcuts visible
  // Only sets the value if user has changed it
  const PrefService::Preference* pref =
      profile->GetPrefs()->FindPreference(kNewTabPageShowTopSites);
  if (pref->HasUserSetting()) {
    profile->GetPrefs()->SetBoolean(prefs::kNtpShortcutsVisible,
      profile->GetPrefs()->GetBoolean(kNewTabPageShowTopSites));
  }

  // Clear deprecated prefs.
  ClearNewTabPageProfilePrefs(profile);
}

void RegisterNewTabPagePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(kNewTabPageShowTopSites, true);
}

}  // namespace new_tab_page
