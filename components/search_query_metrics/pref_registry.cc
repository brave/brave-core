/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/search_query_metrics/pref_registry.h"

#include "brave/components/search_query_metrics/pref_names.h"
#include "components/prefs/pref_registry_simple.h"

namespace metrics {

void RegisterLocalStatePrefs(PrefRegistrySimple* const registry) {
  registry->RegisterStringPref(prefs::kObliviousHttpKeyConfig, "");
  registry->RegisterTimePref(prefs::kObliviousHttpKeyConfigExpiresAt,
                             base::Time());
}

void RegisterProfilePrefs(PrefRegistrySimple* const registry) {
  registry->RegisterTimePref(prefs::kLastReportedAt, base::Time());
}

}  // namespace metrics
