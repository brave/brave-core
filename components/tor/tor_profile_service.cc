/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_profile_service.h"

#include <string>

#include "base/time/time.h"
#include "brave/components/tor/pref_names.h"
#include "components/prefs/pref_registry_simple.h"

namespace tor {

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
  registry->RegisterBooleanPref(prefs::kOnionOnlyInTorWindows, true);
}

}  // namespace tor
