/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_pref_migration.h"

#include "brave/components/speedreader/speedreader_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace speedreader {

void RegisterProfilePrefsForMigration(PrefRegistrySimple* registry) {
  // Legacy preference - keep for migration purposes only
  registry->RegisterBooleanPref(kSpeedreaderPrefEnabledDeprecated, false);
}

void MigrateObsoleteProfilePrefs(PrefService* prefs) {
  // Check if the deprecated pref has been explicitly set by a user
  if (prefs->HasPrefPath(kSpeedreaderPrefEnabledDeprecated)) {
    bool old_value = prefs->GetBoolean(kSpeedreaderPrefEnabledDeprecated);

    // The old "enabled" pref controlled whether speedreader was enabled for
    // all sites. Migrate only the all-sites preference - let the feature
    // toggle use its default value (enabled).
    prefs->SetBoolean(kSpeedreaderAllowedForAllReadableSites, old_value);

    // Clear the deprecated preference
    prefs->ClearPref(kSpeedreaderPrefEnabledDeprecated);
  }
}

}  // namespace speedreader
