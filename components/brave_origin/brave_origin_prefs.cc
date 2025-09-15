/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_prefs.h"

#include "brave/components/brave_origin/pref_names.h"
#include "components/prefs/pref_registry_simple.h"

namespace brave_origin {

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  // Register the dictionary preference that stores all Brave policy values
  registry->RegisterDictionaryPref(kBraveOriginPolicies);
}

}  // namespace brave_origin
