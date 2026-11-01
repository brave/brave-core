/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_profile_service.h"

#include "base/time/time.h"
#include "brave/components/tor/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace tor {

namespace {

// Restricted .onion requests to Tor windows. Removed 10/2026.
constexpr char kObsoleteOnionOnlyInTorWindows[] =
    "tor.onion_only_in_tor_windows";

}  // namespace

TorProfileService::TorProfileService() = default;

TorProfileService::~TorProfileService() = default;

// static
void TorProfileService::RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kTorDisabled, false);
  registry->RegisterDictionaryPref(prefs::kBridgesConfig);
  registry->RegisterTimePref(prefs::kBuiltinBridgesRequestTime, base::Time());
}

// static
void TorProfileService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kAutoOnionRedirect, false);
}

// static
void TorProfileService::RegisterProfilePrefsForMigration(
    PrefRegistrySimple* registry) {
  // Added 10/2026
  registry->RegisterBooleanPref(kObsoleteOnionOnlyInTorWindows, true);
}

// static
void TorProfileService::MigrateObsoleteProfilePrefs(PrefService* prefs) {
  // Added 10/2026
  prefs->ClearPref(kObsoleteOnionOnlyInTorWindows);
}

}  // namespace tor
