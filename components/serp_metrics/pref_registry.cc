/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/pref_registry.h"

#include "brave/components/serp_metrics/pref_names.h"
#include "components/prefs/pref_registry_simple.h"

namespace metrics {

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(prefs::kSerpMetricsTimePeriodStorage);
}

}  // namespace metrics
